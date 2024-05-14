#pragma once
#include <YYToolkit/Shared.hpp>
#include <map>

typedef YYTK::RValue& ScriptFunction(YYTK::CInstance* self, YYTK::CInstance* other, YYTK::RValue& return_value,
                                     int num_args, YYTK::RValue** args);
typedef void ScriptDetour(YYTK::CInstance* self, YYTK::CInstance* other, YYTK::RValue& return_value, int num_args,
                          YYTK::RValue** args, ScriptFunction* trampoline);

class hooks
{
public:
    hooks();

    template <const char* function_name>
    void hook_function(ScriptDetour* detour);

    // string to function, function maybe a lambda
    inline static std::map<std::string, ScriptDetour*> hook_map;
    inline static std::map<std::string, ScriptFunction*> trampoline_map;

private:
    inline static ScriptFunction* g_deal_damage_orig;
};
