/*
 * winmm_dll.h - Unified header for C&C Online Proxy DLL
 *
 * This DLL redirects C&C game network traffic to community servers
 * via API hooks (Detours) and provides SSL proxy + cert bypass patches.
 */

#pragma once

// --- Windows ---
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <psapi.h>
#include <tchar.h>

#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "version.lib")
#endif

// --- STL ---
#include <string>
#include <unordered_map>
#include <vector>
#include <map>
#include <mutex>
#include <atomic>
#include <thread>
#include <memory>
#include <cstring>
#include <cstdint>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <optional>
#include <list>
#include <condition_variable>

// --- Detours (self-contained minimal implementation) ---
#include "minimal_detours.h"

// --- OpenSSL (optional, only needed for ProxySSL) ---
// Define USE_OPENSSL to enable the SSL proxy feature.
// Requires OpenSSL 1.0.2 static libraries (libeay32.lib, ssleay32.lib).
#ifdef USE_OPENSSL
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#ifdef _MSC_VER
#pragma comment(lib, "ssleay32.lib")
#pragma comment(lib, "libeay32.lib")
#endif
#endif

// --- Our simple libraries ---
#include "simple_log.h"
#include "simple_json.h"

// ===================================================================
// Constants
// ===================================================================
#define PACKET_MAX_LENGTH 8192
#define PORT_PEERCHAT 6667
#define PORT_PEERCHAT_ALT 16667
#define PORT_MASTER_SERVER 28910

// ===================================================================
// Function type definitions for Detours
// ===================================================================
typedef HINSTANCE (WINAPI* ShellExecuteW_t)(HWND, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, INT);
typedef int (WSAAPI* connect_t)(SOCKET, const sockaddr*, int);
typedef int (WSAAPI* send_t)(SOCKET, const char*, int, int);
typedef int (WSAAPI* recv_t)(SOCKET, char*, int, int);
typedef struct hostent* (WSAAPI* gethostbyname_t)(const char*);

// ===================================================================
// Game Version Detection
// ===================================================================
enum class GameRelease { UNKNOWN, RETAIL, DIGITAL };

struct GameVersionInfo {
    int major = 0, minor = 0;
    GameRelease release = GameRelease::UNKNOWN;
    std::wstring executableName;
};

class GameVersion {
public:
    static GameVersion& GetInstance() {
        static GameVersion instance;
        return instance;
    }
    GameVersion(const GameVersion&) = delete;
    GameVersion& operator=(const GameVersion&) = delete;

    const GameVersionInfo& GetInfo() const { return info_; }
    bool IsIdentified() const { return identified_; }

private:
    GameVersion() { Identify(); }

    void Identify() {
        HANDLE hProcess = GetCurrentProcess();
        TCHAR buffer[MAX_PATH] = { 0 };
        if (!GetModuleFileNameExW(hProcess, NULL, buffer, sizeof(buffer)/sizeof(TCHAR))) {
            CloseHandle(hProcess);
            return;
        }

        std::wstring execPath(buffer);
        size_t lastSlash = execPath.find_last_of(L"\\");
        info_.executableName = lastSlash != std::wstring::npos ? execPath.substr(lastSlash + 1) : execPath;

        auto nameIs = [this](const wchar_t* expected) {
            return _wcsicmp(info_.executableName.c_str(), expected) == 0;
        };

        if (GetFileVersion(buffer, info_.major, info_.minor)) {
            if (nameIs(L"ra3_1.12.game") && info_.major == 1 && info_.minor == 12) {
                info_.release = GetReleaseVersion("1.12");
                identified_ = true;
                switch (info_.release) {
                    case GameRelease::DIGITAL: LOG_INFO << "Red Alert 3 (v1.12): Digital Release"; break;
                    case GameRelease::RETAIL:  LOG_INFO << "Red Alert 3 (v1.12): Retail Release"; break;
                    default: LOG_INFO << "Red Alert 3 (v1.12): Unknown Release"; break;
                }
            } else if (nameIs(L"ra3_1.13.game") && info_.major == 1 && info_.minor == 13) {
                info_.release = GetReleaseVersion("1.13");
                identified_ = true;
                if (info_.release == GameRelease::DIGITAL)
                    LOG_INFO << "Red Alert 3 (v1.13): Digital Release";
            } else if (nameIs(L"cnc3ep1.dat")) {
                identified_ = true;
                LOG_INFO << "Command & Conquer 3: Kane's Wrath (v" << info_.major << "." << info_.minor << ")";
            } else if (nameIs(L"cnc3game.dat")) {
                identified_ = true;
                LOG_INFO << "Command & Conquer 3: Tiberium Wars (v" << info_.major << "." << info_.minor << ")";
            }
        }
        CloseHandle(hProcess);
    }

