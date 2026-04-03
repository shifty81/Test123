#pragma once

#include "core/Math.h"

#include <cstdint>
#include <string>
#include <vector>

namespace subspace {

/// What kind of thing is selected.
enum class SelectionKind : uint8_t {
    None = 0,
    Block,
    MultiBlock,
    Entity,
    Chunk
};

/// A handle describing the current selection.
struct SelectionHandle {
    SelectionKind kind = SelectionKind::None;
    uint64_t id = 0;
    Vector3Int position = Vector3Int::Zero();
    std::string label;
};

/// Centralized selection service — shared between viewport, outliner, inspector.
class SelectionService {
public:
    void Clear();
    void Select(const SelectionHandle& handle);
    void SelectBlock(const Vector3Int& pos);
    void SelectMultiBlock(const std::vector<Vector3Int>& positions);

    bool HasSelection() const;
    const SelectionHandle& GetSelection() const;
    SelectionKind GetKind() const;
    const std::string& GetLabel() const;

    /// Returns true if the selection changed since the last ClearChanged().
    bool IsChanged() const;
    void ClearChanged();

    /// For multi-block selection.
    const std::vector<Vector3Int>& GetSelectedPositions() const;

private:
    SelectionHandle m_current;
    std::vector<Vector3Int> m_positions;
    bool m_changed = false;
};

} // namespace subspace
