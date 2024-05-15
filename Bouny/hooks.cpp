#include "hooks.h"
#include "globals.h"
#include <array>

static constexpr char pattern_deal_damage_enemy[] = "gml_Script_scr_pattern_deal_damage_enemy";
static constexpr char pattern_deal_damage_enemy_subtract[] = "gml_Script_scr_pattern_deal_damage_enemy_subtract";

hooks::hooks()
{
    hook_script<pattern_deal_damage_enemy>([](YYTK::CInstance* self, YYTK::CInstance* other,
                                              YYTK::RValue& return_value, int num_args,
                                              YYTK::RValue** args, ScriptFunction* trampoline)
    {
        return_value = trampoline(self, other, return_value, num_args, args);
        g_module_interface->Print(CM_LIGHTGREEN, "deal_damage returned: %s",
                                  return_value.AsString().data());
    });

    hook_script<pattern_deal_damage_enemy_subtract>([](YYTK::CInstance* self, YYTK::CInstance* other,
                                                       YYTK::RValue& return_value,
                                                       int num_args, YYTK::RValue** args, ScriptFunction* trampoline)
    {
        if (g_config.cheat_damage) args[2]->m_Real = 42069;
        return_value = trampoline(self, other, return_value, num_args, args);
    });
}

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

// Detour needs to know its index without changing the arguments, therefore we just fill an array with them
template <size_t i>
YYTK::RValue& detour_runtime_func(YYTK::CInstance* self, YYTK::CInstance* other, YYTK::RValue& return_value,
                                  int num_args, YYTK::RValue** args)
{
    auto trampoline = g_hooks->runtime_trampoline_map[i];

    if (hooks::runtime_hook_map.find(i) != g_hooks->runtime_hook_map.end())
    {
        hooks::runtime_hook_map[i](self, other, return_value, num_args, args, trampoline);
    }
    else
    {
        trampoline(self, other, return_value, num_args, args);
    }

    return return_value;
}

template <size_t... Is>
auto detour_runtime_generator(std::integer_sequence<size_t, Is...>)
{
    return std::array{detour_runtime_func<Is>...};
}

template <size_t size>
auto make_runtime_hooks()
{
    return detour_runtime_generator(std::make_index_sequence<size>{});
}

const int max_runtime_hooks = 1000;
auto runtime_hooks = make_runtime_hooks<max_runtime_hooks>();
int runtime_hook_index = 0;

template <const char* function_name>
void hooks::hook_script(ScriptDetour* callback)
{
    auto runner = g_module_interface->GetRunnerInterface();
    auto id = runner.Script_Find_Id(function_name) - 100000;

    CScript* script = nullptr;
    auto status = g_module_interface->GetScriptData(id, script);
    if (!AurieSuccess(status) || script == nullptr)
    {
        g_module_interface->Print(CM_LIGHTRED, "Failed to get script data for %s", function_name);
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

void hooks::hook_script_runtime(
    std::string_view script_name,
    std::function<void(YYTK::CInstance*, YYTK::CInstance*, YYTK::RValue&, int, YYTK::RValue**, ScriptFunction*)>
    callback)
{
    auto runner = g_module_interface->GetRunnerInterface();
    auto id = runner.Script_Find_Id(script_name.data()) - 100000;

    CScript* script = nullptr;
    auto status = g_module_interface->GetScriptData(id, script);
    if (!AurieSuccess(status) || script == nullptr)
    {
        g_module_interface->Print(CM_LIGHTRED, "Failed to get script data for %s", script_name.data());
        return;
    }

    if (runtime_hook_index >= max_runtime_hooks)
    {
        g_module_interface->Print(CM_LIGHTRED, "Max runtime hooks reached");
        return;
    }

    auto detour = runtime_hooks[runtime_hook_index];
    runtime_hook_map[runtime_hook_index] = callback;

    void* trampoline = nullptr;
    MmCreateHook(g_module,
                 script_name,
                 script->m_Functions->m_ScriptFunction,
                 detour,
                 &trampoline);
    runtime_trampoline_map[runtime_hook_index] = reinterpret_cast<ScriptFunction*>(trampoline);

    runtime_hook_index++;
}

void hooks::unhook_script(std::string_view script_name)
{
    MmRemoveHook(g_module, script_name.data());
}