    bool GetFileVersion(TCHAR* szPath, int& major, int& minor) {
        DWORD verSize = GetFileVersionInfoSizeW(szPath, NULL);
        if (verSize == 0) return false;
        std::vector<BYTE> verData(verSize);
        if (!GetFileVersionInfoW(szPath, 0, verSize, verData.data())) return false;
        VS_FIXEDFILEINFO* verInfo = NULL;
        UINT size = 0;
        if (!VerQueryValueW(verData.data(), L"\\", (LPVOID*)&verInfo, &size) || size == 0) return false;
        major = HIWORD(verInfo->dwFileVersionMS);
        minor = LOWORD(verInfo->dwFileVersionMS);
        return true;
    }

    GameRelease GetReleaseVersion(const std::string& ver) {
        // Address map: {version: {release_type: address}}
        using AddressMap = std::unordered_map<GameRelease, uintptr_t>;
        static const std::unordered_map<std::string, AddressMap> VERSIONS = {
            {"1.12", {{GameRelease::DIGITAL, 0xC6262C}, {GameRelease::RETAIL, 0xC5B6C4}}},
            {"1.13", {{GameRelease::DIGITAL, 0xC64DBC}}}
        };
        auto it = VERSIONS.find(ver);
        if (it == VERSIONS.end()) return GameRelease::UNKNOWN;
        for (const auto& [release, addr] : it->second) {
            const char* ptr = reinterpret_cast<const char*>(addr);
            if (ptr && std::string(ptr, 8) == "RedAlert") return release;
        }
        return GameRelease::UNKNOWN;
    }

    GameVersionInfo info_;
    bool identified_ = false;
};

// ===================================================================
// Config (replaces boost::property_tree)
// ===================================================================
class Config {
public:
    static Config& GetInstance() {
        static Config instance;
        return instance;
    }

    // Debug
    bool showConsole = true;
    bool createLog = true;
    bool logDecryption = false;
    int consoleLogLevel = 2;  // info
    int fileLogLevel = 1;     // debug

    // Patches
    bool patchSSL = true;
    bool patchAuthKey = true;

    // RA3BN
    bool ra3bnDelegate = false;
    std::string ra3bnClientDll;
    std::string ra3bnLogFolder;
    int ra3bnWaitSeconds = 120;

    // Proxy
    bool proxy_enable = true;
    unsigned short proxyListenPort = 18840;
    unsigned short proxyDestinationPort = 18840;
    bool proxySSL = false;

    // Game
    std::string gameKey;
    unsigned short peerchatPort = 0;

    // Hostnames
    std::unordered_map<std::string, std::string> hostnames;

    std::string getHostname(const std::string& key) const {
        auto it = hostnames.find(key);
        if (it != hostnames.end()) return it->second;
        LOG_WARN << "Hostname not found: " << key;
        return "";
    }

    std::string getHostname(const std::string& key, const std::string& def) const {
        auto it = hostnames.find(key);
        return (it != hostnames.end()) ? it->second : def;
    }

private:
    Config() { Load(); }

