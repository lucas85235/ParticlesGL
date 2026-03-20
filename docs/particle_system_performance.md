# Sistema de Partículas de Alta Performance — Documento Técnico

> Guia definitivo de implementação para um sistema de partículas com foco em máximo desempenho  
> e mínimo uso de recursos de hardware, aplicado ao contexto do **ParticleGL** (OpenGL 4.3+, C++17).

---

## Índice

1. [Fundamentos de Performance em Sistemas de Partículas](#1-fundamentos-de-performance-em-sistemas-de-partículas)
2. [Layout de Memória — A Base de Tudo](#2-layout-de-memória--a-base-de-tudo)
3. [Gerenciamento de Pool](#3-gerenciamento-de-pool)
4. [Integração com ECS](#4-integração-com-ecs)
5. [Pipeline CPU → GPU](#5-pipeline-cpu--gpu)
6. [Compute Shaders — Simulação GPU-Driven](#6-compute-shaders--simulação-gpu-driven)
7. [Renderização com Indirect Draw](#7-renderização-com-indirect-draw)
8. [Pool Global e Multi-Emitter em 1 Draw Call](#8-pool-global-e-multi-emitter-em-1-draw-call)
9. [Ordenação de Partículas Transparentes](#9-ordenação-de-partículas-transparentes)
10. [Profiling e Métricas](#10-profiling-e-métricas)
11. [Roadmap de Evolução](#11-roadmap-de-evolução)
12. [Referências](#12-referências)

---

## 1. Fundamentos de Performance em Sistemas de Partículas

Antes de qualquer técnica, é preciso entender **onde o tempo de frame é gasto**. Em um sistema de partículas ingênuo com 100k partículas, o tempo de frame se distribui aproximadamente assim:

```
Simulação (CPU)     ████████████████░░░░░░░░░░░░░░░  ~40%
Upload GPU (memcpy) ████████████████████░░░░░░░░░░░  ~30%
Draw Calls          ████████░░░░░░░░░░░░░░░░░░░░░░░  ~15%
Driver overhead     ██████░░░░░░░░░░░░░░░░░░░░░░░░░  ~10%
Outros              ███░░░░░░░░░░░░░░░░░░░░░░░░░░░░   ~5%
```

A estratégia de otimização atacará cada gargalo em ordem de impacto:

1. **Layout de memória** — elimina cache misses na simulação (CPU)
2. **Persistent Mapped Buffer** — elimina memcpy (CPU→GPU)
3. **Compute Shaders** — elimina a simulação da CPU inteiramente
4. **Indirect Draw** — elimina readback de `active_count` e overhead de driver

### 1.1 Princípios que nunca mudam

Independentemente da técnica, os seguintes princípios são absolutos:

**Princípio 1 — Contiguidade:** dados que são acessados juntos devem estar contíguos na memória. Um loop que acessa `position` e `velocity` juntos deve ter ambos na mesma linha de cache, ou em arrays separados mas iterados simultaneamente.

**Princípio 2 — Ausência de alocação no hot path:** `malloc`, `new`, `std::vector::push_back` com resize e qualquer operação que possa tocar o heap são proibidos dentro do loop de simulação de partículas. Todo o espaço necessário é pré-alocado na inicialização.

**Princípio 3 — Separação GPU/CPU:** dados que a GPU lê (posição, cor, escala) e dados que só a CPU usa (velocidade, vida, máximo de vida) nunca devem estar na mesma struct. Misturá-los faz a CPU carregar dados irrelevantes da GPU no cache, e faz o upload de GPU carregar dados irrelevantes da CPU para a VRAM.

**Princípio 4 — O(1) para emit e kill:** o throughput de spawn e morte de partículas deve ser constante independentemente de N. Qualquer algoritmo de gestão de pool que seja O(N) em emit ou kill é inaceitável acima de 10k partículas.

**Princípio 5 — Um draw call por cena, não por emitter:** o objetivo final é renderizar todas as partículas de todos os emitters em um único `glDrawElementsInstanced` ou `glDrawElementsIndirect`.

---

## 2. Layout de Memória — A Base de Tudo

### 2.1 Por que AoS é o inimigo

Array of Structures (AoS) é o layout "natural" de C++:

```cpp
// AoS — o que parece intuitivo
struct Particle {
    glm::vec3 position;   // 12 bytes
    glm::vec4 color;      // 16 bytes
    glm::vec3 velocity;   // 12 bytes
    float     life;       //  4 bytes
    float     maxLife;    //  4 bytes
    float     scale;      //  4 bytes
};                        // Total: 52 bytes por partícula
std::vector<Particle> particles;
```

Uma linha de cache típica tem **64 bytes**. Com AoS e structs de 52 bytes, um loop que só precisa atualizar `life` carrega também `position`, `color`, `velocity` e `maxLife` — que são completamente irrelevantes para aquele loop. Isso significa:

- **Taxa de utilização de cache: ~8%** (4 bytes de `life` / 52 bytes carregados)
- Para 100k partículas, o loop de atualização de vida toca ~5MB de memória no total
- Com SoA, o mesmo loop toca ~400KB — apenas os floats de `life`

### 2.2 Structure of Arrays (SoA) — o modelo correto

A regra é: **um array por campo**, não um campo por struct.

```cpp
// SoA — separação rigorosa por tipo de acesso
struct ParticlePool {
    // ── Dados de GPU (lidos pelo vertex shader via instancing) ──────────
    // Estes arrays são ou espelhos do buffer mapeado, ou o próprio buffer
    std::vector<glm::vec3> position;    // 12 bytes × N
    std::vector<float>     scale;       //  4 bytes × N
    std::vector<glm::vec4> color;       // 16 bytes × N

    // ── Dados de CPU (usados apenas na simulação, nunca vão para GPU) ───
    std::vector<glm::vec3> velocity;    // 12 bytes × N
    std::vector<float>     life;        //  4 bytes × N
    std::vector<float>     maxLife;     //  4 bytes × N

    // ── Controle ─────────────────────────────────────────────────────────
    uint32_t               active_count = 0;
    uint32_t               max_count    = 0;
};
```

**Por que a separação GPU/CPU importa aqui:** o upload para GPU é uma cópia de `[position | scale | color]`. Com SoA, esses arrays são contíguos entre si ou podem ser carregados como buffers separados. Nenhum dado de CPU (`velocity`, `life`) é carregado junto. Isso reduz o volume de upload em ~50%.

### 2.3 Alinhamento de memória

Para habilitar auto-vetorização pelo compilador (SIMD), os arrays devem estar alinhados em 32 bytes (AVX2) ou 16 bytes (SSE4). Use `std::vector` com allocator alinhado:

```cpp
// Allocator alinhado para SIMD (32 bytes para AVX2)
template<typename T>
using AlignedVector = std::vector<T, boost::alignment::aligned_allocator<T, 32>>;
// OU, sem boost:
template<typename T>
using AlignedVector = std::vector<T, AlignedAllocator<T, 32>>;
```

Se não quiser allocator customizado, garanta ao menos que os arrays comecem em endereços alinhados via `aligned_alloc` ou `_mm_malloc` e wrapeie em span.

### 2.4 Interleaved GPU Buffer vs. Buffers Separados

Para o upload de GPU, há duas estratégias:

**Opção A — Buffer Interleaved (posição + escala + cor juntos):**
```
[pos0][scale0][col0][pos1][scale1][col1]...
```
Melhor para vertex fetch quando todos os atributos são lidos simultaneamente no shader. Cache do vertex shader bem utilizado.

**Opção B — Buffers Separados por atributo (Multi-VBO):**
```
VBO0: [pos0][pos1][pos2]...
VBO1: [scale0][scale1][scale2]...
VBO2: [col0][col1][col2]...
```
Mais fácil de integrar com SoA no CPU (sem passo de interleaving). Mais fácil de atualizar parcialmente (ex.: só cor mudou).

**Recomendação para esta POC:** **Buffers Separados por atributo**. O custo de bind de 3 VBOs em vez de 1 é negligenciável (~0.001ms), e a eliminação do passo de interleaving na CPU compensa amplamente.

---

## 3. Gerenciamento de Pool

### 3.1 Slot Pool com Free List

O mecanismo de gerenciamento de slots deve ser O(1) para emit e O(1) para kill, sem fragmentação lógica.

A estratégia é o **swap-and-decrement** já presente na implementação atual, mas implementado como método centralizado que sincroniza todos os arrays simultaneamente:

```
Estado inicial:     [A][B][C][D][E] | [_][_]
                     <── active ──>   <dead>
                                    ^ active_count = 5

Kill C (índice 2):
  1. swap C com E (último ativo):   [A][B][E][D] | [C][_]
     (swap em TODOS os arrays: position, velocity, life, etc.)
  2. active_count--                  active_count = 4

Resultado:          [A][B][E][D] | [C][_]
                     <─ active ─>   <dead>
```

Vantagens:
- Kill é O(1) — apenas swaps e um decremento
- O slice `[0..active_count)` permanece contíguo e compacto — sem buracos
- Upload para GPU é sempre de `active_count` elementos contíguos

**Regra crítica:** a função `killParticle(uint32_t index)` é o único lugar onde swaps ocorrem. Ela deve ser um método privado do `ParticlePool` que sincroniza **todos** os arrays atomicamente.

### 3.2 Emissão de partículas

Emit é um append ao fim do slice ativo:

```
Estado:      [A][B][C] | [_][_][_]
                       ^ active_count = 3

Emit D:
  1. Escreve D em [active_count]
  2. active_count++

Resultado:   [A][B][C][D] | [_][_]
                          ^ active_count = 4
```

Emit é O(1). Se `active_count == max_count`, a emissão é silenciosamente descartada (sem log, sem alloc). Isso é comportamento correto — um emitter no limite do seu pool simplesmente para de spawnar novas partículas.

### 3.3 Tamanho do pool

O pool **nunca cresce dinamicamente** após a inicialização. O tamanho máximo é definido pelo componente `ParticleEmitter.max_particles` no momento de criação do pool.

Regra de dimensionamento:

```
max_particles = emission_rate × max_lifetime × safety_factor
```

Onde `safety_factor = 1.2` para absorver variações de frame rate. Exemplo: emitter com 1000 partículas/s e lifetime de 3s deve ter pool de 3600 slots.

---

## 4. Integração com ECS

### 4.1 O pool como componente nativo

O pool de partículas deve ser um **componente ECS de primeira classe**, não estado interno do `ParticleSystem`. Isso resolve os três problemas da implementação atual:

- Ciclo de vida gerenciado pela `Registry` — destruição automática
- Acesso multissistema via API genérica
- Nenhum mapa paralelo (`pools_`, `emission_accumulators_`) desincronizado

```cpp
// Componente que vive na Registry como qualquer outro componente
struct ParticlePoolComponent {
    // Dados de simulação (CPU-only)
    AlignedVector<glm::vec3> velocity;
    AlignedVector<float>     life;
    AlignedVector<float>     maxLife;

    // Dados de GPU (escritos pelo CPU, lidos pelo vertex shader)
    AlignedVector<glm::vec3> position;
    AlignedVector<float>     scale;
    AlignedVector<glm::vec4> color;

    // Controle
    uint32_t active_count = 0;
    uint32_t max_count    = 0;

    // Acumulador de emissão (float para emissão sub-frame)
    float    emission_accumulator = 0.0f;

    // Referência ao buffer GPU (gerenciado pelo Renderer, não pelo ECS)
    InstanceBuffer* gpu_buffer = nullptr; // não-owning, controlado pelo Renderer
};
```

**Por que `gpu_buffer` é um ponteiro não-owning:** o `InstanceBuffer` (VBO + handles OpenGL) pertence ao `Renderer`, não ao ECS. Destruir um `InstanceBuffer` fora de um contexto OpenGL válido causa undefined behavior. O `Renderer` é responsável por criar e destruir os buffers; o componente apenas guarda uma referência.

### 4.2 Sistema de partículas como consumidor da Registry

```
Registry
  └── Entity E₁
        ├── ParticleEmitter    (config: rate, spread, colors, etc.)
        └── ParticlePoolComponent  (dados: SoA de N slots)

ParticleSystem::update(Registry& reg, float dt)
  └── itera pares <ParticleEmitter, ParticlePoolComponent>
        └── chama simulate(emitter, pool, dt)

ParticleRenderSystem::render(Registry& reg, Renderer& renderer)
  └── itera pares <ParticlePoolComponent, Transform>
        └── chama renderer.uploadAndDraw(pool, transform)
```

A separação de `ParticleSystem` (simulação) e `ParticleRenderSystem` (upload + draw) permite que no futuro a simulação rode em thread separada.

### 4.3 View multiarquétipo eficiente

A query `getEntitiesWith<ParticleEmitter, ParticlePoolComponent>()` só é eficiente se a `Registry` usa **sparse sets** ou **archetypal bitsets**. Para o ParticleGL, a implementação mínima correta é:

- Cada tipo de componente tem um bitset de entidades que o possuem
- A query faz `AND` dos bitsets e itera apenas os bits ativos
- Complexidade: O(max_entity_count / 64) — praticamente O(1) para cenas pequenas

---

## 5. Pipeline CPU → GPU

### 5.1 Estratégia de upload: evolução em três fases

As três fases a seguir representam a progressão natural de otimização do pipeline de upload.

---

#### Fase 1 — `glBufferSubData` (implementação atual, aceitável até ~50k partículas)

```
[CPU Frame]
  simulate() → modifica position[], color[], scale[] em RAM
  [Barreira de sincronismo implícita: draw anterior terminou?]
  glBufferSubData → copia active_count × stride bytes de RAM para VRAM
  glDrawArraysInstanced
```

Problemas:
- `glBufferSubData` bloqueia se o driver ainda usa o buffer no frame anterior (pipeline stall)
- Cada frame realiza uma cópia desnecessária de dados que já estavam atualizados

---

#### Fase 2 — Double Buffering + Orphaning (sem stall, até ~200k partículas)

O "buffer orphaning" é uma técnica onde o buffer é descartado logicamente antes de ser escrito, permitindo que o driver aloque novo espaço sem esperar o frame anterior:

```cpp
// A cada frame:
glBindBuffer(GL_ARRAY_BUFFER, vbo);

// "Orphaning": declara que o conteúdo anterior pode ser descartado
glBufferData(GL_ARRAY_BUFFER, maxSize, nullptr, GL_STREAM_DRAW);

// Copia apenas os dados atuais (sem stall porque o buffer é "novo")
glBufferSubData(GL_ARRAY_BUFFER, 0, active_count * stride, data_ptr);
```

Vantagem: elimina pipeline stalls sem `glMapBuffer`. Desvantagem: ainda há uma cópia CPU→VRAM a cada frame.

---

#### Fase 3 — Persistent Mapped Buffer (zero-copy, recomendado para produção)

Esta é a técnica de menor overhead para uploads frequentes de grandes buffers. O buffer é criado **uma única vez** com memória que o CPU pode escrever diretamente:

```cpp
// Inicialização — feita UMA VEZ
GLuint vbo;
glGenBuffers(1, &vbo);
glBindBuffer(GL_ARRAY_BUFFER, vbo);

// GL_MAP_PERSISTENT_BIT: o mapeamento persiste entre draw calls
// GL_MAP_COHERENT_BIT: escritas da CPU ficam visíveis para GPU sem flush explícito
// GL_MAP_WRITE_BIT: CPU escreve, GPU lê
glBufferStorage(GL_ARRAY_BUFFER, maxSizeBytes, nullptr,
    GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

void* gpuPtr = glMapBufferRange(GL_ARRAY_BUFFER, 0, maxSizeBytes,
    GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

// Guarda o ponteiro — ele é válido até o buffer ser deletado
pool.mapped_gpu_ptr = gpuPtr;
```

A partir daí, **a simulação escreve diretamente no buffer da GPU**:

```cpp
// No loop de simulação (não há mais "flush para GPU")
glm::vec3* gpu_positions = static_cast<glm::vec3*>(pool.mapped_gpu_ptr);
gpu_positions[i] = newPosition; // ← vai direto para VRAM
```

A única sincronização necessária antes do draw call é uma **fence** para garantir que a GPU não está lendo enquanto a CPU escreve:

```cpp
// Antes de simular: espera a GPU terminar de ler o frame anterior
if (pool.sync_fence) {
    GLenum result = glClientWaitSync(pool.sync_fence, GL_SYNC_FLUSH_COMMANDS_BIT, TIMEOUT_NS);
    glDeleteSync(pool.sync_fence);
}

// Simula e escreve no buffer mapeado...

// Após o draw call: cria nova fence para o próximo frame
pool.sync_fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
```

**Custo do Persistent Mapped Buffer:** uma fence por emitter por frame (nanosegundos se a GPU está na frente do CPU). **Benefício:** elimina completamente o memcpy de RAM para VRAM, que para 100k partículas representa ~2.4MB × 60fps = ~144MB/s de bandwidth economizada.

### 5.2 Layout do buffer de instâncias

O buffer de instâncias deve ser declarado no shader exatamente como está organizado no CPU. Recomendação: **buffers separados por atributo** para máxima compatibilidade com o SoA do CPU.

```glsl
// Vertex Shader — particle.vert
#version 430 core

// Geometria base do quad (igual para todas as instâncias)
layout(location = 0) in vec2 a_vertexPos;

// Dados por instância (um valor por partícula, divisor = 1)
layout(location = 1) in vec3  a_position; // VBO separado
layout(location = 2) in float a_scale;    // VBO separado
layout(location = 3) in vec4  a_color;    // VBO separado

out vec4 v_color;

uniform mat4 u_viewProj;

void main() {
    vec3 worldPos = a_position + vec3(a_vertexPos * a_scale, 0.0);
    gl_Position   = u_viewProj * vec4(worldPos, 1.0);
    v_color       = a_color;
}
```

Configuração dos atributos de instância no VAO:

```cpp
// Atributo de posição de instância (location = 1, VBO = vbo_position)
glBindBuffer(GL_ARRAY_BUFFER, vbo_position);
glEnableVertexAttribArray(1);
glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), 0);
glVertexAttribDivisor(1, 1); // ← divisor 1 = um valor por instância

// Atributo de escala (location = 2, VBO = vbo_scale)
glBindBuffer(GL_ARRAY_BUFFER, vbo_scale);
glEnableVertexAttribArray(2);
glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(float), 0);
glVertexAttribDivisor(2, 1);

// Atributo de cor (location = 3, VBO = vbo_color)
glBindBuffer(GL_ARRAY_BUFFER, vbo_color);
glEnableVertexAttribArray(3);
glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), 0);
glVertexAttribDivisor(3, 1);
```

---

## 6. Compute Shaders — Simulação GPU-Driven

Esta é a maior alavanca de performance disponível. Ao mover a simulação para a GPU:

- A CPU fica livre para outras tarefas (lógica de jogo, áudio, física)
- A simulação é paralelizada massivamente (milhares de threads simultâneas)
- O pipeline CPU→GPU de dados de simulação desaparece — os dados nunca saem da GPU

### 6.1 Pré-requisitos

- OpenGL 4.3+ (disponível em qualquer GPU de 2012+)
- SSBOs (`GL_SHADER_STORAGE_BUFFER`) para leitura/escrita de arrays arbitrários
- Atomic Counters para `active_count`

### 6.2 Layout dos buffers no lado da GPU

```
SSBO 0 — Positions  [vec3 × MAX_PARTICLES]  → lido pelo vertex shader
SSBO 1 — Scales     [float × MAX_PARTICLES] → lido pelo vertex shader
SSBO 2 — Colors     [vec4 × MAX_PARTICLES]  → lido pelo vertex shader
SSBO 3 — Velocities [vec3 × MAX_PARTICLES]  → CPU-only (escrito no spawn)
SSBO 4 — Lives      [float × MAX_PARTICLES] → CPU-only
SSBO 5 — MaxLives   [float × MAX_PARTICLES] → CPU-only
SSBO 6 — KillList   [uint × MAX_PARTICLES]  → escrito pelo compute, lido pelo compact
Atomic Counter — active_count
```

### 6.3 Compute Shader de simulação

```glsl
// particle_simulate.comp
#version 430 core

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

// Buffers de dados de simulação
layout(std430, binding = 0) buffer Positions  { vec3  positions[];  };
layout(std430, binding = 1) buffer Velocities { vec3  velocities[]; };
layout(std430, binding = 2) buffer Lives      { float lives[];      };
layout(std430, binding = 3) buffer MaxLives   { float maxLives[];   };
layout(std430, binding = 4) buffer Colors     { vec4  colors[];     };

// Kill list: armazena índices das partículas mortas neste frame
layout(std430, binding = 5) buffer KillList   { uint killList[];    };
layout(binding = 0) uniform atomic_uint killCount;
layout(binding = 1) uniform atomic_uint activeCount;

uniform float u_dt;
uniform vec3  u_gravity;    // (0, -9.8, 0) ou configurável

void main() {
    uint i = gl_GlobalInvocationID.x;

    // Threads além do ativo são descartadas
    uint current_active = atomicCounter(activeCount);
    if (i >= current_active) return;

    // Integração de Euler semi-implícita
    velocities[i] += u_gravity * u_dt;
    positions[i]  += velocities[i] * u_dt;

    // Atualização de vida
    lives[i] += u_dt;

    // Fade de cor por lifetime (interpolação linear CPU-less)
    float t         = clamp(lives[i] / maxLives[i], 0.0, 1.0);
    colors[i].a     = 1.0 - t;   // fade out por alpha

    // Partícula morreu: registra na kill list
    if (lives[i] >= maxLives[i]) {
        uint killIdx = atomicCounterIncrement(killCount);
        killList[killIdx] = i;
    }
}
```

**Por que `local_size_x = 64`:** os warps de GPU têm 32 (NVIDIA) ou 64 (AMD) threads. Usar múltiplos de 64 garanta ocupação máxima em ambas as arquiteturas sem desperdiçar threads.

### 6.4 Compute Shader de compactação (stream compaction)

Após o simulate, partículas mortas estão listadas no `killList` mas ainda ocupam slots. O compactor remove os buracos:

```glsl
// particle_compact.comp
#version 430 core

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding = 0) buffer Positions  { vec3  positions[];  };
layout(std430, binding = 1) buffer Velocities { vec3  velocities[]; };
layout(std430, binding = 2) buffer Lives      { float lives[];      };
layout(std430, binding = 3) buffer MaxLives   { float maxLives[];   };
layout(std430, binding = 4) buffer Colors     { vec4  colors[];     };
layout(std430, binding = 5) buffer KillList   { uint  killList[];   };

layout(binding = 0) uniform atomic_uint killCount;
layout(binding = 1) uniform atomic_uint activeCount;

void swapWithLast(uint deadIdx, uint lastIdx) {
    positions[deadIdx]  = positions[lastIdx];
    velocities[deadIdx] = velocities[lastIdx];
    lives[deadIdx]      = lives[lastIdx];
    maxLives[deadIdx]   = maxLives[lastIdx];
    colors[deadIdx]     = colors[lastIdx];
}

void main() {
    uint kills  = atomicCounter(killCount);
    uint active = atomicCounter(activeCount);

    // Compacta: para cada morto, swap com o último ativo
    for (uint k = 0; k < kills; k++) {
        uint deadIdx = killList[k];
        if (deadIdx < active) {
            swapWithLast(deadIdx, active - 1);
            active--;
        }
    }

    // Reseta contadores
    atomicCounterExchange(activeCount, active);
    atomicCounterExchange(killCount, 0);
}
```

> **Nota:** este compactor é single-threaded na GPU por simplicidade. Para N > 500k, substituir por um **parallel prefix sum** (scan + scatter) que paralelize a compactação. A implementação de prefix sum em GLSL é um padrão bem documentado e está além do escopo desta POC.

### 6.5 Despacho do compute no frame

```cpp
// No ParticleRenderSystem::dispatchSimulation()

// 1. Bind dos SSBOs
glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_positions);
glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_velocities);
// ... etc.

// 2. Dispatch do simulate — ceil(activeCount / 64) grupos
uint32_t groups = (active_count + 63) / 64;
glUseProgram(simulate_shader);
glUniform1f(dt_loc, delta_time);
glDispatchCompute(groups, 1, 1);

// 3. Barreira de memória: garante que o simulate terminou antes do compact
glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT);

// 4. Dispatch do compact (single group, sequencial)
glUseProgram(compact_shader);
glDispatchCompute(1, 1, 1);

// 5. Barreira: garante que o compact terminou antes do draw
glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT
              | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

// 6. Draw (próxima seção)
```

---

## 7. Renderização com Indirect Draw

Com simulação GPU-driven, o `active_count` vive na GPU como atomic counter. Fazer `glGetBufferSubData` para ler esse valor na CPU e passá-lo para `glDrawArraysInstanced(…, activeCount, …)` introduz um **roundtrip GPU→CPU** que é um dos piores stalls possíveis (~0.5–2ms de latência).

A solução é `glDrawArraysIndirect` (ou `glDrawElementsIndirect`): o draw count é lido **diretamente da GPU**, sem passar pela CPU.

### 7.1 Struct do comando indireto

```cpp
// Estrutura que fica em um buffer na GPU (GL_DRAW_INDIRECT_BUFFER)
struct DrawArraysIndirectCommand {
    uint32_t count;         // vértices por instância (6 para um quad)
    uint32_t instanceCount; // ← activeCount, escrito pelo compact shader
    uint32_t first;         // índice do primeiro vértice (0)
    uint32_t baseInstance;  // primeiro índice de instância (0)
};
```

### 7.2 O compact shader escreve o comando indiretamente

```glsl
// Adicionado ao particle_compact.comp — ao final da compactação:
layout(std430, binding = 6) buffer DrawCommand {
    uint vertexCount;    // constante: 6 (dois triângulos)
    uint instanceCount;  // ← escrevemos aqui
    uint first;
    uint baseInstance;
} drawCommand;

// No main(), após atualizar active:
drawCommand.instanceCount = active;
```

### 7.3 Chamada de draw sem roundtrip

```cpp
glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirect_command_buffer);
glDrawArraysIndirect(GL_TRIANGLES, 0); // ← lê instanceCount diretamente do buffer
```

Benefício: zero latência de roundtrip. O driver nunca espera a CPU. A GPU emite o draw assim que o compact termina.

---

## 8. Pool Global e Multi-Emitter em 1 Draw Call

O objetivo final é renderizar todos os emitters da cena com **um único draw call**. Isso é feito através de um **pool global de partículas** que todos os emitters compartilham.

### 8.1 Arquitetura do pool global

```
Pool Global: MAX_TOTAL = 1.000.000 slots

[  Emitter A: slots 0..9999  |  Emitter B: slots 10000..49999  |  livre...  ]
   active=7500                   active=32000
```

Cada `ParticleEmitter` no ECS armazena apenas:
- `pool_offset`: índice do início do seu range no pool global
- `pool_size`: tamanho máximo reservado
- `active_count`: quantas partículas estão vivas dentro do seu range

O `ParticleRenderSystem` despacha um único compute por cena e um único draw call que cobre todos os slots ativos de todos os emitters.

### 8.2 Alocador de ranges

O pool global usa um **alocador de ranges** simples (linear, sem fragmentação na POC):

```
[Em uso por E1: 0..9999][Em uso por E2: 10000..49999][Livre: 50000..999999]
                                                      ^
                                                      next_free_slot
```

Quando um emitter é destruído, o range é liberado e o próximo emitter pode reutilizá-lo. Para a POC, uma free list de ranges ordenados é suficiente.

### 8.3 Vantagens do pool global

- **1 draw call** em vez de N: o overhead de driver por draw call é ~0.01–0.05ms. Com 100 emitters, isso representa 1–5ms economizados.
- **1 compute dispatch** em vez de N: o mesmo argumento se aplica ao simulate compute.
- **Melhor ocupação de GPU**: um dispatch de 1.000.000 partículas usa a GPU mais eficientemente do que 100 dispatches de 10.000 cada.

---

## 9. Ordenação de Partículas Transparentes

Partículas com alpha blending requerem renderização **back-to-front** (de trás para frente em relação à câmera). Sem ordenação, a composição de alpha está incorreta.

### 9.1 Quando ordenar é necessário

- Partículas com `color.a < 1.0` que usam `GL_ONE_MINUS_SRC_ALPHA` blending
- Partículas que se interpenetram visualmente

### 9.2 Quando NÃO ordenar (additive blending)

Muitos efeitos de partícula (fogo, fumaça brilhante, energia) usam **additive blending** (`GL_ONE, GL_ONE`). Com additive blending, a ordem não importa — a soma de cores é comutativa. Sempre prefira additive blending para partículas de efeito quando artisticamente possível; elimina o custo de ordenação.

### 9.3 Radix Sort para ordenação obrigatória

Para partículas com alpha blending, um **Radix Sort** em O(N) é a única abordagem viável acima de 10k elementos. Bubble sort, insertion sort e quicksort são todos O(N log N) ou piores.

O Radix Sort de 32 bits funciona em 4 passes de 8 bits cada:

```
Input:  depth_values[N] = dot(position[i] - cameraPos, cameraForward)
        // Distância da câmera por partícula — scalar, calculado no simulate

Pass 0: sort por bits 0..7   (LSB)
Pass 1: sort por bits 8..15
Pass 2: sort por bits 16..23
Pass 3: sort por bits 24..31 (MSB)

Output: sorted_indices[N] — índices das partículas ordenadas back-to-front
```

Com SoA, apenas um array de floats (`depth`) precisa ser ordenado. Os outros arrays são reordenados indiretamente via `sorted_indices` no shader.

Para a POC, um radix sort em CPU para emitters até 50k é aceitável. Para produção, existe implementação de GPU Radix Sort em compute shaders como o **GPUSorting** (biblioteca open-source usada pela Unity VFX Graph).

---

## 10. Profiling e Métricas

### 10.1 Métricas obrigatórias no StatsPanel

| Métrica | Como medir |
|---|---|
| Partículas vivas total | `sum(pool.active_count)` para todos os emitters |
| Draw calls por frame | Contador incrementado no Renderer |
| GPU frame time (ms) | `GL_TIME_ELAPSED` via `glGenQueries` + `glBeginQuery` |
| CPU simulate time (ms) | `std::chrono::high_resolution_clock` no ParticleSystem::update |
| Upload bandwidth (MB/s) | `active_count × stride_bytes × 60` (estimativa) |
| Cache miss rate | Via perf/VTune/Nsight — não exibido na UI, usado offline |

### 10.2 GPU Timer Queries

```cpp
// Medir o tempo de execute do compute + draw na GPU
GLuint query;
glGenQueries(1, &query);
glBeginQuery(GL_TIME_ELAPSED, query);

// ... dispatch compute, draw call ...

glEndQuery(GL_TIME_ELAPSED);

// Leitura assíncrona (não bloqueia o frame atual)
GLuint64 elapsed_ns = 0;
glGetQueryObjectui64v(query, GL_QUERY_RESULT_NO_WAIT, &elapsed_ns);
float elapsed_ms = elapsed_ns / 1e6f;
```

### 10.3 Thresholds de alerta

| Condição | Ação recomendada |
|---|---|
| GPU frame time > 8ms (para 60fps) | Reduzir `max_particles`, investigar shader |
| CPU simulate > 2ms | Verificar se SoA está ativo, checar auto-vetorização |
| Upload > 50MB/s | Migrar para Persistent Mapped Buffer |
| Draw calls > 10 por frame | Implementar pool global unificado |

---

## 11. Roadmap de Evolução

A sequência abaixo respeita a relação custo/benefício documentada na análise comparativa e deve ser seguida em ordem. Não pule fases.

### Fase 1 — Fundação (POC atual → ~50k partículas @ 60fps)

**O que implementar:**

- Converter `ParticlePool` de AoS para SoA completo (seção 2.2)
- Mover o pool para componente ECS nativo `ParticlePoolComponent` (seção 4.1)
- Mover `emission_accumulator` para dentro do `ParticlePoolComponent`
- Eliminar `std::unordered_map<Entity, ParticlePool>` do `ParticleSystem`

**Resultado esperado:** 3–5× melhora em throughput de simulação. Ciclo de vida correto. Eliminação de mapas paralelos desincronizados.

---

### Fase 2 — Upload Zero-Copy (~50k–200k partículas @ 60fps)

**O que implementar:**

- Substituir `glBufferSubData` por `GL_MAP_PERSISTENT_BIT` + `GL_MAP_COHERENT_BIT`
- Adicionar fence sync por emitter (`glFenceSync` / `glClientWaitSync`)
- Mover a simulação para escrever diretamente no ponteiro mapeado

**Resultado esperado:** eliminação do maior custo de bandwidth CPU→GPU. Redução de ~30% no frame time para emitters grandes.

---

### Fase 3 — Compute Shaders (~200k–2M partículas @ 60fps)

**O que implementar:**

- `particle_simulate.comp` — simulação paralela de todos os slots ativos (seção 6.3)
- `particle_compact.comp` — stream compaction para manter contiguidade (seção 6.4)
- Migrar `position`, `velocity`, `life`, `color` para SSBOs
- Substituir draw call convencional por `glDrawArraysIndirect` (seção 7)
- Atomic counter para `active_count` na GPU

**Resultado esperado:** CPU quase completamente livre do hot path de partículas. 10–50× de melhora para emitters de 200k+.

---

### Fase 4 — Pool Global (~2M+ partículas @ 60fps)

**O que implementar:**

- Pool global de N = 1.000.000 slots compartilhado por todos os emitters
- Alocador de ranges (linear ou free-list) no `ParticleManager`
- Um único dispatch de compute + um único draw call por frame

**Resultado esperado:** eliminação do overhead de N draw calls e N dispatches. Máxima ocupação de GPU.

---

### Fase 5 — Ordenação e Features Avançadas (opcional, pós-MVP)

**O que implementar:**

- GPU Radix Sort para partículas transparentes (seção 9.3)
- Sub-emitters (partículas que emitem outras partículas ao morrer)
- Collision response simples via depth buffer sampling no compute shader
- LOD de partículas (distância → reduz active_count de emitters distantes)

---

## 12. Referências

| Fonte | Relevância |
|---|---|
| **Christer Ericson — GDC 2008, "Data Oriented Design"** | Fundamento teórico de SoA vs AoS e cache efficiency |
| **Humus — GPU Particles (2015)** | Implementação de referência de compute-driven particles com indirect draw |
| **OpenGL Wiki — Persistent Mapping** | Especificação e exemplos de `GL_MAP_PERSISTENT_BIT` |
| **Unity VFX Graph — Technical Deep Dive** | Arquitetura de pool global e GPU-driven simulation em produção |
| **Unreal Niagara — GDC 2019** | Pool hierárquico, LOD de partículas, compute dispatch strategy |
| **GPU Pro 7 — Particle Simulation using GLSL** | Radix sort em compute, prefix sum paralelo |
| **Intel Optimization Manual** | Alinhamento de memória para AVX2, latências de instruções SIMD |
| **Análise Comparativa ParticleGL (particle_comparison.md)** | Baseline das três abordagens do projeto, benchmarks de referência |

---

> **Versão deste documento:** 1.0  
> **Última atualização:** Março 2026  
> **Escopo:** ParticleGL POC/MVP — OpenGL 4.3 Core Profile, C++17  
> **Próxima revisão:** ao completar a Fase 2 do roadmap
