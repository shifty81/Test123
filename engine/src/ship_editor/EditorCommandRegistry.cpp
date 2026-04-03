#include "ship_editor/EditorCommandRegistry.h"

namespace subspace {

// ---------------------------------------------------------------------------
// CommandHistory
// ---------------------------------------------------------------------------

void CommandHistory::Push(std::shared_ptr<IEditorCommand> cmd) {
    if (!cmd) return;
    cmd->Execute();
    m_undoStack.push_back(std::move(cmd));
    m_redoStack.clear();
}

void CommandHistory::UndoLast() {
    if (m_undoStack.empty()) return;
    auto cmd = std::move(m_undoStack.back());
    m_undoStack.pop_back();
    cmd->Undo();
    m_redoStack.push_back(std::move(cmd));
}

void CommandHistory::RedoLast() {
    if (m_redoStack.empty()) return;
    auto cmd = std::move(m_redoStack.back());
    m_redoStack.pop_back();
    cmd->Execute();
    m_undoStack.push_back(std::move(cmd));
}

void CommandHistory::Clear() {
    m_undoStack.clear();
    m_redoStack.clear();
}

// ---------------------------------------------------------------------------
// EditorCommandRegistry
// ---------------------------------------------------------------------------

bool EditorCommandRegistry::Register(const RegisteredCommand& command) {
    if (command.id.empty()) return false;
    auto result = m_commands.emplace(command.id, command);
    return result.second;
}

bool EditorCommandRegistry::CanExecute(const std::string& id) const {
    auto it = m_commands.find(id);
    if (it == m_commands.end()) return false;
    if (it->second.canExecute) return it->second.canExecute();
    return true;
}

bool EditorCommandRegistry::Execute(const std::string& id) {
    auto it = m_commands.find(id);
    if (it == m_commands.end()) return false;
    if (it->second.canExecute && !it->second.canExecute()) return false;
    if (it->second.execute) it->second.execute();
    return true;
}

const RegisteredCommand* EditorCommandRegistry::Find(const std::string& id) const {
    auto it = m_commands.find(id);
    if (it == m_commands.end()) return nullptr;
    return &it->second;
}

std::vector<std::string> EditorCommandRegistry::GetRegisteredIds() const {
    std::vector<std::string> ids;
    ids.reserve(m_commands.size());
    for (const auto& pair : m_commands) {
        ids.push_back(pair.first);
    }
    return ids;
}

size_t EditorCommandRegistry::Count() const {
    return m_commands.size();
}

} // namespace subspace
