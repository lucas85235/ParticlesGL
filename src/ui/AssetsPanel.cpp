#include "AssetsPanel.hpp"

#include "../ecs/components/Renderable.hpp"
#include <imgui.h>

#include <algorithm>
#include <cstring>

namespace ParticleGL::UI {

void AssetsPanel::setAssetsRoot(const std::string &root) {
  root_ = root;
  refresh();
}

void AssetsPanel::refresh() {
  entries_.clear();
  if (!std::filesystem::exists(root_)) {
    return;
  }
  scanDirectory(root_);
  std::sort(entries_.begin(), entries_.end(),
            [](const AssetEntry &a, const AssetEntry &b) {
              return a.path.filename() < b.path.filename();
            });
}

void AssetsPanel::scanDirectory(const std::filesystem::path &dir) {
  for (const auto &entry : std::filesystem::recursive_directory_iterator(dir)) {
    if (!entry.is_regular_file()) {
      continue;
    }
    std::string ext = entry.path().extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    AssetEntry ae;
    ae.path = entry.path();
    ae.type = extensionToType(ext);
    entries_.push_back(ae);
  }
}

AssetType AssetsPanel::extensionToType(const std::string &ext) {
  if (ext == ".obj") {
    return AssetType::Mesh;
  }
  return AssetType::Unknown;
}

const char *AssetsPanel::assetTypeIcon(AssetType type) {
  switch (type) {
  case AssetType::Mesh:
    return "[M]";
  default:
    return "[?]";
  }
}

const char *AssetsPanel::assetTypeName(AssetType type) {
  switch (type) {
  case AssetType::Mesh:
    return "Meshes";
  default:
    return "Other";
  }
}

std::optional<std::string> AssetsPanel::consumeSelectedMeshPath() {
  auto result = pending_mesh_path_;
  pending_mesh_path_.reset();
  return result;
}

void AssetsPanel::onImGuiRender() {
  ImGui::Begin("Assets");

  // Toolbar
  if (ImGui::Button("Refresh")) {
    refresh();
  }
  ImGui::SameLine();
  ImGui::TextDisabled("%zu file(s)", entries_.size());

  ImGui::Separator();

  // Group by type using a simple tree
  auto renderGroup = [&](AssetType filterType) {
    bool hasAny = std::any_of(
        entries_.begin(), entries_.end(),
        [filterType](const AssetEntry &e) { return e.type == filterType; });
    if (!hasAny) {
      return;
    }

    const char *groupName = assetTypeName(filterType);
    bool open = ImGui::TreeNodeEx(groupName, ImGuiTreeNodeFlags_DefaultOpen);
    if (!open) {
      return;
    }

    for (const auto &entry : entries_) {
      if (entry.type != filterType) {
        continue;
      }

      std::string filename = entry.path.filename().string();
      std::string relPath = entry.path.string();

      bool isSelected = selected_path_ == relPath;
      std::string label =
          std::string(assetTypeIcon(entry.type)) + "  " + filename;

      if (ImGui::Selectable(label.c_str(), isSelected,
                            ImGuiSelectableFlags_AllowDoubleClick)) {
        selected_path_ = relPath;

        if (ImGui::IsMouseDoubleClicked(0)) {
          // Assign immediately to Renderable on selected entity
          if (registry_ && selected_entity_ && entry.type == AssetType::Mesh) {
            if (registry_->hasComponent<ECS::Components::Renderable>(
                    *selected_entity_)) {
              auto &r = registry_->getComponent<ECS::Components::Renderable>(
                  *selected_entity_);
              r.meshPath = relPath;
            }
            // Also expose via consume so InspectorPanel can pick it up
            pending_mesh_path_ = relPath;
          }
        }
      }

      // Tooltip showing full path on hover
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", relPath.c_str());
        ImGui::BeginTooltip();
        ImGui::Text("Path: %s", relPath.c_str());
        ImGui::Text("Type: %s", assetTypeName(entry.type));
        if (entry.type == AssetType::Mesh) {
          ImGui::TextDisabled("Double-click to assign to selected entity");
        }
        ImGui::EndTooltip();
      }
    }

    ImGui::TreePop();
  };

  renderGroup(AssetType::Mesh);
  renderGroup(AssetType::Unknown);

  ImGui::End();
}

} // namespace ParticleGL::UI
