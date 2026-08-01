#!/bin/bash
out=/home/yuri/claude/cnc_game_proxy_merged/forwarders.c

cat > "$out" << 'HEADER'
/* Auto-generated winmm forwarding stubs. */
typedef unsigned short wchar_t;
#define DECLSPEC __declspec(dllexport)
#define NAKED __attribute__((naked))

void* __stdcall LoadLibraryW(const wchar_t*);
void* __stdcall LoadLibraryExW(const wchar_t*, void*, unsigned long);
int   __stdcall GetProcAddress(void*, const char*);
unsigned int __stdcall GetSystemDirectoryW(wchar_t*, unsigned int);
unsigned int __stdcall GetTempPathW(unsigned int, wchar_t*);
unsigned int __stdcall GetModuleFileNameW(void*, wchar_t*, unsigned int);
void* __stdcall GetModuleHandleW(const wchar_t*);
void* __stdcall CopyFileW(const wchar_t*, const wchar_t*, int);
int   __stdcall DeleteFileW(const wchar_t*);

static void* fwd_ptrs[179];

DECLSPEC void init_winmm_forwarders(void) {
    wchar_t sysPath[260], ourPath[260], tmpPath[260];
    unsigned int n;

    // Get System32 winmm.dll path
    n = GetSystemDirectoryW(sysPath, 260);
    if (!n || n >= 248) return;
    sysPath[n++] = L'\\';
    sysPath[n++] = L'w'; sysPath[n++] = L'i'; sysPath[n++] = L'n';
    sysPath[n++] = L'm'; sysPath[n++] = L'm'; sysPath[n++] = L'.';
    sysPath[n++] = L'd'; sysPath[n++] = L'l'; sysPath[n++] = L'l';
    sysPath[n] = 0;

    // Get our own DLL path
    void* ourModule = GetModuleHandleW(NULL);
    // Actually we need our own module, not the exe. Use a trick: get the
    // address of this function and query the module.
    // Simpler: just compare by trying to load and checking.
    
    // Try loading by full path. If the returned module is the same as
    // our own winmm.dll, we need to use a temp copy with a different name.
    void* h = LoadLibraryW(sysPath);
    if (!h) return;

    // Check if we got our own DLL instead of the real one
    // by comparing paths
    GetModuleFileNameW(h, ourPath, 260);
    
    // Simple heuristic: if the loaded module path contains "System32" 
    // (case-insensitive), it's the real one. Otherwise, we got our own.
    int isSystem32 = 0;
    for (int i = 0; ourPath[i] && i < 250; i++) {
        wchar_t c = ourPath[i];
        if (c >= L'A' && c <= L'Z') c += 32;
        if (c == L's' && ourPath[i+1] && (ourPath[i+1]==L'y'||ourPath[i+1]==L'Y')) {
            // Check for "system32"
            int match = 1;
            const wchar_t* pat = L"system32";
            for (int j = 0; j < 8; j++) {
                wchar_t pc = ourPath[i+j];
                if (pc >= L'A' && pc <= L'Z') pc += 32;
                if (pc != pat[j]) { match = 0; break; }
            }
            if (match) { isSystem32 = 1; break; }
        }
    }

    if (!isSystem32) {
        // We got our own DLL! Copy to temp with unique name.
        GetTempPathW(260, tmpPath);
        n = 0;
        while (tmpPath[n]) n++;
        tmpPath[n++] = L'r'; tmpPath[n++] = L'e'; tmpPath[n++] = L'a';
        tmpPath[n++] = L'l'; tmpPath[n++] = L'w'; tmpPath[n++] = L'm';
        tmpPath[n++] = L'm'; tmpPath[n++] = L'.'; tmpPath[n++] = L'd';
        tmpPath[n++] = L'l'; tmpPath[n++] = L'l'; tmpPath[n] = 0;

        // Delete any stale copy first
        DeleteFileW(tmpPath);
        // Copy System32 winmm.dll to temp
        if (!CopyFileW(sysPath, tmpPath, 0)) return;
        // Load the temp copy (different name, no conflict!)
        h = LoadLibraryW(tmpPath);
        // Clean up temp file immediately (Windows keeps the file until unloaded)
        DeleteFileW(tmpPath);
        if (!h) return;
    }

HEADER

i=0
grep '=' /home/yuri/claude/cnc_game_proxy_merged/winmm_mgw.def | while read line; do
    func=$(echo "$line" | awk '{print $1}')
    target=$(echo "$line" | sed 's/.*winmm\.//')
    echo "    fwd_ptrs[$i] = (void*)GetProcAddress(h, \"$target\"); // $func" >> "$out"
    i=$((i+1))
done

cat >> "$out" << 'EOF'
}

EOF

i=0
grep '=' /home/yuri/claude/cnc_game_proxy_merged/winmm_mgw.def | while read line; do
    func=$(echo "$line" | awk '{print $1}')
    echo "DECLSPEC NAKED void $func(void) { __asm__(\"jmp *_fwd_ptrs+$((i*4))\" : : : ); }" >> "$out"
    i=$((i+1))
done

echo "Generated $out ($(wc -l < $out) lines)"
