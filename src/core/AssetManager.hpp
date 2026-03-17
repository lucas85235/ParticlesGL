#pragma once

#include "../renderer/Material.hpp"
#include "../renderer/Mesh.hpp"
#include "../renderer/Shader.hpp"
#include <memory>
#include <string>
#include <unordered_map>

namespace ParticleGL::Core {

class AssetManager {
public:
  static void init();
  static void shutdown();

  // Shaders
  static std::shared_ptr<Renderer::Shader> getShader(const std::string &name);
  static void addShader(const std::string &name,
                        std::shared_ptr<Renderer::Shader> shader);

  // Materials
  static std::shared_ptr<Renderer::Material>
  getMaterial(const std::string &name);
  static void addMaterial(const std::string &name,
                          std::shared_ptr<Renderer::Material> material);
  static std::shared_ptr<Renderer::Material> getDefaultMaterial();

  // Meshes
  static std::shared_ptr<Renderer::Mesh> getMesh(const std::string &path);
  static void addMesh(const std::string &name,
                      std::shared_ptr<Renderer::Mesh> mesh);
  static std::shared_ptr<Renderer::Mesh> getDefaultMesh();

private:
  static std::unordered_map<std::string, std::shared_ptr<Renderer::Shader>>
      shaders_;
  static std::unordered_map<std::string, std::shared_ptr<Renderer::Material>>
      materials_;
  static std::unordered_map<std::string, std::shared_ptr<Renderer::Mesh>>
      meshes_;

  static std::shared_ptr<Renderer::Mesh> default_mesh_;
  static std::shared_ptr<Renderer::Material> default_material_;

  static std::shared_ptr<Renderer::Mesh>
  loadMeshFromFile(const std::string &path);
};

} // namespace ParticleGL::Core
