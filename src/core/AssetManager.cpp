#include "AssetManager.hpp"
#include "Logger.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

namespace ParticleGL::Core {

std::unordered_map<std::string, std::shared_ptr<Renderer::Shader>>
    AssetManager::shaders_;
std::unordered_map<std::string, std::shared_ptr<Renderer::Material>>
    AssetManager::materials_;
std::unordered_map<std::string, std::shared_ptr<Renderer::Mesh>>
    AssetManager::meshes_;

std::shared_ptr<Renderer::Mesh> AssetManager::default_mesh_ = nullptr;
std::shared_ptr<Renderer::Material> AssetManager::default_material_ = nullptr;

void AssetManager::init() {
  PGL_INFO("AssetManager initialized.");

  // Default Unlit Shader
  std::string vertexSrc = R"(
        #version 430 core
        layout (location = 0) in vec3 aPos;

        uniform mat4 u_ViewProjection;
        uniform mat4 u_Transform;

        void main() {
            gl_Position = u_ViewProjection * u_Transform * vec4(aPos, 1.0);
        }
    )";
  std::string fragmentSrc = R"(
        #version 430 core

        uniform vec4 u_BaseColor;
        out vec4 FragColor;

        void main() {
            FragColor = u_BaseColor;
        }
    )";

  auto unlitShader = std::make_shared<Renderer::Shader>(vertexSrc, fragmentSrc);
  addShader("default_unlit", unlitShader);

  // Default Material
  default_material_ =
      std::make_shared<Renderer::Material>("default", unlitShader);
  addMaterial("default", default_material_);

  // Default Triangle Mesh (fallback)
  std::vector<float> vertices = {-0.5f, -0.5f, 0.0f, 0.5f, -0.5f,
                                 0.0f,  0.0f,  0.5f, 0.0f};
  std::vector<uint32_t> indices = {0, 1, 2};
  default_mesh_ = std::make_shared<Renderer::Mesh>(vertices, indices,
                                                   std::vector<uint32_t>{3});
  addMesh("default_triangle", default_mesh_);
}

void AssetManager::shutdown() {
  shaders_.clear();
  materials_.clear();
  meshes_.clear();
  default_mesh_.reset();
  default_material_.reset();
}

std::shared_ptr<Renderer::Shader>
AssetManager::getShader(const std::string &name) {
  auto it = shaders_.find(name);
  return it != shaders_.end() ? it->second : nullptr;
}

void AssetManager::addShader(const std::string &name,
                             std::shared_ptr<Renderer::Shader> shader) {
  shaders_[name] = shader;
}

std::shared_ptr<Renderer::Material>
AssetManager::getMaterial(const std::string &name) {
  auto it = materials_.find(name);
  return it != materials_.end() ? it->second : default_material_;
}

void AssetManager::addMaterial(const std::string &name,
                               std::shared_ptr<Renderer::Material> material) {
  materials_[name] = material;
}

std::shared_ptr<Renderer::Material> AssetManager::getDefaultMaterial() {
  return default_material_;
}

std::shared_ptr<Renderer::Mesh> AssetManager::getMesh(const std::string &path) {
  if (path.empty()) {
    return default_mesh_;
  }

  auto it = meshes_.find(path);
  if (it != meshes_.end()) {
    return it->second;
  }

  auto mesh = loadMeshFromFile(path);
  if (mesh) {
    meshes_[path] = mesh;
    return mesh;
  }

  PGL_ERROR("Failed to load mesh: " << path << ". Using default triangle.");
  return default_mesh_;
}

void AssetManager::addMesh(const std::string &name,
                           std::shared_ptr<Renderer::Mesh> mesh) {
  meshes_[name] = mesh;
}

std::shared_ptr<Renderer::Mesh> AssetManager::getDefaultMesh() {
  return default_mesh_;
}

std::shared_ptr<Renderer::Mesh>
AssetManager::loadMeshFromFile(const std::string &path) {
  PGL_INFO("Loading mesh from: " << path);

  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string warn, err;

  if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
                        path.c_str())) {
    PGL_ERROR("TinyObjLoader failed for '" << path << "': " << err);
    return nullptr;
  }

  if (!warn.empty()) {
    PGL_INFO("TinyObjLoader warning for '" << warn << "'");
  }

  std::vector<float> vertices;
  std::vector<uint32_t> indices;

  for (const auto &shape : shapes) {
    for (const auto &index : shape.mesh.indices) {
      vertices.push_back(attrib.vertices[3 * index.vertex_index + 0]);
      vertices.push_back(attrib.vertices[3 * index.vertex_index + 1]);
      vertices.push_back(attrib.vertices[3 * index.vertex_index + 2]);
      indices.push_back(static_cast<uint32_t>(indices.size()));
    }
  }

  PGL_INFO("Loaded mesh '" << path << "': " << vertices.size() / 3 << " verts");
  return std::make_shared<Renderer::Mesh>(vertices, indices,
                                          std::vector<uint32_t>{3});
}

} // namespace ParticleGL::Core
