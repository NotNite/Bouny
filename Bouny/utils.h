#pragma once
#include <YYToolkit/Shared.hpp>

class utils
{
public:
    static std::string rvalue_to_string(YYTK::RValue* rvalue);
    static void log_hook(std::string name, YYTK::RValue& return_value, int num_args, YYTK::RValue** args);
    static int hook_and_log(std::string name);
};
