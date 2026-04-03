#include "ship_editor/EditorUI.h"
#include "ship_editor/BlockPalette.h"
#include "ship_editor/SymmetrySystem.h"
#include "ships/Ship.h"

#include <string>

namespace subspace {

namespace {

const MaterialType kAllMaterials[] = {
    MaterialType::Iron,
    MaterialType::Titanium,
    MaterialType::Naonite,
    MaterialType::Trinium,
    MaterialType::Xanion,
    MaterialType::Ogonite,
    MaterialType::Avorion
};

constexpr size_t kMaterialCount = sizeof(kAllMaterials) / sizeof(kAllMaterials[0]);

const char* MaterialTypeName(MaterialType m) {
    switch (m) {
        case MaterialType::Iron:     return "Iron";
        case MaterialType::Titanium: return "Titanium";
        case MaterialType::Naonite:  return "Naonite";
        case MaterialType::Trinium:  return "Trinium";
        case MaterialType::Xanion:   return "Xanion";
        case MaterialType::Ogonite:  return "Ogonite";
        case MaterialType::Avorion:  return "Avorion";
    }
    return "Unknown";
}

const char* BlockTypeName(BlockType t) {
    switch (t) {
        case BlockType::Hull:        return "Hull";
        case BlockType::Armor:       return "Armor";
        case BlockType::Engine:      return "Engine";
        case BlockType::Generator:   return "Generator";
        case BlockType::Gyro:        return "Gyro";
        case BlockType::Cargo:       return "Cargo";
        case BlockType::WeaponMount: return "WeaponMount";
    }
    return "Unknown";
}

const char* BlockShapeName(BlockShape s) {
    switch (s) {
        case BlockShape::Cube:   return "Cube";
        case BlockShape::Rect:   return "Rect";
        case BlockShape::Wedge:  return "Wedge";
        case BlockShape::Corner: return "Corner";
        case BlockShape::Slope:  return "Slope";
    }
    return "Unknown";
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

EditorUI::EditorUI(ShipEditorController& controller,
                   EditorToolContext& toolContext,
                   SelectionService& selection,
                   PropertyInspectorSystem& propertyInspector,
                   EditorCommandRegistry& commands,
                   CommandHistory& commandHistory)
    : m_controller(controller)
    , m_toolContext(toolContext)
    , m_selection(selection)
    , m_propertyInspector(propertyInspector)
    , m_commands(commands)
    , m_commandHistory(commandHistory)
{}

// ---------------------------------------------------------------------------
// Initialization — create panels and register with UISystem
// ---------------------------------------------------------------------------

void EditorUI::Initialize(UISystem& uiSystem) {
    // Toolbar — top strip, full width, 40px tall
    {
        auto panel = std::make_shared<UIPanel>();
        panel->SetTitle("Toolbar");
        panel->SetPosition(0.0f, 0.0f);
        panel->SetSize(1920.0f, 40.0f);
        panel->SetLayoutDirection(LayoutDirection::Horizontal);
        panel->SetPadding(4.0f);
        panel->SetSpacing(4.0f);
        panel->SetBackgroundColor(Color::DarkGray());
        m_toolbarPanel = uiSystem.AddPanel("Toolbar", panel);
    }

    // Block Palette — left side, 250px wide, below toolbar
    {
        auto panel = std::make_shared<UIPanel>();
        panel->SetTitle("Block Palette");
        panel->SetPosition(0.0f, 40.0f);
        panel->SetSize(250.0f, 910.0f);
        panel->SetLayoutDirection(LayoutDirection::Vertical);
        panel->SetPadding(8.0f);
        panel->SetSpacing(4.0f);
        panel->SetBackgroundColor(Color(0.15f, 0.15f, 0.15f, 1.0f));
        m_palettePanel = uiSystem.AddPanel("BlockPalette", panel);
    }

    // Inspector — right side, 300px wide, below toolbar
    {
        auto panel = std::make_shared<UIPanel>();
        panel->SetTitle("Inspector");
        panel->SetPosition(1620.0f, 40.0f);
        panel->SetSize(300.0f, 910.0f);
        panel->SetLayoutDirection(LayoutDirection::Vertical);
        panel->SetPadding(8.0f);
        panel->SetSpacing(4.0f);
        panel->SetBackgroundColor(Color(0.15f, 0.15f, 0.15f, 1.0f));
        m_inspectorPanel = uiSystem.AddPanel("Inspector", panel);
    }

    // Validation — above status bar, full width, 100px tall
    {
        auto panel = std::make_shared<UIPanel>();
        panel->SetTitle("Validation");
        panel->SetPosition(0.0f, 950.0f);
        panel->SetSize(1920.0f, 100.0f);
        panel->SetLayoutDirection(LayoutDirection::Vertical);
        panel->SetPadding(4.0f);
        panel->SetSpacing(2.0f);
        panel->SetBackgroundColor(Color(0.12f, 0.12f, 0.12f, 1.0f));
        m_validationPanel = uiSystem.AddPanel("Validation", panel);
    }

    // Status Bar — bottom strip, full width, 30px tall
    {
        auto panel = std::make_shared<UIPanel>();
        panel->SetTitle("Status Bar");
        panel->SetPosition(0.0f, 1050.0f);
        panel->SetSize(1920.0f, 30.0f);
        panel->SetLayoutDirection(LayoutDirection::Horizontal);
        panel->SetPadding(4.0f);
        panel->SetSpacing(16.0f);
        panel->SetBackgroundColor(Color(0.1f, 0.1f, 0.1f, 1.0f));
        m_statusBarPanel = uiSystem.AddPanel("StatusBar", panel);
    }

    // Populate initial content
    BuildToolbarPanel();
    BuildPalettePanel();
    BuildInspectorPanel();
    BuildStatusBarPanel();
    BuildValidationPanel();
}

// ---------------------------------------------------------------------------
// Per-frame update
// ---------------------------------------------------------------------------

void EditorUI::Update(float deltaTime) {
    (void)deltaTime;
    RefreshToolbar();
    RefreshStatusBar();
    if (m_selection.IsChanged()) {
        RefreshInspector();
        m_selection.ClearChanged();
    }
}

void EditorUI::SyncToSelection() {
    RefreshInspector();
}

void EditorUI::UpdateValidation(const ValidationResult& result) {
    m_lastValidation = result;
    RefreshValidation();
}

void EditorUI::SetWorldDirty(bool dirty) {
    m_worldDirty = dirty;
    RefreshStatusBar();
}

// ---------------------------------------------------------------------------
// Build helpers — create child elements for each panel
// ---------------------------------------------------------------------------

void EditorUI::BuildToolbarPanel() {
    m_toolbarPanel->ClearChildren();

    // Tool mode buttons
    struct ToolDef { const char* label; EditorToolMode mode; };
    const ToolDef tools[] = {
        {"Select",  EditorToolMode::Select},
        {"Place",   EditorToolMode::Place},
        {"Remove",  EditorToolMode::Remove},
        {"Paint",   EditorToolMode::Paint},
        {"Inspect", EditorToolMode::Inspect}
    };

    for (const auto& tool : tools) {
        auto btn = std::make_shared<UIButton>();
        btn->SetId(std::string("tool_") + tool.label);
        btn->SetLabel(tool.label);
        btn->SetSize(70.0f, 28.0f);
        const EditorToolMode mode = tool.mode;
        btn->SetOnClick([this, mode]() {
            m_toolContext.activeMode = mode;
        });
        btn->SetBackgroundColor((m_toolContext.activeMode == mode)
            ? Color(0.2f, 0.4f, 0.8f, 1.0f)
            : Color::DarkGray());
        m_toolbarPanel->AddChild(btn);
    }

    // Separator after tool buttons
    {
        auto sep = std::make_shared<UISeparator>();
        sep->SetId("sep_tools");
        sep->SetSize(2.0f, 28.0f);
        m_toolbarPanel->AddChild(sep);
    }

    // Undo / Redo buttons
    {
        auto btn = std::make_shared<UIButton>();
        btn->SetId("btn_undo");
        btn->SetLabel("Undo");
        btn->SetSize(60.0f, 28.0f);
        btn->SetBackgroundColor(Color::DarkGray());
        btn->SetEnabled(m_commandHistory.CanUndo());
        btn->SetOnClick([this]() { m_commandHistory.UndoLast(); });
        m_toolbarPanel->AddChild(btn);
    }
    {
        auto btn = std::make_shared<UIButton>();
        btn->SetId("btn_redo");
        btn->SetLabel("Redo");
        btn->SetSize(60.0f, 28.0f);
        btn->SetBackgroundColor(Color::DarkGray());
        btn->SetEnabled(m_commandHistory.CanRedo());
        btn->SetOnClick([this]() { m_commandHistory.RedoLast(); });
        m_toolbarPanel->AddChild(btn);
    }

    // Separator after undo/redo
    {
        auto sep = std::make_shared<UISeparator>();
        sep->SetId("sep_undoredo");
        sep->SetSize(2.0f, 28.0f);
        m_toolbarPanel->AddChild(sep);
    }

    // Symmetry checkboxes
    const uint8_t sym = m_controller.GetState().symmetry;
    {
        auto chk = std::make_shared<UICheckbox>();
        chk->SetId("chk_sym_x");
        chk->SetLabel("Sym X");
        chk->SetSize(70.0f, 28.0f);
        chk->SetChecked((sym & SymmetryMirrorX) != 0);
        chk->SetOnChange([this](bool) {
            m_controller.GetState().ToggleSymmetryX();
        });
        m_toolbarPanel->AddChild(chk);
    }
    {
        auto chk = std::make_shared<UICheckbox>();
        chk->SetId("chk_sym_y");
        chk->SetLabel("Sym Y");
        chk->SetSize(70.0f, 28.0f);
        chk->SetChecked((sym & SymmetryMirrorY) != 0);
        chk->SetOnChange([this](bool) {
            m_controller.GetState().ToggleSymmetryY();
        });
        m_toolbarPanel->AddChild(chk);
    }
    {
        auto chk = std::make_shared<UICheckbox>();
        chk->SetId("chk_sym_z");
        chk->SetLabel("Sym Z");
        chk->SetSize(70.0f, 28.0f);
        chk->SetChecked((sym & SymmetryMirrorZ) != 0);
        chk->SetOnChange([this](bool) {
            m_controller.GetState().ToggleSymmetryZ();
        });
        m_toolbarPanel->AddChild(chk);
    }
}

void EditorUI::BuildPalettePanel() {
    m_palettePanel->ClearChildren();

    const BlockPalette& palette = m_controller.GetPalette();

    // Section header: Categories & block types
    {
        auto header = std::make_shared<UILabel>();
        header->SetId("lbl_categories");
        header->SetText("Categories");
        header->SetSize(230.0f, 20.0f);
        header->SetColor(Color::White());
        header->SetFontSize(16);
        m_palettePanel->AddChild(header);
    }

    const auto categories = palette.GetCategories();
    const auto& allEntries = palette.GetAll();

    for (const auto& cat : categories) {
        // Category header button
        {
            auto btn = std::make_shared<UIButton>();
            btn->SetId("cat_" + cat);
            btn->SetLabel(cat);
            btn->SetSize(230.0f, 24.0f);
            btn->SetBackgroundColor(Color(0.25f, 0.25f, 0.25f, 1.0f));
            btn->SetTextColor(Color::Yellow());
            m_palettePanel->AddChild(btn);
        }

        // Block entries in this category
        auto entries = palette.GetByCategory(cat);
        for (const auto& entry : entries) {
            int blockIndex = 0;
            for (size_t i = 0; i < allEntries.size(); ++i) {
                if (allEntries[i].name == entry.name) {
                    blockIndex = static_cast<int>(i);
                    break;
                }
            }

            auto btn = std::make_shared<UIButton>();
            btn->SetId("block_" + entry.name);
            btn->SetLabel(entry.name);
            btn->SetSize(230.0f, 22.0f);
            bool selected = (m_toolContext.selectedBlockTypeIndex == blockIndex);
            btn->SetBackgroundColor(selected
                ? Color(0.2f, 0.4f, 0.8f, 1.0f)
                : Color::DarkGray());
            btn->SetOnClick([this, blockIndex]() {
                m_toolContext.selectedBlockTypeIndex = blockIndex;
            });
            m_palettePanel->AddChild(btn);
        }
    }

    // Separator before materials
    {
        auto sep = std::make_shared<UISeparator>();
        sep->SetId("sep_materials");
        sep->SetSize(230.0f, 2.0f);
        m_palettePanel->AddChild(sep);
    }

    // Section header: Materials
    {
        auto header = std::make_shared<UILabel>();
        header->SetId("lbl_materials");
        header->SetText("Materials");
        header->SetSize(230.0f, 20.0f);
        header->SetColor(Color::White());
        header->SetFontSize(16);
        m_palettePanel->AddChild(header);
    }

    for (size_t i = 0; i < kMaterialCount; ++i) {
        const int matIndex = static_cast<int>(i);
        auto btn = std::make_shared<UIButton>();
        btn->SetId(std::string("mat_") + MaterialTypeName(kAllMaterials[i]));
        btn->SetLabel(MaterialTypeName(kAllMaterials[i]));
        btn->SetSize(230.0f, 22.0f);
        bool selected = (m_toolContext.selectedMaterialIndex == matIndex);
        btn->SetBackgroundColor(selected
            ? Color(0.2f, 0.4f, 0.8f, 1.0f)
            : Color::DarkGray());
        btn->SetOnClick([this, matIndex]() {
            m_toolContext.selectedMaterialIndex = matIndex;
        });
        m_palettePanel->AddChild(btn);
    }
}

void EditorUI::BuildInspectorPanel() {
    m_inspectorPanel->ClearChildren();

    if (!m_selection.HasSelection()) {
        auto lbl = std::make_shared<UILabel>();
        lbl->SetId("lbl_no_sel");
        lbl->SetText("No Selection");
        lbl->SetSize(280.0f, 20.0f);
        lbl->SetColor(Color::Gray());
        m_inspectorPanel->AddChild(lbl);
        return;
    }

    // Selection header
    {
        auto lbl = std::make_shared<UILabel>();
        lbl->SetId("lbl_sel_header");
        lbl->SetText("Selection: " + m_selection.GetLabel());
        lbl->SetSize(280.0f, 20.0f);
        lbl->SetColor(Color::White());
        lbl->SetFontSize(14);
        m_inspectorPanel->AddChild(lbl);
    }

    // Properties from PropertyInspectorSystem
    if (m_propertyInspector.HasProperties()) {
        const PropertySet& props = m_propertyInspector.GetPropertySet();

        {
            auto lbl = std::make_shared<UILabel>();
            lbl->SetId("lbl_prop_title");
            lbl->SetText(props.title);
            lbl->SetSize(280.0f, 20.0f);
            lbl->SetColor(Color::Yellow());
            lbl->SetFontSize(14);
            m_inspectorPanel->AddChild(lbl);
        }

        for (size_t i = 0; i < props.entries.size(); ++i) {
            const PropertyEntry& entry = props.entries[i];
            std::string display = entry.name + ": "
                + PropertyInspectorSystem::ToDisplayString(entry);

            auto lbl = std::make_shared<UILabel>();
            lbl->SetId("prop_" + std::to_string(i));
            lbl->SetText(display);
            lbl->SetSize(280.0f, 18.0f);
            lbl->SetColor(entry.dirty ? Color::Cyan() : Color::White());
            m_inspectorPanel->AddChild(lbl);
        }
    }

    // Block details when a single block is selected
    if (m_selection.GetKind() == SelectionKind::Block) {
        const Vector3Int& pos = m_selection.GetSelection().position;

        {
            auto sep = std::make_shared<UISeparator>();
            sep->SetId("sep_block_detail");
            sep->SetSize(280.0f, 2.0f);
            m_inspectorPanel->AddChild(sep);
        }

        {
            auto lbl = std::make_shared<UILabel>();
            lbl->SetId("lbl_block_pos");
            lbl->SetText("Position: (" + std::to_string(pos.x) + ", "
                          + std::to_string(pos.y) + ", "
                          + std::to_string(pos.z) + ")");
            lbl->SetSize(280.0f, 18.0f);
            lbl->SetColor(Color::White());
            m_inspectorPanel->AddChild(lbl);
        }

        // Show type, material, shape from the actual block in the ship
        const Ship& ship = m_controller.GetShip();
        auto it = ship.occupiedCells.find(pos);
        if (it != ship.occupiedCells.end() && it->second) {
            const Block& block = *(it->second);

            auto addDetail = [&](const std::string& id,
                                 const std::string& text) {
                auto lbl = std::make_shared<UILabel>();
                lbl->SetId(id);
                lbl->SetText(text);
                lbl->SetSize(280.0f, 18.0f);
                lbl->SetColor(Color::White());
                m_inspectorPanel->AddChild(lbl);
            };

            addDetail("lbl_block_type",
                       std::string("Type: ") + BlockTypeName(block.type));
            addDetail("lbl_block_material",
                       std::string("Material: ") + MaterialTypeName(block.material));
            addDetail("lbl_block_shape",
                       std::string("Shape: ") + BlockShapeName(block.shape));
        }
    }
}

void EditorUI::BuildStatusBarPanel() {
    m_statusBarPanel->ClearChildren();

    // Tool mode
    {
        auto lbl = std::make_shared<UILabel>();
        lbl->SetId("lbl_mode");
        lbl->SetText(std::string("Mode: ")
                     + EditorToolModeName(m_toolContext.activeMode));
        lbl->SetSize(150.0f, 22.0f);
        lbl->SetColor(Color::White());
        m_statusBarPanel->AddChild(lbl);
    }

    // Selection count
    {
        auto lbl = std::make_shared<UILabel>();
        lbl->SetId("lbl_selection");
        lbl->SetText("Selected: "
                     + std::to_string(m_selection.GetSelectedPositions().size()));
        lbl->SetSize(150.0f, 22.0f);
        lbl->SetColor(Color::White());
        m_statusBarPanel->AddChild(lbl);
    }

    // Dirty indicator
    {
        auto lbl = std::make_shared<UILabel>();
        lbl->SetId("lbl_dirty");
        lbl->SetText(m_worldDirty ? "Modified" : "Saved");
        lbl->SetSize(100.0f, 22.0f);
        lbl->SetColor(m_worldDirty ? Color::Yellow() : Color::Green());
        m_statusBarPanel->AddChild(lbl);
    }

    // Block count
    {
        auto lbl = std::make_shared<UILabel>();
        lbl->SetId("lbl_blockcount");
        lbl->SetText("Blocks: "
                     + std::to_string(m_controller.GetShip().BlockCount()));
        lbl->SetSize(150.0f, 22.0f);
        lbl->SetColor(Color::White());
        m_statusBarPanel->AddChild(lbl);
    }
}

void EditorUI::BuildValidationPanel() {
    m_validationPanel->ClearChildren();

    if (m_lastValidation.errors.empty() && m_lastValidation.warnings.empty()) {
        auto lbl = std::make_shared<UILabel>();
        lbl->SetId("lbl_valid");
        lbl->SetText("Design Valid");
        lbl->SetSize(900.0f, 18.0f);
        lbl->SetColor(Color::Green());
        m_validationPanel->AddChild(lbl);
        return;
    }

    int idx = 0;
    for (const auto& err : m_lastValidation.errors) {
        auto lbl = std::make_shared<UILabel>();
        lbl->SetId("val_err_" + std::to_string(idx++));
        lbl->SetText("ERROR: " + err);
        lbl->SetSize(900.0f, 16.0f);
        lbl->SetColor(Color::Red());
        m_validationPanel->AddChild(lbl);
    }

    for (const auto& warn : m_lastValidation.warnings) {
        auto lbl = std::make_shared<UILabel>();
        lbl->SetId("val_warn_" + std::to_string(idx++));
        lbl->SetText("WARN: " + warn);
        lbl->SetSize(900.0f, 16.0f);
        lbl->SetColor(Color::Yellow());
        m_validationPanel->AddChild(lbl);
    }
}

// ---------------------------------------------------------------------------
// Refresh helpers — update existing panel contents efficiently
// ---------------------------------------------------------------------------

void EditorUI::RefreshToolbar() {
    // Update active-mode highlights on tool buttons
    const char* toolNames[] = {"Select", "Place", "Remove", "Paint", "Inspect"};
    const EditorToolMode modes[] = {
        EditorToolMode::Select, EditorToolMode::Place,
        EditorToolMode::Remove, EditorToolMode::Paint,
        EditorToolMode::Inspect
    };

    for (int i = 0; i < 5; ++i) {
        UIElement* elem = m_toolbarPanel->FindChild(
            std::string("tool_") + toolNames[i]);
        if (elem) {
            static_cast<UIButton*>(elem)->SetBackgroundColor(
                (m_toolContext.activeMode == modes[i])
                    ? Color(0.2f, 0.4f, 0.8f, 1.0f)
                    : Color::DarkGray());
        }
    }

    // Update undo/redo enabled state
    if (UIElement* e = m_toolbarPanel->FindChild("btn_undo"))
        e->SetEnabled(m_commandHistory.CanUndo());
    if (UIElement* e = m_toolbarPanel->FindChild("btn_redo"))
        e->SetEnabled(m_commandHistory.CanRedo());
}

void EditorUI::RefreshPalette() {
    BuildPalettePanel();
}

void EditorUI::RefreshInspector() {
    BuildInspectorPanel();
}

void EditorUI::RefreshStatusBar() {
    if (UIElement* e = m_statusBarPanel->FindChild("lbl_mode")) {
        static_cast<UILabel*>(e)->SetText(
            std::string("Mode: ") + EditorToolModeName(m_toolContext.activeMode));
    }

    if (UIElement* e = m_statusBarPanel->FindChild("lbl_selection")) {
        static_cast<UILabel*>(e)->SetText(
            "Selected: "
            + std::to_string(m_selection.GetSelectedPositions().size()));
    }

    if (UIElement* e = m_statusBarPanel->FindChild("lbl_dirty")) {
        auto* lbl = static_cast<UILabel*>(e);
        lbl->SetText(m_worldDirty ? "Modified" : "Saved");
        lbl->SetColor(m_worldDirty ? Color::Yellow() : Color::Green());
    }

    if (UIElement* e = m_statusBarPanel->FindChild("lbl_blockcount")) {
        static_cast<UILabel*>(e)->SetText(
            "Blocks: "
            + std::to_string(m_controller.GetShip().BlockCount()));
    }
}

void EditorUI::RefreshValidation() {
    BuildValidationPanel();
}

} // namespace subspace
