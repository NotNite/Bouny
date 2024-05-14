#include "hooks.h"
#include "globals.h"

static constexpr char pattern_deal_damage_enemy[] = "gml_Script_scr_pattern_deal_damage_enemy";
static constexpr char pattern_deal_damage_enemy_subtract[] = "gml_Script_scr_pattern_deal_damage_enemy_subtract";

hooks::hooks()
{
    hook_function<pattern_deal_damage_enemy>([](YYTK::CInstance* self, YYTK::CInstance* other,
                                                YYTK::RValue& return_value, int num_args,
                                                YYTK::RValue** args, ScriptFunction* trampoline)
    {
        return_value = trampoline(self, other, return_value, num_args, args);
        g_module_interface->Print(CM_LIGHTGREEN, "deal_damage returned: %s",
                                  return_value.AsString().data());
    });

    hook_function<pattern_deal_damage_enemy_subtract>([](YYTK::CInstance* self, YYTK::CInstance* other,
                                                         YYTK::RValue& return_value,
                                                         int num_args, YYTK::RValue** args, ScriptFunction* trampoline)
    {
        if (g_config.cheat_damage) args[2]->m_Real = 42069;
        return_value = trampoline(self, other, return_value, num_args, args);
    });
}

// template where it calls the fnuction
template <const char* func>
YYTK::RValue& detour_func(YYTK::CInstance* self, YYTK::CInstance* other, YYTK::RValue& return_value,
                          int num_args, YYTK::RValue** args)
{
    auto trampoline = g_hooks->trampoline_map[func];

    if (hooks::hook_map.find(func) != g_hooks->hook_map.end())
    {
        hooks::hook_map[func](self, other, return_value, num_args, args, trampoline);
    }
    else
    {
        trampoline(self, other, return_value, num_args, args);
    }

    return return_value;
}

template <const char* function_name>
void hooks::hook_function(ScriptDetour* callback)
{
    auto runner = g_module_interface->GetRunnerInterface();
    auto id = runner.Script_Find_Id(function_name) - 100000;
    g_module_interface->Print(CM_LIGHTGREEN, "id: %d", id);
    CScript* script = nullptr;

    auto status = g_module_interface->GetScriptData(id, script);
    if (!AurieSuccess(status))
    {
        g_module_interface->Print(CM_LIGHTRED, "Failed to get script data for %s", function_name);
        return;
    }

    if (script == nullptr)
    {
        g_module_interface->Print(CM_LIGHTRED, "Script data for %s is null", function_name);
        return;
    }

    auto detour = detour_func<function_name>;
    hook_map[function_name] = callback;

    void* trampoline = nullptr;
    MmCreateHook(g_module,
                 function_name,
                 script->m_Functions->m_ScriptFunction,
                 detour,
                 &trampoline);
    trampoline_map[function_name] = reinterpret_cast<ScriptFunction*>(trampoline);
}
