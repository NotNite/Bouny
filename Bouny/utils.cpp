#include "utils.h"

#include "globals.h"

std::string utils::rvalue_to_string(const YYTK::RValue* rvalue)
{
    return std::string(g_module_interface->GetRunnerInterface().YYGetString(rvalue, 0));
}
