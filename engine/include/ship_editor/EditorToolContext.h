#pragma once

#include <cstdint>

namespace subspace {

/// Active tool mode in the editor (mirrors NovaForge EditorToolMode).
enum class EditorToolMode : uint8_t {
    Select = 0,
    Place,
    Remove,
    Paint,
    Inspect
};

/// Returns a display name string for the given tool mode.
inline const char* EditorToolModeName(EditorToolMode mode) {
    switch (mode) {
        case EditorToolMode::Select:  return "Select";
        case EditorToolMode::Place:   return "Place";
        case EditorToolMode::Remove:  return "Remove";
        case EditorToolMode::Paint:   return "Paint";
        case EditorToolMode::Inspect: return "Inspect";
        default: return "Unknown";
    }
}

/// Shared tool state for the editor, updated by input handler and read by panels.
struct EditorToolContext {
    EditorToolMode activeMode = EditorToolMode::Select;
    bool cameraNavigationActive = false;
    bool worldDirty = false;
    int selectedBlockTypeIndex = 0;
    int selectedMaterialIndex = 0;
};

} // namespace subspace
