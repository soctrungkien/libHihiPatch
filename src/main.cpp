#include <filesystem>
#include <fstream>
#include <functional>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <string>
#include <vector>
#include <pthread.h>
#include <unistd.h>
#include <android/log.h>
#include <dobby.h> 

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "MinecraftBedrockArchive", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "MinecraftBedrockArchive", __VA_ARGS__)

namespace fs = std::filesystem;

class OreUIConfig {
public:
    void *mUnknown1;
    void *mUnknown2;
    std::function<bool()> mUnknown3;
    std::function<bool()> mUnknown4;
};

class OreUi {
public:
    std::unordered_map<std::string, OreUIConfig> mConfigs;
};

#ifndef USE_PATH_MOD
std::string getPackageName() {
    std::ifstream cmdline("/proc/self/cmdline");
    std::string pkgName;
    if (std::getline(cmdline, pkgName, '\0') && !pkgName.empty()) {
        return pkgName;
    }
    return "com.mojang.minecraftpe"; 
}
#endif

std::string getConfigDir() {
    std::string primary = "";
#ifdef USE_PATH_MOD
    primary = "/sdcard/games/MinecraftBedrockArchive/";
#else
    std::string pkgName = getPackageName();
    primary = "/sdcard/Android/data/" + pkgName + "/files/mods/MinecraftBedrockArchive/";
#endif
    std::error_code ec;
    fs::create_directories(primary, ec); 
    return primary;
}

void saveJson(const std::string &path, const nlohmann::ordered_json &j) {
    std::error_code ec;
    fs::create_directories(fs::path(path).parent_path(), ec);
    FILE *f = std::fopen(path.c_str(), "w");
    if (!f) {
        LOGE("Failed to open config file for writing: %s", path.c_str());
        return;
    }
    std::string jsonStr = j.dump(4);
    std::fwrite(jsonStr.data(), 1, jsonStr.size(), f);
    std::fclose(f);
}

// OreUI hook
void (*orig_OreUi_init)(OreUi&, void*, void*, void*, void*, void*);

void hook_OreUi_init(OreUi &a1, void *a2, void *a3, void *a4, void *a5, void *a6) {
    orig_OreUi_init(a1, a2, a3, a4, a5, a6);

    std::string filePath = getConfigDir() + "ForceCloseOreUI.json";
    nlohmann::ordered_json oreUiJson;
    bool updated = false;

    if (fs::exists(filePath)) {
        std::ifstream inFile(filePath);
        if (inFile.is_open()) {
            inFile >> oreUiJson;
            inFile.close();
        }
    }

    nlohmann::ordered_json newJson;
    bool isEnabled = true;

    if (oreUiJson.contains("Settings") && oreUiJson["Settings"].contains("enabled") && oreUiJson["Settings"]["enabled"].is_boolean()) {
        isEnabled = oreUiJson["Settings"]["enabled"];
    } else {
        updated = true;
    }
    newJson["Settings"]["enabled"] = isEnabled;

    for (auto &data : a1.mConfigs) {
        bool value = false;
        
        if (oreUiJson.contains("Screens") && oreUiJson["Screens"].contains(data.first) && oreUiJson["Screens"][data.first].is_boolean()) {
            value = oreUiJson["Screens"][data.first];
        } else {
            updated = true;
        }

        newJson["Screens"][data.first] = value;

        if (isEnabled) {
            data.second.mUnknown3 = [value]() { return value; };
            data.second.mUnknown4 = [value]() { return value; };
        }
    }

    if (updated || !fs::exists(filePath)) {
        saveJson(filePath, newJson);
    }
}

// NoDisconnect hook
bool (*orig_isInEDUMultiplayerSession)(void*);

bool hook_isInEDUMultiplayerSession(void* _this) {
    return true;
}

// Immortality hook from AN1.COM
void* (*orig_Immortality)(void*, void*, void*, void*);

void* hook_Immortality(void* a1, void* a2, void* a3, void* a4) {
    return nullptr; 
}

// Host Max Players hook from ApollonClient
bool g_MaxPlayersEnabled = true;
int g_MaxPlayersCount = 100;

