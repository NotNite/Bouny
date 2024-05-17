#include "utils.h"

#include <numeric>

#include "globals.h"

std::map<RValueType, std::string> names = {
    {VALUE_REAL, "real"},
    {VALUE_STRING, "string"},
    {VALUE_ARRAY, "array"},
    {VALUE_PTR, "pointer"},
    {VALUE_VEC3, "vec3"},
    {VALUE_UNDEFINED, "undefined"},
    {VALUE_OBJECT, "object"},
    {VALUE_INT32, "int32"},
    {VALUE_VEC4, "vec4"},
    {VALUE_VEC44, "vec44"},
    {VALUE_INT64, "int64"},
    {VALUE_ACCESSOR, "accessor"},
    {VALUE_NULL, "null"},
    {VALUE_BOOL, "bool"},
    {VALUE_ITERATOR, "iterator"},
    {VALUE_REF, "reference"},
    {VALUE_UNSET, "unset"},
};

std::string utils::rvalue_to_string(YYTK::RValue* rvalue)
{
    if (rvalue == nullptr) return "nullptr";
    if (rvalue->m_Kind == VALUE_UNSET) return "unset";
    return std::string(rvalue->AsString());
}

void utils::log_hook(std::string name, YYTK::RValue& return_value, int num_args, YYTK::RValue** args)
{
    if (num_args == 0)
    {
        g_module_interface->Print(CM_GRAY, "%s() -> %s", name.c_str(), rvalue_to_string(&return_value).c_str());
    }
    else
    {
        std::string args_str = std::accumulate(args, args + num_args, std::string{},
                                               [](std::string acc, YYTK::RValue* arg)
                                               {
                                                   auto arg_str = rvalue_to_string(arg);
                                                   auto type_str = names.contains(arg->m_Kind)
                                                                       ? names[arg->m_Kind]
                                                                       : "unknown";
                                                   return acc + arg_str + " (" + type_str + "), ";
                                               });

        // remove last ,
        args_str.pop_back();
        args_str.pop_back();

        g_module_interface->Print(CM_GRAY, "%s(%s) -> %s",
                                  name.c_str(),
                                  args_str.c_str(),
                                  rvalue_to_string(&return_value).c_str());
    }
}

int utils::hook_and_log(std::string name)
{
    return g_hooks->hook_script(
        name, [name](auto self, auto other, auto return_value, auto num_args, auto args, auto original)
        {
            return_value = original(self, other, return_value, num_args, args);
            log_hook(name, return_value, num_args, args);
        });
}
