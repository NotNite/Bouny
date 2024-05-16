#include "utils.h"

#include <numeric>

#include "globals.h"

std::string utils::rvalue_to_string(YYTK::RValue* rvalue)
{
    if (rvalue->m_Kind == VALUE_UNSET) return "unset";
    return std::string(rvalue->AsString());
}

int utils::hook_and_log(std::string name)
{
    return g_hooks->hook_script(name, [name](auto self, auto other, auto return_value, auto num_args, auto args, auto original)
    {
        original(self, other, return_value, num_args, args);

        if (num_args == 0)
        {
            g_module_interface->Print(CM_GRAY, "%s() -> %s", name.c_str(), rvalue_to_string(&return_value).c_str());
        }
        else
        {
            std::string args_str = std::accumulate(args, args + num_args, std::string{},
                                                   [](std::string acc, YYTK::RValue* arg)
                                                   {
                                                       return acc + rvalue_to_string(arg) + ", ";
                                                   });

            // remove last ,
            args_str.pop_back();
            args_str.pop_back();

            g_module_interface->Print(CM_GRAY, "%s(%s) -> %s",
                                      name.c_str(),
                                      args_str.c_str(),
                                      rvalue_to_string(&return_value).c_str());
        }
    });
}
