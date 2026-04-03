#pragma once

#include "ui/UISystem.h"
#include "ui/UIPanel.h"
#include "ui/UIElement.h"
#include "ship_editor/ShipEditorController.h"
#include "ship_editor/EditorToolContext.h"
#include "ship_editor/SelectionService.h"
#include "ship_editor/PropertyInspectorSystem.h"
#include "ship_editor/EditorCommandRegistry.h"
#include "ship_editor/ShipValidator.h"

#include <memory>
#include <string>

namespace subspace {

/// Builds and manages all editor-specific UI panels within the UISystem.
///
/// Panel layout (modeled after NovaForge docking):
///   Top:    Toolbar (mode buttons, symmetry toggles, undo/redo)
///   Left:   Block Palette (categories, block types, materials)
///   Right:  Inspector (properties of selected block/selection)
///   Bottom: Status Bar (tool mode, selection info, dirty state, block count)
///   Lower:  Validation Panel (errors/warnings from ShipValidator)
class EditorUI {
public:
    EditorUI(ShipEditorController& controller,
             EditorToolContext& toolContext,
             SelectionService& selection,
             PropertyInspectorSystem& propertyInspector,
             EditorCommandRegistry& commands,
             CommandHistory& commandHistory);

    /// Create all panels and register them with the UISystem.
    void Initialize(UISystem& uiSystem);

    /// Update all panel contents from live editor state. Call once per frame.
    void Update(float deltaTime);

    /// Rebuild panel contents when selection or properties change.
    void SyncToSelection();

    /// Rebuild validation panel from latest ShipValidator results.
    void UpdateValidation(const ValidationResult& result);

    /// Mark the world as dirty/clean (shown in status bar).
    void SetWorldDirty(bool dirty);

private:
    void BuildToolbarPanel();
    void BuildPalettePanel();
    void BuildInspectorPanel();
    void BuildStatusBarPanel();
    void BuildValidationPanel();

    void RefreshToolbar();
    void RefreshPalette();
    void RefreshInspector();
    void RefreshStatusBar();
    void RefreshValidation();

    ShipEditorController&    m_controller;
    EditorToolContext&       m_toolContext;
    SelectionService&        m_selection;
    PropertyInspectorSystem& m_propertyInspector;
    EditorCommandRegistry&   m_commands;
    CommandHistory&          m_commandHistory;

    // Panels (shared_ptr owned by UISystem, we keep raw pointers for update)
    UIPanel* m_toolbarPanel    = nullptr;
    UIPanel* m_palettePanel    = nullptr;
    UIPanel* m_inspectorPanel  = nullptr;
    UIPanel* m_statusBarPanel  = nullptr;
    UIPanel* m_validationPanel = nullptr;

    bool m_worldDirty = false;
    ValidationResult m_lastValidation;
};

} // namespace subspace
