import sys
import os

# Clear import cache
if "api" in sys.modules:
    del sys.modules["api"]

sys.path.append(os.path.dirname(os.path.realpath(__file__)))
__package__ = "api"  # is this correct?
from api import api


def handle_vars(addr):
    global api
    print("handle_vars: {:02X}".format(addr))
    while True:
        variable = api.read_u64(addr)
        if variable == 0:
            break

        string = api.read_str(api.read_u64(variable))
        api.set_name(variable + 8, "var_" + string, None)

        addr += 8


def handle_funcs(addr):
    global api
    print("handle_funcs: {:02X}".format(addr))
    while True:
        func_addr = api.read_u64(addr)
        if func_addr == 0:
            break

        func_name_addr = api.read_u64(func_addr)
        if func_name_addr == 0:
            break

        func_name = api.read_str(func_name_addr)

        reference = func_addr + 8
        api.set_name(reference, "func_" + func_name, None)

        addr += 8


def handle_gml_funcs(addr, num_funcs):
    global api
    print("handle_gml_funcs: {:02X}".format(addr))
    i = 0
    while True:
        if i >= num_funcs:
            break
        i += 1

        func_addr = api.read_u64(addr)
        if func_addr == 0:
            break

        func_name = api.read_str(func_addr)
        if func_name is None:
            break

        func = api.read_u64(addr + 8)
        api.set_name(func, func_name, "function")

        # is this correct?
        func_ref = api.read_u64(addr + 16) + 8
        api.set_name(func_ref, "funcref_" + func_name, None)

        api.set_func_arg_type(func, 0, "CInstance", 1, "self")
        api.set_func_arg_type(func, 1, "CInstance", 1, "other")
        api.set_func_arg_type(func, 2, "RValue", 1, "return_value")
        api.set_func_arg_type(func, 3, "int", 0, "num_args")
        api.set_func_arg_type(func, 4, "RValue", 2, "args")
        api.set_func_ret_type(func, "RValue", 1)

        addr += 24


def parse_init_llvm():
    global api
    init_llvm = api.scan("E8 ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? 8B 41 14")
    if init_llvm is None:
        return
    print("init_llvm: {:02X}".format(init_llvm))

    insns = list(api.get_instructions(init_llvm))
    for insn in insns:
        if len(insn.operands) < 1:
            continue

        if insn.opcode == "mov":
            offset = insn.operands[0]
            if offset == 0x18 or offset == 0x20 or offset == 0x28:
                for i in range(insns.index(insn) - 1, -1, -1):
                    insn2 = insns[i]
                    if len(insn2.operands) < 1:
                        continue

                    if insn2.opcode == "lea":
                        # Shitty Ghidra hack
                        addr = None
                        if len(insn2.operands) == 1:
                            addr = insn2.operands[0]
                        else:
                            addr = insn2.operands[1]

                        if offset == 0x18:
                            handle_vars(addr)
                        if offset == 0x20:
                            handle_funcs(addr)
                        if offset == 0x28:
                            num_funcs = 0
                            for j in range(insns.index(insn2) + 1, len(insns)):
                                if (
                                    insns[j].opcode == "mov"
                                    and insns[j].operands[0] == 0x14
                                ):
                                    num_funcs = insns[j].values[1]
                                    break

                            handle_gml_funcs(addr, num_funcs)
                        break


parse_init_llvm()
