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
    // stdcall first — same order as the verified-good dsound proxy
    // (native_invoke_entry_point merely forwards to the stdcall entry).
    if (entryStdcall) entryStdcall(&info);
    else if (entryCdecl) entryCdecl(&info);

    dbg("Delegate done — NativeDll invoked");
    return 0;
}

// ===================================================================
// FPU precision mode — runtime-selectable via env var RA3BN_FPU:
//   off | 53 | 80   (default: 53)
//
//   off : don't touch FPU/SSE at all — identical to the loaders that
//         never desync (dsound proxy / Tacitus / cnc_game_proxy),
//         which all do zero FPU work and disable thread callbacks.
//   53  : force x87 PC=53-bit double on every thread — the REAL
//         Windows default control word 0x027F.  Wine already sets
//         0x027F in RtlUserThreadStart, so this is a no-op there and
//         only acts as a safety net elsewhere.
//   80  : old behavior (PC=11, 80-bit extended) — kept only for A/B
//         comparison; NOT the Windows default.
//
// History: the old code forced PC=11 (80-bit) claiming that was the
// "Windows default" — that premise was inverted (the Windows default
// is 0x027F = 53-bit).  80-vs-53-bit divergence between players
// accumulates over lockstep play and causes 失去同步 (out of sync).
//
// Compile-time default override: -DRA3BN_FPU_DEFAULT=0|1|2.
// ===================================================================
enum FpuMode { FPU_OFF = 0, FPU_53 = 1, FPU_80 = 2 };

#ifndef RA3BN_FPU_DEFAULT
#define RA3BN_FPU_DEFAULT FPU_53
#endif

static int g_fpu_mode = RA3BN_FPU_DEFAULT;

// RA3BN_FPU_LOG=1 → append per-thread CW/MXCSR before/after values
// to %TEMP%\ra3bn_fpu.log (first 32 lines only).
static int g_fpu_log_enabled = 0;
static volatile long g_fpu_log_count = 0;
static const long FPU_LOG_MAX = 32;
static HANDLE g_fpu_log_file = INVALID_HANDLE_VALUE;
static const char* const FPU_MODE_NAMES[] = { "off", "53", "80" };

// Manual compare — no CRT helpers (DllMain must stay loader-lock
// friendly; GetEnvironmentVariableA is a pure PEB read).
static int FpuModeFromEnv(void) {
    char buf[16] = {0};
    DWORD n = GetEnvironmentVariableA("RA3BN_FPU", buf, sizeof(buf));
    if (n == 0 || n >= sizeof(buf)) return RA3BN_FPU_DEFAULT;
    for (char* p = buf; *p; p++)
        if (*p >= 'A' && *p <= 'Z') *p += 32;
    if (buf[0]=='o' && buf[1]=='f' && buf[2]=='f' && buf[3]==0) return FPU_OFF;
    if (buf[0]=='8' && buf[1]=='0' && buf[2]==0)               return FPU_80;
    if (buf[0]=='5' && buf[1]=='3' && buf[2]==0)               return FPU_53;
    return RA3BN_FPU_DEFAULT;
}

static int FpuLogFromEnv(void) {
    char buf[8] = {0};
    DWORD n = GetEnvironmentVariableA("RA3BN_FPU_LOG", buf, sizeof(buf));
    return (n == 1 && buf[0] == '1');
}

