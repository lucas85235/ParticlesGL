#pragma once

#include "../ecs/Registry.hpp"
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace ParticleGL::UI {

// Categories of assets we know about
enum class AssetType { Mesh, Unknown };

struct AssetEntry {
  std::filesystem::path path;
  AssetType type;
};

class AssetsPanel {
public:
  AssetsPanel() = default;
  ~AssetsPanel() = default;

  void setAssetsRoot(const std::string &root);
  void setRegistry(ECS::Registry *registry) { registry_ = registry; }
  void setSelectedEntity(std::optional<ECS::Entity> entity) {
    selected_entity_ = entity;
  }

  // Panels call this to check if a mesh was just double-clicked
  std::optional<std::string> consumeSelectedMeshPath();

  void onImGuiRender();
  void refresh();

private:
  std::filesystem::path root_;
  ECS::Registry *registry_ = nullptr;
  std::optional<ECS::Entity> selected_entity_;

  std::vector<AssetEntry> entries_;
  std::string selected_path_;
  std::optional<std::string> pending_mesh_path_;

  void scanDirectory(const std::filesystem::path &dir);
  static AssetType extensionToType(const std::string &ext);
  static const char *assetTypeIcon(AssetType type);
  static const char *assetTypeName(AssetType type);
};

} // namespace ParticleGL::UI
