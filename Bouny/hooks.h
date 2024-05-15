#pragma once
#include <YYToolkit/Shared.hpp>
#include <map>
#include <asmjit/x86.h>

typedef YYTK::RValue& ScriptFunction(YYTK::CInstance* self, YYTK::CInstance* other, YYTK::RValue& return_value,
                                     int num_args, YYTK::RValue** args);
typedef YYTK::RValue& (*
    GeneratedDetourFunction)(YYTK::CInstance* self, YYTK::CInstance* other, YYTK::RValue& return_value, int num_args,
                             YYTK::RValue** args);
typedef std::function<void(YYTK::CInstance* self, YYTK::CInstance* other, YYTK::RValue& return_value, int num_args,
                           YYTK::RValue** args, ScriptFunction* original)>
ScriptDetour;

class hooks
{
public:
    hooks();

    int hook_script(std::string_view script_name, ScriptDetour callback);
    void unhook_script(int id);

    inline static asmjit::JitRuntime runtime;
    inline static std::map<int, ScriptDetour> detour_map;
    inline static std::map<int, ScriptFunction*> original_map;

private:
    inline static ScriptFunction* g_deal_damage_orig;
};
