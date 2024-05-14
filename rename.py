import os
import idaapi

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