void* (*orig_setMaxPlayers)(void*, unsigned int);

void* hook_setMaxPlayers(void* _this, unsigned int maxPlayers) {
    if (g_MaxPlayersEnabled) {
        maxPlayers = (unsigned int)g_MaxPlayersCount;
    }
    return orig_setMaxPlayers(_this, maxPlayers);
}

// Hotspot Multiplayer Fix hook from ApollonClient
bool g_HotspotFixEnabled = true;

bool (*orig_HotspotFix)(void*);

bool hook_HotspotFix(void* _this) {
    if (g_HotspotFixEnabled) {
        return true;
    }
    return orig_HotspotFix(_this);
}

// Signatures
const char* OREUI_PATTERN = "? ? ? D1 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? 91 ? ? ? D5 FA 03 03 2A F7 03 02 2A ? ? ? F9 F4 03 01 AA";
const char* EDU_MULTIPLAYER_PATTERN = "? ? ? D1 ? ? ? A9 ? ? ? F9 ? ? ? A9 ? ? ? 91 55 D0 3B D5 F3 03 00 AA ? ? ? F9 ? ? ? F8 ? ? ? F9 ? ? ? F9 ? ? ? 91 20 01 3F D6 ? ? ? F9 ? ? ? B4 ? ? ? 39";
const char* IMMORTALITY_PATTERN = "E8 0F 19 FC FD 7B 01 A9 FC 6F 02 A9 FA 67 03 A9 F8 5F 04 A9 F6 57 05 A9 F4 4F 06 A9 FD 43 00 91 FF C3 0F D1 58 D0 3B D5 F3 03 02 AA 08 40 20 1E";
const char* MAX_PLAYERS_PATTERN = "FF 83 02 D1 E8 23 00 FD FD 7B 05 A9 FA 67 06 A9 F8 5F 07 A9 F6 57 08 A9 F4 4F 09 A9 FD 43 01 91 57 D0 3B D5 D6 FA 03 B0 D6 42 00 91 E8 16 40 F9";
const char* HOTSPOT_FIX_PATTERN = "60 96 40 F9 7F 96 00 F9 40 00 00 B4 41 8B F6 95"; 

static uintptr_t ResolveSignature(const char* sig) {
    std::vector<int> pattern;
    const char* p = sig;
    while (*p) {
        if (*p == ' ') { p++; continue; }
        if (*p == '?') { pattern.push_back(-1); p++; if(*p=='?') p++; continue; }
        pattern.push_back(strtol(p, nullptr, 16));
        p += 2;
    }

    FILE* fp = fopen("/proc/self/maps", "r");
    if (!fp) return 0;

    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        if (!strstr(line, "libminecraftpe.so") || !strstr(line, "r-x")) continue; 
        
        uintptr_t start, end;
        if (sscanf(line, "%lx-%lx", &start, &end) != 2) continue;

        uint8_t* scan_base = (uint8_t*)start;
        size_t size = end - start;
        if (size < pattern.size()) continue;

        for (size_t i = 0; i < size - pattern.size(); i++) {
            bool found = true;
            for (size_t j = 0; j < pattern.size(); j++) {
                if (pattern[j] != -1 && scan_base[i + j] != pattern[j]) {
                    found = false;
                    break;
                }
            }
            if (found) {
                fclose(fp);
                return (uintptr_t)(scan_base + i);
            }
        }
    }
    fclose(fp);
    return 0;
}

