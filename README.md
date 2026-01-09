# AeroInject

A lightweight, high-performance DLL injector for Windows. **AeroInject** stands out by supporting both traditional PID attachment and a smart "Launch & Inject" mode that handles modern game launchers and process restarts automatically.

## Features

- **Smart Launch**: Automatically detects if a launched process (like a game launcher) spawns a new instance and tracks it for injection.
- **PID Attachment**: Robust injection into already running processes.
- **Minimal Footprint**: Written in pure C++ with minimal dependencies.
- **Modern CLI**: User-friendly command-line interface with clear status reporting.

## Usage

```powershell
AeroInject.exe <path_to_dll> <target>
```

- `<path_to_dll>`: The path to the DLL you want to inject.
- `<target>`: 
  - A **PID** (numeric) to attach to an existing process.
  - An **EXE path** (e.g., `C:\Games\Game.exe`) to launch and inject.

### Examples

**Attach to PID:**
```powershell
.\AeroInject.exe .\my_module.dll 1234
```

**Launch and Inject:**
```powershell
.\AeroInject.exe .\my_module.dll "C:\Path\To\Game.exe"
```

## Building

This project uses the MSVC compiler.

1. Open a **Developer Command Prompt for VS**.
2. Run the build script:
   ```powershell
   .\build.bat
   ```

## Roadmap

- [ ] Improve x64 injection stability.
- [ ] Improve detection for already launched instances.
- [ ] Add compatibility with Mono injection.

## Contributors

- **Eugene** ([phys-winner](https://github.com/phys-winner)) - Lead Developer
- **Antigravity** (AI Assistant) - Architecture and Documentation

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
