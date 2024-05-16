#pragma once
#include <YYToolkit/Shared.hpp>

class utils
{
public:
    static std::string rvalue_to_string(YYTK::RValue* rvalue);
    static int hook_and_log(std::string name);
};
