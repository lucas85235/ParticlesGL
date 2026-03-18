# Renderer & Material System

ParticleGL abstracts modern OpenGL (4.3 Core) into an Object-Oriented hardware interface layer, facilitating dynamic 3D rendering and easy model loading.

## Rendering Abstractions

- **`Renderer`**: The central static class (`src/renderer/Renderer.hpp`) that dictates global GPU states (Depth testing, blending) and orchestrated `glDrawElements` or `glDrawElementsInstanced` calls.
- **`Shader`**: Represents a compiled and linked string of GLSL Vertex and Fragment code. Features a uniform location cache (`std::unordered_map`) for fast bindings. File-loading is done via `Shader::loadFromFile()`.
- **`Mesh`**: Holds interleaved `std::vector<float>` vertex data and `std::vector<uint32_t>` indices. It cleanly abstracts Vertex Array Objects (VAOs), Vertex Buffer Objects (VBOs), and Element Buffer Objects (EBOs).
- **`Framebuffer`**: Abstraction for off-screen rendering. The entire scene is drawn to a frame buffer texture, which is then exposed to ImGui's `ViewportPanel` for docking and resizing. Check `src/renderer/Framebuffer.hpp`.

## Material System & AssetManager

ParticleGL boasts a robust asset management and material tracking system inside `src/core/AssetManager.hpp`.

### `AssetManager`
The `AssetManager` is globally available and performs caching. It guarantees that an external disk `.obj` file or a `.vert` shader is only parsed and uploaded to the GPU once.

```cpp
auto mesh = Core::AssetManager::getMesh("assets/models/suzan.obj");
```
If the mesh is missing or fails to parse, it smoothly defaults to a fallback triangle geometry rather than crashing the engine.

### `Material`
A `Material` (`src/renderer/Material.hpp`) combines a `Shader` and specific visual properties (like `baseColor`). 
- Materials in ParticleGL are reference-counted (`std::shared_ptr`).
- Multiple entities can reference the same `Material ID` via their `Renderable` component. Editing a material in the editor immediately updates all entities assigned to it.
- Materials track their own string `id` and `shaderId` to ensure they can fully persist via JSON serialization.
