@echo off
setlocal

:: Check if cl.exe is in PATH
where cl.exe >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo [!] Error: cl.exe not found. 
    echo Please run this script from a 'Developer Command Prompt for VS'.
    exit /b 1
)

echo [*] Compiling...
cl /EHsc /O2 src\main.cpp /link /out:AeroInject.exe

if %ERRORLEVEL% equ 0 (
    echo [+] Build successful: AeroInject.exe
) else (
    echo [-] Build failed.
)

endlocal
