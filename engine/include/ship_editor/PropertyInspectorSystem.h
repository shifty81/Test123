#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace subspace {

/// Value types supported by the property inspector.
using PropertyValue = std::variant<bool, int, float, std::string>;

/// Hint for which widget to render.
enum class PropertyWidgetHint : uint8_t {
    Default = 0,
    Checkbox,
    NumericField,
    TextField,
    ReadOnlyLabel
};

/// A single property entry in the inspector.
struct PropertyEntry {
    std::string name;
    PropertyWidgetHint widgetHint = PropertyWidgetHint::Default;
    PropertyValue value{};
    PropertyValue originalValue{};
    bool readOnly = false;
    bool dirty = false;
};

/// A set of properties shown in the inspector for the current selection.
struct PropertySet {
    std::string title = "Properties";
    std::vector<PropertyEntry> entries;
    bool dirty = false;
};

/// Manages property display and editing for the editor inspector panel.
class PropertyInspectorSystem {
public:
    void Clear();
    void SetPropertySet(PropertySet properties);
    bool HasProperties() const;
    const PropertySet& GetPropertySet() const;
    PropertySet& GetMutablePropertySet();

    /// Apply an edit to a named property. Returns true if found and changed.
    bool ApplyEdit(const std::string& propertyName, const PropertyValue& newValue);

    bool IsDirty() const;
    void ClearDirty();

    /// Format a property entry as a display string.
    static std::string ToDisplayString(const PropertyEntry& entry);

private:
    PropertySet m_propertySet;
    bool m_hasProperties = false;
};

} // namespace subspace
