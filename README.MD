# ⚡ ParticleGL — POC/MVP de Sistema de Partículas com OpenGL

> Documento de regras, arquitetura e diretrizes para agentes de IA contribuindo neste projeto.  
> **Nenhum código deve ser gerado sem que as regras aqui definidas sejam seguidas.**

---

## Índice

1. [Visão Geral do Projeto](#1-visão-geral-do-projeto)
2. [Objetivos da POC/MVP](#2-objetivos-da-pocmvp)
3. [Stack Tecnológica](#3-stack-tecnológica)
4. [Estrutura de Pastas](#4-estrutura-de-pastas)
5. [Arquitetura — ECS](#5-arquitetura--ecs)
6. [Sistema de Partículas com Instancing](#6-sistema-de-partículas-com-instancing)
7. [Interface (UI em tempo real)](#7-interface-ui-em-tempo-real)
8. [Serialização de Estado](#8-serialização-de-estado)
9. [TDD — Test Driven Development](#9-tdd--test-driven-development)
10. [Padrões de Código](#10-padrões-de-código)
11. [Fluxo de Contribuição do Agente](#11-fluxo-de-contribuição-do-agente)
12. [O que NÃO deve ser feito](#12-o-que-não-deve-ser-feito)
13. [Glossário](#13-glossário)

---

## 1. Visão Geral do Projeto

**ParticleGL** é um renderizador em OpenGL desenvolvido como **POC/MVP** de um sistema de partículas de alta performance que utiliza **GPU Instancing** para renderizar milhares de partículas com um único draw call.

O projeto tem três pilares fundamentais:

- **Renderização** — OpenGL moderno (core profile) com instancing via `glDrawArraysInstanced` / `glDrawElementsInstanced`
- **ECS (Entity Component System)** — arquitetura orientada a dados, desacoplada e extensível
- **Interface em tempo real** — painel de inspeção e edição de propriedades dos componentes via Dear ImGui, com suporte a serialização/desserialização do estado da cena

---

## 2. Objetivos da POC/MVP

### ✅ Deve ser entregue na POC
- Renderização de partículas via instancing (posição, cor, escala, lifetime por instância)
- Loop principal com update e render separados
- ECS funcional com pelo menos os componentes: `Transform`, `ParticleEmitter`, `Renderable`, `Lifetime`
- Interface ImGui com painel de inspeção de entidades e componentes
- Serialização e desserialização de cena em JSON
- Edição de propriedades em tempo real com reflexão imediata na renderização

### 🚫 Fora de escopo para a POC
- Física complexa (colisões, fluidos)
- Suporte a múltiplos renderers (Vulkan, Metal)
- Editor de shaders em tempo real
- Networking / multiplayer
- Exportação de vídeo/screenshot

---

## 3. Stack Tecnológica

| Camada | Tecnologia | Justificativa |
|---|---|---|
| Linguagem | **C++17** | Compatibilidade ampla, performance, suporte moderno a templates e `std::optional` / `std::variant` |
| OpenGL | **OpenGL 4.3 Core Profile** | Suporte a instancing, compute shaders e SSBOs |
| Carregamento de OpenGL | **GLAD** (gerado para GL 4.3) | Leve, sem dependência em runtime |
| Janela & Contexto | **GLFW 3.x** | Portável, simples, sem overhead |
| Matemática | **GLM** | Padrão de mercado, alinhada com GLSL |
| Interface | **Dear ImGui** (docking branch) | Leve, sem dependência em estilo, integra com OpenGL/GLFW via backends oficiais |
| Serialização | **nlohmann/json** (header-only) | Simples, legível, amplamente adotado |
| Build System | **CMake 3.20+** | Portável, suporte a FetchContent para dependências |
| Testes | **Google Test (gtest)** | Maduro, suportado por todos os CIs |
| CI | **GitHub Actions** | Gratuito, integrado ao repositório |
| Gerenciamento de dependências | **CMake FetchContent** (ou vcpkg como alternativa) | Reprodutível, sem necessidade de submodules manuais |

> **Regra:** Nenhuma dependência nova pode ser adicionada sem atualizar este documento e justificar a escolha na seção de Stack.

---

## 4. Estrutura de Pastas

```
ParticleGL/
│
├── CMakeLists.txt                  # Build raiz
├── README.md                       # Este arquivo
├── .github/
│   └── workflows/
│       └── ci.yml                  # Pipeline de CI
│
├── cmake/
│   └── dependencies.cmake          # FetchContent das libs externas
│
├── assets/
│   ├── shaders/
│   │   ├── particle.vert           # Vertex shader com instancing
│   │   ├── particle.frag           # Fragment shader
│   │   └── quad.vert               # Shader auxiliar de UI/debug
│   └── scenes/
│       └── default.json            # Cena padrão serializada
│
├── src/
│   ├── main.cpp                    # Ponto de entrada — apenas bootstrap
│   │
│   ├── core/
│   │   ├── Application.hpp/.cpp    # Loop principal, init, shutdown
│   │   ├── Window.hpp/.cpp         # Wrapper GLFW
│   │   └── Logger.hpp              # Logging simples (macros)
│   │
│   ├── ecs/
│   │   ├── Registry.hpp/.cpp       # Gerenciador de entidades e componentes
│   │   ├── Entity.hpp              # Typedef de ID de entidade (uint32_t)
│   │   ├── ComponentPool.hpp       # Pool genérico por tipo de componente
│   │   ├── System.hpp              # Interface base de System
│   │   └── components/
│   │       ├── Transform.hpp
│   │       ├── ParticleEmitter.hpp
│   │       ├── Renderable.hpp
│   │       └── Lifetime.hpp
│   │
│   ├── renderer/
│   │   ├── Renderer.hpp/.cpp       # Orquestra os draw calls
│   │   ├── Shader.hpp/.cpp         # Compilação e binding de shaders
│   │   ├── InstanceBuffer.hpp/.cpp # VBO de dados por instância
│   │   ├── Mesh.hpp/.cpp           # VAO/VBO de geometria base
│   │   └── Camera.hpp/.cpp         # View/Projection matrix
│   │
│   ├── particles/
│   │   ├── ParticleSystem.hpp/.cpp # System ECS de update de partículas
│   │   ├── Emitter.hpp/.cpp        # Lógica de emissão (spawn, lifetime)
│   │   └── ParticleData.hpp        # Struct de dados por instância (POD)
│   │
│   ├── ui/
│   │   ├── UILayer.hpp/.cpp        # Init ImGui, begin/end frame
│   │   ├── InspectorPanel.hpp/.cpp # Painel de inspeção de entidade
│   │   ├── ScenePanel.hpp/.cpp     # Lista de entidades da cena
│   │   └── StatsPanel.hpp/.cpp     # FPS, contagem de partículas, GPU ms
│   │
│   └── serialization/
│       ├── SceneSerializer.hpp/.cpp  # Salva/carrega cena em JSON
│       └── ComponentSerializer.hpp  # Funções to_json/from_json por componente
│
└── tests/
    ├── CMakeLists.txt
    ├── ecs/
    │   ├── test_registry.cpp
    │   ├── test_component_pool.cpp
    │   └── test_system.cpp
    ├── particles/
    │   ├── test_emitter.cpp
    │   └── test_particle_lifetime.cpp
    └── serialization/
        ├── test_scene_serializer.cpp
        └── test_component_serializer.cpp
```

> **Regra:** O agente nunca deve criar arquivos fora desta hierarquia sem antes documentar a nova entrada aqui e descrever sua responsabilidade.

---

## 5. Arquitetura — ECS

### Princípios

O ECS segue o modelo **data-oriented**: componentes são dados puros (POD ou quase-POD), sistemas contêm toda a lógica, entidades são apenas IDs inteiros.

### Responsabilidades

| Elemento | Responsabilidade |
|---|---|
| `Entity` | `uint32_t` — identificador único. Sem métodos, sem dados. |
| `Component` | Struct de dados pura. Sem lógica de negócio. |
| `System` | Itera sobre entidades com determinados componentes e aplica lógica. |
| `Registry` | Cria/destrói entidades, armazena component pools, fornece views de iteração. |
| `ComponentPool<T>` | Armazenamento contíguo em memória para componentes do tipo T. |

### Regras do ECS

1. **Componentes nunca contêm métodos de lógica** — apenas dados e, quando necessário, helpers matemáticos triviais (ex: `Transform::matrix()`).
2. **Sistemas nunca armazenam estado de cena** — todo estado pertence aos componentes registrados na `Registry`.
3. **A `Registry` é a única fonte de verdade** sobre quais entidades existem e quais componentes possuem.
4. **Sistemas devem declarar explicitamente seus componentes requeridos** — isso facilita paralelização futura e testes.
5. **IDs de entidade nunca são reutilizados** na mesma sessão — use geração monotônica com lista de IDs livres para sessões longas.

### Componentes obrigatórios para a POC

```
Transform        — posição (vec3), rotação (quat), escala (vec3)
ParticleEmitter  — taxa de emissão, velocidade inicial, spread, cor inicial/final
Renderable       — referência ao mesh e shader a usar
Lifetime         — tempo de vida atual, tempo de vida máximo, ativo (bool)
```

---

## 6. Sistema de Partículas com Instancing

### Conceito

Todas as partículas de um emitter compartilham **a mesma geometria base** (ex: um quad unitário). A diferenciação entre instâncias ocorre via **Instance Buffer**: um VBO separado que carrega dados por instância (posição, cor, escala, opacidade) e é atualizado no CPU a cada frame antes do draw call.

### Fluxo de renderização por frame

```
[ParticleSystem::update(dt)]
  └─ Itera entidades com ParticleEmitter + Lifetime
      └─ Spawna/destrói partículas conforme lifetime e taxa de emissão
      └─ Atualiza posição, cor, escala por interpolação de tempo de vida

[Renderer::render()]
  └─ Para cada emitter ativo:
      └─ Faz upload do InstanceBuffer atualizado para a GPU (glBufferSubData)
      └─ Executa glDrawArraysInstanced com N = partículas vivas
```

### Regras do sistema de partículas

1. **Nenhuma alocação heap deve ocorrer no loop de update das partículas** — use pools pré-alocados com tamanho máximo configurável.
2. **O tamanho máximo do pool de partículas por emitter deve ser configurável** via componente `ParticleEmitter`, com default de 10.000.
3. **A transferência CPU→GPU deve usar `glBufferSubData`**, nunca recriar o buffer a cada frame.
4. **Partículas mortas não devem ser enviadas para a GPU** — mantenha um contador de partículas vivas e só faça upload do slice ativo do buffer.
5. **Dados por instância devem ser structs POD alinhadas** para garantir compatibilidade com o layout de atributos do shader.

---

## 7. Interface (UI em tempo real)

### Tecnologia: Dear ImGui (docking branch)

A UI é uma camada separada que **lê e escreve diretamente nos componentes via `Registry`**. Ela não possui lógica de simulação.

### Painéis obrigatórios para a POC

| Painel | Descrição |
|---|---|
| **Scene Panel** | Lista todas as entidades. Permite selecionar, renomear e deletar. |
| **Inspector Panel** | Exibe todos os componentes da entidade selecionada com campos editáveis. |
| **Stats Panel** | FPS, frame time, contagem de partículas vivas/total, draw calls. |
| **Emitter Controls** | Sliders e campos para taxa de emissão, velocidade, cor, spread — alterações refletidas imediatamente. |

### Regras de UI

1. **A UI nunca deve ter cópias dos dados** — ela acessa os componentes diretamente pela `Registry`.
2. **Cada componente deve fornecer uma função `drawInspector(Registry&, Entity)`** que a UI chama — isso mantém o conhecimento de como renderizar um componente próximo ao próprio componente.
3. **Alterações via UI são aplicadas imediatamente** — não há buffer de "aplicar". O que o usuário arrasta, já está no componente.
4. **A UI não deve influenciar o frame time de simulação** — se necessário, medir e logar separadamente.

---

## 8. Serialização de Estado

### Formato: JSON via nlohmann/json

Toda a cena (entidades + componentes) deve ser serializável para JSON e desserializável de volta, permitindo salvar e carregar estados.

### Estrutura do JSON de cena

```json
{
  "scene": {
    "name": "default",
    "entities": [
      {
        "id": 1,
        "name": "Emitter_01",
        "components": {
          "Transform": {
            "position": [0.0, 0.0, 0.0],
            "rotation": [0.0, 0.0, 0.0, 1.0],
            "scale": [1.0, 1.0, 1.0]
          },
          "ParticleEmitter": {
            "emission_rate": 500,
            "initial_velocity": [0.0, 5.0, 0.0],
            "spread_angle": 30.0,
            "color_start": [1.0, 0.5, 0.0, 1.0],
            "color_end": [1.0, 0.0, 0.0, 0.0],
            "max_particles": 10000
          },
          "Lifetime": {
            "max_lifetime": 2.5,
            "active": true
          }
        }
      }
    ]
  }
}
```

### Regras de serialização

1. **Todo componente adicionado ao projeto DEVE ter funções `to_json` e `from_json`** implementadas antes de ser integrado ao Inspector.
2. **A serialização de uma cena não deve depender de estado global** — apenas da `Registry` passada como parâmetro.
3. **IDs de entidade no JSON são apenas referências de arquivo** — ao carregar, novos IDs são gerados pela `Registry`.
4. **Campos desconhecidos no JSON são ignorados silenciosamente** para compatibilidade futura.
5. **A cena padrão (`assets/scenes/default.json`) deve ser mantida válida** a cada commit que altere o schema.

---

## 9. TDD — Test Driven Development

### Filosofia

O agente **deve escrever o teste antes da implementação** em toda nova funcionalidade. Exceções são aceitas apenas para código de glue (main, init de janela, ImGui bootstrap) onde testes de unidade não fazem sentido prático.

### Cobertura mínima exigida

| Módulo | Cobertura mínima |
|---|---|
| `ecs/` | **90%** de cobertura de linhas |
| `particles/` | **85%** |
| `serialization/` | **95%** |
| `renderer/` | Sem cobertura obrigatória (depende de GPU) — testar apenas lógica pura |
| `ui/` | Sem cobertura obrigatória — apenas smoke tests se possível |

### Regras de TDD

1. **Nenhum PR / commit de feature pode ser aberto sem testes correspondentes.**
2. **Testes devem ser nomeados no formato `test_<módulo>_<comportamento_esperado>`**, ex: `test_registry_entity_destruction_releases_components`.
3. **Testes não devem depender de contexto OpenGL** — se uma classe precisar de contexto para ser testada, ela está mal projetada e deve ser refatorada para separar lógica de GPU.
4. **Testes de serialização devem cobrir**: round-trip (serialize → deserialize → compare), campos ausentes, campos com tipo errado.
5. **Mocks são permitidos** somente para dependências externas (GPU, sistema de arquivos). Use fakes simples, não frameworks de mock pesados.
6. **O CI deve falhar se qualquer teste falhar** — não há "testes opcionais" no pipeline.

### Executando os testes

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
cmake --build build
cd build && ctest --output-on-failure
```

---

## 10. Padrões de Código

### Nomenclatura

| Elemento | Convenção | Exemplo |
|---|---|---|
| Classes e Structs | `PascalCase` | `ParticleEmitter`, `InstanceBuffer` |
| Funções e métodos | `camelCase` | `updateParticles()`, `drawInspector()` |
| Variáveis locais | `snake_case` | `particle_count`, `delta_time` |
| Membros privados | `snake_case` com sufixo `_` | `buffer_id_`, `entity_count_` |
| Constantes / enum values | `SCREAMING_SNAKE_CASE` | `MAX_PARTICLES`, `DEFAULT_LIFETIME` |
| Arquivos `.hpp`/`.cpp` | `PascalCase` | `ParticleSystem.hpp` |
| Pastas | `lowercase` | `ecs/`, `renderer/`, `ui/` |

### Regras gerais

1. **C++17 apenas** — proibido usar features de C++20 ou superior nesta POC.
2. **Sem raw `new`/`delete`** — use `std::unique_ptr`, `std::shared_ptr` ou containers padrão.
3. **Sem includes desnecessários** — cada arquivo inclui apenas o que usa diretamente.
4. **Headers devem ter include guards** (`#pragma once` é aceito).
5. **Funções com mais de 40 linhas devem ser refatoradas** — se necessário, justificar no PR.
6. **Sem números mágicos no código** — use constantes nomeadas ou parâmetros de componente.
7. **Todo `TODO` ou `FIXME` deve ter o formato `// TODO(nome): descrição`** e criar uma issue correspondente.
8. **Warnings do compilador tratados como erros** — compilar com `-Wall -Wextra -Werror` no CI.

### Organização de headers

Ordem de includes obrigatória (separados por linha em branco):

```cpp
// 1. Header próprio do arquivo (em .cpp)
#include "ParticleSystem.hpp"

// 2. Headers do projeto
#include "ecs/Registry.hpp"
#include "particles/ParticleData.hpp"

// 3. Bibliotecas de terceiros
#include <glm/glm.hpp>

// 4. STL
#include <vector>
#include <optional>
```

---

## 11. Fluxo de Contribuição do Agente

Ao receber uma tarefa, o agente de IA **deve seguir este fluxo obrigatoriamente**:

```
1. LEIA este README inteiro antes de qualquer ação.

2. IDENTIFIQUE qual módulo será tocado:
   - ECS? → siga as regras da seção 5
   - Renderer/Particles? → siga as regras das seções 6 e 10
   - UI? → siga as regras da seção 7
   - Serialização? → siga as regras da seção 8

3. ESCREVA o teste antes do código (TDD — seção 9).

4. IMPLEMENTE o código respeitando:
   - Nomenclatura (seção 10)
   - Estrutura de pastas (seção 4)
   - Sem dependências não listadas na seção 3

5. VERIFIQUE se:
   - [ ] Todos os testes passam
   - [ ] Nenhum arquivo foi criado fora da estrutura definida
   - [ ] Nenhuma nova dependência foi adicionada sem atualizar a seção 3
   - [ ] Nenhum componente novo foi criado sem serialização correspondente
   - [ ] O código compila sem warnings com -Wall -Wextra

6. DOCUMENTE qualquer decisão de design não trivial com um comentário
   no arquivo ou atualização deste README.
```

> **Regra crítica:** O agente **nunca deve gerar código que funciona mas viola as regras arquiteturais**. Código correto porém fora do padrão será rejeitado.

---

## 12. O que NÃO deve ser feito

| ❌ Proibido | ✅ Alternativa |
|---|---|
| Lógica de simulação em componentes | Mova para um `System` |
| Alocação heap no hot loop de partículas | Use pool pré-alocado |
| Acesso direto a membros de outro sistema | Use a `Registry` como intermediário |
| Copiar dados de componente para a UI | UI acessa componente via referência da `Registry` |
| Adicionar dependência sem atualizar README | Atualize a seção 3 e justifique |
| Criar arquivo fora da estrutura de pastas | Atualize a seção 4 primeiro |
| Commitar código sem testes nos módulos obrigatórios | Escreva o teste antes |
| Usar `glMapBuffer` para upload de instâncias a cada frame | Use `glBufferSubData` em buffer pré-alocado |
| Recriar VAO/VBO a cada frame | Crie uma vez, atualize apenas os dados |
| Misturar lógica de UI com lógica de simulação | Camadas estritamente separadas |

---

## 13. Glossário

| Termo | Definição no contexto deste projeto |
|---|---|
| **ECS** | Entity Component System — padrão arquitetural orientado a dados |
| **Entity** | ID inteiro (`uint32_t`) que agrupa componentes na `Registry` |
| **Component** | Struct de dados pura sem lógica de negócio |
| **System** | Classe que itera sobre entidades e aplica lógica de negócio |
| **Instancing** | Técnica OpenGL que renderiza múltiplas cópias de uma geometria com um único draw call, diferenciando-as via dados por instância |
| **Instance Buffer** | VBO que contém dados específicos por instância (posição, cor, escala) atualizados a cada frame |
| **POD** | Plain Old Data — struct C++ sem herança, virtual, ou construtores complexos; layout de memória previsível |
| **Registry** | Objeto central do ECS que gerencia entidades, component pools e fornece iteração |
| **Hot loop** | Loop de update de partículas executado milhares de vezes por frame — crítico para performance |
| **Round-trip** | Processo de serializar e desserializar um objeto verificando que o resultado é idêntico ao original |
| **POC** | Proof of Concept — demonstração mínima de viabilidade técnica |
| **MVP** | Minimum Viable Product — versão mínima funcional entregável |

---

> **Versão deste documento:** 1.0  
> **Última atualização:** Março 2026  
> **Responsável:** Definido pelo time — qualquer alteração neste README requer revisão explícita.