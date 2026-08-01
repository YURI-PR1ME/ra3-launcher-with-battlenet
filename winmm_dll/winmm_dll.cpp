/*
 * winmm_dll.cpp — RA3 BattleNet proxy DLL
 *
 * Loads as winmm.dll in RA3 Data/ directory.
 * Forwards all winmm calls to real System32 winmm.dll.
 * Loads RA3 BattleNet's NativeDll.dll into the game process.
 *
 * Requires: ra3bn.ini next to the game exe (E:\RA3\RA3\ra3bn.ini)
 *   [ra3bn]
 *   path=Z:\path\to\RA3BattleNet\contents\NativeDll.dll
 */

#include <windows.h>
#include <cstdio>
#include <cstdint>

// ===================================================================
// Default path (used if ra3bn.ini not found)
// ===================================================================
static const wchar_t* DEFAULT_PATH =
    L"Z:\\mnt\\game_disk\\battlenet\\RA3BattleNet\\contents\\NativeDll.dll";

static const DWORD GAME_WINDOW_TIMEOUT = 120; // seconds

static const wchar_t* WINDOW_CLASSES[] = {
    L"41DAF790-16F5-4881-8754-59FD8CF3B8D2",
    L"71DAF790-16F5-4881-8754-59FD8CF3B8D2"
};

// ===================================================================
// Forward from forwarders.c
// ===================================================================
extern "C" void init_winmm_forwarders(void);

// ===================================================================
// Debug
// ===================================================================
static void dbg(const char* msg) { OutputDebugStringA(msg); }

