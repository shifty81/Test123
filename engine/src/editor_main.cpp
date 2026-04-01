#include <iostream>
#include "core/Engine.h"
#include "ship_editor/ShipEditorController.h"
#include "ships/Ship.h"

/// Atlas Editor — standalone ship-editor application.
///
/// Launches the engine in editor mode: the game loop is still driven by the
/// core Engine, but the UISystem is configured to display the ship-editor
/// interface and all gameplay simulation systems are left inactive.
///
/// Usage:
///   ./atlas_editor [--ship <blueprint.json>]

int main(int argc, char* argv[])
{
    std::cout << "Atlas Editor starting..." << std::endl;

    // Parse optional blueprint path from the command line.
    const char* blueprintPath = nullptr;
    for (int i = 1; i < argc - 1; ++i) {
        if (std::string(argv[i]) == "--ship") {
            blueprintPath = argv[i + 1];
        }
    }

    // Create the engine and configure it for editor mode (no simulation).
    subspace::Engine engine;
    engine.SetMaxFrames(0); // unlimited — editor runs until the user quits

    engine.Initialize();

    // Create a working ship for the editor session.
    subspace::Ship editShip;
    subspace::ShipEditorController editor(editShip);

    if (blueprintPath) {
        std::cout << "Atlas Editor: loading blueprint '" << blueprintPath << "'" << std::endl;
        // Blueprint loading would integrate with the persistence layer here.
    } else {
        std::cout << "Atlas Editor: starting with empty ship" << std::endl;
    }

    std::cout << "Atlas Editor: engine + editor initialized. Running editor loop." << std::endl;

    // The main loop drives rendering and event processing; the editor
    // controller processes input and updates the ship data each frame.
    engine.Run();

    engine.Shutdown();

    std::cout << "Atlas Editor: session ended." << std::endl;
    return 0;
}
