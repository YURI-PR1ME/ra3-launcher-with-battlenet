/*
 * d3d9_dll.cpp — RA3 BattleNet proxy DLL (d3d9 version)
 *
 * Loads as d3d9.dll in the game directory.
 * Forwards all d3d9 calls to real System32 d3d9.dll.
 * Loads RA3 BattleNet's NativeDll.dll into the game process.
 *
 * Unlike winmm.dll, d3d9.dll is NOT a Windows Known DLL, so
 * DLL search-order hijacking works natively on both Windows and Wine.
 *
 * Requires: ra3bn.ini next to the game exe (E:\RA3\RA3\ra3bn.ini)
 *   [ra3bn]
 *   path=Z:\path\to\RA3BattleNet\contents\NativeDll.dll
 *
 * Platform support:
 *   - Windows: drop d3d9.dll next to RA3.exe, no extra config needed
 *   - Wine:    needs d3d9=native,builtin override (launcher writes registry)
 */

#include <windows.h>
#include <cstdio>
#include <cstdint>
#include <ctime>

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
extern "C" void init_d3d9_forwarders(void);

// ===================================================================
// Debug — writes to both OutputDebugString and %TEMP%\d3d9_proxy.log
// ===================================================================
static void dbg(const char* msg) {
    OutputDebugStringA(msg);

    static FILE* logFile = nullptr;
    if (!logFile) {
        wchar_t logPath[MAX_PATH];
        if (GetTempPathW(MAX_PATH, logPath) && logPath[0]) {
            wcscat(logPath, L"d3d9_proxy.log");
            logFile = _wfopen(logPath, L"a");
        }
        if (!logFile) {
            // last resort: try current directory
            logFile = _wfopen(L"d3d9_proxy.log", L"a");
        }
    }
    if (logFile) {
        SYSTEMTIME st;
        GetLocalTime(&st);
        fprintf(logFile, "%02d:%02d:%02d.%03d %s\n",
                st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, msg);
        fflush(logFile);
    }
}

// ===================================================================
// Simple INI reader — just reads [ra3bn] path=...
// ===================================================================
static bool ReadConfig(wchar_t* out, size_t outLen) {
    // Only check 2 locations: current dir + parent dir (game root)
    wchar_t paths[2][MAX_PATH];
    int nPaths = 0;

    // 1. Current directory
    if (GetCurrentDirectoryW(MAX_PATH, paths[nPaths]) && paths[nPaths][0]) {
        wcscat(paths[nPaths], L"\\ra3bn.ini");
        nPaths++;
    }

    // 2. Parent directory (game root)
    if (GetCurrentDirectoryW(MAX_PATH, paths[nPaths]) && paths[nPaths][0]) {
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
    dbg("d3d9: Delegate worker started");
    Sleep(2000);  // small delay to let game init

    dbg("d3d9: Waiting for game window...");
    if (!WaitForGameWindow(GAME_WINDOW_TIMEOUT)) {
        dbg("d3d9: Window timeout");
        return 1;
    }
    dbg("d3d9: Game window found");

    // Read config or use default
    wchar_t dllPath[MAX_PATH];
    if (!ReadConfig(dllPath, MAX_PATH)) {
        wcscpy(dllPath, DEFAULT_PATH);
        dbg("d3d9: Using default NativeDll path");
    } else {
        dbg("d3d9: Using path from ra3bn.ini");
    }

    {
        char buf[520];
        int n = WideCharToMultiByte(CP_ACP, 0, dllPath, -1, buf, sizeof(buf)-1, NULL, NULL);
        if (n > 0) { buf[n-1] = 0; dbg(buf); }
    }

    // Check file existence first
    {
        DWORD attr = GetFileAttributesW(dllPath);
        if (attr == INVALID_FILE_ATTRIBUTES) {
            char buf[128];
            wsprintfA(buf, "d3d9: NativeDll file not found (err=%d)", GetLastError());
            dbg(buf);
            return 1;
        }
        dbg("d3d9: NativeDll file exists on disk");
    }

    HMODULE mod = LoadLibraryW(dllPath);
    if (!mod) {
        char buf[128];
        wsprintfA(buf, "d3d9: LoadLibrary failed: err=0x%08X", GetLastError());
        dbg(buf);
        return 1;
    }
    dbg("d3d9: NativeDll loaded");

    auto entryCdecl = (void(__cdecl*)(void*))
        GetProcAddress(mod, "native_invoke_entry_point");
    auto entryStdcall = (void(__stdcall*)(void*))
        GetProcAddress(mod, "_NativeInjectionEntryPoint@4");

    if (!entryCdecl && !entryStdcall) {
        char buf[128];
        wsprintfA(buf, "d3d9: No entry point found (mod=%p)", mod);
        dbg(buf);
        return 1;
    }
    {
        char buf[64];
        wsprintfA(buf, "d3d9: Entry point found (%s)",
                  entryCdecl ? "cdecl" : "stdcall");
        dbg(buf);
    }

    RemoteEntryInfo info = {};
    info.host_pid = GetCurrentProcessId();

    // Try both entry points.  Also try hook_by_set_windows_hook
    // which is the third export — it may be a standalone init path.
    auto entryHook = (void(__stdcall*)(void*))
        GetProcAddress(mod, "hook_by_set_windows_hook");

    dbg("d3d9: Trying stdcall entry...");
    if (entryStdcall) {
        entryStdcall(&info);
        dbg("d3d9: stdcall entry RETURNED");
    }

    dbg("d3d9: Trying hook_by_set_windows_hook...");
    if (entryHook) {
        entryHook(&info);
        dbg("d3d9: hook_by_set_windows_hook RETURNED");
    }

    dbg("d3d9: Trying cdecl entry...");
    if (entryCdecl) {
        entryCdecl(&info);
        dbg("d3d9: cdecl entry RETURNED");
    }

    dbg("d3d9: All done");
    return 0;
}

// ===================================================================
// Set FPU+SSE to Windows defaults — called for every thread
//
// Needed on Wine/Linux (which defaults to 64-bit x87 precision,
// causing RTS OOS vs Windows players).  On real Windows this is
// already the default — the sequence is a harmless no-op there.
// ===================================================================
static void fix_float_precision(void) {
    // x87 FPU: 80-bit extended precision (PC=11)
    unsigned short cw;
    __asm__ volatile("fnstcw %0" : "=m"(cw));
    cw = (cw & 0xFCFF) | 0x0300;
    __asm__ volatile("fldcw %0" : : "m"(cw));

    // SSE MXCSR: Windows default = 0x1F80
    __attribute__((aligned(16))) unsigned int mxcsr = 0x1F80;
    __asm__ volatile("ldmxcsr %0" : : "m"(mxcsr));
}

// ===================================================================
// DllMain
// ===================================================================
BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID) {
    switch (reason) {
    case DLL_PROCESS_ATTACH:
        dbg("d3d9: DLL_PROCESS_ATTACH");
        fix_float_precision();
        dbg("d3d9: FPU+SSE set to Windows defaults (main thread)");

        // Do NOT disable thread calls — we need DLL_THREAD_ATTACH
        // to fix FPU/SSE for every game-created thread.
        init_d3d9_forwarders();
        {
            HANDLE t = CreateThread(nullptr, 0, DelegateWorker, nullptr, 0, nullptr);
            if (t) CloseHandle(t);
        }
        dbg("d3d9: DllMain done");
        break;

    case DLL_THREAD_ATTACH:
        // Fix FPU/SSE for every new thread the game spawns
        fix_float_precision();
        break;
    }
    return TRUE;
}
