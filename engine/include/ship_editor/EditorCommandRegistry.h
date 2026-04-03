#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace subspace {

/// Undoable editor command interface (for undo/redo stack).
class IEditorCommand {
public:
    virtual ~IEditorCommand() = default;
    virtual void Execute() = 0;
    virtual void Undo() = 0;
};

/// Manages an undo/redo stack of undoable editor commands.
class CommandHistory {
public:
    /// Push and immediately execute a command.
    void Push(std::shared_ptr<IEditorCommand> cmd);
    void UndoLast();
    void RedoLast();
    bool CanUndo() const { return !m_undoStack.empty(); }
    bool CanRedo() const { return !m_redoStack.empty(); }
    size_t UndoCount() const { return m_undoStack.size(); }
    size_t RedoCount() const { return m_redoStack.size(); }
    void Clear();

private:
    std::vector<std::shared_ptr<IEditorCommand>> m_undoStack;
    std::vector<std::shared_ptr<IEditorCommand>> m_redoStack;
};

/// A registered editor command (like File.Exit, Edit.Undo, Tools.Select, etc.)
struct RegisteredCommand {
    std::string id;
    std::string displayName;
    std::function<bool()> canExecute;
    std::function<void()> execute;
};

/// Central registry for named editor commands (File.Exit, Edit.Undo, etc.)
class EditorCommandRegistry {
public:
    bool Register(const RegisteredCommand& command);
    bool CanExecute(const std::string& id) const;
    bool Execute(const std::string& id);
    const RegisteredCommand* Find(const std::string& id) const;
    std::vector<std::string> GetRegisteredIds() const;
    size_t Count() const;

private:
    std::unordered_map<std::string, RegisteredCommand> m_commands;
};

} // namespace subspace
