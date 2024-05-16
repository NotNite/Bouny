#include "hooks.h"
#include "globals.h"

static int* current_hook_id = static_cast<int*>(malloc(sizeof(int)));
static int hook_index = 0;

hooks::hooks()
{
    *current_hook_id = -1;

    hook_script("scr_pattern_deal_damage_enemy_subtract",
                [](auto self, auto other, auto& return_value, auto num_args, auto args, auto original)
                {
                    if (g_config.cheat_damage) args[2]->m_Real = 42069;
                    return_value = original(self, other, return_value, num_args, args);
                });

    static bool tailwinds_triggered = false;
    hook_script("bpatt_add", [](auto self, auto other, auto& return_value, auto num_args, auto args, auto original)
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

    hook_script("scrdt_encounter",
                [](auto self, auto other, auto& return_value, auto num_args, auto args, auto original)
                {
                    tailwinds_triggered = false;
                    return_value = original(self, other, return_value, num_args, args);
                });
}

YYTK::RValue& detour_func(YYTK::CInstance* self, YYTK::CInstance* other, YYTK::RValue& return_value,
                          int num_args, YYTK::RValue** args)
{
    auto id = *current_hook_id;
    auto script = hooks::id_map[id];
    auto original = hooks::original_map[script];
    auto detours = hooks::detour_map[script];

    // call the first detour, where next calls the next detour
    // iterators hate me here so I'm just gonna do it lazily
    auto funcs = std::vector<ScriptDetour>{};
    for (auto& [id, func] : detours) funcs.push_back(func);
    auto i = 0;
    auto max = static_cast<int>(funcs.size());

    // this probably isn't thread safe
    static ScriptOriginal next;
    next = [original, &i, max, funcs](YYTK::CInstance* self, YYTK::CInstance* other, YYTK::RValue& return_value,
                                      int num_args, YYTK::RValue** args) -> YYTK::RValue& {
        if (i >= max) return original(self, other, return_value, num_args, args);
        auto func = funcs[i];
        i++;
        func(self, other, return_value, num_args, args, next);
        return return_value;
    };

    return_value = next(self, other, return_value, num_args, args);
    return return_value;
}

ScriptFunction* hooks::get_script(std::string script_name)
{
    auto runner = g_module_interface->GetRunnerInterface();
    auto id = runner.Script_Find_Id(script_name.c_str()) - 100000;

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
    std::string script_name,
    ScriptDetour callback
)
{
    auto fn = get_script(script_name);
    if (fn == nullptr)
    {
        g_module_interface->Print(CM_LIGHTRED, "Failed to get script %s", script_name.data());
        return -1;
    }

    auto hook_id = hook_index;

    if (!detour_map.contains(script_name)) detour_map[script_name] = {};
    detour_map[script_name][hook_id] = callback;
    id_map[hook_id] = script_name;

    if (!original_map.contains(script_name))
    {
        auto code = asmjit::CodeHolder();
        code.init(runtime.environment(), runtime.cpuFeatures());
        auto assembler = asmjit::x86::Assembler(&code);

        auto immediate = asmjit::Imm(hook_id);
        auto ptr = reinterpret_cast<uint64_t>(current_hook_id);
        auto dest = asmjit::x86::ptr(ptr, 4);

        assembler.long_().mov(dest, immediate);
        assembler.jmp(detour_func);

        GeneratedDetourFunction detour;
        runtime.add(&detour, &code);
        generated_detour_map[script_name] = detour;

        void* original = nullptr;
        MmCreateHook(g_module,
                     "Bouny-" + script_name,
                     fn,
                     detour,
                     &original);
        original_map[script_name] = reinterpret_cast<ScriptFunction*>(original);
    }

    // Get the next unique ID
    int i = 0;
    while (id_map.contains(i)) i++;
    hook_index = i;

    return hook_id;
}

void hooks::unhook_script(int id)
{
    for (auto& [script_name, map] : detour_map)
    {
        if (map.contains(id))
        {
            map.erase(id);

            if (map.empty())
            {
                MmRemoveHook(g_module, "Bouny-" + script_name);
                runtime.release(generated_detour_map[script_name]);
                generated_detour_map.erase(script_name);
                original_map.erase(script_name);
                id_map.erase(id);
            }

            break;
        }
    }
}
