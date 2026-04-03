#include "ship_editor/EditorViewport.h"

#include <algorithm>
#include <cmath>

namespace subspace {

static constexpr float kPi        = 3.14159265f;
static constexpr float kDegToRad  = kPi / 180.0f;

// ---------------------------------------------------------------------------
// EditorCamera
// ---------------------------------------------------------------------------

Vector3 EditorCamera::GetPosition() const {
    float pitchRad = pitch * kDegToRad;
    float yawRad   = yaw   * kDegToRad;

    float cosPitch = std::cos(pitchRad);
    float sinPitch = std::sin(pitchRad);
    float cosYaw   = std::cos(yawRad);
    float sinYaw   = std::sin(yawRad);

    return {
        target.x + distance * cosPitch * sinYaw,
        target.y + distance * sinPitch,
        target.z + distance * cosPitch * cosYaw
    };
}

Vector3 EditorCamera::GetForward() const {
    return (target - GetPosition()).normalized();
}

void EditorCamera::OrbitBy(float deltaYaw, float deltaPitch) {
    yaw   += deltaYaw;
    pitch += deltaPitch;
    pitch  = std::max(-89.0f, std::min(89.0f, pitch));
}

void EditorCamera::Zoom(float delta) {
    distance -= delta;
    distance  = std::max(1.0f, std::min(500.0f, distance));
}

void EditorCamera::Pan(float dx, float dy) {
    float yawRad = yaw * kDegToRad;

    // Camera-local right vector (lies in XZ plane, perpendicular to forward)
    Vector3 right = { std::cos(yawRad), 0.0f, -std::sin(yawRad) };
    Vector3 up    = { 0.0f, 1.0f, 0.0f };

    target = target + right * dx + up * dy;
}

void EditorCamera::Reset() {
    target   = Vector3::Zero();
    distance = 30.0f;
    yaw      = 0.0f;
    pitch    = 30.0f;
    fov      = 60.0f;
    nearClip = 0.1f;
    farClip  = 1000.0f;
    mode     = EditorCameraMode::Orbit;
}

// ---------------------------------------------------------------------------
// EditorViewport
// ---------------------------------------------------------------------------

EditorViewport::EditorViewport(ShipEditorController& controller)
    : m_controller(controller) {}

void EditorViewport::Update(float /*deltaTime*/) {
    const Ship& ship = m_controller.GetShip();
    size_t currentCount = ship.BlockCount();

    if (currentCount != m_lastBlockCount) {
        m_lastBlockCount = currentCount;
        RebuildShipMesh();
    }

    RebuildGhostBlocks();
    RebuildGridLines();
    RebuildSelectionHighlights();
}

void EditorViewport::SetScreenSize(float width, float height) {
    m_screenWidth  = width;
    m_screenHeight = height;
}

// ---------------------------------------------------------------------------
// Raycasting
// ---------------------------------------------------------------------------

EditorRaycastResult EditorViewport::Raycast(float screenX, float screenY) const {
    EditorRaycastResult result;

    // Normalised device coordinates [-1, 1]
    float ndcX = (2.0f * screenX / m_screenWidth)  - 1.0f;
    float ndcY = 1.0f - (2.0f * screenY / m_screenHeight);

    // Simple perspective ray direction using fov and aspect ratio
    float aspect   = m_screenWidth / m_screenHeight;
    float halfFovRad = (m_camera.fov * 0.5f) * kDegToRad;
    float tanHalf  = std::tan(halfFovRad);

    // Ray direction in camera-local space (right-handed, looking -Z)
    Vector3 localDir = { ndcX * tanHalf * aspect, ndcY * tanHalf, -1.0f };

    // Build a simple camera basis from yaw / pitch
    float yawRad   = m_camera.yaw   * kDegToRad;
    float pitchRad = m_camera.pitch * kDegToRad;

    float cy = std::cos(yawRad),  sy = std::sin(yawRad);
    float cp = std::cos(pitchRad), sp = std::sin(pitchRad);

    // Camera basis vectors
    Vector3 camRight   = {  cy, 0.0f, -sy };
    Vector3 camUp      = {  sy * sp, cp, cy * sp };
    Vector3 camForward = { -sy * cp, sp, -cy * cp };

    // Transform local direction to world space
    Vector3 worldDir = {
        camRight.x * localDir.x + camUp.x * localDir.y + camForward.x * localDir.z,
        camRight.y * localDir.x + camUp.y * localDir.y + camForward.y * localDir.z,
        camRight.z * localDir.x + camUp.z * localDir.y + camForward.z * localDir.z
    };
    worldDir = worldDir.normalized();

    Vector3 origin = m_camera.GetPosition();

    // Intersect with the Y = hoverY plane
    float hoverY = static_cast<float>(m_controller.GetState().hoverCell.y);

    if (std::abs(worldDir.y) < 1e-6f) {
        return result; // Ray is parallel to the grid plane
    }

    float t = (hoverY - origin.y) / worldDir.y;
    if (t < 0.0f) {
        return result; // Intersection behind camera
    }

    Vector3 hitPoint = origin + worldDir * t;

    const EditorGrid& grid = m_controller.GetGrid();
    Vector3Int cell = grid.SnapToGrid(hitPoint.x, hitPoint.y, hitPoint.z);

    result.hit        = true;
    result.gridCell   = cell;
    result.worldPoint = hitPoint;
    result.distance   = t;
    return result;
}

void EditorViewport::UpdateHoverFromMouse(float screenX, float screenY) {
    EditorRaycastResult hit = Raycast(screenX, screenY);
    if (hit.hit) {
        m_controller.SetHoverCell(hit.gridCell);
    }
}

// ---------------------------------------------------------------------------
// Rebuild helpers
// ---------------------------------------------------------------------------

void EditorViewport::RebuildShipMesh() {
    m_shipMesh = ShipRenderer::BuildMeshData(m_controller.GetShip());
    m_shipMeshDirty = true;
}

void EditorViewport::RebuildGhostBlocks() {
    Block ghost = m_controller.BuildGhostBlock();
    uint8_t symmetry = m_controller.GetState().symmetry;
    m_ghostBlocks = GhostRenderer::BuildGhostPreview(
        m_controller.GetShip(), ghost, symmetry);
}

void EditorViewport::RebuildGridLines() {
    m_gridLines.clear();

    const EditorGrid& grid = m_controller.GetGrid();
    if (!grid.IsVisible()) {
        return;
    }

    int extent  = grid.GetExtent();
    int cellSz  = grid.GetCellSize();
    float halfW = static_cast<float>(extent * cellSz);
    float y     = 0.0f;

    // Lines parallel to the X axis
    for (int i = -extent; i <= extent; ++i) {
        float z = static_cast<float>(i * cellSz);
        m_gridLines.push_back({ -halfW, y, z });
        m_gridLines.push_back({  halfW, y, z });
    }

    // Lines parallel to the Z axis
    for (int i = -extent; i <= extent; ++i) {
        float x = static_cast<float>(i * cellSz);
        m_gridLines.push_back({ x, y, -halfW });
        m_gridLines.push_back({ x, y,  halfW });
    }
}

void EditorViewport::RebuildSelectionHighlights() {
    m_selectionHighlights.clear();

    const EditorGrid& grid = m_controller.GetGrid();
    std::vector<Vector3Int> positions = m_controller.GetSelection().GetPositions();

    m_selectionHighlights.reserve(positions.size());
    for (const auto& pos : positions) {
        m_selectionHighlights.push_back(grid.CellToWorld(pos));
    }
}

// ---------------------------------------------------------------------------
// Focus helpers
// ---------------------------------------------------------------------------

void EditorViewport::FocusOnShip() {
    const Ship& ship = m_controller.GetShip();
    if (ship.IsEmpty()) {
        m_camera.target = Vector3::Zero();
        return;
    }

    Vector3 sum = Vector3::Zero();
    for (const auto& block : ship.blocks) {
        sum = sum + Vector3(
            static_cast<float>(block->gridPos.x),
            static_cast<float>(block->gridPos.y),
            static_cast<float>(block->gridPos.z));
    }

    float count = static_cast<float>(ship.blocks.size());
    m_camera.target = { sum.x / count, sum.y / count, sum.z / count };
}

void EditorViewport::FrameShip() {
    const Ship& ship = m_controller.GetShip();
    if (ship.IsEmpty()) {
        m_camera.Reset();
        return;
    }

    FocusOnShip();

    // Compute bounding extent
    float maxExtent = 0.0f;
    for (const auto& block : ship.blocks) {
        Vector3 pos = {
            static_cast<float>(block->gridPos.x),
            static_cast<float>(block->gridPos.y),
            static_cast<float>(block->gridPos.z)
        };
        Vector3 diff = pos - m_camera.target;
        float ext = diff.length();
        if (ext > maxExtent) {
            maxExtent = ext;
        }
    }

    // Set distance so the ship fits within the fov
    float halfFovRad = (m_camera.fov * 0.5f) * kDegToRad;
    float tanHalf    = std::tan(halfFovRad);
    float needed     = (maxExtent + 2.0f) / tanHalf; // small padding
    m_camera.distance = std::max(1.0f, std::min(500.0f, needed));
}

} // namespace subspace
