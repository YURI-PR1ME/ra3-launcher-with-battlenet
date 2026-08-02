#!/bin/bash
# Regenerate forwarders.c from d3d9_mgw.def
# (rarely needed — d3d9 exports are stable)

out=forwarders.c

cat > "$out" << 'HEADER'
/* Auto-generated. Loads real d3d9 via temp copy (avoids module name collision) */
typedef unsigned short wchar_t;
#define DECLSPEC __declspec(dllexport)
#define NAKED __attribute__((naked))
void* __stdcall LoadLibraryW(const wchar_t*);
int   __stdcall GetProcAddress(void*, const char*);
unsigned int __stdcall GetSystemDirectoryW(wchar_t*, unsigned int);
unsigned int __stdcall GetTempPathW(unsigned int, wchar_t*);
int   __stdcall CopyFileW(const wchar_t*, const wchar_t*, int);
int   __stdcall DeleteFileW(const wchar_t*);
unsigned int __stdcall GetModuleFileNameW(void*, wchar_t*, unsigned int);

// Non-static: must be visible to asm stubs
void* fwd_ptrs[COUNT_PLACEHOLDER];

DECLSPEC void init_d3d9_forwarders(void) {
    wchar_t sysPath[260], tmpPath[260];
    void* h = 0;
    unsigned int n = GetSystemDirectoryW(sysPath, 260);
    if (!n || n >= 240) return;
    sysPath[n++] = L'\\';
    sysPath[n++] = L'd'; sysPath[n++] = L'3'; sysPath[n++] = L'd';
    sysPath[n++] = L'9'; sysPath[n++] = L'.'; sysPath[n++] = L'd';
    sysPath[n++] = L'l'; sysPath[n++] = L'l';
    sysPath[n] = 0;

    // Strategy 1: copy to %TEMP% with unique name
    n = GetTempPathW(260, tmpPath);
    if (n && n < 240) {
        tmpPath[n++] = L'd'; tmpPath[n++] = L'3'; tmpPath[n++] = L'd';
        tmpPath[n++] = L'9'; tmpPath[n++] = L'_'; tmpPath[n++] = L'p';
        tmpPath[n++] = L'r'; tmpPath[n++] = L'o'; tmpPath[n++] = L'x';
        tmpPath[n++] = L'y'; tmpPath[n++] = L'.'; tmpPath[n++] = L'd';
        tmpPath[n++] = L'l'; tmpPath[n++] = L'l'; tmpPath[n] = 0;
        DeleteFileW(tmpPath);
        if (CopyFileW(sysPath, tmpPath, 0)) {
            h = LoadLibraryW(tmpPath);
            DeleteFileW(tmpPath);
        }
    }

    // Strategy 2: copy to current directory (Proton/Wine fallback)
    if (!h) {
        tmpPath[0] = L'd'; tmpPath[1] = L'3'; tmpPath[2] = L'd';
        tmpPath[3] = L'9'; tmpPath[4] = L'_'; tmpPath[5] = L'p';
        tmpPath[6] = L'r'; tmpPath[7] = L'o'; tmpPath[8] = L'x';
        tmpPath[9] = L'y'; tmpPath[10] = L'.'; tmpPath[11] = L'd';
        tmpPath[12] = L'l'; tmpPath[13] = L'l'; tmpPath[14] = 0;
        DeleteFileW(tmpPath);
        if (CopyFileW(sysPath, tmpPath, 0)) {
            h = LoadLibraryW(tmpPath);
            DeleteFileW(tmpPath);
        }
    }

    // Strategy 3 (last resort): direct load — verify it's not our own DLL
    if (!h) {
        h = LoadLibraryW(sysPath);
        if (h) {
            wchar_t loadedPath[260];
            GetModuleFileNameW(h, loadedPath, 260);
            int isSys = 0;
            for (int i = 0; loadedPath[i] && i < 255; i++) {
                if ((loadedPath[i] == L'S' || loadedPath[i] == L's') &&
                    (loadedPath[i+1] == L'Y' || loadedPath[i+1] == L'y'))
                    { isSys = 1; break; }
            }
            if (!isSys) return;  // our own DLL — abort!
        }
    }
    if (!h) return;

HEADER

i=0
grep -v '^$' d3d9_mgw.def | grep -v '^LIBRARY' | grep -v '^EXPORTS' | while read line; do
    func=$(echo "$line" | awk '{print $1}')
    target="$func"  # export name = function name for d3d9
    echo "    fwd_ptrs[$i] = (void*)GetProcAddress(h, \"$target\"); // $func" >> "$out"
    i=$((i+1))
done

cat >> "$out" << 'EOF'
}

EOF

i=0
grep -v '^$' d3d9_mgw.def | grep -v '^LIBRARY' | grep -v '^EXPORTS' | while read line; do
    func=$(echo "$line" | awk '{print $1}')
    echo "DECLSPEC NAKED void $func(void) { __asm__(\"jmp *_fwd_ptrs+$((i*4))\" : : : ); }" >> "$out"
    i=$((i+1))
done

# Replace placeholder with actual count
count=$i
sed -i "s/COUNT_PLACEHOLDER/$count/" "$out"

echo "Generated $out ($(wc -l < $out) lines, $count exports)"
