import os
import idaapi
import ida_nalt
import ida_bytes
import idautils
import ida_funcs
import ida_allins


def try_set_func_arg_type(ea, arg_idx, type, ptr=0, name=None):
    setting_type = type != "_" and type is not None

    # Create type info for this function
    tinfo = idaapi.tinfo_t()
    if not ida_nalt.get_tinfo(tinfo, ea):
        # print("Unable to get type info for 0x{0:X}".format(ea))
        return

    # Get function data from type info
    funcdata = idaapi.func_type_data_t()
    if not tinfo.get_func_details(funcdata):
        # print("Unable to get function details for 0x{0:X}".format(ea))
        return

    # Safety checks
    if funcdata.size() <= arg_idx:
        # print("Argument index out of bounds for 0x{0:X}".format(ea))
        return

    # Set name if provided
    if name is not None:
        funcdata[arg_idx].name = name

    type_tinfo = idaapi.tinfo_t()
    ptr_tinfo = None
    if setting_type:
        if not type_tinfo.get_named_type(idaapi.get_idati(), type):
            terminated = type + ";"
            if (
                idaapi.parse_decl(
                    type_tinfo, idaapi.get_idati(), terminated, idaapi.PT_SIL
                )
                is None
            ):
                # print("Unable to parse type '{0}' for 0x{1:X}".format(type, ea))
                return

        # Make it a pointer if specified
        if ptr > 0:
            ptr_tinfo = idaapi.tinfo_t()
            for i in range(ptr):
                if not ptr_tinfo.create_ptr(type_tinfo):
                    # print("Unable to create pointer type for 0x{0:X}".format(ea))
                    return
        else:
            ptr_tinfo = type_tinfo

        orig_type = funcdata[arg_idx].type
        if orig_type.get_size() < ptr_tinfo.get_size():
            # print(
            #    "Error: Type size mismatch for {0} at 0x{1:X} (expected {2:X}, got {3:X})".format(
            #        name, ea, orig_type.get_size(), ptr_tinfo.get_size()
            #    )
            # )
            return

        funcdata[arg_idx].type = ptr_tinfo

    new_tinfo = idaapi.tinfo_t()
    if not new_tinfo.create_func(funcdata):
        # print(
        #    "Error: Unable to create new function type for {0} at 0x{1:X}".format(
        #        name, ea
        #    )
        # )
        return

    idaapi.apply_tinfo(ea, new_tinfo, idaapi.TINFO_DEFINITE)


def try_set_func_ret_type(ea, type, ptr=0):
    tinfo = idaapi.tinfo_t()
    ptr_tinfo = None
    if not tinfo.get_named_type(idaapi.get_idati(), type):
        terminated = type + ";"
        if (
            idaapi.parse_decl(tinfo, idaapi.get_idati(), terminated, idaapi.PT_SIL)
            is None
        ):
            # print("Unable to parse return type for 0x{0:X}".format(ea))
            return

    # Get the data from the original function to replace
    func_tinfo = idaapi.tinfo_t()
    funcdata = idaapi.func_type_data_t()

    if not ida_nalt.get_tinfo(func_tinfo, ea):
        # print("Unable to get type info for 0x{0:X}".format(ea))
        return

    if not func_tinfo.get_func_details(funcdata):
        # print("Unable to get function details for 0x{0:X}".format(ea))
        return

    # Make it a pointer if specified
    if ptr > 0:
        ptr_tinfo = idaapi.tinfo_t()
        for i in range(ptr):
            if not ptr_tinfo.create_ptr(tinfo):
                # print("Unable to create pointer type for 0x{0:X}".format(ea))
                return
    else:
        ptr_tinfo = tinfo

    if funcdata.rettype.get_size() < ptr_tinfo.get_size():
        # print(
        #    "Error: Return type size mismatch for 0x{0:X} (expected {1:X}, got {2:X})".format(
        #        ea, funcdata.rettype.get_size(), ptr_tinfo.get_size()
        #    )
        # )
        return

    # Create a new type info with our applied type
    funcdata.rettype = ptr_tinfo
    new_tinfo = idaapi.tinfo_t()
    if not new_tinfo.create_func(funcdata):
        # print("Error: Unable to create new function type for 0x{0:X}".format(ea))
        return

    idaapi.apply_tinfo(ea, new_tinfo, idaapi.TINFO_DEFINITE)