void* InjectionThread(void* arg) {
    LOGI("MinecraftBedrockArchive Turbo Thread started.");

    bool isLoaded = false;
    while (!isLoaded) {
        FILE* fp = fopen("/proc/self/maps", "r");
        if (fp) {
            char line[512];
            while (fgets(line, sizeof(line), fp)) {
                if (strstr(line, "libminecraftpe.so") && strstr(line, "r-x")) {
                    isLoaded = true;
                    break;
                }
            }
            fclose(fp);
        }
        if (!isLoaded) usleep(10000); 
    }

    LOGI("libminecraftpe.so mapped! Scanning memory instantly...");

    // NoDisconnect config
    std::string noDisconnectPath = getConfigDir() + "NoDisconnect.json";
    nlohmann::ordered_json noDisconnectJson;
    nlohmann::ordered_json newNdJson;
    bool ndUpdated = false;
    bool ndEnabled = true;

    if (fs::exists(noDisconnectPath)) {
        std::ifstream inFile(noDisconnectPath);
        if (inFile.is_open()) {
            inFile >> noDisconnectJson;
            inFile.close();
        }
    }

    if (noDisconnectJson.contains("Settings") && noDisconnectJson["Settings"].contains("enabled") && noDisconnectJson["Settings"]["enabled"].is_boolean()) {
        ndEnabled = noDisconnectJson["Settings"]["enabled"];
    } else {
        ndUpdated = true;
    }
    
    newNdJson["Settings"]["enabled"] = ndEnabled;

    if (ndUpdated || !fs::exists(noDisconnectPath)) {
        saveJson(noDisconnectPath, newNdJson);
    }

    // Immortality config
    std::string immortalityPath = getConfigDir() + "Immormality.json";
    nlohmann::ordered_json immortalityJson;
    nlohmann::ordered_json newImJson;
    bool imUpdated = false;
    bool imEnabled = false;

    if (fs::exists(immortalityPath)) {
        std::ifstream inFile(immortalityPath);
        if (inFile.is_open()) {
            inFile >> immortalityJson;
            inFile.close();
        }
    }

    if (immortalityJson.contains("Settings") && immortalityJson["Settings"].contains("enabled") && immortalityJson["Settings"]["enabled"].is_boolean()) {
        imEnabled = immortalityJson["Settings"]["enabled"];
    } else {
        imUpdated = true;
    }
    
    newImJson["Settings"]["enabled"] = imEnabled;

    if (imUpdated || !fs::exists(immortalityPath)) {
        saveJson(immortalityPath, newImJson);
    }

    // Host Max Players config
    std::string maxPlayersPath = getConfigDir() + "HostMaxPlayers.json";
    nlohmann::ordered_json maxPlayersJson;
    nlohmann::ordered_json newMpJson;
    bool mpUpdated = false;

    if (fs::exists(maxPlayersPath)) {
        std::ifstream inFile(maxPlayersPath);
        if (inFile.is_open()) {
            inFile >> maxPlayersJson;
            inFile.close();
        }
    }

    if (maxPlayersJson.contains("Settings") && maxPlayersJson["Settings"].contains("enabled") && maxPlayersJson["Settings"]["enabled"].is_boolean()) {
        g_MaxPlayersEnabled = maxPlayersJson["Settings"]["enabled"];
    } else {
        mpUpdated = true;
    }

    if (maxPlayersJson.contains("Settings") && maxPlayersJson["Settings"].contains("max_players") && maxPlayersJson["Settings"]["max_players"].is_number()) {
        g_MaxPlayersCount = maxPlayersJson["Settings"]["max_players"];
    } else {
        mpUpdated = true;
    }
    
    newMpJson["Settings"]["enabled"] = g_MaxPlayersEnabled;
    newMpJson["Settings"]["max_players"] = g_MaxPlayersCount;

    if (mpUpdated || !fs::exists(maxPlayersPath)) {
        saveJson(maxPlayersPath, newMpJson);
    }

    // Hotspot Multiplayer Fix config
    std::string hotspotFixPath = getConfigDir() + "HotspotFix.json";
    nlohmann::ordered_json hotspotFixJson;
    nlohmann::ordered_json newHfJson;
    bool hfUpdated = false;

    if (fs::exists(hotspotFixPath)) {
        std::ifstream inFile(hotspotFixPath);
        if (inFile.is_open()) {
            inFile >> hotspotFixJson;
            inFile.close();
        }
    }

    if (hotspotFixJson.contains("Settings") && hotspotFixJson["Settings"].contains("enabled") && hotspotFixJson["Settings"]["enabled"].is_boolean()) {
        g_HotspotFixEnabled = hotspotFixJson["Settings"]["enabled"];
    } else {
        hfUpdated = true;
    }
    
    newHfJson["Settings"]["enabled"] = g_HotspotFixEnabled;

    if (hfUpdated || !fs::exists(hotspotFixPath)) {
        saveJson(hotspotFixPath, newHfJson);
    }

    // Hook application loop
    bool oreUiHooked = false;
    bool noDisconnectHooked = false;
    bool immortalityHooked = false;
    bool maxPlayersHooked = false;
    bool hotspotFixHooked = false;

    for (int attempts = 1; attempts <= 100; attempts++) {
        if (!oreUiHooked) {
            uintptr_t addr = ResolveSignature(OREUI_PATTERN);
            if (addr != 0) {
                LOGI("SUCCESS: Found OreUI signature! Applying DobbyHook...");
                DobbyHook((void*)addr, (void*)hook_OreUi_init, (void**)&orig_OreUi_init);
                oreUiHooked = true;
            }
        }

        if (!noDisconnectHooked) {
            if (ndEnabled) {
                uintptr_t addr = ResolveSignature(EDU_MULTIPLAYER_PATTERN);
                if (addr != 0) {
                    LOGI("SUCCESS: Found EduMultiplayer signature! Applying NoDisconnect DobbyHook...");
                    DobbyHook((void*)addr, (void*)hook_isInEDUMultiplayerSession, (void**)&orig_isInEDUMultiplayerSession);
                    noDisconnectHooked = true;
                }
            } else {
                noDisconnectHooked = true;
            }
        }

        if (!immortalityHooked) {
            if (imEnabled) {
                uintptr_t addr = ResolveSignature(IMMORTALITY_PATTERN);
                if (addr != 0) {
                    LOGI("SUCCESS: Found Immortality signature! Applying DobbyHook...");
                    DobbyHook((void*)addr, (void*)hook_Immortality, (void**)&orig_Immortality);
                    immortalityHooked = true;
                }
            } else {
                immortalityHooked = true;
            }
        }

        if (!maxPlayersHooked) {
            if (g_MaxPlayersEnabled) {
                uintptr_t addr = ResolveSignature(MAX_PLAYERS_PATTERN);
                if (addr != 0) {
                    LOGI("SUCCESS: Found MaxPlayers signature! Applying DobbyHook...");
                    DobbyHook((void*)addr, (void*)hook_setMaxPlayers, (void**)&orig_setMaxPlayers);
                    maxPlayersHooked = true;
                }
            } else {
                maxPlayersHooked = true;
            }
        }

        if (!hotspotFixHooked) {
            if (g_HotspotFixEnabled) {
                uintptr_t addr = ResolveSignature(HOTSPOT_FIX_PATTERN);
                if (addr != 0) {
                    LOGI("SUCCESS: Found HotspotFix signature! Applying DobbyHook...");
                    DobbyHook((void*)addr, (void*)hook_HotspotFix, (void**)&orig_HotspotFix);
                    hotspotFixHooked = true;
                }
            } else {
                hotspotFixHooked = true;
            }
        }

        if (oreUiHooked && noDisconnectHooked && immortalityHooked && maxPlayersHooked && hotspotFixHooked) break;
        usleep(50000); 
    }

    if (!oreUiHooked) LOGE("FATAL: Could not find OreUI pattern in memory.");
    if (ndEnabled && !noDisconnectHooked) LOGE("FATAL: Could not find EduMultiplayer pattern in memory.");
    if (imEnabled && !immortalityHooked) LOGE("FATAL: Could not find Immortality pattern in memory.");
    if (g_MaxPlayersEnabled && !maxPlayersHooked) LOGE("FATAL: Could not find MaxPlayers pattern in memory.");
    if (g_HotspotFixEnabled && !hotspotFixHooked) LOGE("FATAL: Could not find HotspotFix pattern in memory.");

    return nullptr;
}

__attribute__((constructor))
void MinecraftBedrockArchive_Init() {
    pthread_t thread;
    pthread_create(&thread, nullptr, InjectionThread, nullptr);
    pthread_detach(thread);
}
