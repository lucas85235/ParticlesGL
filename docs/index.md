# ParticleGL Engine Overview

Welcome to the **ParticleGL** documentation! ParticleGL is a custom 3D graphics engine built in C++17, focused on a modern Entity Component System (ECS), instance-based particle rendering, and providing an integrated real-time Editor UI.

## Core Architecture

The engine is primarily structured around the `ParticleGL::Core::Application` class, which serves as the main entry point and orchestrates the core loop. The loop is responsible for:
1. Simulating logic (e.g., ticking the `ParticleSystem`).
2. Rendering the active Scene into a `Framebuffer`.
3. Drawing the Editor UI using Dear ImGui over the main window.
4. Handling OS events via the `Window` class (GLFW wrapping).

## Subsystems

The codebase is divided into clear and specialized modules:
- **[Core](index.md)**: Bootstrapping, Logging, OS Windowing, and `AssetManager` resource handling.
- **[ECS](ecs.md)**: A custom-built sparse-set Entity Component System handling `Transform`, `Renderable`, and `ParticleEmitter` logic.
- **[Renderer](renderer.md)**: An OpenGL hardware abstraction layer (`Renderer`, `Shader`, `Mesh`, `Framebuffer`, `Material`).
- **[Particles](particles.md)**: High-performance instanced particle rendering and CPU-based simulation.
- **[Editor & Serialization](editor.md)**: Dear ImGui-powered panels (`ScenePanel`, `InspectorPanel`, `MaterialsPanel`) that allow real-time manipulation of ECS states and JSON persistence.

## Dependencies
- **OpenGL 4.3+** for rendering
- **GLFW** for window creation and input handling
- **GLAD** for OpenGL function loading
- **GLM** for matrix mathematics
- **Dear ImGui** (docking branch) for the editor interface
- **nlohmann/json** for scene and component serialization
- **tinyobjloader** for `.obj` model parsing
- **GoogleTest** for TDD coverage of the ECS and rendering systems
