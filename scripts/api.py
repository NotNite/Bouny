try:
    from typing import List
except ImportError:
    # It's... just typing. Doesn't matter.
    pass


class Instruction:
    def __init__(self, address, opcode, operands, values):
        # type: (int, str, List[int], List[int]) -> None
        self.address = address
        self.opcode = opcode
        self.operands = operands
        self.values = values


class BaseApi(object):
    def scan(self, signature):
        # type: (str) -> int
        pass

    def get_instructions(self, address):
        # type: (int) -> List[Instruction]
        pass

    def get_name(self, address):
        # type: (int) -> str
        pass

    def set_name(self, address, name):
        # type: (int, str) -> bool
        pass

    def read_u64(self, address):
        # type: (int) -> int
        pass

    def read_str(self, address):
        # type: (int) -> str
        pass

    def set_func_arg_type(self, func, index, type, ptr, name):
        # type: (int, int, str, int, str) -> bool
        pass

    def set_func_ret_type(self, func, type, ptr):
        # type: (int, str, int) -> bool
        pass


api = None  # type: BaseApi

try:
    import ida_bytes
    import ida_ida
    import ida_idaapi
    import ida_funcs
    import idautils
    import ida_ua
    import ida_name
    import ida_typeinf
    import ida_nalt

    class IDAApi(BaseApi):
        def scan(self, signature):
            patterns = ida_bytes.compiled_binpat_vec_t()
            ida_bytes.parse_binpat_str(patterns, 0, signature, 16)

            addr = ida_bytes.bin_search(
                ida_ida.inf_get_min_ea(), ida_ida.inf_get_max_ea(), patterns, 0
            )
            if addr == ida_idaapi.BADADDR:
                print("oops")
                return None

            if signature.startswith("E8") or signature.startswith("E9"):
                # Displacement
                offset = ida_bytes.get_wide_dword(addr + 1)
                offset = ~0x7FFFFFFF | offset  # convert to signed
                addr += 5 + offset

            return addr

        def get_instructions(self, address):
            func = ida_funcs.get_func(address)
            if func is None:
                return []

            insns = list(idautils.FuncItems(func.start_ea))
            for insn_addr in insns:
                insn = ida_ua.insn_t()
                ida_ua.decode_insn(insn, insn_addr)
                yield Instruction(
                    insn_addr,
                    insn.get_canon_mnem(),
                    list(op.addr for op in insn.ops),
                    list(op.value for op in insn.ops),
                )

        def get_name(self, address):
            return ida_funcs.get_func_name(address)

        def set_name(self, address, name):
            return ida_name.set_name(address, name, ida_name.SN_FORCE)

        def read_u64(self, address):
            return ida_bytes.get_qword(address)

        def read_str(self, address):
            str = ida_bytes.get_strlit_contents(address, -1, ida_nalt.STRTYPE_C)
            if str is None:
                return None
            return str.decode("utf-8")

        def set_func_arg_type(self, func, index, type, ptr, name):
            tinfo = ida_typeinf.tinfo_t()
            if not ida_nalt.get_tinfo(tinfo, func):
                return False

            func_data = ida_typeinf.func_type_data_t()
            if not tinfo.get_func_details(func_data):
                return False

            if index >= func_data.size():
                return False

            if name is not None:
                func_data[index].name = name

            type_tinfo = ida_typeinf.tinfo_t()
            idati = ida_typeinf.get_idati()
            if not type_tinfo.get_named_type(idati, type):
                terminated = type + ";"
                if not ida_typeinf.parse_decl(
                    type_tinfo, idati, terminated, ida_typeinf.PT_SIL
                ):
                    return False

            if ptr is not None:
                for _ in range(ptr):
                    type_tinfo.create_ptr(type_tinfo)

            orig_type = func_data[index].type
            if orig_type.get_size() < type_tinfo.get_size():
                return False

            func_data[index].type = type_tinfo

            new_tinfo = ida_typeinf.tinfo_t()
            if not new_tinfo.create_func(func_data):
                return False

            return ida_typeinf.apply_tinfo(func, new_tinfo, ida_typeinf.TINFO_DEFINITE)

        def set_func_ret_type(self, func, type, ptr):
            func_tinfo = ida_typeinf.tinfo_t()
            if not ida_nalt.get_tinfo(func_tinfo, func):
                return False

            func_data = ida_typeinf.func_type_data_t()
            if not func_tinfo.get_func_details(func_data):
                return False

            type_tinfo = ida_typeinf.tinfo_t()
            idati = ida_typeinf.get_idati()
            if not type_tinfo.get_named_type(idati, type):
                terminated = type + ";"
                if not ida_typeinf.parse_decl(
                    type_tinfo, idati, terminated, ida_typeinf.PT_SIL
                ):
                    return False

            if ptr is not None:
                for _ in range(ptr):
                    type_tinfo.create_ptr(type_tinfo)

            if func_data.rettype.get_size() < type_tinfo.get_size():
                return False

            func_data.rettype = type_tinfo
            new_tinfo = ida_typeinf.tinfo_t()
            if not new_tinfo.create_func(func_data):
                return False

            return ida_typeinf.apply_tinfo(func, new_tinfo, ida_typeinf.TINFO_DEFINITE)

    api = IDAApi()
except ImportError:
    import ghidra

    # https://github.com/VDOO-Connected-Trust/ghidra-pyi-generator
    # ...but the types still seem a little broken
    try:
        from typing import List
        from ghidra.ghidra_builtins import *
    except:
        pass

    # todo lmao
    class GhidraApi(BaseApi):
        pass

    api = GhidraApi()
