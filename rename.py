import os
import idaapi
import ida_nalt


def try_set_func_arg_type(ea, arg_idx, type, ptr=0, name=None):
    setting_type = type != "_" and type is not None

    # Create type info for this function
    tinfo = idaapi.tinfo_t()
    if not ida_nalt.get_tinfo(tinfo, ea):
        print("Unable to get type info for 0x{0:X}".format(ea))
        return

    # Get function data from type info
    funcdata = idaapi.func_type_data_t()
    if not tinfo.get_func_details(funcdata):
        print("Unable to get function details for 0x{0:X}".format(ea))
        return

    # Safety checks
    if funcdata.size() <= arg_idx:
        print("Argument index out of bounds for 0x{0:X}".format(ea))
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
                print("Unable to parse type '{0}' for 0x{1:X}".format(type, ea))
                return

        # Make it a pointer if specified
        if ptr > 0:
            ptr_tinfo = idaapi.tinfo_t()
            for i in range(ptr):
                if not ptr_tinfo.create_ptr(type_tinfo):
                    print("Unable to create pointer type for 0x{0:X}".format(ea))
                    return
        else:
            ptr_tinfo = type_tinfo

        orig_type = funcdata[arg_idx].type
        if orig_type.get_size() < ptr_tinfo.get_size():
            print(
                "Error: Type size mismatch for {0} at 0x{1:X} (expected {2:X}, got {3:X})".format(
                    name, ea, orig_type.get_size(), ptr_tinfo.get_size()
                )
            )
            return

        funcdata[arg_idx].type = ptr_tinfo

    new_tinfo = idaapi.tinfo_t()
    if not new_tinfo.create_func(funcdata):
        print(
            "Error: Unable to create new function type for {0} at 0x{1:X}".format(
                name, ea
            )
        )
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
            print("Unable to parse return type for 0x{0:X}".format(ea))
            return

    # Get the data from the original function to replace
    func_tinfo = idaapi.tinfo_t()
    funcdata = idaapi.func_type_data_t()

    if not ida_nalt.get_tinfo(func_tinfo, ea):
        print("Unable to get type info for 0x{0:X}".format(ea))
        return

    if not func_tinfo.get_func_details(funcdata):
        print("Unable to get function details for 0x{0:X}".format(ea))
        return

    # Make it a pointer if specified
    if ptr > 0:
        ptr_tinfo = idaapi.tinfo_t()
        for i in range(ptr):
            if not ptr_tinfo.create_ptr(tinfo):
                print("Unable to create pointer type for 0x{0:X}".format(ea))
                return
    else:
        ptr_tinfo = tinfo

    if funcdata.rettype.get_size() < ptr_tinfo.get_size():
        print(
            "Error: Return type size mismatch for 0x{0:X} (expected {1:X}, got {2:X})".format(
                ea, funcdata.rettype.get_size(), ptr_tinfo.get_size()
            )
        )
        return

    # Create a new type info with our applied type
    funcdata.rettype = ptr_tinfo
    new_tinfo = idaapi.tinfo_t()
    if not new_tinfo.create_func(funcdata):
        print("Error: Unable to create new function type for 0x{0:X}".format(ea))
        return

    idaapi.apply_tinfo(ea, new_tinfo, idaapi.TINFO_DEFINITE)


file = os.path.join(os.path.dirname(os.path.realpath(__file__)), "dump.txt")
with open(file, "r") as f:
    lines = f.readlines()

    for line in lines:
        words = line.split(" ")

        func_name = words[0]
        rva = words[1]
        rva = int(rva, 16)
        rva = rva + 0x140000000  # lol

        idaapi.set_name(rva, func_name)

        # set function signature
        try_set_func_arg_type(rva, 0, "CInstance", 1, "self")
        try_set_func_arg_type(rva, 1, "CInstance", 1, "other")
        try_set_func_arg_type(rva, 2, "RValue", 1, "return_value")
        try_set_func_arg_type(rva, 3, "int", 0, "num_args")
        try_set_func_arg_type(rva, 4, "RValue", 2, "args")
        try_set_func_ret_type(rva, "RValue", 1)
