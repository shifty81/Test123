#include <fstream>
#include <iostream>
#include <string>

#include "core/Engine.h"
#include "core/logging/Logger.h"
#include "ships/Ship.h"
#include "ships/Blueprint.h"
#include "ship_editor/ShipEditorController.h"
#include "ship_editor/EditorViewport.h"
#include "ship_editor/EditorInputHandler.h"
#include "ship_editor/EditorUI.h"
#include "ship_editor/EditorToolContext.h"
#include "ship_editor/SelectionService.h"
#include "ship_editor/PropertyInspectorSystem.h"
#include "ship_editor/EditorCommandRegistry.h"
#include "ship_editor/ShipValidator.h"
#include "ui/UISystem.h"

/// Atlas Editor — standalone ship-editor application.
///
/// Launches the engine in editor mode: gameplay simulation systems are
/// skipped and the UISystem is configured with the ship-editor panels
/// (toolbar, block palette, inspector, validation, status bar).
///
/// Usage:
///   ./atlas_editor [--ship <blueprint.json>]

// ---------------------------------------------------------------------------
// Helper: read a file into a string.
// ---------------------------------------------------------------------------
static std::string ReadFile(const char* path) {
    std::ifstream ifs(path);
    if (!ifs.is_open()) return {};
    return std::string(std::istreambuf_iterator<char>(ifs),
                       std::istreambuf_iterator<char>());
}

// ---------------------------------------------------------------------------
// Helper: write a string to a file.
// ---------------------------------------------------------------------------
static bool WriteFile(const char* path, const std::string& content) {
    std::ofstream ofs(path);
    if (!ofs.is_open()) return false;
    ofs << content;
    return ofs.good();
}

