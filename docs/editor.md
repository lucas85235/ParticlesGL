# Editor UI & Serialization

ParticleGL includes an integrated development environment (IDE)-like Editor wrapped seamlessly into the engine runtime through **Dear ImGui**.

## UI Layout

The `UILayer` (`src/ui/UILayer.hpp`) manages context and frames. The application takes advantage of the ImGui docking branch, registering a core `DockSpace` spanning the main Window.

By default, the layout generates 5 fundamental panels:
1. **Scene Panel**: Lists active ECS entities recursively. Clicking on an Entity selects it, broadcasting its index to all other systems. 
2. **Assets Panel**: Rebuilds an internal tree view of the files from the disk (specifically `.obj`). Intuitively allows the user to double-click an asset to auto-assign the Mesh to the current Entity.
3. **Inspector Panel**: Exposes raw ECS component logic (`Transform`, `ParticleEmitter`, `Renderable`). Values edited here instantaneously push changes safely into memory.
4. **Materials Panel**: The central hub for defining rendering shaders and properties (colors). Lets you create new materials and swap shaders effortlessly.
5. **Stats Panel / Viewport Panel**: Monitors frame execution timing. The `ViewportPanel` effectively mounts the `Framebuffer` texture as an Image, separating the user workspace from the final rendered Game view.

## JSON Serialization

ParticleGL saves and fully recovers scenes natively using `nlohmann::json`.
- `ComponentSerializer.hpp`: Defines standalone mapping functions translating struct properties directly to/from generic JSON maps. 
- `SceneSerializer.hpp`: Operates iteratively. It dumps all standard `Transforms` and `ParticleEmitters`, alongside preserving dynamically created `Materials` contained exclusively within the `AssetManager`. Upon reload (`deserialize()`), it first recreates the materials before assigning them, ensuring consistent scene loading with purely unique identifiers!
