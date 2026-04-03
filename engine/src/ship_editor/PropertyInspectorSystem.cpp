#include "ship_editor/PropertyInspectorSystem.h"

#include <sstream>

namespace subspace {

void PropertyInspectorSystem::Clear() {
    m_propertySet = PropertySet{};
    m_hasProperties = false;
}

void PropertyInspectorSystem::SetPropertySet(PropertySet properties) {
    for (auto& entry : properties.entries) {
        entry.originalValue = entry.value;
        entry.dirty = false;
    }
    properties.dirty = false;
    m_propertySet = std::move(properties);
    m_hasProperties = true;
}

bool PropertyInspectorSystem::HasProperties() const {
    return m_hasProperties;
}

const PropertySet& PropertyInspectorSystem::GetPropertySet() const {
    return m_propertySet;
}

PropertySet& PropertyInspectorSystem::GetMutablePropertySet() {
    return m_propertySet;
}

bool PropertyInspectorSystem::ApplyEdit(const std::string& propertyName,
                                        const PropertyValue& newValue) {
    for (auto& entry : m_propertySet.entries) {
        if (entry.name == propertyName) {
            if (entry.readOnly) return false;
            if (entry.value == newValue) return false;
            entry.value = newValue;
            entry.dirty = (entry.value != entry.originalValue);
            m_propertySet.dirty = false;
            for (const auto& e : m_propertySet.entries) {
                if (e.dirty) {
                    m_propertySet.dirty = true;
                    break;
                }
            }
            return true;
        }
    }
    return false;
}

bool PropertyInspectorSystem::IsDirty() const {
    return m_propertySet.dirty;
}

void PropertyInspectorSystem::ClearDirty() {
    for (auto& entry : m_propertySet.entries) {
        entry.originalValue = entry.value;
        entry.dirty = false;
    }
    m_propertySet.dirty = false;
}

std::string PropertyInspectorSystem::ToDisplayString(const PropertyEntry& entry) {
    std::ostringstream oss;
    oss << entry.name << ": ";

    std::visit([&oss](const auto& val) {
        using T = std::decay_t<decltype(val)>;
        if constexpr (std::is_same_v<T, bool>) {
            oss << (val ? "true" : "false");
        } else if constexpr (std::is_same_v<T, std::string>) {
            oss << val;
        } else {
            oss << val;
        }
    }, entry.value);

    return oss.str();
}

} // namespace subspace
