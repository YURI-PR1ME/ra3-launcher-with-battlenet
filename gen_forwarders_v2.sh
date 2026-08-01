#!/bin/bash
out=/home/yuri/claude/cnc_game_proxy_merged/forwarders.c

cat > "$out" << 'HEADER'
/* Auto-generated winmm forwarding stubs.
 * Loads System32 winmm.dll via temp copy to avoid module name collision
 * (our DLL is also named winmm.dll, so LoadLibrary would return our handle).
 */
typedef unsigned short wchar_t;
#define DECLSPEC __declspec(dllexport)
#define NAKED __attribute__((naked))

void* __stdcall LoadLibraryW(const wchar_t*);
int   __stdcall GetProcAddress(void*, const char*);
unsigned int __stdcall GetSystemDirectoryW(wchar_t*, unsigned int);
unsigned int __stdcall GetTempPathW(unsigned int, wchar_t*);
int   __stdcall CopyFileW(const wchar_t*, const wchar_t*, int);
int   __stdcall DeleteFileW(const wchar_t*);

static void* fwd_ptrs[179];

DECLSPEC void init_winmm_forwarders(void) {
    wchar_t sysPath[260], tmpPath[260];
    unsigned int n;

    // Build System32 winmm path
    n = GetSystemDirectoryW(sysPath, 260);
    if (!n || n >= 240) return;
    sysPath[n++] = L'\\';
    sysPath[n++] = L'w'; sysPath[n++] = L'i'; sysPath[n++] = L'n';
    sysPath[n++] = L'm'; sysPath[n++] = L'm'; sysPath[n++] = L'.';
    sysPath[n++] = L'd'; sysPath[n++] = L'l'; sysPath[n++] = L'l';
    sysPath[n] = 0;

    // We CANNOT use LoadLibrary("C:\\Windows\\System32\\winmm.dll")
    // because our DLL is ALSO named "winmm.dll" and the loader returns
    // our own handle (module name collision).
    //
    // Workaround: copy to temp with unique name, load that, delete temp.
    n = GetTempPathW(260, tmpPath);
    if (!n || n >= 240) return;
    // Append unique name
    tmpPath[n++] = L'r'; tmpPath[n++] = L'w'; tmpPath[n++] = L'm';
    tmpPath[n++] = L'm'; tmpPath[n++] = L'_'; tmpPath[n++] = L'p';
    tmpPath[n++] = L'r'; tmpPath[n++] = L'o'; tmpPath[n++] = L'x';
    tmpPath[n++] = L'y'; tmpPath[n++] = L'.'; tmpPath[n++] = L'd';
    tmpPath[n++] = L'l'; tmpPath[n++] = L'l'; tmpPath[n] = 0;

    DeleteFileW(tmpPath);                      // cleanup stale
    if (!CopyFileW(sysPath, tmpPath, 0)) return; // copy
    void* h = LoadLibraryW(tmpPath);           // load (different name!)
    DeleteFileW(tmpPath);                      // delete (Windows keeps mapping)

    if (!h) {
        // Fallback: try direct load as last resort
        h = LoadLibraryW(sysPath);
    }
    if (!h) return;

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
