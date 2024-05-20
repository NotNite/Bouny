#pragma once
#include <YYToolkit/Shared.hpp>

class utils
{
public:
    static std::string rvalue_to_string(YYTK::RValue* rvalue);
    static void log_hook(std::string name, YYTK::RValue& return_value, int num_args, YYTK::RValue** args);
    static int hook_and_log(std::string name);

    static void draw_search(std::map<std::string, std::string>& search_results, std::string& search_query);
    static void enumerate_members(YYTK::RValue rvalue, std::map<std::string, std::string>& results, bool ignore_functions = false);
};
