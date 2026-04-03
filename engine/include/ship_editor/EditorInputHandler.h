#pragma once

#include "ship_editor/ShipEditorController.h"
#include "ship_editor/EditorViewport.h"

#include <cstdint>
#include <functional>

namespace subspace {

/// Keyboard key codes used by the editor.
/// These map to platform-neutral key identifiers.
enum class EditorKey : uint16_t {
    None = 0,
    // Letters
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    // Numbers
    Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9, Num0,
    // Function keys
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    // Navigation
    Escape, Tab, Space, Enter, Backspace, Delete,
    Up, Down, Left, Right,
    // Modifiers (tracked as state, not as key events)
    LeftShift, RightShift, LeftCtrl, RightCtrl, LeftAlt, RightAlt
};

/// Mouse button identifiers.
enum class EditorMouseButton : uint8_t {
    Left = 0,
    Right = 1,
    Middle = 2
};

/// Modifier key flags.
enum EditorModifiers : uint8_t {
    ModNone  = 0,
    ModShift = 1,
    ModCtrl  = 2,
    ModAlt   = 4
};

/// Translates input events into editor actions.
///
/// Usage (once per frame):
///   handler.OnMouseMove(mx, my);
///   handler.OnMouseButton(button, pressed);
///   handler.OnKeyEvent(key, pressed);
///   handler.OnScroll(delta);
///   handler.SetModifiers(mods);
class EditorInputHandler {
public:
    EditorInputHandler(ShipEditorController& controller, EditorViewport& viewport);

    // ---- Input events ----

    /// Mouse moved to (screenX, screenY).
    void OnMouseMove(float screenX, float screenY);

    /// Mouse button pressed or released.
    void OnMouseButton(EditorMouseButton button, bool pressed);

    /// Keyboard key pressed or released.
    void OnKeyEvent(EditorKey key, bool pressed);

    /// Mouse scroll wheel.
    void OnScroll(float delta);

    /// Set modifier key state (called each frame or on change).
    void SetModifiers(uint8_t mods);
    uint8_t GetModifiers() const { return m_modifiers; }

    // ---- State queries ----

    bool IsMouseDragging()  const { return m_dragging; }
    float GetMouseX()       const { return m_mouseX; }
    float GetMouseY()       const { return m_mouseY; }

    // ---- Optional callbacks ----

    /// Called when the user requests to save (Ctrl+S).
    using SaveCallback = std::function<void()>;
    void SetOnSave(SaveCallback cb) { m_onSave = std::move(cb); }

    /// Called when the user requests to load (Ctrl+O).
    using LoadCallback = std::function<void()>;
    void SetOnLoad(LoadCallback cb) { m_onLoad = std::move(cb); }

    /// Called when the user requests to quit (Escape).
    using QuitCallback = std::function<void()>;
    void SetOnQuit(QuitCallback cb) { m_onQuit = std::move(cb); }

private:
    void HandleKeyPress(EditorKey key);
    void HandlePrimaryClick();
    void HandleSecondaryClick();

    ShipEditorController& m_controller;
    EditorViewport&       m_viewport;

    float   m_mouseX     = 0.0f;
    float   m_mouseY     = 0.0f;
    float   m_dragStartX = 0.0f;
    float   m_dragStartY = 0.0f;
    bool    m_dragging   = false;
    uint8_t m_modifiers  = ModNone;

    bool    m_middleDown = false;  // for camera orbit

    SaveCallback m_onSave;
    LoadCallback m_onLoad;
    QuitCallback m_onQuit;
};

} // namespace subspace
