#pragma once

#include "core/Math.h"
#include "ships/Ship.h"
#include "rendering/ShipRenderer.h"
#include "rendering/GhostRenderer.h"
#include "ship_editor/ShipEditorController.h"
#include "ui/UITypes.h"

#include <unordered_map>
#include <vector>

namespace subspace {

/// Camera mode for the editor viewport.
enum class EditorCameraMode {
    Orbit,      // Rotate around the ship
    Free        // Free-look camera
};

/// Orbit camera data for the editor.
struct EditorCamera {
    Vector3 target   = Vector3::Zero();   // Look-at point
    float   distance = 30.0f;             // Distance from target
    float   yaw      = 0.0f;             // Horizontal angle in degrees
    float   pitch    = 30.0f;            // Vertical angle in degrees
    float   fov      = 60.0f;            // Field of view in degrees
    float   nearClip = 0.1f;
    float   farClip  = 1000.0f;

    EditorCameraMode mode = EditorCameraMode::Orbit;

    /// Compute the camera's world position from orbit parameters.
    Vector3 GetPosition() const;

    /// Compute forward direction.
    Vector3 GetForward() const;

    /// Orbit: rotate around target
    void OrbitBy(float deltaYaw, float deltaPitch);

    /// Zoom in/out
    void Zoom(float delta);

    /// Pan the target position
    void Pan(float dx, float dy);

    /// Reset to default orientation
    void Reset();
};

/// Raycast result from screen to editor grid.
struct EditorRaycastResult {
    bool     hit       = false;
    Vector3Int gridCell = Vector3Int::Zero();
    Vector3  worldPoint = Vector3::Zero();
    float    distance   = 0.0f;
};

/// Bridges the editor controller to the rendering subsystem.
///
/// Responsibilities:
/// - Manages the editor camera (orbit / free)
/// - Builds and caches ship mesh data via ShipRenderer
/// - Builds ghost block previews via GhostRenderer
/// - Generates grid line data for the editor grid
/// - Performs screen → grid raycasting for mouse interaction
/// - Highlights selected blocks
class EditorViewport {
public:
    explicit EditorViewport(ShipEditorController& controller);

    /// Update camera and rebuild any dirty render data. Call once per frame.
    void Update(float deltaTime);

    // ---- Camera ----
    EditorCamera&       GetCamera()       { return m_camera; }
    const EditorCamera& GetCamera() const { return m_camera; }

    void SetScreenSize(float width, float height);
    float GetScreenWidth()  const { return m_screenWidth; }
    float GetScreenHeight() const { return m_screenHeight; }

    // ---- Raycasting ----

    /// Cast a ray from screen coordinates into the grid and return the
    /// nearest grid cell hit.
    EditorRaycastResult Raycast(float screenX, float screenY) const;

    /// Update the hover cell on the editor controller from screen position.
    void UpdateHoverFromMouse(float screenX, float screenY);

    // ---- Render data access ----

    /// Ship mesh data (rebuilt when dirty).
    const ShipMeshData& GetShipMeshData() const { return m_shipMesh; }

    /// Ghost block previews for the current frame.
    const std::vector<GhostBlockData>& GetGhostBlocks() const { return m_ghostBlocks; }

    /// Grid line endpoints (pairs of Vector3: start, end, start, end, ...).
    const std::vector<Vector3>& GetGridLines() const { return m_gridLines; }

    /// Positions of selected blocks (for highlighting).
    const std::vector<Vector3>& GetSelectionHighlights() const { return m_selectionHighlights; }

    /// Whether the ship mesh needs a GPU re-upload.
    bool IsShipMeshDirty() const { return m_shipMeshDirty; }
    void ClearShipMeshDirty() { m_shipMeshDirty = false; }

    // ---- Focus ----

    /// Center the camera on the ship's center of mass.
    void FocusOnShip();

    /// Frame the entire ship in view (auto-zoom).
    void FrameShip();

private:
    void RebuildShipMesh();
    void RebuildGhostBlocks();
    void RebuildGridLines();
    void RebuildSelectionHighlights();

    ShipEditorController& m_controller;

    EditorCamera m_camera;
    float m_screenWidth  = 1920.0f;
    float m_screenHeight = 1080.0f;

    // Cached render data
    ShipMeshData                 m_shipMesh;
    std::vector<GhostBlockData>  m_ghostBlocks;
    std::vector<Vector3>         m_gridLines;
    std::vector<Vector3>         m_selectionHighlights;
    bool                         m_shipMeshDirty = true;

    // Track ship block count to detect changes
    size_t m_lastBlockCount = 0;
};

} // namespace subspace
