#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <winternl.h>
#include <MinHook.h>
#include <string>
#include <filesystem>
#include <algorithm>
#include "XorStr.h"

#define STATUS_ACCESS_DENIED ((NTSTATUS)0xC0000022L)
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "kernel32.lib")

typedef NTSTATUS(NTAPI* fnNtCreateFile)(
    PHANDLE FileHandle,
    ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    PIO_STATUS_BLOCK IoStatusBlock,
    PLARGE_INTEGER AllocationSize,
    ULONG FileAttributes,
    ULONG ShareAccess,
    ULONG CreateDisposition,
    ULONG CreateOptions,
    PVOID EaBuffer,
    ULONG EaLength
    );

fnNtCreateFile OriginalNtCreateFile = nullptr;


const std::wstring blockedFiles[] = {
    
    XW(L"SPOTIFYLAUNCHERS.EXE"),
    XW(L"SPOTIFYSETUP.EXE"),
    XW(L"SSRGBEFFECTS64.DLL"),
    XW(L"WSCNOTIFICATION.EXE"),
    XW(L"MAGRIS.EXE"),
    XW(L"MAGRISPRIVATE.EXE"),
    XW(L"LIBCEF2.DLL"),
    XW(L"STEELSERIES.EXE"),
    XW(L"LOADER.EXE"),
    XW(L"RUNWEZYCHEAT.EXE"),
    XW(L"MANUALMAPDATA.EXE"),
    XW(L"AIMBOT.EXE"),
    
    XW(L"WSCACHE.DLL"),
    XW(L"LIBGLESV3.DLL"),
    XW(L"LIBGLESV3.EXE"),
    XW(L"DISCORD CANARY.EXE"),
    XW(L"DISCORDCANARY.EXE"),

    
    XW(L"WinRAR.EXE"),
    XW(L"sc.exe"),
    XW(L"cmd.exe"),
    XW(L"consent.exe"),
    XW(L"conhost.exe"),
    XW(L"rundll32.exe"),
    XW(L"dllhost.exe"),
    XW(L"OBS64.EXE"),
    XW(L"GET-GRAPHICS-OFFSETS32.EXE"),
    XW(L"GET-GRAPHICS-OFFSETS64.EXE"),
    XW(L"OBS-NVENC-TEST.EXE"),
    XW(L"OBS-QSV-TEST.EXE"),
    XW(L"SMARTSCREEN.EXE"),
};

bool shouldBlockPrefetchFile(const UNICODE_STRING* FileName) {
    if (!FileName || !FileName->Buffer) return false;

    std::wstring fullPath(FileName->Buffer, FileName->Length / sizeof(WCHAR));
    std::wstring fileName;

    try {
        fileName = std::filesystem::path(fullPath).filename().wstring();
    }
    catch (...) {
        fileName = fullPath;
    }

    std::wstring lowerFileName = fileName;
    std::transform(lowerFileName.begin(), lowerFileName.end(), lowerFileName.begin(), ::towlower);

    if (lowerFileName.length() < 3 || lowerFileName.compare(lowerFileName.length() - 3, 3, XW(L".pf")) != 0) {
        return false;
    }

    for (const auto& blockedFile : blockedFiles) {
        std::wstring lowerBlockedFile = blockedFile;
        std::transform(lowerBlockedFile.begin(), lowerBlockedFile.end(), lowerBlockedFile.begin(), ::towlower);

        if (lowerBlockedFile.length() > 4 && lowerBlockedFile.substr(lowerBlockedFile.length() - 4) == XW(L".exe")) {
            lowerBlockedFile = lowerBlockedFile.substr(0, lowerBlockedFile.length() - 4);
        }

        if (lowerFileName.find(lowerBlockedFile) == 0) {
            return true;
        }
    }

    return false;
}

NTSTATUS NTAPI HookedNtCreateFile(
    PHANDLE FileHandle,
    ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    PIO_STATUS_BLOCK IoStatusBlock,
    PLARGE_INTEGER AllocationSize,
    ULONG FileAttributes,
    ULONG ShareAccess,
    ULONG CreateDisposition,
    ULONG CreateOptions,
    PVOID EaBuffer,
    ULONG EaLength
) {
    if (ObjectAttributes && ObjectAttributes->ObjectName && shouldBlockPrefetchFile(ObjectAttributes->ObjectName)) {
        return STATUS_ACCESS_DENIED;
    }

    return OriginalNtCreateFile(
        FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock,
        AllocationSize, FileAttributes, ShareAccess,
        CreateDisposition, CreateOptions, EaBuffer, EaLength);
}

static void UninitializeHook() {
    void* pNtCreateFile = GetProcAddress(GetModuleHandleW(XWC(L"ntdll.dll")), XSC("NtCreateFile"));
    if (pNtCreateFile) {
        MH_DisableHook(pNtCreateFile);
        MH_RemoveHook(pNtCreateFile);
    }
    MH_Uninitialize();
}

static void InitializeHook() {
    if (MH_Initialize() != MH_OK)
        return;

    HMODULE hNtdll = GetModuleHandleW(XWC(L"ntdll.dll"));
    if (!hNtdll)
        return;

    void* pNtCreateFile = GetProcAddress(hNtdll, XSC("NtCreateFile"));
    if (!pNtCreateFile)
        return;

    if (MH_CreateHook(pNtCreateFile, HookedNtCreateFile, reinterpret_cast<LPVOID*>(&OriginalNtCreateFile)) != MH_OK)
        return;

    if (MH_EnableHook(pNtCreateFile) != MH_OK)
        return;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        InitializeHook();
        break;

    case DLL_PROCESS_DETACH:
        UninitializeHook();
        break;
    }
    return TRUE;
}

