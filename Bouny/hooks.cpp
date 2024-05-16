#include "hooks.h"
#include "globals.h"

static int* current_hook_id = static_cast<int*>(malloc(sizeof(int)));
static int hook_index = 0;

hooks::hooks()
{
    *current_hook_id = -1;

    hook_script("scr_pattern_deal_damage_enemy_subtract",
                [](YYTK::CInstance* self, YYTK::CInstance* other,
                   YYTK::RValue& return_value, int num_args, YYTK::RValue** args,
                   ScriptFunction* original)
                {
                    if (g_config.cheat_damage) args[2]->m_Real = 42069;
                    return_value = original(self, other, return_value, num_args, args);
                });

    static bool tailwinds_triggered = false;
    hook_script("bpatt_add", [](YYTK::CInstance* self, YYTK::CInstance* other,
                                YYTK::RValue& return_value, int num_args, YYTK::RValue** args,
                                ScriptFunction* original)
    {
        if (!tailwinds_triggered && g_config.tailwinds)
        {
            tailwinds_triggered = true;
            auto arg = RValue(g_module_interface->GetRunnerInterface().Script_Find_Id("bp_tailwind_permanent"));
            RValue* arg_ptr = &arg;
            auto arg_array = &arg_ptr;
            original(self, other, return_value, 1, arg_array);
        }

        return_value = original(self, other, return_value, num_args, args);
    });

    hook_script("scrdt_encounter", [](YYTK::CInstance* self, YYTK::CInstance* other,
                                      YYTK::RValue& return_value, int num_args, YYTK::RValue** args,
                                      ScriptFunction* original)
    {
        tailwinds_triggered = false;
        return_value = original(self, other, return_value, num_args, args);
    });
}

YYTK::RValue& detour_func(YYTK::CInstance* self, YYTK::CInstance* other, YYTK::RValue& return_value,
                          int num_args, YYTK::RValue** args)
{
    auto id = *current_hook_id;
    auto original = hooks::original_map[id];
    if (hooks::detour_map.contains(id))
    {
        hooks::detour_map[id](self, other, return_value, num_args, args, original);
    }
    else
    {
        original(self, other, return_value, num_args, args);
    }

    return return_value;
}

ScriptFunction* hooks::get_script(std::string_view script_name)
{
    auto runner = g_module_interface->GetRunnerInterface();
    auto id = runner.Script_Find_Id(script_name.data()) - 100000;

    CScript* script = nullptr;
    auto status = g_module_interface->GetScriptData(id, script);
    if (!AurieSuccess(status) || script == nullptr)
    {
        g_module_interface->Print(CM_LIGHTRED, "Failed to get script data for %s", script_name.data());
        return nullptr;
    }

    return script->m_Functions->m_ScriptFunction;
}


int hooks::hook_script(
    std::string_view script_name,
    ScriptDetour callback
)
{
    auto fn = get_script(script_name);
    if (fn == nullptr)
    {
        g_module_interface->Print(CM_LIGHTRED, "Failed to get script %s", script_name.data());
        return -1;
    }

    auto code = asmjit::CodeHolder();
    code.init(runtime.environment(), runtime.cpuFeatures());
    auto assembler = asmjit::x86::Assembler(&code);

    auto hook_id = hook_index;

    auto immediate = asmjit::Imm(hook_id);
    auto ptr = reinterpret_cast<uint64_t>(current_hook_id);
    auto dest = asmjit::x86::ptr(ptr, 4);

    assembler.long_().mov(dest, immediate);
    assembler.jmp(detour_func);

    GeneratedDetourFunction detour;
    runtime.add(&detour, &code);

    detour_map[hook_id] = callback;

    void* original = nullptr;
    MmCreateHook(g_module,
                 "Bouny-" + std::to_string(hook_id),
                 fn,
                 detour,
                 &original);
    original_map[hook_id] = reinterpret_cast<ScriptFunction*>(original);

    hook_index++;
    return hook_id;
}

void hooks::unhook_script(int id)
{
    // TODO: Doesn't actually remove the allocated function from asmjit...
    auto script_name = "Bouny-" + std::to_string(id);
    MmRemoveHook(g_module, script_name);
}
