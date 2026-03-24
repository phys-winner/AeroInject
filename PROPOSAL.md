# Feature Proposal: Stealth Manual Mapping Injection

## Overview
Currently, AeroInject uses the `LoadLibraryA` technique via `CreateRemoteThread`. While reliable, this method is easily detected by modern anti-cheat (AC) and antivirus (AV) systems. **Manual Mapping** is a more advanced technique where the injector manually parses the PE headers of the DLL, allocates memory in the target, handles relocations, and resolves imports without involving the Windows Loader.

## Reasoning
- **Stealth**: Avoids triggering system-wide DLL load hooks and does not register the DLL in the process's module list.
- **Performance**: Bypassess the overhead of the Windows Loader for subsequent injections.
- **Project Alignment**: Aligns with AeroInject's goal of being a "high-performance" and "smart" injector for modern applications and games.

## Impacted Files
- `src/main.cpp`: Add `--manual-map` flag and integration logic.
- `src/injector.h/cpp`: Refactor injection logic to support multiple methods.
- `src/manual_mapper.cpp` (New): Implementation of the PE loader.

## Brief Implementation Plan
1. **PE Parser**: Implement a utility to read the DLL file and parse its `IMAGE_NT_HEADERS` and `IMAGE_SECTION_HEADER`.
2. **Remote Allocation**: Use `VirtualAllocEx` to allocate a block of memory in the target process.
3. **Mapping Sections**: Copy sections to their respective virtual offsets in the target process using `WriteProcessMemory`.
4. **Relocation Handling**: Process the `IMAGE_DIRECTORY_ENTRY_BASERELOC` table to adjust absolute addresses.
5. **Import Resolution**: Walk the `IMAGE_DIRECTORY_ENTRY_IMPORT` table and resolve function addresses in the target's IAT.
6. **Entry Point Execution**: Use `CreateRemoteThread` to call the DLL's entry point (`DllMain`).
