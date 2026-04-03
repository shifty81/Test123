#include "ship_editor/SelectionService.h"

#include <sstream>

namespace subspace {

void SelectionService::Clear() {
    m_current = SelectionHandle{};
    m_positions.clear();
    m_changed = true;
}

void SelectionService::Select(const SelectionHandle& handle) {
    m_current = handle;
    m_positions.clear();
    m_changed = true;
}

void SelectionService::SelectBlock(const Vector3Int& pos) {
    m_current.kind = SelectionKind::Block;
    m_current.position = pos;
    m_current.id = 0;

    std::ostringstream oss;
    oss << "Block (" << pos.x << "," << pos.y << "," << pos.z << ")";
    m_current.label = oss.str();

    m_positions.clear();
    m_positions.push_back(pos);
    m_changed = true;
}

void SelectionService::SelectMultiBlock(const std::vector<Vector3Int>& positions) {
    m_current.kind = SelectionKind::MultiBlock;
    m_current.id = 0;
    m_current.position = positions.empty() ? Vector3Int::Zero() : positions.front();

    m_current.label = std::to_string(positions.size()) + " blocks";

    m_positions = positions;
    m_changed = true;
}

bool SelectionService::HasSelection() const {
    return m_current.kind != SelectionKind::None;
}

const SelectionHandle& SelectionService::GetSelection() const {
    return m_current;
}

SelectionKind SelectionService::GetKind() const {
    return m_current.kind;
}

const std::string& SelectionService::GetLabel() const {
    return m_current.label;
}

bool SelectionService::IsChanged() const {
    return m_changed;
}

void SelectionService::ClearChanged() {
    m_changed = false;
}

const std::vector<Vector3Int>& SelectionService::GetSelectedPositions() const {
    return m_positions;
}

} // namespace subspace
