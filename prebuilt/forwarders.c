/* Auto-generated. Loads real winmm via temp copy (avoids module name collision) */
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
void* fwd_ptrs[179];

DECLSPEC void init_winmm_forwarders(void) {
    wchar_t sysPath[260], tmpPath[260];
    void* h = 0;
    unsigned int n = GetSystemDirectoryW(sysPath, 260);
    if (!n || n >= 240) return;
    sysPath[n++] = L'\\';
    sysPath[n++] = L'w'; sysPath[n++] = L'i'; sysPath[n++] = L'n';
    sysPath[n++] = L'm'; sysPath[n++] = L'm'; sysPath[n++] = L'.';
    sysPath[n++] = L'd'; sysPath[n++] = L'l'; sysPath[n++] = L'l';
    sysPath[n] = 0;

    // Strategy 1: copy to %TEMP% with unique name
    n = GetTempPathW(260, tmpPath);
    if (n && n < 240) {
        tmpPath[n++] = L'r'; tmpPath[n++] = L'w'; tmpPath[n++] = L'm';
        tmpPath[n++] = L'm'; tmpPath[n++] = L'_'; tmpPath[n++] = L'p';
        tmpPath[n++] = L'r'; tmpPath[n++] = L'o'; tmpPath[n++] = L'x';
        tmpPath[n++] = L'y'; tmpPath[n++] = L'.'; tmpPath[n++] = L'd';
        tmpPath[n++] = L'l'; tmpPath[n++] = L'l'; tmpPath[n] = 0;
        DeleteFileW(tmpPath);
        if (CopyFileW(sysPath, tmpPath, 0)) {
            h = LoadLibraryW(tmpPath);
            DeleteFileW(tmpPath);
        }
    }

    // Strategy 2: copy to current directory (Proton fallback)
    if (!h) {
        tmpPath[0] = L'r'; tmpPath[1] = L'w'; tmpPath[2] = L'm';
        tmpPath[3] = L'm'; tmpPath[4] = L'_'; tmpPath[5] = L'p';
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

    fwd_ptrs[0] = (void*)GetProcAddress(h, "auxGetDevCapsA"); // auxGetDevCapsA
    fwd_ptrs[1] = (void*)GetProcAddress(h, "auxGetDevCapsW"); // auxGetDevCapsW
    fwd_ptrs[2] = (void*)GetProcAddress(h, "auxGetNumDevs"); // auxGetNumDevs
    fwd_ptrs[3] = (void*)GetProcAddress(h, "auxGetVolume"); // auxGetVolume
    fwd_ptrs[4] = (void*)GetProcAddress(h, "auxOutMessage"); // auxOutMessage
    fwd_ptrs[5] = (void*)GetProcAddress(h, "auxSetVolume"); // auxSetVolume
    fwd_ptrs[6] = (void*)GetProcAddress(h, "CloseDriver"); // CloseDriver
    fwd_ptrs[7] = (void*)GetProcAddress(h, "DefDriverProc"); // DefDriverProc
    fwd_ptrs[8] = (void*)GetProcAddress(h, "DriverCallback"); // DriverCallback
    fwd_ptrs[9] = (void*)GetProcAddress(h, "DrvGetModuleHandle"); // DrvGetModuleHandle
    fwd_ptrs[10] = (void*)GetProcAddress(h, "GetDriverModuleHandle"); // GetDriverModuleHandle
    fwd_ptrs[11] = (void*)GetProcAddress(h, "joyConfigChanged"); // joyConfigChanged
    fwd_ptrs[12] = (void*)GetProcAddress(h, "joyGetDevCapsA"); // joyGetDevCapsA
    fwd_ptrs[13] = (void*)GetProcAddress(h, "joyGetDevCapsW"); // joyGetDevCapsW
    fwd_ptrs[14] = (void*)GetProcAddress(h, "joyGetNumDevs"); // joyGetNumDevs
    fwd_ptrs[15] = (void*)GetProcAddress(h, "joyGetPos"); // joyGetPos
    fwd_ptrs[16] = (void*)GetProcAddress(h, "joyGetPosEx"); // joyGetPosEx
    fwd_ptrs[17] = (void*)GetProcAddress(h, "joyGetThreshold"); // joyGetThreshold
    fwd_ptrs[18] = (void*)GetProcAddress(h, "joyReleaseCapture"); // joyReleaseCapture
    fwd_ptrs[19] = (void*)GetProcAddress(h, "joySetCapture"); // joySetCapture
    fwd_ptrs[20] = (void*)GetProcAddress(h, "joySetThreshold"); // joySetThreshold
    fwd_ptrs[21] = (void*)GetProcAddress(h, "mciDriverNotify"); // mciDriverNotify
    fwd_ptrs[22] = (void*)GetProcAddress(h, "mciDriverYield"); // mciDriverYield
    fwd_ptrs[23] = (void*)GetProcAddress(h, "mciExecute"); // mciExecute
    fwd_ptrs[24] = (void*)GetProcAddress(h, "mciFreeCommandResource"); // mciFreeCommandResource
    fwd_ptrs[25] = (void*)GetProcAddress(h, "mciGetCreatorTask"); // mciGetCreatorTask
    fwd_ptrs[26] = (void*)GetProcAddress(h, "mciGetDeviceIDA"); // mciGetDeviceIDA
    fwd_ptrs[27] = (void*)GetProcAddress(h, "mciGetDeviceIDFromElementIDA"); // mciGetDeviceIDFromElementIDA
    fwd_ptrs[28] = (void*)GetProcAddress(h, "mciGetDeviceIDFromElementIDW"); // mciGetDeviceIDFromElementIDW
    fwd_ptrs[29] = (void*)GetProcAddress(h, "mciGetDeviceIDW"); // mciGetDeviceIDW
    fwd_ptrs[30] = (void*)GetProcAddress(h, "mciGetDriverData"); // mciGetDriverData
    fwd_ptrs[31] = (void*)GetProcAddress(h, "mciGetErrorStringA"); // mciGetErrorStringA
    fwd_ptrs[32] = (void*)GetProcAddress(h, "mciGetErrorStringW"); // mciGetErrorStringW
    fwd_ptrs[33] = (void*)GetProcAddress(h, "mciGetYieldProc"); // mciGetYieldProc
    fwd_ptrs[34] = (void*)GetProcAddress(h, "mciLoadCommandResource"); // mciLoadCommandResource
    fwd_ptrs[35] = (void*)GetProcAddress(h, "mciSendCommandA"); // mciSendCommandA
    fwd_ptrs[36] = (void*)GetProcAddress(h, "mciSendCommandW"); // mciSendCommandW
    fwd_ptrs[37] = (void*)GetProcAddress(h, "mciSendStringA"); // mciSendStringA
    fwd_ptrs[38] = (void*)GetProcAddress(h, "mciSendStringW"); // mciSendStringW
    fwd_ptrs[39] = (void*)GetProcAddress(h, "mciSetDriverData"); // mciSetDriverData
    fwd_ptrs[40] = (void*)GetProcAddress(h, "mciSetYieldProc"); // mciSetYieldProc
    fwd_ptrs[41] = (void*)GetProcAddress(h, "midiConnect"); // midiConnect
    fwd_ptrs[42] = (void*)GetProcAddress(h, "midiDisconnect"); // midiDisconnect
    fwd_ptrs[43] = (void*)GetProcAddress(h, "midiInAddBuffer"); // midiInAddBuffer
    fwd_ptrs[44] = (void*)GetProcAddress(h, "midiInClose"); // midiInClose
    fwd_ptrs[45] = (void*)GetProcAddress(h, "midiInGetDevCapsA"); // midiInGetDevCapsA
    fwd_ptrs[46] = (void*)GetProcAddress(h, "midiInGetDevCapsW"); // midiInGetDevCapsW
    fwd_ptrs[47] = (void*)GetProcAddress(h, "midiInGetErrorTextA"); // midiInGetErrorTextA
    fwd_ptrs[48] = (void*)GetProcAddress(h, "midiInGetErrorTextW"); // midiInGetErrorTextW
    fwd_ptrs[49] = (void*)GetProcAddress(h, "midiInGetID"); // midiInGetID
    fwd_ptrs[50] = (void*)GetProcAddress(h, "midiInGetNumDevs"); // midiInGetNumDevs
    fwd_ptrs[51] = (void*)GetProcAddress(h, "midiInMessage"); // midiInMessage
    fwd_ptrs[52] = (void*)GetProcAddress(h, "midiInOpen"); // midiInOpen
    fwd_ptrs[53] = (void*)GetProcAddress(h, "midiInPrepareHeader"); // midiInPrepareHeader
    fwd_ptrs[54] = (void*)GetProcAddress(h, "midiInReset"); // midiInReset
    fwd_ptrs[55] = (void*)GetProcAddress(h, "midiInStart"); // midiInStart
    fwd_ptrs[56] = (void*)GetProcAddress(h, "midiInStop"); // midiInStop
    fwd_ptrs[57] = (void*)GetProcAddress(h, "midiInUnprepareHeader"); // midiInUnprepareHeader
    fwd_ptrs[58] = (void*)GetProcAddress(h, "midiOutCacheDrumPatches"); // midiOutCacheDrumPatches
    fwd_ptrs[59] = (void*)GetProcAddress(h, "midiOutCachePatches"); // midiOutCachePatches
    fwd_ptrs[60] = (void*)GetProcAddress(h, "midiOutClose"); // midiOutClose
    fwd_ptrs[61] = (void*)GetProcAddress(h, "midiOutGetDevCapsA"); // midiOutGetDevCapsA
    fwd_ptrs[62] = (void*)GetProcAddress(h, "midiOutGetDevCapsW"); // midiOutGetDevCapsW
    fwd_ptrs[63] = (void*)GetProcAddress(h, "midiOutGetErrorTextA"); // midiOutGetErrorTextA
    fwd_ptrs[64] = (void*)GetProcAddress(h, "midiOutGetErrorTextW"); // midiOutGetErrorTextW
    fwd_ptrs[65] = (void*)GetProcAddress(h, "midiOutGetID"); // midiOutGetID
    fwd_ptrs[66] = (void*)GetProcAddress(h, "midiOutGetNumDevs"); // midiOutGetNumDevs
    fwd_ptrs[67] = (void*)GetProcAddress(h, "midiOutGetVolume"); // midiOutGetVolume
    fwd_ptrs[68] = (void*)GetProcAddress(h, "midiOutLongMsg"); // midiOutLongMsg
    fwd_ptrs[69] = (void*)GetProcAddress(h, "midiOutMessage"); // midiOutMessage
    fwd_ptrs[70] = (void*)GetProcAddress(h, "midiOutOpen"); // midiOutOpen
    fwd_ptrs[71] = (void*)GetProcAddress(h, "midiOutPrepareHeader"); // midiOutPrepareHeader
    fwd_ptrs[72] = (void*)GetProcAddress(h, "midiOutReset"); // midiOutReset
    fwd_ptrs[73] = (void*)GetProcAddress(h, "midiOutSetVolume"); // midiOutSetVolume
    fwd_ptrs[74] = (void*)GetProcAddress(h, "midiOutShortMsg"); // midiOutShortMsg
    fwd_ptrs[75] = (void*)GetProcAddress(h, "midiOutUnprepareHeader"); // midiOutUnprepareHeader
    fwd_ptrs[76] = (void*)GetProcAddress(h, "midiStreamClose"); // midiStreamClose
    fwd_ptrs[77] = (void*)GetProcAddress(h, "midiStreamOpen"); // midiStreamOpen
    fwd_ptrs[78] = (void*)GetProcAddress(h, "midiStreamOut"); // midiStreamOut
    fwd_ptrs[79] = (void*)GetProcAddress(h, "midiStreamPause"); // midiStreamPause
    fwd_ptrs[80] = (void*)GetProcAddress(h, "midiStreamPosition"); // midiStreamPosition
    fwd_ptrs[81] = (void*)GetProcAddress(h, "midiStreamProperty"); // midiStreamProperty
    fwd_ptrs[82] = (void*)GetProcAddress(h, "midiStreamRestart"); // midiStreamRestart
    fwd_ptrs[83] = (void*)GetProcAddress(h, "midiStreamStop"); // midiStreamStop
    fwd_ptrs[84] = (void*)GetProcAddress(h, "mixerClose"); // mixerClose
    fwd_ptrs[85] = (void*)GetProcAddress(h, "mixerGetControlDetailsA"); // mixerGetControlDetailsA
    fwd_ptrs[86] = (void*)GetProcAddress(h, "mixerGetControlDetailsW"); // mixerGetControlDetailsW
    fwd_ptrs[87] = (void*)GetProcAddress(h, "mixerGetDevCapsA"); // mixerGetDevCapsA
    fwd_ptrs[88] = (void*)GetProcAddress(h, "mixerGetDevCapsW"); // mixerGetDevCapsW
    fwd_ptrs[89] = (void*)GetProcAddress(h, "mixerGetID"); // mixerGetID
    fwd_ptrs[90] = (void*)GetProcAddress(h, "mixerGetLineControlsA"); // mixerGetLineControlsA
    fwd_ptrs[91] = (void*)GetProcAddress(h, "mixerGetLineControlsW"); // mixerGetLineControlsW
    fwd_ptrs[92] = (void*)GetProcAddress(h, "mixerGetLineInfoA"); // mixerGetLineInfoA
    fwd_ptrs[93] = (void*)GetProcAddress(h, "mixerGetLineInfoW"); // mixerGetLineInfoW
    fwd_ptrs[94] = (void*)GetProcAddress(h, "mixerGetNumDevs"); // mixerGetNumDevs
    fwd_ptrs[95] = (void*)GetProcAddress(h, "mixerMessage"); // mixerMessage
    fwd_ptrs[96] = (void*)GetProcAddress(h, "mixerOpen"); // mixerOpen
    fwd_ptrs[97] = (void*)GetProcAddress(h, "mixerSetControlDetails"); // mixerSetControlDetails
    fwd_ptrs[98] = (void*)GetProcAddress(h, "mmDrvInstall"); // mmDrvInstall
    fwd_ptrs[99] = (void*)GetProcAddress(h, "mmGetCurrentTask"); // mmGetCurrentTask
    fwd_ptrs[100] = (void*)GetProcAddress(h, "mmioAdvance"); // mmioAdvance
    fwd_ptrs[101] = (void*)GetProcAddress(h, "mmioAscend"); // mmioAscend
    fwd_ptrs[102] = (void*)GetProcAddress(h, "mmioClose"); // mmioClose
    fwd_ptrs[103] = (void*)GetProcAddress(h, "mmioCreateChunk"); // mmioCreateChunk
    fwd_ptrs[104] = (void*)GetProcAddress(h, "mmioDescend"); // mmioDescend
    fwd_ptrs[105] = (void*)GetProcAddress(h, "mmioFlush"); // mmioFlush
    fwd_ptrs[106] = (void*)GetProcAddress(h, "mmioGetInfo"); // mmioGetInfo
    fwd_ptrs[107] = (void*)GetProcAddress(h, "mmioInstallIOProcA"); // mmioInstallIOProcA
    fwd_ptrs[108] = (void*)GetProcAddress(h, "mmioInstallIOProcW"); // mmioInstallIOProcW
    fwd_ptrs[109] = (void*)GetProcAddress(h, "mmioOpenA"); // mmioOpenA
    fwd_ptrs[110] = (void*)GetProcAddress(h, "mmioOpenW"); // mmioOpenW
    fwd_ptrs[111] = (void*)GetProcAddress(h, "mmioRead"); // mmioRead
    fwd_ptrs[112] = (void*)GetProcAddress(h, "mmioRenameA"); // mmioRenameA
    fwd_ptrs[113] = (void*)GetProcAddress(h, "mmioRenameW"); // mmioRenameW
    fwd_ptrs[114] = (void*)GetProcAddress(h, "mmioSeek"); // mmioSeek
    fwd_ptrs[115] = (void*)GetProcAddress(h, "mmioSendMessage"); // mmioSendMessage
    fwd_ptrs[116] = (void*)GetProcAddress(h, "mmioSetBuffer"); // mmioSetBuffer
    fwd_ptrs[117] = (void*)GetProcAddress(h, "mmioSetInfo"); // mmioSetInfo
    fwd_ptrs[118] = (void*)GetProcAddress(h, "mmioStringToFOURCCA"); // mmioStringToFOURCCA
    fwd_ptrs[119] = (void*)GetProcAddress(h, "mmioStringToFOURCCW"); // mmioStringToFOURCCW
    fwd_ptrs[120] = (void*)GetProcAddress(h, "mmioWrite"); // mmioWrite
    fwd_ptrs[121] = (void*)GetProcAddress(h, "mmTaskBlock"); // mmTaskBlock
    fwd_ptrs[122] = (void*)GetProcAddress(h, "mmTaskCreate"); // mmTaskCreate
    fwd_ptrs[123] = (void*)GetProcAddress(h, "mmTaskSignal"); // mmTaskSignal
    fwd_ptrs[124] = (void*)GetProcAddress(h, "mmTaskYield"); // mmTaskYield
    fwd_ptrs[125] = (void*)GetProcAddress(h, "OpenDriver"); // OpenDriver
    fwd_ptrs[126] = (void*)GetProcAddress(h, "PlaySound"); // PlaySound
    fwd_ptrs[127] = (void*)GetProcAddress(h, "PlaySoundA"); // PlaySoundA
    fwd_ptrs[128] = (void*)GetProcAddress(h, "PlaySoundW"); // PlaySoundW
    fwd_ptrs[129] = (void*)GetProcAddress(h, "SendDriverMessage"); // SendDriverMessage
    fwd_ptrs[130] = (void*)GetProcAddress(h, "sndPlaySoundA"); // sndPlaySoundA
    fwd_ptrs[131] = (void*)GetProcAddress(h, "sndPlaySoundW"); // sndPlaySoundW
    fwd_ptrs[132] = (void*)GetProcAddress(h, "timeBeginPeriod"); // timeBeginPeriod
    fwd_ptrs[133] = (void*)GetProcAddress(h, "timeEndPeriod"); // timeEndPeriod
    fwd_ptrs[134] = (void*)GetProcAddress(h, "timeGetDevCaps"); // timeGetDevCaps
    fwd_ptrs[135] = (void*)GetProcAddress(h, "timeGetSystemTime"); // timeGetSystemTime
    fwd_ptrs[136] = (void*)GetProcAddress(h, "timeGetTime"); // timeGetTime
    fwd_ptrs[137] = (void*)GetProcAddress(h, "timeKillEvent"); // timeKillEvent
    fwd_ptrs[138] = (void*)GetProcAddress(h, "timeSetEvent"); // timeSetEvent
    fwd_ptrs[139] = (void*)GetProcAddress(h, "waveInAddBuffer"); // waveInAddBuffer
    fwd_ptrs[140] = (void*)GetProcAddress(h, "waveInClose"); // waveInClose
    fwd_ptrs[141] = (void*)GetProcAddress(h, "waveInGetDevCapsA"); // waveInGetDevCapsA
    fwd_ptrs[142] = (void*)GetProcAddress(h, "waveInGetDevCapsW"); // waveInGetDevCapsW
    fwd_ptrs[143] = (void*)GetProcAddress(h, "waveInGetErrorTextA"); // waveInGetErrorTextA
    fwd_ptrs[144] = (void*)GetProcAddress(h, "waveInGetErrorTextW"); // waveInGetErrorTextW
    fwd_ptrs[145] = (void*)GetProcAddress(h, "waveInGetID"); // waveInGetID
    fwd_ptrs[146] = (void*)GetProcAddress(h, "waveInGetNumDevs"); // waveInGetNumDevs
    fwd_ptrs[147] = (void*)GetProcAddress(h, "waveInGetPosition"); // waveInGetPosition
    fwd_ptrs[148] = (void*)GetProcAddress(h, "waveInMessage"); // waveInMessage
    fwd_ptrs[149] = (void*)GetProcAddress(h, "waveInOpen"); // waveInOpen
    fwd_ptrs[150] = (void*)GetProcAddress(h, "waveInPrepareHeader"); // waveInPrepareHeader
    fwd_ptrs[151] = (void*)GetProcAddress(h, "waveInReset"); // waveInReset
    fwd_ptrs[152] = (void*)GetProcAddress(h, "waveInStart"); // waveInStart
    fwd_ptrs[153] = (void*)GetProcAddress(h, "waveInStop"); // waveInStop
    fwd_ptrs[154] = (void*)GetProcAddress(h, "waveInUnprepareHeader"); // waveInUnprepareHeader
    fwd_ptrs[155] = (void*)GetProcAddress(h, "waveOutBreakLoop"); // waveOutBreakLoop
    fwd_ptrs[156] = (void*)GetProcAddress(h, "waveOutClose"); // waveOutClose
    fwd_ptrs[157] = (void*)GetProcAddress(h, "waveOutGetDevCapsA"); // waveOutGetDevCapsA
    fwd_ptrs[158] = (void*)GetProcAddress(h, "waveOutGetDevCapsW"); // waveOutGetDevCapsW
    fwd_ptrs[159] = (void*)GetProcAddress(h, "waveOutGetErrorTextA"); // waveOutGetErrorTextA
    fwd_ptrs[160] = (void*)GetProcAddress(h, "waveOutGetErrorTextW"); // waveOutGetErrorTextW
    fwd_ptrs[161] = (void*)GetProcAddress(h, "waveOutGetID"); // waveOutGetID
    fwd_ptrs[162] = (void*)GetProcAddress(h, "waveOutGetNumDevs"); // waveOutGetNumDevs
    fwd_ptrs[163] = (void*)GetProcAddress(h, "waveOutGetPitch"); // waveOutGetPitch
    fwd_ptrs[164] = (void*)GetProcAddress(h, "waveOutGetPlaybackRate"); // waveOutGetPlaybackRate
    fwd_ptrs[165] = (void*)GetProcAddress(h, "waveOutGetPosition"); // waveOutGetPosition
    fwd_ptrs[166] = (void*)GetProcAddress(h, "waveOutGetVolume"); // waveOutGetVolume
    fwd_ptrs[167] = (void*)GetProcAddress(h, "waveOutMessage"); // waveOutMessage
    fwd_ptrs[168] = (void*)GetProcAddress(h, "waveOutOpen"); // waveOutOpen
    fwd_ptrs[169] = (void*)GetProcAddress(h, "waveOutPause"); // waveOutPause
    fwd_ptrs[170] = (void*)GetProcAddress(h, "waveOutPrepareHeader"); // waveOutPrepareHeader
    fwd_ptrs[171] = (void*)GetProcAddress(h, "waveOutReset"); // waveOutReset
    fwd_ptrs[172] = (void*)GetProcAddress(h, "waveOutRestart"); // waveOutRestart
    fwd_ptrs[173] = (void*)GetProcAddress(h, "waveOutSetPitch"); // waveOutSetPitch
    fwd_ptrs[174] = (void*)GetProcAddress(h, "waveOutSetPlaybackRate"); // waveOutSetPlaybackRate
    fwd_ptrs[175] = (void*)GetProcAddress(h, "waveOutSetVolume"); // waveOutSetVolume
    fwd_ptrs[176] = (void*)GetProcAddress(h, "waveOutUnprepareHeader"); // waveOutUnprepareHeader
    fwd_ptrs[177] = (void*)GetProcAddress(h, "waveOutWrite"); // waveOutWrite
    fwd_ptrs[178] = (void*)GetProcAddress(h, "WOWAppExit"); // WOWAppExit
}

DECLSPEC NAKED void auxGetDevCapsA(void) { __asm__("jmp *_fwd_ptrs+0" : : : ); }
DECLSPEC NAKED void auxGetDevCapsW(void) { __asm__("jmp *_fwd_ptrs+4" : : : ); }
DECLSPEC NAKED void auxGetNumDevs(void) { __asm__("jmp *_fwd_ptrs+8" : : : ); }
DECLSPEC NAKED void auxGetVolume(void) { __asm__("jmp *_fwd_ptrs+12" : : : ); }
DECLSPEC NAKED void auxOutMessage(void) { __asm__("jmp *_fwd_ptrs+16" : : : ); }
DECLSPEC NAKED void auxSetVolume(void) { __asm__("jmp *_fwd_ptrs+20" : : : ); }
DECLSPEC NAKED void CloseDriver(void) { __asm__("jmp *_fwd_ptrs+24" : : : ); }
DECLSPEC NAKED void DefDriverProc(void) { __asm__("jmp *_fwd_ptrs+28" : : : ); }
DECLSPEC NAKED void DriverCallback(void) { __asm__("jmp *_fwd_ptrs+32" : : : ); }
DECLSPEC NAKED void DrvGetModuleHandle(void) { __asm__("jmp *_fwd_ptrs+36" : : : ); }
DECLSPEC NAKED void GetDriverModuleHandle(void) { __asm__("jmp *_fwd_ptrs+40" : : : ); }
DECLSPEC NAKED void joyConfigChanged(void) { __asm__("jmp *_fwd_ptrs+44" : : : ); }
DECLSPEC NAKED void joyGetDevCapsA(void) { __asm__("jmp *_fwd_ptrs+48" : : : ); }
DECLSPEC NAKED void joyGetDevCapsW(void) { __asm__("jmp *_fwd_ptrs+52" : : : ); }
DECLSPEC NAKED void joyGetNumDevs(void) { __asm__("jmp *_fwd_ptrs+56" : : : ); }
DECLSPEC NAKED void joyGetPos(void) { __asm__("jmp *_fwd_ptrs+60" : : : ); }
DECLSPEC NAKED void joyGetPosEx(void) { __asm__("jmp *_fwd_ptrs+64" : : : ); }
DECLSPEC NAKED void joyGetThreshold(void) { __asm__("jmp *_fwd_ptrs+68" : : : ); }
DECLSPEC NAKED void joyReleaseCapture(void) { __asm__("jmp *_fwd_ptrs+72" : : : ); }
DECLSPEC NAKED void joySetCapture(void) { __asm__("jmp *_fwd_ptrs+76" : : : ); }
DECLSPEC NAKED void joySetThreshold(void) { __asm__("jmp *_fwd_ptrs+80" : : : ); }
DECLSPEC NAKED void mciDriverNotify(void) { __asm__("jmp *_fwd_ptrs+84" : : : ); }
DECLSPEC NAKED void mciDriverYield(void) { __asm__("jmp *_fwd_ptrs+88" : : : ); }
DECLSPEC NAKED void mciExecute(void) { __asm__("jmp *_fwd_ptrs+92" : : : ); }
DECLSPEC NAKED void mciFreeCommandResource(void) { __asm__("jmp *_fwd_ptrs+96" : : : ); }
DECLSPEC NAKED void mciGetCreatorTask(void) { __asm__("jmp *_fwd_ptrs+100" : : : ); }
DECLSPEC NAKED void mciGetDeviceIDA(void) { __asm__("jmp *_fwd_ptrs+104" : : : ); }
DECLSPEC NAKED void mciGetDeviceIDFromElementIDA(void) { __asm__("jmp *_fwd_ptrs+108" : : : ); }
DECLSPEC NAKED void mciGetDeviceIDFromElementIDW(void) { __asm__("jmp *_fwd_ptrs+112" : : : ); }
DECLSPEC NAKED void mciGetDeviceIDW(void) { __asm__("jmp *_fwd_ptrs+116" : : : ); }
DECLSPEC NAKED void mciGetDriverData(void) { __asm__("jmp *_fwd_ptrs+120" : : : ); }
DECLSPEC NAKED void mciGetErrorStringA(void) { __asm__("jmp *_fwd_ptrs+124" : : : ); }
DECLSPEC NAKED void mciGetErrorStringW(void) { __asm__("jmp *_fwd_ptrs+128" : : : ); }
DECLSPEC NAKED void mciGetYieldProc(void) { __asm__("jmp *_fwd_ptrs+132" : : : ); }
DECLSPEC NAKED void mciLoadCommandResource(void) { __asm__("jmp *_fwd_ptrs+136" : : : ); }
DECLSPEC NAKED void mciSendCommandA(void) { __asm__("jmp *_fwd_ptrs+140" : : : ); }
DECLSPEC NAKED void mciSendCommandW(void) { __asm__("jmp *_fwd_ptrs+144" : : : ); }
DECLSPEC NAKED void mciSendStringA(void) { __asm__("jmp *_fwd_ptrs+148" : : : ); }
DECLSPEC NAKED void mciSendStringW(void) { __asm__("jmp *_fwd_ptrs+152" : : : ); }
DECLSPEC NAKED void mciSetDriverData(void) { __asm__("jmp *_fwd_ptrs+156" : : : ); }
DECLSPEC NAKED void mciSetYieldProc(void) { __asm__("jmp *_fwd_ptrs+160" : : : ); }
DECLSPEC NAKED void midiConnect(void) { __asm__("jmp *_fwd_ptrs+164" : : : ); }
DECLSPEC NAKED void midiDisconnect(void) { __asm__("jmp *_fwd_ptrs+168" : : : ); }
DECLSPEC NAKED void midiInAddBuffer(void) { __asm__("jmp *_fwd_ptrs+172" : : : ); }
DECLSPEC NAKED void midiInClose(void) { __asm__("jmp *_fwd_ptrs+176" : : : ); }
DECLSPEC NAKED void midiInGetDevCapsA(void) { __asm__("jmp *_fwd_ptrs+180" : : : ); }
DECLSPEC NAKED void midiInGetDevCapsW(void) { __asm__("jmp *_fwd_ptrs+184" : : : ); }
DECLSPEC NAKED void midiInGetErrorTextA(void) { __asm__("jmp *_fwd_ptrs+188" : : : ); }
DECLSPEC NAKED void midiInGetErrorTextW(void) { __asm__("jmp *_fwd_ptrs+192" : : : ); }
DECLSPEC NAKED void midiInGetID(void) { __asm__("jmp *_fwd_ptrs+196" : : : ); }
DECLSPEC NAKED void midiInGetNumDevs(void) { __asm__("jmp *_fwd_ptrs+200" : : : ); }
DECLSPEC NAKED void midiInMessage(void) { __asm__("jmp *_fwd_ptrs+204" : : : ); }
DECLSPEC NAKED void midiInOpen(void) { __asm__("jmp *_fwd_ptrs+208" : : : ); }
DECLSPEC NAKED void midiInPrepareHeader(void) { __asm__("jmp *_fwd_ptrs+212" : : : ); }
DECLSPEC NAKED void midiInReset(void) { __asm__("jmp *_fwd_ptrs+216" : : : ); }
DECLSPEC NAKED void midiInStart(void) { __asm__("jmp *_fwd_ptrs+220" : : : ); }
DECLSPEC NAKED void midiInStop(void) { __asm__("jmp *_fwd_ptrs+224" : : : ); }
DECLSPEC NAKED void midiInUnprepareHeader(void) { __asm__("jmp *_fwd_ptrs+228" : : : ); }
DECLSPEC NAKED void midiOutCacheDrumPatches(void) { __asm__("jmp *_fwd_ptrs+232" : : : ); }
DECLSPEC NAKED void midiOutCachePatches(void) { __asm__("jmp *_fwd_ptrs+236" : : : ); }
DECLSPEC NAKED void midiOutClose(void) { __asm__("jmp *_fwd_ptrs+240" : : : ); }
DECLSPEC NAKED void midiOutGetDevCapsA(void) { __asm__("jmp *_fwd_ptrs+244" : : : ); }
DECLSPEC NAKED void midiOutGetDevCapsW(void) { __asm__("jmp *_fwd_ptrs+248" : : : ); }
DECLSPEC NAKED void midiOutGetErrorTextA(void) { __asm__("jmp *_fwd_ptrs+252" : : : ); }
DECLSPEC NAKED void midiOutGetErrorTextW(void) { __asm__("jmp *_fwd_ptrs+256" : : : ); }
DECLSPEC NAKED void midiOutGetID(void) { __asm__("jmp *_fwd_ptrs+260" : : : ); }
DECLSPEC NAKED void midiOutGetNumDevs(void) { __asm__("jmp *_fwd_ptrs+264" : : : ); }
DECLSPEC NAKED void midiOutGetVolume(void) { __asm__("jmp *_fwd_ptrs+268" : : : ); }
DECLSPEC NAKED void midiOutLongMsg(void) { __asm__("jmp *_fwd_ptrs+272" : : : ); }
DECLSPEC NAKED void midiOutMessage(void) { __asm__("jmp *_fwd_ptrs+276" : : : ); }
DECLSPEC NAKED void midiOutOpen(void) { __asm__("jmp *_fwd_ptrs+280" : : : ); }
DECLSPEC NAKED void midiOutPrepareHeader(void) { __asm__("jmp *_fwd_ptrs+284" : : : ); }
DECLSPEC NAKED void midiOutReset(void) { __asm__("jmp *_fwd_ptrs+288" : : : ); }
DECLSPEC NAKED void midiOutSetVolume(void) { __asm__("jmp *_fwd_ptrs+292" : : : ); }
DECLSPEC NAKED void midiOutShortMsg(void) { __asm__("jmp *_fwd_ptrs+296" : : : ); }
DECLSPEC NAKED void midiOutUnprepareHeader(void) { __asm__("jmp *_fwd_ptrs+300" : : : ); }
DECLSPEC NAKED void midiStreamClose(void) { __asm__("jmp *_fwd_ptrs+304" : : : ); }
DECLSPEC NAKED void midiStreamOpen(void) { __asm__("jmp *_fwd_ptrs+308" : : : ); }
DECLSPEC NAKED void midiStreamOut(void) { __asm__("jmp *_fwd_ptrs+312" : : : ); }
DECLSPEC NAKED void midiStreamPause(void) { __asm__("jmp *_fwd_ptrs+316" : : : ); }
DECLSPEC NAKED void midiStreamPosition(void) { __asm__("jmp *_fwd_ptrs+320" : : : ); }
DECLSPEC NAKED void midiStreamProperty(void) { __asm__("jmp *_fwd_ptrs+324" : : : ); }
DECLSPEC NAKED void midiStreamRestart(void) { __asm__("jmp *_fwd_ptrs+328" : : : ); }
DECLSPEC NAKED void midiStreamStop(void) { __asm__("jmp *_fwd_ptrs+332" : : : ); }
DECLSPEC NAKED void mixerClose(void) { __asm__("jmp *_fwd_ptrs+336" : : : ); }
DECLSPEC NAKED void mixerGetControlDetailsA(void) { __asm__("jmp *_fwd_ptrs+340" : : : ); }
DECLSPEC NAKED void mixerGetControlDetailsW(void) { __asm__("jmp *_fwd_ptrs+344" : : : ); }
DECLSPEC NAKED void mixerGetDevCapsA(void) { __asm__("jmp *_fwd_ptrs+348" : : : ); }
DECLSPEC NAKED void mixerGetDevCapsW(void) { __asm__("jmp *_fwd_ptrs+352" : : : ); }
DECLSPEC NAKED void mixerGetID(void) { __asm__("jmp *_fwd_ptrs+356" : : : ); }
DECLSPEC NAKED void mixerGetLineControlsA(void) { __asm__("jmp *_fwd_ptrs+360" : : : ); }
DECLSPEC NAKED void mixerGetLineControlsW(void) { __asm__("jmp *_fwd_ptrs+364" : : : ); }
DECLSPEC NAKED void mixerGetLineInfoA(void) { __asm__("jmp *_fwd_ptrs+368" : : : ); }
DECLSPEC NAKED void mixerGetLineInfoW(void) { __asm__("jmp *_fwd_ptrs+372" : : : ); }
DECLSPEC NAKED void mixerGetNumDevs(void) { __asm__("jmp *_fwd_ptrs+376" : : : ); }
DECLSPEC NAKED void mixerMessage(void) { __asm__("jmp *_fwd_ptrs+380" : : : ); }
DECLSPEC NAKED void mixerOpen(void) { __asm__("jmp *_fwd_ptrs+384" : : : ); }
DECLSPEC NAKED void mixerSetControlDetails(void) { __asm__("jmp *_fwd_ptrs+388" : : : ); }
DECLSPEC NAKED void mmDrvInstall(void) { __asm__("jmp *_fwd_ptrs+392" : : : ); }
DECLSPEC NAKED void mmGetCurrentTask(void) { __asm__("jmp *_fwd_ptrs+396" : : : ); }
DECLSPEC NAKED void mmioAdvance(void) { __asm__("jmp *_fwd_ptrs+400" : : : ); }
DECLSPEC NAKED void mmioAscend(void) { __asm__("jmp *_fwd_ptrs+404" : : : ); }
DECLSPEC NAKED void mmioClose(void) { __asm__("jmp *_fwd_ptrs+408" : : : ); }
DECLSPEC NAKED void mmioCreateChunk(void) { __asm__("jmp *_fwd_ptrs+412" : : : ); }
DECLSPEC NAKED void mmioDescend(void) { __asm__("jmp *_fwd_ptrs+416" : : : ); }
DECLSPEC NAKED void mmioFlush(void) { __asm__("jmp *_fwd_ptrs+420" : : : ); }
DECLSPEC NAKED void mmioGetInfo(void) { __asm__("jmp *_fwd_ptrs+424" : : : ); }
DECLSPEC NAKED void mmioInstallIOProcA(void) { __asm__("jmp *_fwd_ptrs+428" : : : ); }
DECLSPEC NAKED void mmioInstallIOProcW(void) { __asm__("jmp *_fwd_ptrs+432" : : : ); }
DECLSPEC NAKED void mmioOpenA(void) { __asm__("jmp *_fwd_ptrs+436" : : : ); }
DECLSPEC NAKED void mmioOpenW(void) { __asm__("jmp *_fwd_ptrs+440" : : : ); }
DECLSPEC NAKED void mmioRead(void) { __asm__("jmp *_fwd_ptrs+444" : : : ); }
DECLSPEC NAKED void mmioRenameA(void) { __asm__("jmp *_fwd_ptrs+448" : : : ); }
DECLSPEC NAKED void mmioRenameW(void) { __asm__("jmp *_fwd_ptrs+452" : : : ); }
DECLSPEC NAKED void mmioSeek(void) { __asm__("jmp *_fwd_ptrs+456" : : : ); }
DECLSPEC NAKED void mmioSendMessage(void) { __asm__("jmp *_fwd_ptrs+460" : : : ); }
DECLSPEC NAKED void mmioSetBuffer(void) { __asm__("jmp *_fwd_ptrs+464" : : : ); }
DECLSPEC NAKED void mmioSetInfo(void) { __asm__("jmp *_fwd_ptrs+468" : : : ); }
DECLSPEC NAKED void mmioStringToFOURCCA(void) { __asm__("jmp *_fwd_ptrs+472" : : : ); }
DECLSPEC NAKED void mmioStringToFOURCCW(void) { __asm__("jmp *_fwd_ptrs+476" : : : ); }
DECLSPEC NAKED void mmioWrite(void) { __asm__("jmp *_fwd_ptrs+480" : : : ); }
DECLSPEC NAKED void mmTaskBlock(void) { __asm__("jmp *_fwd_ptrs+484" : : : ); }
DECLSPEC NAKED void mmTaskCreate(void) { __asm__("jmp *_fwd_ptrs+488" : : : ); }
DECLSPEC NAKED void mmTaskSignal(void) { __asm__("jmp *_fwd_ptrs+492" : : : ); }
DECLSPEC NAKED void mmTaskYield(void) { __asm__("jmp *_fwd_ptrs+496" : : : ); }
DECLSPEC NAKED void OpenDriver(void) { __asm__("jmp *_fwd_ptrs+500" : : : ); }
DECLSPEC NAKED void PlaySound(void) { __asm__("jmp *_fwd_ptrs+504" : : : ); }
DECLSPEC NAKED void PlaySoundA(void) { __asm__("jmp *_fwd_ptrs+508" : : : ); }
DECLSPEC NAKED void PlaySoundW(void) { __asm__("jmp *_fwd_ptrs+512" : : : ); }
DECLSPEC NAKED void SendDriverMessage(void) { __asm__("jmp *_fwd_ptrs+516" : : : ); }
DECLSPEC NAKED void sndPlaySoundA(void) { __asm__("jmp *_fwd_ptrs+520" : : : ); }
DECLSPEC NAKED void sndPlaySoundW(void) { __asm__("jmp *_fwd_ptrs+524" : : : ); }
DECLSPEC NAKED void timeBeginPeriod(void) { __asm__("jmp *_fwd_ptrs+528" : : : ); }
DECLSPEC NAKED void timeEndPeriod(void) { __asm__("jmp *_fwd_ptrs+532" : : : ); }
DECLSPEC NAKED void timeGetDevCaps(void) { __asm__("jmp *_fwd_ptrs+536" : : : ); }
DECLSPEC NAKED void timeGetSystemTime(void) { __asm__("jmp *_fwd_ptrs+540" : : : ); }
DECLSPEC NAKED void timeGetTime(void) { __asm__("jmp *_fwd_ptrs+544" : : : ); }
DECLSPEC NAKED void timeKillEvent(void) { __asm__("jmp *_fwd_ptrs+548" : : : ); }
DECLSPEC NAKED void timeSetEvent(void) { __asm__("jmp *_fwd_ptrs+552" : : : ); }
DECLSPEC NAKED void waveInAddBuffer(void) { __asm__("jmp *_fwd_ptrs+556" : : : ); }
DECLSPEC NAKED void waveInClose(void) { __asm__("jmp *_fwd_ptrs+560" : : : ); }
DECLSPEC NAKED void waveInGetDevCapsA(void) { __asm__("jmp *_fwd_ptrs+564" : : : ); }
DECLSPEC NAKED void waveInGetDevCapsW(void) { __asm__("jmp *_fwd_ptrs+568" : : : ); }
DECLSPEC NAKED void waveInGetErrorTextA(void) { __asm__("jmp *_fwd_ptrs+572" : : : ); }
DECLSPEC NAKED void waveInGetErrorTextW(void) { __asm__("jmp *_fwd_ptrs+576" : : : ); }
DECLSPEC NAKED void waveInGetID(void) { __asm__("jmp *_fwd_ptrs+580" : : : ); }
DECLSPEC NAKED void waveInGetNumDevs(void) { __asm__("jmp *_fwd_ptrs+584" : : : ); }
DECLSPEC NAKED void waveInGetPosition(void) { __asm__("jmp *_fwd_ptrs+588" : : : ); }
DECLSPEC NAKED void waveInMessage(void) { __asm__("jmp *_fwd_ptrs+592" : : : ); }
DECLSPEC NAKED void waveInOpen(void) { __asm__("jmp *_fwd_ptrs+596" : : : ); }
DECLSPEC NAKED void waveInPrepareHeader(void) { __asm__("jmp *_fwd_ptrs+600" : : : ); }
DECLSPEC NAKED void waveInReset(void) { __asm__("jmp *_fwd_ptrs+604" : : : ); }
DECLSPEC NAKED void waveInStart(void) { __asm__("jmp *_fwd_ptrs+608" : : : ); }
DECLSPEC NAKED void waveInStop(void) { __asm__("jmp *_fwd_ptrs+612" : : : ); }
DECLSPEC NAKED void waveInUnprepareHeader(void) { __asm__("jmp *_fwd_ptrs+616" : : : ); }
DECLSPEC NAKED void waveOutBreakLoop(void) { __asm__("jmp *_fwd_ptrs+620" : : : ); }
DECLSPEC NAKED void waveOutClose(void) { __asm__("jmp *_fwd_ptrs+624" : : : ); }
DECLSPEC NAKED void waveOutGetDevCapsA(void) { __asm__("jmp *_fwd_ptrs+628" : : : ); }
DECLSPEC NAKED void waveOutGetDevCapsW(void) { __asm__("jmp *_fwd_ptrs+632" : : : ); }
DECLSPEC NAKED void waveOutGetErrorTextA(void) { __asm__("jmp *_fwd_ptrs+636" : : : ); }
DECLSPEC NAKED void waveOutGetErrorTextW(void) { __asm__("jmp *_fwd_ptrs+640" : : : ); }
DECLSPEC NAKED void waveOutGetID(void) { __asm__("jmp *_fwd_ptrs+644" : : : ); }
DECLSPEC NAKED void waveOutGetNumDevs(void) { __asm__("jmp *_fwd_ptrs+648" : : : ); }
DECLSPEC NAKED void waveOutGetPitch(void) { __asm__("jmp *_fwd_ptrs+652" : : : ); }
DECLSPEC NAKED void waveOutGetPlaybackRate(void) { __asm__("jmp *_fwd_ptrs+656" : : : ); }
DECLSPEC NAKED void waveOutGetPosition(void) { __asm__("jmp *_fwd_ptrs+660" : : : ); }
DECLSPEC NAKED void waveOutGetVolume(void) { __asm__("jmp *_fwd_ptrs+664" : : : ); }
DECLSPEC NAKED void waveOutMessage(void) { __asm__("jmp *_fwd_ptrs+668" : : : ); }
DECLSPEC NAKED void waveOutOpen(void) { __asm__("jmp *_fwd_ptrs+672" : : : ); }
DECLSPEC NAKED void waveOutPause(void) { __asm__("jmp *_fwd_ptrs+676" : : : ); }
DECLSPEC NAKED void waveOutPrepareHeader(void) { __asm__("jmp *_fwd_ptrs+680" : : : ); }
DECLSPEC NAKED void waveOutReset(void) { __asm__("jmp *_fwd_ptrs+684" : : : ); }
DECLSPEC NAKED void waveOutRestart(void) { __asm__("jmp *_fwd_ptrs+688" : : : ); }
DECLSPEC NAKED void waveOutSetPitch(void) { __asm__("jmp *_fwd_ptrs+692" : : : ); }
DECLSPEC NAKED void waveOutSetPlaybackRate(void) { __asm__("jmp *_fwd_ptrs+696" : : : ); }
DECLSPEC NAKED void waveOutSetVolume(void) { __asm__("jmp *_fwd_ptrs+700" : : : ); }
DECLSPEC NAKED void waveOutUnprepareHeader(void) { __asm__("jmp *_fwd_ptrs+704" : : : ); }
DECLSPEC NAKED void waveOutWrite(void) { __asm__("jmp *_fwd_ptrs+708" : : : ); }
DECLSPEC NAKED void WOWAppExit(void) { __asm__("jmp *_fwd_ptrs+712" : : : ); }
