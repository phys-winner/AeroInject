#include <windows.h>
/* clang-format off */
#include <tlhelp32.h>
/* clang-format on */
#include <iostream>
#include <string>
#include <vector>

// Get filename from path
std::string GetFileName(const std::string &path) {
  size_t lastSlash = path.find_last_of("\\/");
  if (lastSlash == std::string::npos)
    return path;
  return path.substr(lastSlash + 1);
}

// Find process ID by name (case-insensitive)
DWORD GetProcessIdByName(const std::string &processName, DWORD excludePid = 0) {
  DWORD foundPid = 0;
  HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (snapshot != INVALID_HANDLE_VALUE) {
    PROCESSENTRY32 processEntry;
    processEntry.dwSize = sizeof(processEntry);
    if (Process32First(snapshot, &processEntry)) {
      do {
        std::string exeFile = processEntry.szExeFile;
        if (exeFile.length() == processName.length()) {
          bool match = true;
          for (size_t i = 0; i < processName.length(); ++i) {
            if (tolower(processName[i]) != tolower(exeFile[i])) {
              match = false;
              break;
            }
          }
          if (match && processEntry.th32ProcessID != excludePid) {
            foundPid = processEntry.th32ProcessID;
            break;
          }
        }
      } while (Process32Next(snapshot, &processEntry));
    }
    CloseHandle(snapshot);
  }
  return foundPid;
}

// Check if string is a valid PID (all digits)
bool IsNumeric(const std::string &str) {
  for (char c : str) {
    if (!isdigit(c))
      return false;
  }
  return true;
}

bool InjectDLL(HANDLE hProcess, const std::string &dllPath) {
  if (!hProcess)
    return false;

  void *pRemotePath = VirtualAllocEx(hProcess, nullptr, dllPath.length() + 1,
                                     MEM_COMMIT, PAGE_READWRITE);
  if (!pRemotePath) {
    std::cerr << "Failed to allocate memory. Error: " << GetLastError()
              << std::endl;
    return false;
  }

  if (!WriteProcessMemory(hProcess, pRemotePath, dllPath.c_str(),
                          dllPath.length() + 1, nullptr)) {
    std::cerr << "Failed to write memory. Error: " << GetLastError()
              << std::endl;
    VirtualFreeEx(hProcess, pRemotePath, 0, MEM_RELEASE);
    return false;
  }

  HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
  LPTHREAD_START_ROUTINE pLoadLibrary =
      (LPTHREAD_START_ROUTINE)GetProcAddress(hKernel32, "LoadLibraryA");

  HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0, pLoadLibrary,
                                      pRemotePath, 0, nullptr);
  if (!hThread) {
    std::cerr << "Failed to create remote thread. Error: " << GetLastError()
              << std::endl;
    VirtualFreeEx(hProcess, pRemotePath, 0, MEM_RELEASE);
    return false;
  }

  WaitForSingleObject(hThread, INFINITE);
  VirtualFreeEx(hProcess, pRemotePath, 0, MEM_RELEASE);
  CloseHandle(hThread);
  return true;
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    std::cout << "Usage: " << argv[0] << " <path_to_dll> <target>" << std::endl;
    std::cout << "Target can be a PID (e.g. 1234) to attach, or an EXE path to "
                 "launch."
              << std::endl;
    return 1;
  }

  std::string dllPath = argv[1];
  std::string target = argv[2];

  char fullDllPath[MAX_PATH];
  if (GetFullPathNameA(dllPath.c_str(), MAX_PATH, fullDllPath, nullptr) == 0) {
    std::cerr << "Invalid DLL path." << std::endl;
    return 1;
  }

  HANDLE hProcess = nullptr;
  DWORD pid = 0;

  if (IsNumeric(target)) {
    // Mode 1: Attach to PID
    pid = std::stoul(target);
    std::cout << "Attaching to PID: " << pid << std::endl;
    hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProcess) {
      std::cerr << "Failed to open process. Error: " << GetLastError()
                << std::endl;
      return 1;
    }
  } else {
    // Mode 2: Launch Process
    std::cout << "Launching process: " << target << std::endl;
    STARTUPINFOA si = {sizeof(si)};
    PROCESS_INFORMATION pi = {0};

    // CreateProcess requires mutable command line
    std::vector<char> cmdLine(target.begin(), target.end());
    cmdLine.push_back(0);

    if (!CreateProcessA(nullptr, cmdLine.data(), nullptr, nullptr, FALSE, 0,
                        nullptr, nullptr, &si, &pi)) {
      std::cerr << "Failed to launch process. Error: " << GetLastError()
                << std::endl;
      return 1;
    }

    hProcess = pi.hProcess;
    pid = pi.dwProcessId;
    std::cout << "Process launched. PID: " << pid << std::endl;

    // Wait for initialization and check if it restarts or spawns a new instance
    std::cout << "Waiting for process to stabilize..." << std::endl;
    Sleep(3000);

    DWORD exitCode;
    std::string processName = GetFileName(target);

    // If the process we launched exited, it might have restarted itself
    if (GetExitCodeProcess(hProcess, &exitCode) && exitCode != STILL_ACTIVE) {
      std::cout << "Initial process exited (Exit Code: " << exitCode
                << "). Searching for new instance of " << processName << "..."
                << std::endl;

      bool found = false;
      for (int i = 0; i < 20; ++i) { // Retry for ~10 seconds
        DWORD newPid = GetProcessIdByName(processName, pid);
        if (newPid != 0) {
          HANDLE hNewProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, newPid);
          if (hNewProcess) {
            std::cout << "Found new instance. PID: " << newPid << std::endl;
            CloseHandle(hProcess);
            hProcess = hNewProcess;
            pid = newPid;
            found = true;
            break;
          }
        }
        Sleep(500);
      }

      if (!found) {
        std::cerr << "Could not find a new instance of the process."
                  << std::endl;
        CloseHandle(hProcess);
        return 1;
      }
    } else {
      // Even if the initial process is still running, check if there's a newer
      // instance that it might have spawned (some launchers work this way)
      DWORD newPid = GetProcessIdByName(processName, pid);
      if (newPid != 0) {
        std::cout << "Detected a different instance of " << processName
                  << " (PID: " << newPid
                  << "). It might be the actual target. Switching..."
                  << std::endl;
        HANDLE hNewProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, newPid);
        if (hNewProcess) {
          CloseHandle(hProcess);
          hProcess = hNewProcess;
          pid = newPid;
        }
      }
    }

    CloseHandle(pi.hThread);
  }

  std::cout << "Injecting DLL: " << fullDllPath << std::endl;
  if (InjectDLL(hProcess, fullDllPath)) {
    std::cout << "Injection successful!" << std::endl;
  } else {
    std::cerr << "Injection failed." << std::endl;
  }

  CloseHandle(hProcess);
  return 0;
}
