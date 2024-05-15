#pragma once
#include <YYToolkit/Shared.hpp>
#include <map>

typedef YYTK::RValue& ScriptFunction(YYTK::CInstance* self, YYTK::CInstance* other, YYTK::RValue& return_value,
                                     int num_args, YYTK::RValue** args);
typedef void ScriptDetour(YYTK::CInstance* self, YYTK::CInstance* other, YYTK::RValue& return_value, int num_args,
                          YYTK::RValue** args, ScriptFunction* trampoline);
typedef std::function<void(YYTK::CInstance*, YYTK::CInstance*, YYTK::RValue&, int, YYTK::RValue**, ScriptFunction*)> ScriptRuntimeDetour;

class hooks
{
public:
    hooks();

    template <const char* function_name>
    void hook_script(ScriptDetour* callback);
    void hook_script_runtime( std::string_view script_name, ScriptRuntimeDetour callback);
    void unhook_script(std::string_view script_name);

    inline static std::map<std::string, ScriptDetour*> hook_map;
    inline static std::map<std::string, ScriptFunction*> trampoline_map;
    inline static std::map<int, ScriptRuntimeDetour> runtime_hook_map;
    inline static std::map<int, ScriptFunction*> runtime_trampoline_map;

private:
    inline static ScriptFunction* g_deal_damage_orig;
};
