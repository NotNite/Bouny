#pragma once
#include <YYToolkit/Shared.hpp>
#include <Aurie/shared.hpp>

using namespace Aurie;
using namespace YYTK;

#include "ui_manager.h"
#include "hooks.h"

inline AurieModule* g_module = nullptr;
inline YYTKInterface* g_module_interface = nullptr;

inline ui_manager* g_ui_manager = nullptr;
inline hooks* g_hooks = nullptr;

struct Config
{
    bool cheat_damage;
    bool tailwinds;
};

inline Config g_config;