// ---- optional diagnostics (kernel32 only, no CRT, no allocations) ----
static void FpuLogOpen(void) {
    wchar_t path[MAX_PATH];
    if (!GetTempPathW(MAX_PATH, path) || !path[0]) return;
    wcscat(path, L"ra3bn_fpu.log");
    g_fpu_log_file = CreateFileW(path, FILE_APPEND_DATA, FILE_SHARE_READ,
                                 NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
}

static void FpuHex16(char* out, unsigned short v) {
    static const char h[] = "0123456789ABCDEF";
    out[0] = '0'; out[1] = 'x';
    for (int i = 0; i < 4; i++) out[2 + i] = h[(v >> (12 - 4 * i)) & 0xF];
    out[6] = 0;
}

static void FpuHex32(char* out, unsigned int v) {
    static const char h[] = "0123456789ABCDEF";
    out[0] = '0'; out[1] = 'x';
    for (int i = 0; i < 8; i++) out[2 + i] = h[(v >> (28 - 4 * i)) & 0xF];
    out[10] = 0;
}

static void FpuLog(const char* tag,
                   unsigned short preCW, unsigned short postCW,
                   unsigned int preMX, unsigned int postMX) {
    if (g_fpu_log_file == INVALID_HANDLE_VALUE) return;
    if (InterlockedIncrement(&g_fpu_log_count) > FPU_LOG_MAX) return;
    char line[160], *p = line;
    while (*tag) *p++ = *tag++;
    *p++ = ' ';
    const char* m = FPU_MODE_NAMES[g_fpu_mode];
    while (*m) *p++ = *m++;
    *p++ = ' '; *p++ = 'p'; *p++ = 'r'; *p++ = 'e'; *p++ = 'C'; *p++ = 'W'; *p++ = '=';
    FpuHex16(p, preCW);  p += 6;
    *p++ = ' '; *p++ = 'p'; *p++ = 'o'; *p++ = 's'; *p++ = 't'; *p++ = 'C'; *p++ = 'W'; *p++ = '=';
    FpuHex16(p, postCW); p += 6;
    *p++ = ' '; *p++ = 'p'; *p++ = 'r'; *p++ = 'e'; *p++ = 'M'; *p++ = 'X'; *p++ = '=';
    FpuHex32(p, preMX);  p += 10;
    *p++ = ' '; *p++ = 'p'; *p++ = 'o'; *p++ = 's'; *p++ = 't'; *p++ = 'M'; *p++ = 'X'; *p++ = '=';
    FpuHex32(p, postMX); p += 10;
    *p++ = '\r'; *p++ = '\n';
    DWORD written = 0;
    WriteFile(g_fpu_log_file, line, (DWORD)(p - line), &written, NULL);
}

// 16-byte-aligned static buffer: stmxcsr/ldmxcsr fault on unaligned
// addresses; a static sidesteps 32-bit stack-alignment pitfalls.
static __attribute__((aligned(16))) unsigned int fpu_mxcsr_buf = 0x1F80;

// ===================================================================
// Set FPU+SSE to the selected mode — called once per game thread
// ===================================================================
static void fix_float_precision(void) {
    if (g_fpu_mode == FPU_OFF) return;

    unsigned short preCW;
    __asm__ volatile("fnstcw %0" : "=m"(preCW));
    unsigned int preMX;
    __asm__ volatile("stmxcsr %0" : "=m"(fpu_mxcsr_buf));
    preMX = fpu_mxcsr_buf;

    // Set only the PC (precision-control) bits 8-9; preserve RC
    // (rounding) and exception masks.  53-bit → 0x0200 (real Windows
    // default 0x027F); 80-bit → 0x0300 (old behavior, comparison only).
    unsigned short cw = (unsigned short)((preCW & 0xFCFF) |
                          (g_fpu_mode == FPU_80 ? 0x0300 : 0x0200));
    __asm__ volatile("fldcw %0" : : "m"(cw));

    // SSE MXCSR: Windows default = 0x1F80 (round-nearest, all
    // exceptions masked, no FTZ/DAZ).  Already correct — unchanged.
    fpu_mxcsr_buf = 0x1F80;
    __asm__ volatile("ldmxcsr %0" : : "m"(fpu_mxcsr_buf));

    if (g_fpu_log_enabled) {
        unsigned short postCW;
        unsigned int postMX;
        __asm__ volatile("fnstcw %0" : "=m"(postCW));
        __asm__ volatile("stmxcsr %0" : "=m"(fpu_mxcsr_buf));
        postMX = fpu_mxcsr_buf;
        FpuLog("thread", preCW, postCW, preMX, postMX);
    }
}

// One snapshot line for process attach (works in off mode too).
static void FpuLogSnapshot(const char* tag) {
    if (!g_fpu_log_enabled) return;
    unsigned short cw;
    __asm__ volatile("fnstcw %0" : "=m"(cw));
    __asm__ volatile("stmxcsr %0" : "=m"(fpu_mxcsr_buf));
    unsigned int mx = fpu_mxcsr_buf;
    FpuLog(tag, cw, cw, mx, mx);
}

// ===================================================================
// DllMain
// ===================================================================
BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID) {
    switch (reason) {
    case DLL_PROCESS_ATTACH:
        dbg("DLL_PROCESS_ATTACH");

        // Read FPU mode once; the process environment is fixed from here on.
        g_fpu_mode = FpuModeFromEnv();
        if (FpuLogFromEnv()) { g_fpu_log_enabled = 1; FpuLogOpen(); }

        fix_float_precision();   // main thread
        FpuLogSnapshot("attach");

        if (g_fpu_mode == FPU_OFF) {
            // off mode: exactly like the loaders that never desync —
            // no per-thread FPU work at all, no thread callbacks.
            DisableThreadLibraryCalls(h);
        }
        // modes 53/80: keep DLL_THREAD_ATTACH so every game thread
        // gets Windows-default precision before its routine runs (if
        // the game sets its own CW later, the game wins — the same as
        // on real Windows).

        init_winmm_forwarders();
        {
            HANDLE t = CreateThread(nullptr, 0, DelegateWorker, nullptr, 0, nullptr);
            if (t) CloseHandle(t);
        }
        dbg("DllMain done");
        break;

    case DLL_THREAD_ATTACH:
        // Apply FPU mode to every new thread the game spawns
        // (no-op in off mode — callbacks are disabled there anyway).
        fix_float_precision();
        break;
    }
    return TRUE;
}