    void Load() {
        SimpleJson root;

        // Prefer config.ini, then config.json
        if (GetFileAttributesA("config.ini") != INVALID_FILE_ATTRIBUTES) {
            LOG_INFO << "Reading config from ini...";
            root = ParseIni("config.ini");
        } else if (GetFileAttributesA("config.json") != INVALID_FILE_ATTRIBUTES) {
            LOG_INFO << "Reading config from json...";
            root = ParseJson("config.json");
        } else {
            LOG_INFO << "No config file found, using defaults.";
        }

        // Debug section
        showConsole = root.get("debug.showConsole", true);
        createLog = root.get("debug.createLog", true);
        logDecryption = root.get("debug.logDecryption", false);
        consoleLogLevel = root.get("debug.logLevelConsole", 2);
        fileLogLevel = root.get("debug.logLevelFile", 1);

        // Patches section
        patchSSL = root.get("patches.SSL", true);
        patchAuthKey = root.get("patches.AuthKey", true);

        // RA3BN section
        ra3bnDelegate = root.get("ra3bn.delegate", false);
        ra3bnClientDll = root.get("ra3bn.clientDll", "");
        ra3bnLogFolder = root.get("ra3bn.logFolder", "");
        ra3bnWaitSeconds = root.get("ra3bn.waitSeconds", 120);

        // Proxy section
        proxy_enable = root.get("proxy.enable", true);
        proxyListenPort = root.get("proxy.listenPort", (unsigned short)18840);
        proxyDestinationPort = root.get("proxy.destinationPort", (unsigned short)18840);
        proxySSL = root.get("proxy.secure", false);

        // Game section
        gameKey = root.get("game.gameKey", "");
        peerchatPort = root.get("game.peerchatPort", (unsigned short)0);

        // Hostnames
        auto* hn = root.resolve("hostnames");
        if (hn && hn->type() == SimpleJson::OBJECT) {
            for (const auto& [key, val] : hn->children()) {
                hostnames[key] = val.str();
            }
            LOG_INFO << "Loaded " << hostnames.size() << " hostnames from config";
        } else {
            hostnames = {
                {"host", "http.server.cnc-online.net"},
                {"login", "login.server.cnc-online.net"},
                {"gpcm", "gpcm.server.cnc-online.net"},
                {"peerchat", "peerchat.server.cnc-online.net"},
                {"master", "master.server.cnc-online.net"},
                {"natneg", "natneg.server.cnc-online.net"},
                {"stats", "gamestats.server.cnc-online.net"},
                {"sake", "sake.server.cnc-online.net"},
                {"server", "server.cnc-online.net"},
                {"register", "https://cnc-online.net/en/connect/register/"},
                {"website", "https://cnc-online.net/en/"},
                {"tos", "https://cnc-online.net/en/faq/"}
            };
            LOG_INFO << "Using default hostnames";
        }
    }

    SimpleJson ParseJson(const char* path) {
        std::ifstream f(path, std::ios::binary);
        if (!f.is_open()) return SimpleJson();
        std::string content((std::istreambuf_iterator<char>(f)),
                            std::istreambuf_iterator<char>());
        try { return SimpleJson::parse(content); }
        catch (std::exception& e) {
            LOG_ERR << "JSON parse error: " << e.what();
            return SimpleJson();
        }
    }

    // Minimal INI parser (section.key = value)
    SimpleJson ParseIni(const char* path) {
        std::ifstream f(path);
        if (!f.is_open()) return SimpleJson();
        std::string line, section;
        std::unordered_map<std::string, std::unordered_map<std::string, std::string>> data;

        while (std::getline(f, line)) {
            // Trim
            while (!line.empty() && (line[0] == ' ' || line[0] == '\t')) line.erase(0, 1);
            while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) line.pop_back();

            if (line.empty() || line[0] == ';' || line[0] == '#') continue;

            if (line[0] == '[') {
                size_t end = line.find(']');
                if (end != std::string::npos) section = line.substr(1, end - 1);
            } else {
                size_t eq = line.find('=');
                if (eq != std::string::npos) {
                    std::string key = line.substr(0, eq);
                    std::string val = line.substr(eq + 1);
                    // Trim key
                    while (!key.empty() && key.back() == ' ') key.pop_back();
                    // Trim val
                    while (!val.empty() && val[0] == ' ') val.erase(0, 1);
                    while (!val.empty() && val.back() == ' ') val.pop_back();
                    if (!section.empty()) {
                        data[section][key] = val;
                    }
                }
            }
        }

        // Build fake JSON structure from INI
        // Returns a SimpleJson with children, each representing a section
        // Not needed for our actual config format (only .ini files have it)
        // Since INI only has leaf values, we'd need to reconstruct nested objects
        // For simplicity, just try JSON format and fall through
        return SimpleJson();  // INI support not fully implemented for non-nested format
    }
};

// ===================================================================
// Utility functions
// ===================================================================
inline std::wstring toWString(const std::string& s) {
    int size = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.length(), nullptr, 0);
    std::wstring buf(size, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.length(), &buf[0], size);
    return buf;
}