def scan(sig):
    patterns = ida_bytes.compiled_binpat_vec_t()
    ida_bytes.parse_binpat_str(patterns, 0, sig, 16)
    addr = ida_bytes.bin_search(
        idaapi.cvar.inf.min_ea, idaapi.cvar.inf.max_ea, patterns, 0
    )
    if addr == idaapi.BADADDR:
        print(f"Unable to find signature {sig}")
        return None

    if sig.startswith("E8") or sig.startswith("E9"):
        offset = ida_bytes.get_wide_dword(addr + 1)
        offset = ~0x7FFFFFFF | offset  # convert to signed
        addr += 5 + offset

    return addr


def handle_vars(addr):
    print(f"handle_vars: {addr:X}")
    while True:
        variable = ida_bytes.get_qword(addr)
        if variable == 0:
            break

        string = ida_bytes.get_strlit_contents(
            ida_bytes.get_qword(variable), -1, ida_nalt.STRTYPE_C
        )
        string = string.decode("utf-8")
        idaapi.set_name(variable + 8, f"var_{string}", idaapi.SN_FORCE)

        addr += 8


def handle_funcs(addr):
    print(f"handle_funcs: {addr:X}")
    while True:
        func_addr = ida_bytes.get_qword(addr)
        if func_addr == 0:
            break

        func_name_addr = ida_bytes.get_qword(func_addr)
        if func_name_addr == 0:
            break

        func_name = ida_bytes.get_strlit_contents(
            func_name_addr, -1, ida_nalt.STRTYPE_C
        )
        if func_name is None:
            break
        func_name = func_name.decode("utf-8")

        reference = func_addr + 8
        idaapi.set_name(reference, f"func_{func_name}", idaapi.SN_FORCE)

        addr += 8


def handle_gml_funcs(addr):
    print(f"handle_gml_funcs: {addr:X}")
    while True:
        func_addr = ida_bytes.get_qword(addr)
        if func_addr == 0:
            break

        func_name = ida_bytes.get_strlit_contents(func_addr, -1, ida_nalt.STRTYPE_C)
        if func_name is None:
            break
        func_name = func_name.decode("utf-8")

        func = ida_bytes.get_qword(addr + 8)
        idaapi.set_name(func, func_name, idaapi.SN_FORCE)

        # is this correct?
        func_ref = ida_bytes.get_qword(addr + 16) + 8
        idaapi.set_name(func_ref, f"funcref_{func_name}", idaapi.SN_FORCE)

        try_set_func_arg_type(func, 0, "CInstance", 1, "self")
        try_set_func_arg_type(func, 1, "CInstance", 1, "other")
        try_set_func_arg_type(func, 2, "RValue", 1, "return_value")
        try_set_func_arg_type(func, 3, "int", 0, "num_args")
        try_set_func_arg_type(func, 4, "RValue", 2, "args")
        try_set_func_ret_type(func, "RValue", 1)

        addr += 24


def parse_init_llvm():
    init_llvm = scan("E8 ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? 8B 41 14")
    if init_llvm is None:
        return

    print(f"init_llvm: {init_llvm:X}")
    f = ida_funcs.get_func(init_llvm)
    insns = list(idautils.FuncItems(f.start_ea))
    for ea in insns:
        insn = idaapi.insn_t()
        idaapi.decode_insn(insn, ea)

        if insn.itype == ida_allins.NN_mov:
            offset = insn.ops[0].addr
            if offset == 0x18 or offset == 0x20 or offset == 0x28:
                for i in range(insns.index(ea) - 1, -1, -1):
                    insn = idaapi.insn_t()
                    idaapi.decode_insn(insn, insns[i])
                    if insn.itype == ida_allins.NN_lea:
                        addr = insn.ops[1].addr
                        if offset == 0x18:
                            handle_vars(addr)
                        if offset == 0x20:
                            handle_funcs(addr)
                        if offset == 0x28:
                            handle_gml_funcs(addr)
                        break


parse_init_llvm()
