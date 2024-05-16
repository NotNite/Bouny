#pragma once
#include <YYToolkit/Shared.hpp>
#include <map>
#include <asmjit/x86.h>

typedef std::function<YYTK::RValue& (YYTK::CInstance* self, YYTK::CInstance* other, YYTK::RValue& return_value,
                                    int num_args,
                                    YYTK::RValue** args)> ScriptOriginal;
typedef YYTK::RValue& ScriptFunction(YYTK::CInstance* self, YYTK::CInstance* other, YYTK::RValue& return_value,
                                     int num_args, YYTK::RValue** args);
typedef YYTK::RValue& (*
    GeneratedDetourFunction)(YYTK::CInstance* self, YYTK::CInstance* other, YYTK::RValue& return_value, int num_args,
                             YYTK::RValue** args);
typedef std::function<void(YYTK::CInstance* self, YYTK::CInstance* other, YYTK::RValue& return_value, int num_args,
                           YYTK::RValue** args, ScriptOriginal original)>
ScriptDetour;

class hooks
{
public:
    hooks();

    int hook_script(std::string script_name, ScriptDetour callback);
    ScriptFunction* get_script(std::string script_name);
    void unhook_script(int id);

    inline static asmjit::JitRuntime runtime;
    inline static std::map<std::string, GeneratedDetourFunction> generated_detour_map;
    inline static std::map<int, std::string> id_map;
    inline static std::map<std::string, std::map<int, ScriptDetour>> detour_map;
    inline static std::map<std::string, ScriptFunction*> original_map;
};