inline void print_hex(const char* buffer, size_t length) {
    std::ostringstream oss;
    for (size_t i = 0; i < length; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0')
            << (int)(unsigned char)buffer[i] << " ";
    }
    LOG_DEBUG << "hex: " << oss.str();
    LOG_DEBUG << "text: " << std::string(buffer, length);
}

inline DWORD GetModuleSize(HMODULE hModule) {
    auto* dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(hModule);
    if (!dosHeader || dosHeader->e_magic != IMAGE_DOS_SIGNATURE) return 0;
    auto* ntHeader = reinterpret_cast<PIMAGE_NT_HEADERS>(
        reinterpret_cast<DWORD>(hModule) + dosHeader->e_lfanew);
    if (!ntHeader || ntHeader->Signature != IMAGE_NT_SIGNATURE) return 0;
    return ntHeader->OptionalHeader.SizeOfImage;
}

inline DWORD GetEntryPointOffset(HMODULE hModule) {
    auto* dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(hModule);
    if (!dosHeader || dosHeader->e_magic != IMAGE_DOS_SIGNATURE) return 0;
    auto* ntHeader = reinterpret_cast<PIMAGE_NT_HEADERS>(
        reinterpret_cast<DWORD>(hModule) + dosHeader->e_lfanew);
    if (!ntHeader || ntHeader->Signature != IMAGE_NT_SIGNATURE) return 0;
    return ntHeader->OptionalHeader.BaseOfCode;
}

// ===================================================================
// Peerchat Cipher (RC4-like)
// ===================================================================
class PeerchatCipher {
public:
    PeerchatCipher() : pc1(0), pc2(0), initialized(false) {
        table.resize(256);
        for (int i = 0; i < 256; i++) table[i] = (uint8_t)(255 - i);
    }

    void init(const std::string& challenge, const std::string& gamekey) {
        if (gamekey.empty() || challenge.empty()) { initialized = false; return; }
        this->challenge = challenge;
        pc1 = pc2 = 0;
        for (int i = 0; i < 256; i++) table[i] = (uint8_t)(255 - i);

        std::vector<uint8_t> ch(challenge.size());
        for (size_t i = 0; i < challenge.size(); i++)
            ch[i] = (uint8_t)challenge[i] ^ (uint8_t)gamekey[i % gamekey.size()];

        uint8_t tmp = 0;
        for (size_t i = 0; i < table.size(); i++) {
            tmp = (tmp + ch[i % ch.size()] + table[i]) & 0xFF;
            std::swap(table[i], table[tmp]);
        }
        initialized = true;
    }

    std::vector<uint8_t> crypt(const uint8_t* data, size_t len) {
        std::vector<uint8_t> output(len);
        for (size_t i = 0; i < len; i++) {
            pc1 = (pc1 + 1) & 0xFF;
            uint8_t tmp = table[pc1];
            pc2 = (pc2 + tmp) & 0xFF;
            std::swap(table[pc1], table[pc2]);
            tmp = (tmp + table[pc1]) & 0xFF;
            output[i] = data[i] ^ table[tmp];
        }
        return output;
    }

    std::string cryptToString(const char* data, size_t len) {
        auto decrypted = crypt(reinterpret_cast<const uint8_t*>(data), len);
        return std::string(decrypted.begin(), decrypted.end());
    }

    bool isInitialized() const { return initialized; }
private:
    std::vector<uint8_t> table;
    uint8_t pc1, pc2;
    std::string challenge;
    bool initialized;
};

struct PeerchatState {
    SOCKET socket = INVALID_SOCKET;
    bool cryptRequested = false;
    bool encryptionEnabled = false;
    PeerchatCipher sendCipher, recvCipher;
};

// ===================================================================
// EncTypeX Cipher (Master Server)
// ===================================================================
class EncTypeXCipher {
public:
    EncTypeXCipher() : start(0), n1(0), n2(0), initialized(false) {
        encxkey.resize(261, 0);
    }

    void init(const std::string& key, const std::string& validate) {
        if (key.empty()) { initialized = false; return; }
        this->key.assign(key.begin(), key.end());
        this->validate.assign(validate.begin(), validate.end());
        start = 0;
    }

