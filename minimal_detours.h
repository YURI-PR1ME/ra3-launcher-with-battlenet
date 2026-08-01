/*
 * minimal_detours.h - Self-contained minimal implementation of Microsoft Detours API
 *
 * Provides just enough to support our use case:
 *   DetourTransactionBegin / DetourUpdateThread / DetourAttach / DetourTransactionCommit
 *
 * Implements a simple 5-byte JMP rel32 trampoline.
 * Only works for x86 (32-bit) targets.
 */

#pragma once
#include <windows.h>
#include <vector>
#include <cstring>

// Size of a near JMP rel32 instruction (E9 xx xx xx xx)
#define DETOUR_JMP_SIZE 5

struct DetourEntry {
    PBYTE target;       // original function address
    PBYTE detour;       // our hook function address
    BYTE  original[DETOUR_JMP_SIZE];  // saved original bytes
};

static std::vector<DetourEntry> g_detours;

inline LONG DetourTransactionBegin(void) {
    g_detours.clear();
    return NO_ERROR;
}

inline LONG DetourUpdateThread(HANDLE hThread) {
    // No-op: we operate on the current thread
    (void)hThread;
    return NO_ERROR;
}

inline LONG DetourAttach(PVOID *ppTarget, PVOID pDetour) {
    if (!ppTarget || !pDetour) return ERROR_INVALID_PARAMETER;

    DetourEntry entry;
    entry.target = (PBYTE)*ppTarget;
    entry.detour = (PBYTE)pDetour;

    // Save original bytes
    memcpy(entry.original, entry.target, DETOUR_JMP_SIZE);

    g_detours.push_back(entry);
    return NO_ERROR;
}

inline LONG DetourDetach(PVOID *ppTarget, PVOID pDetour) {
    if (!ppTarget || !pDetour) return ERROR_INVALID_PARAMETER;

    PBYTE target = (PBYTE)*ppTarget;
    for (auto& e : g_detours) {
        if (e.target == target && e.detour == (PBYTE)pDetour) {
            DWORD old;
            VirtualProtect(target, DETOUR_JMP_SIZE, PAGE_EXECUTE_READWRITE, &old);
            memcpy(target, e.original, DETOUR_JMP_SIZE);
            VirtualProtect(target, DETOUR_JMP_SIZE, old, &old);
            return NO_ERROR;
        }
    }
    return ERROR_NOT_FOUND;
}

inline LONG DetourTransactionCommit(void) {
    for (auto& entry : g_detours) {
        // Calculate relative JMP offset
        // JMP rel32 = target - (jmp_addr + 5)
        INT32 offset = (INT32)(entry.detour - entry.target - DETOUR_JMP_SIZE);

        // Build JMP instruction: E9 <rel32>
        BYTE patch[DETOUR_JMP_SIZE];
        patch[0] = 0xE9;  // JMP rel32
        memcpy(patch + 1, &offset, sizeof(offset));

        // Apply patch
        DWORD oldProtect;
        if (!VirtualProtect(entry.target, DETOUR_JMP_SIZE, PAGE_EXECUTE_READWRITE, &oldProtect)) {
            return GetLastError();
        }
        memcpy(entry.target, patch, DETOUR_JMP_SIZE);
        VirtualProtect(entry.target, DETOUR_JMP_SIZE, oldProtect, &oldProtect);

        FlushInstructionCache(GetCurrentProcess(), entry.target, DETOUR_JMP_SIZE);
    }
    return NO_ERROR;
}

// Stub for DetourRemove (not used by us)
inline LONG DetourRemove(PVOID pDetour) {
    (void)pDetour;
    return NO_ERROR;
}