int main(int argc, char* argv[])
{
    std::cout << "Atlas Editor v0.2.0 starting..." << std::endl;

    // ------------------------------------------------------------------
    // Parse command-line arguments.
    // ------------------------------------------------------------------
    const char* blueprintPath = nullptr;
    for (int i = 1; i < argc - 1; ++i) {
        if (std::string(argv[i]) == "--ship") {
            blueprintPath = argv[i + 1];
        }
    }

    // ------------------------------------------------------------------
    // 1. Create the engine in editor mode — skips gameplay systems.
    // ------------------------------------------------------------------
    subspace::Engine engine;
    engine.SetEditorMode(true);
    engine.SetMaxFrames(0);   // unlimited — editor runs until the user quits
    engine.Initialize();

    std::cout << "Atlas Editor: engine initialized (editor mode, gameplay systems disabled)." << std::endl;

    // ------------------------------------------------------------------
    // 2. Create the working ship for the editor session.
    // ------------------------------------------------------------------
    subspace::Ship editShip;
    subspace::ShipEditorController editorController(editShip);

    // Load blueprint if provided.
    std::string currentBlueprintPath;
    if (blueprintPath) {
        currentBlueprintPath = blueprintPath;
        std::string json = ReadFile(blueprintPath);
        if (!json.empty()) {
            subspace::Blueprint bp = subspace::Blueprint::FromJson(json);
            if (bp.Validate()) {
                editShip = bp.ToShip();
                std::cout << "Atlas Editor: loaded blueprint '" << blueprintPath
                          << "' (" << editShip.BlockCount() << " blocks)." << std::endl;
            } else {
                std::cerr << "Atlas Editor: blueprint '" << blueprintPath
                          << "' failed validation, starting empty." << std::endl;
            }
        } else {
            std::cerr << "Atlas Editor: could not read '" << blueprintPath
                      << "', starting empty." << std::endl;
        }
    } else {
        std::cout << "Atlas Editor: starting with empty ship." << std::endl;
    }

    // ------------------------------------------------------------------
    // 3. Create all editor subsystems (modeled after NovaForge EditorApp).
    // ------------------------------------------------------------------
    subspace::EditorToolContext       toolContext;
    subspace::SelectionService        selectionService;
    subspace::PropertyInspectorSystem propertyInspector;
    subspace::EditorCommandRegistry   commandRegistry;
    subspace::CommandHistory          commandHistory;

    subspace::EditorViewport    viewport(editorController);
    subspace::EditorInputHandler inputHandler(editorController, viewport);

    subspace::EditorUI editorUI(editorController, toolContext, selectionService,
                                propertyInspector, commandRegistry, commandHistory);

    // ------------------------------------------------------------------
    // 4. Register editor commands.
    // ------------------------------------------------------------------

    // File.Save — save the current ship as a blueprint.
    commandRegistry.Register({
        "File.Save", "Save Blueprint",
        [&]() { return !editShip.IsEmpty(); },
        [&]() {
            subspace::Blueprint bp = subspace::Blueprint::FromShip(editShip, "EditorShip", "Editor");
            std::string json = bp.ToJson();
            const char* savePath = currentBlueprintPath.empty()
                ? "editor_ship.json" : currentBlueprintPath.c_str();
            if (WriteFile(savePath, json)) {
                toolContext.worldDirty = false;
                std::cout << "Atlas Editor: saved blueprint to '"
                          << savePath << "'." << std::endl;
            } else {
                std::cerr << "Atlas Editor: failed to save blueprint." << std::endl;
            }
        }
    });

    // File.Load — load a blueprint (stub path for now).
    commandRegistry.Register({
        "File.Load", "Load Blueprint",
        [&]() { return true; },
        [&]() {
            std::cout << "Atlas Editor: File.Load — use --ship <path> to load on start." << std::endl;
        }
    });

    // File.Exit — request engine shutdown.
    commandRegistry.Register({
        "File.Exit", "Exit Editor",
        [&]() { return true; },
        [&]() { engine.RequestShutdown(); }
    });

    // Edit.Undo — undo last undoable command.
    commandRegistry.Register({
        "Edit.Undo", "Undo",
        [&]() { return commandHistory.CanUndo() || editorController.CanUndo(); },
        [&]() {
            if (commandHistory.CanUndo()) {
                commandHistory.UndoLast();
            } else if (editorController.CanUndo()) {
                editorController.Undo();
            }
            toolContext.worldDirty = true;
        }
    });

    // Edit.Redo — redo last undone command.
    commandRegistry.Register({
        "Edit.Redo", "Redo",
        [&]() { return commandHistory.CanRedo() || editorController.CanRedo(); },
        [&]() {
            if (commandHistory.CanRedo()) {
                commandHistory.RedoLast();
            } else if (editorController.CanRedo()) {
                editorController.Redo();
            }
            toolContext.worldDirty = true;
        }
    });

    // Edit.Validate — validate the ship design.
    commandRegistry.Register({
        "Edit.Validate", "Validate Ship",
        [&]() { return true; },
        [&]() {
            auto result = editorController.ValidateShip();
            editorUI.UpdateValidation(result);
            std::cout << "Atlas Editor: validation — "
                      << result.errors.size() << " errors, "
                      << result.warnings.size() << " warnings." << std::endl;
        }
    });

    // Tools.Select / Tools.Place / Tools.Remove / Tools.Paint / Tools.Inspect
    commandRegistry.Register({
        "Tools.Select", "Select Tool",
        [&]() { return true; },
        [&]() { toolContext.activeMode = subspace::EditorToolMode::Select; }
    });
    commandRegistry.Register({
        "Tools.Place", "Place Tool",
        [&]() { return true; },
        [&]() { toolContext.activeMode = subspace::EditorToolMode::Place; }
    });
    commandRegistry.Register({
        "Tools.Remove", "Remove Tool",
        [&]() { return true; },
        [&]() { toolContext.activeMode = subspace::EditorToolMode::Remove; }
    });
    commandRegistry.Register({
        "Tools.Paint", "Paint Tool",
        [&]() { return true; },
        [&]() { toolContext.activeMode = subspace::EditorToolMode::Paint; }
    });
    commandRegistry.Register({
        "Tools.Inspect", "Inspect Tool",
        [&]() { return true; },
        [&]() { toolContext.activeMode = subspace::EditorToolMode::Inspect; }
    });

    std::cout << "Atlas Editor: " << commandRegistry.Count()
              << " commands registered." << std::endl;

    // ------------------------------------------------------------------
    // 5. Wire input handler callbacks.
    // ------------------------------------------------------------------
    inputHandler.SetOnSave([&]() { commandRegistry.Execute("File.Save"); });
    inputHandler.SetOnLoad([&]() { commandRegistry.Execute("File.Load"); });
    inputHandler.SetOnQuit([&]() { commandRegistry.Execute("File.Exit"); });

    // ------------------------------------------------------------------
    // 6. Initialize the editor UI panels within the engine's UISystem.
    // ------------------------------------------------------------------
    // The UISystem is created by Engine::RegisterSystems() and is accessible
    // via the entity manager.  We grab it by pointer from the engine.
    // Since Engine doesn't expose UISystem directly but does own it, we
    // create a temporary UISystem reference to pass into EditorUI.  In
    // practice the panels render through UIRenderer in the engine loop.

    // Create a local UISystem to host editor panels.  The engine's own
    // UISystem is for game-mode panels; editor panels are managed separately
    // and rendered into the same UIRenderer.
    subspace::UISystem editorUISystem;
    editorUI.Initialize(editorUISystem);

    std::cout << "Atlas Editor: UI panels initialized." << std::endl;

    // Initial validation.
    {
        auto result = editorController.ValidateShip();
        editorUI.UpdateValidation(result);
    }

    // ------------------------------------------------------------------
    // 7. Run the editor loop.
    // ------------------------------------------------------------------
    std::cout << "Atlas Editor: entering main loop." << std::endl;

    // Use external loop control so we can interleave editor updates between
    // engine ticks.
    while (engine.GetState() == subspace::EngineState::Running ||
           engine.GetState() == subspace::EngineState::Paused) {

        float dt = engine.GetLastDeltaTime();
        if (dt <= 0.0f) dt = 1.0f / 60.0f;  // sensible default before first tick

        // Update editor subsystems.
        viewport.Update(dt);
        editorUI.Update(dt);

        // Render editor panels into the engine's UIRenderer.
        editorUISystem.Render(engine.GetUIRenderer());

        // Advance the engine one frame (processes events, renders UI).
        engine.Tick();

        // Honour frame cap for testing.
        if (engine.GetFrameCount() > 0 &&
            engine.GetState() == subspace::EngineState::ShuttingDown) {
            break;
        }
    }

    // ------------------------------------------------------------------
    // 8. Shutdown.
    // ------------------------------------------------------------------
    engine.Shutdown();

    std::cout << "Atlas Editor: session ended." << std::endl;
    return 0;
}