    std::string decode(const uint8_t* data, size_t len) {
        if (!initialized || len == 0)
            return std::string(reinterpret_cast<const char*>(data), len);
        size_t offset = 0;
        if (start == 0 && len >= 1) {
            size_t hdrLen = (data[0] ^ 0xEC) + 2;
            if (len < hdrLen) return "";
            size_t ivLen = data[hdrLen - 1] ^ 0xEA;
            start = hdrLen + ivLen;
            if (len < start) return "";
            std::vector<uint8_t> iv(data + hdrLen, data + hdrLen + ivLen);
            initDecoder(iv);
            offset = start;
        }
        if (offset >= len) return "";
        return decrypt(data + offset, len - offset);
    }

    bool isInitialized() const { return initialized; }

private:
    std::vector<uint8_t> key, validate, salt, iv, encxkey;
    size_t start;
    int n1, n2;
    bool initialized;

    void initDecoder(const std::vector<uint8_t>& salt) {
        this->salt = salt;
        iv = validate;
        for (size_t i = 0; i < salt.size(); i++) {
            size_t keyIdx = i % key.size();
            size_t ivIdx = (key[keyIdx] * i) & 7;
            if (ivIdx < iv.size() && (i & 7) < iv.size())
                iv[ivIdx] ^= iv[i & 7] ^ salt[i];
        }
        encxkey.resize(261);
        for (int i = 0; i < 256; i++) encxkey[i] = (uint8_t)i;
        for (int i = 256; i < 261; i++) encxkey[i] = 0;
        n1 = n2 = 0;
        if (iv.empty()) return;
        for (int i = 255; i >= 0; i--) {
            uint8_t t1 = func5(i), t2 = encxkey[i];
            encxkey[i] = encxkey[t1];
            encxkey[t1] = t2;
        }
        encxkey[256] = encxkey[1]; encxkey[257] = encxkey[3];
        encxkey[258] = encxkey[5]; encxkey[259] = encxkey[7];
        encxkey[260] = encxkey[n1 & 0xff];
        initialized = true;
    }

    uint8_t func5(int cnt) {
        if (cnt == 0) return 0;
        int mask = 0;
        while (mask < cnt) mask = (mask << 1) + 1;
        int i = 0, tmp;
        while (true) {
            n1 = encxkey[n1 & 0xff] + iv[n2];
            n2++;
            if (n2 >= (int)iv.size()) { n2 = 0; n1 += (int)iv.size(); }
            tmp = n1 & mask;
            i++;
            if (i > 11) tmp %= cnt;
            if (tmp <= cnt) break;
        }
        return (uint8_t)tmp;
    }

    std::string decrypt(const uint8_t* data, size_t len) {
        std::vector<uint8_t> output(len);
        for (size_t i = 0; i < len; i++) {
            uint8_t d = data[i];
            uint8_t a = encxkey[256], b = encxkey[257], c = encxkey[a];
            encxkey[256] = (a + 1) & 0xff;
            encxkey[257] = (b + c) & 0xff;
            a = encxkey[260]; b = encxkey[257]; b = encxkey[b]; c = encxkey[a]; encxkey[a] = b;
            a = encxkey[259]; b = encxkey[257]; a = encxkey[a]; encxkey[b] = a;
            a = encxkey[256]; b = encxkey[259]; a = encxkey[a]; encxkey[b] = a;
            a = encxkey[256]; encxkey[a] = c;
            b = encxkey[258]; a = encxkey[c]; c = encxkey[259]; b = (b + a) & 0xff; encxkey[258] = b;
            a = b; c = encxkey[c]; b = encxkey[257]; b = encxkey[b]; a = encxkey[a]; c = (c + b) & 0xff;
            b = encxkey[260]; b = encxkey[b]; c = (c + b) & 0xff;
            b = encxkey[c]; c = encxkey[256]; c = encxkey[c]; a = (a + c) & 0xff;
            c = encxkey[b]; b = encxkey[a];
            c ^= b ^ d;
            encxkey[259] = c; encxkey[260] = d;
            output[i] = c;
        }
        return std::string(output.begin(), output.end());
    }
};

struct MasterServerState {
    SOCKET socket = INVALID_SOCKET;
    std::string validate;
    bool cipherReady = false;
    EncTypeXCipher decoder;
};

// ===================================================================
// Forward declarations
// ===================================================================
class ProxySSL;   // defined in winmm_dll.cpp
class RA3BNDelegate;  // defined in winmm_dll.cpp
