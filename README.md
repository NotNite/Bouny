# Bouny

Rabbit and Steel reverse engineering.

## Dependencies

- [Aurie](https://github.com/AurieFramework/Aurie)
- [YYToolkit](https://github.com/AurieFramework/YYToolkit)

## Reverse engineering

Import the structs.h header file into your decompiler, then run the rename.py script. It should automatically fix function names and arguments, along with provide context for static variables, functions, and function references.

Currently only supports IDA.

## Compiling

Uses vcpkg:

```shell
git submodule update --init --recursive -f
.\vcpkg\bootstrap-vcpkg.bat
```

Then build with MSBuild. Copy Bouny.dll to mods/Aurie. Any other .dlls (like asmjit) go in the root game folder.
