#include <YYToolkit/Shared.hpp>
#include <YYToolkit/Shared.cpp>  // NOLINT(bugprone-suspicious-include) lmao
#include <Aurie/shared.hpp>
#include "globals.h"

#include "ui_manager.h"

EXPORTED AurieStatus ModuleInitialize(
    IN AurieModule* module,
    IN const fs::path& module_path
)
{
    g_module = module;

    auto last_status = ObGetInterface(
        "YYTK_Main",
        reinterpret_cast<AurieInterfaceBase*&>(g_module_interface)
    );

    if (!AurieSuccess(last_status))
        return AURIE_MODULE_DEPENDENCY_NOT_RESOLVED;

    g_module_interface->Print(CM_LIGHTGREEN, ":3");

    g_ui_manager = new ui_manager();
    g_hooks = new hooks();

    return AURIE_SUCCESS;
}
