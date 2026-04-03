#include "ship_editor/EditorInputHandler.h"

namespace subspace {

EditorInputHandler::EditorInputHandler(ShipEditorController& controller,
                                       EditorViewport& viewport)
    : m_controller(controller)
    , m_viewport(viewport) {}

// ---- Input events ----

void EditorInputHandler::OnMouseMove(float screenX, float screenY) {
    float dx = screenX - m_mouseX;
    float dy = screenY - m_mouseY;
    m_mouseX = screenX;
    m_mouseY = screenY;

    if (m_middleDown) {
        if (m_modifiers & ModShift) {
            m_viewport.GetCamera().Pan(dx, dy);
        } else {
            m_viewport.GetCamera().OrbitBy(dx, dy);
        }
    }

    m_viewport.UpdateHoverFromMouse(screenX, screenY);
}

void EditorInputHandler::OnMouseButton(EditorMouseButton button, bool pressed) {
    switch (button) {
    case EditorMouseButton::Left:
        if (pressed) {
            HandlePrimaryClick();
        }
        break;

    case EditorMouseButton::Right:
        if (pressed) {
            HandleSecondaryClick();
        }
        break;

    case EditorMouseButton::Middle:
        m_middleDown = pressed;
        if (pressed) {
            m_dragging = true;
            m_dragStartX = m_mouseX;
            m_dragStartY = m_mouseY;
        } else {
            m_dragging = false;
        }
        break;
    }
}

void EditorInputHandler::OnKeyEvent(EditorKey key, bool pressed) {
    if (pressed) {
        HandleKeyPress(key);
    }
}

void EditorInputHandler::OnScroll(float delta) {
    m_viewport.GetCamera().Zoom(delta);
}

void EditorInputHandler::SetModifiers(uint8_t mods) {
    m_modifiers = mods;
}

// ---- Key handling ----

void EditorInputHandler::HandleKeyPress(EditorKey key) {
    bool ctrl = (m_modifiers & ModCtrl) != 0;
    ShipEditorState& state = m_controller.GetState();

    // Ctrl-modified shortcuts
    if (ctrl) {
        switch (key) {
        case EditorKey::Z: m_controller.Undo();          return;
        case EditorKey::Y: m_controller.Redo();          return;
        case EditorKey::C: m_controller.CopySelection(); return;
        case EditorKey::X: m_controller.CutSelection();  return;
        case EditorKey::V: m_controller.PasteAtHover();  return;
        case EditorKey::S:
            if (m_onSave) { m_onSave(); }
            return;
        case EditorKey::O:
            if (m_onLoad) { m_onLoad(); }
            return;
        case EditorKey::A: {
            // Select all blocks in the ship
            EditorSelection& sel = m_controller.GetSelection();
            const Ship& ship = m_controller.GetShip();
            for (const auto& block : ship.blocks) {
                sel.Add(block->gridPos);
            }
            return;
        }
        default: break;
        }
    }

    // Non-Ctrl shortcuts
    switch (key) {
    case EditorKey::Delete:
        m_controller.RemoveSelected();
        break;

    case EditorKey::Escape:
        if (!m_controller.GetSelection().IsEmpty()) {
            m_controller.GetSelection().Clear();
        } else if (m_onQuit) {
            m_onQuit();
        }
        break;

    case EditorKey::B:
        state.mode = BuildMode::Place;
        break;

    case EditorKey::R:
        // R without Ctrl → Remove mode; with Ctrl → handled above (Redo is Ctrl+Y)
        if (!ctrl) {
            state.mode = BuildMode::Remove;
        }
        break;

    case EditorKey::P:
        state.mode = BuildMode::Paint;
        break;

    case EditorKey::S:
        if (!ctrl) {
            state.mode = BuildMode::Select;
        }
        break;

    case EditorKey::X:
        if (!ctrl) {
            state.ToggleSymmetryX();
        }
        break;

    case EditorKey::Y:
        if (!ctrl) {
            state.ToggleSymmetryY();
        }
        break;

    case EditorKey::Z:
        if (!ctrl) {
            state.ToggleSymmetryZ();
        }
        break;

    case EditorKey::F:
        m_viewport.FocusOnShip();
        break;

    case EditorKey::G:
        // Toggle grid visibility
        m_controller.GetGrid().SetVisible(!m_controller.GetGrid().IsVisible());
        break;

    // Material shortcuts: Num1-Num7
    case EditorKey::Num1: state.selectedMaterial = MaterialType::Iron;     break;
    case EditorKey::Num2: state.selectedMaterial = MaterialType::Titanium; break;
    case EditorKey::Num3: state.selectedMaterial = MaterialType::Naonite;  break;
    case EditorKey::Num4: state.selectedMaterial = MaterialType::Trinium;  break;
    case EditorKey::Num5: state.selectedMaterial = MaterialType::Xanion;   break;
    case EditorKey::Num6: state.selectedMaterial = MaterialType::Ogonite;  break;
    case EditorKey::Num7: state.selectedMaterial = MaterialType::Avorion;  break;

    case EditorKey::Tab:
        state.NextBlockType();
        break;

    case EditorKey::Q:
        state.PrevShape();
        break;

    case EditorKey::E:
        state.NextShape();
        break;

    default:
        break;
    }
}

// ---- Click handling ----

void EditorInputHandler::HandlePrimaryClick() {
    switch (m_controller.GetState().mode) {
    case BuildMode::Place:
        m_controller.Place();
        break;
    case BuildMode::Remove:
        m_controller.RemoveAtHover();
        break;
    case BuildMode::Paint:
        m_controller.PaintAtHover();
        break;
    case BuildMode::Select:
        m_controller.GetSelection().Toggle(m_controller.GetState().hoverCell);
        break;
    }
}

void EditorInputHandler::HandleSecondaryClick() {
    switch (m_controller.GetState().mode) {
    case BuildMode::Place:
        // Convenience: right-click removes in Place mode
        m_controller.RemoveAtHover();
        break;
    case BuildMode::Select:
        m_controller.GetSelection().Clear();
        break;
    default:
        break;
    }
}

} // namespace subspace