// ===================================================================
// Simple INI reader — just reads [ra3bn] path=...
// ===================================================================
static bool ReadConfig(wchar_t* out, size_t outLen) {
    // Only check 2 locations: current dir + parent dir (game root)
    wchar_t paths[2][MAX_PATH];
    int nPaths = 0;

    // 1. Current directory (Data\)
    if (GetCurrentDirectoryW(MAX_PATH, paths[nPaths]) && paths[nPaths][0]) {
        wcscat(paths[nPaths], L"\\ra3bn.ini");
        nPaths++;
    }

    // 2. Parent directory (game root E:\RA3\RA3\)
    if (GetCurrentDirectoryW(MAX_PATH, paths[nPaths]) && paths[nPaths][0]) {
        // Strip last component
        wchar_t* p = paths[nPaths] + wcslen(paths[nPaths]) - 1;
        while (p > paths[nPaths] && *p != L'\\' && *p != L'/') p--;
        if (p > paths[nPaths]) {
            *p = 0;
            wcscat(paths[nPaths], L"\\ra3bn.ini");
            nPaths++;
        }
    }

    for (int i = 0; i < nPaths; i++) {
        HANDLE h = CreateFileW(paths[i], GENERIC_READ, FILE_SHARE_READ,
                               NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (h == INVALID_HANDLE_VALUE) continue;

        char buf[1024];
        DWORD read = 0;
        ReadFile(h, buf, sizeof(buf) - 1, &read, NULL);
        CloseHandle(h);
        if (read == 0) continue;
        buf[read] = 0;

        char* pathLine = strstr(buf, "path=");
        if (!pathLine) continue;
        pathLine += 5;
        while (*pathLine == ' ' || *pathLine == '\t') pathLine++;
        char* end = pathLine;
        while (*end && *end != '\r' && *end != '\n') end++;

        int wlen = MultiByteToWideChar(CP_UTF8, 0, pathLine,
                                        (int)(end - pathLine), out, (int)outLen - 1);
        if (wlen > 0) { out[wlen] = 0; return true; }
    }

    dbg("ra3bn.ini not found, using default");
    return false;
}

// ===================================================================
// Check if this is RA3 1.12
// ===================================================================
static bool IsRA3_1_12() {
    wchar_t path[MAX_PATH];
    if (!GetModuleFileNameW(NULL, path, MAX_PATH)) return false;

    DWORD handle; DWORD size = GetFileVersionInfoSizeW(path, &handle);
    if (!size) return false;

    BYTE* data = new BYTE[size];
    if (!GetFileVersionInfoW(path, 0, size, data)) { delete[] data; return false; }

    VS_FIXEDFILEINFO* info = nullptr; UINT len = 0;
    bool ok = VerQueryValueW(data, L"\\", (LPVOID*)&info, &len) && len;
    int major = ok ? HIWORD(info->dwFileVersionMS) : 0;
    int minor = ok ? LOWORD(info->dwFileVersionMS) : 0;
    delete[] data;
    if (!ok) return false;

    wchar_t name[MAX_PATH];
    if (!GetModuleFileNameW(NULL, name, MAX_PATH)) return false;
    wchar_t* last = name;
    for (wchar_t* p = name; *p; p++)
        if (*p == L'\\' || *p == L'/') last = p + 1;
    bool isRA3 = (wcsncmp(last, L"ra3_", 4) == 0 ||
                  wcsncmp(last, L"RA3_", 4) == 0);

    return isRA3 && major == 1 && minor == 12;
}

// ===================================================================
// Wait for game window
// ===================================================================
static bool WaitForGameWindow(DWORD timeoutSec) {
    DWORD pid = GetCurrentProcessId();
    DWORD deadline = GetTickCount() + timeoutSec * 1000;
    for (;;) {
        for (auto* cls : WINDOW_CLASSES) {
            HWND w = nullptr;
            while ((w = FindWindowExW(nullptr, w, cls, nullptr)) != nullptr) {
                DWORD wpid = 0;
                GetWindowThreadProcessId(w, &wpid);
                if (wpid == pid) return true;
            }
        }
        if ((LONG)(GetTickCount() - deadline) >= 0) return false;
        Sleep(250);
    }
}

// ===================================================================
// Delegate Worker
// ===================================================================
#pragma pack(push, 1)
struct RemoteEntryInfo {
    uint32_t host_pid;
    const char* user_data;
    uint32_t user_data_size;
};
#pragma pack(pop)

static DWORD WINAPI DelegateWorker(LPVOID) {
    dbg("Delegate worker started");
    Sleep(2000);  // small delay to let game init

    // Skip IsRA3_1_12() — GetFileVersionInfo crashes under CrossOver.
    // The launcher already ensures correct game version.

    dbg("Waiting for game window...");
    if (!WaitForGameWindow(GAME_WINDOW_TIMEOUT)) { dbg("Window timeout"); return 1; }
    dbg("Game window found");

    // Read config or use default
    wchar_t dllPath[MAX_PATH];
    if (!ReadConfig(dllPath, MAX_PATH)) {
        wcscpy(dllPath, DEFAULT_PATH);
        dbg("Using default NativeDll path");
    } else {
        dbg("Using path from ra3bn.ini");
    }

    {
        char buf[520];
        int n = WideCharToMultiByte(CP_ACP, 0, dllPath, -1, buf, sizeof(buf)-1, NULL, NULL);
        if (n > 0) { buf[n-1] = 0; dbg(buf); }
    }

    HMODULE mod = LoadLibraryW(dllPath);
    if (!mod) {
        char buf[64];
        wsprintfA(buf, "LoadLibrary failed: err=%d", GetLastError());
        dbg(buf);
        return 1;
    }
    dbg("NativeDll loaded");

    auto entryCdecl = (void(__cdecl*)(void*))
        GetProcAddress(mod, "native_invoke_entry_point");
    auto entryStdcall = (void(__stdcall*)(void*))
        GetProcAddress(mod, "_NativeInjectionEntryPoint@4");

    if (!entryCdecl && !entryStdcall) { dbg("No entry point"); return 1; }
    dbg("Entry point found");

    RemoteEntryInfo info = {};
    info.host_pid = GetCurrentProcessId();

    dbg("Invoking entry point...");
    if (entryCdecl) entryCdecl(&info);
    else entryStdcall(&info);

    dbg("Delegate done — NativeDll invoked");
    return 0;
}

// ===================================================================
// Set FPU+SSE to Windows defaults — called for every thread
// ===================================================================
static void fix_float_precision(void) {
    // x87 FPU: 80-bit extended precision (PC=11)
    // Linux/Wine defaults to 64-bit, causing RTS OOS vs Windows players
    unsigned short cw;
    __asm__ volatile("fnstcw %0" : "=m"(cw));
    cw = (cw & 0xFCFF) | 0x0300;
    __asm__ volatile("fldcw %0" : : "m"(cw));

    // SSE MXCSR: Windows default = 0x1F80
    // (round-nearest, all exceptions masked, no flush-to-zero, no denormals-are-zero)
    unsigned int mxcsr = 0x1F80;
    __asm__ volatile("ldmxcsr %0" : : "m"(mxcsr));
}

// ===================================================================
// DllMain
// ===================================================================
BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID) {
    switch (reason) {
    case DLL_PROCESS_ATTACH:
        dbg("DLL_PROCESS_ATTACH");
        fix_float_precision();
        dbg("FPU+SSE set to Windows defaults (main thread)");

        // Do NOT disable thread calls — we need DLL_THREAD_ATTACH
        // to fix FPU/SSE for every game-created thread.
        init_winmm_forwarders();
        {
            HANDLE t = CreateThread(nullptr, 0, DelegateWorker, nullptr, 0, nullptr);
            if (t) CloseHandle(t);
        }
        dbg("DllMain done");
        break;

    case DLL_THREAD_ATTACH:
        // Fix FPU/SSE for every new thread the game spawns
        fix_float_precision();
        break;
    }
    return TRUE;
}
