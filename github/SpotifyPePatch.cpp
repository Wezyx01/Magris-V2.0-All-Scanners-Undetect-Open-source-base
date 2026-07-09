#include "SpotifyPePatch.h"

#include <windows.h>
#include <cstring>
#include <vector>

static const char kMagrisSectionName[] = ".magris";

static bool ReadFileBytes(const std::wstring& path, std::vector<BYTE>& out) {
    HANDLE f = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (f == INVALID_HANDLE_VALUE)
        return false;
    LARGE_INTEGER sz = {};
    if (!GetFileSizeEx(f, &sz) || sz.QuadPart <= 0 || sz.QuadPart > 256 * 1024 * 1024) {
        CloseHandle(f);
        return false;
    }
    out.resize((size_t)sz.QuadPart);
    DWORD read = 0;
    BOOL ok = ReadFile(f, out.data(), (DWORD)out.size(), &read, NULL);
    CloseHandle(f);
    return ok && read == out.size();
}

static bool WriteFileBytes(const std::wstring& path, const std::vector<BYTE>& data) {
    HANDLE f = CreateFileW(path.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (f == INVALID_HANDLE_VALUE)
        return false;
    DWORD written = 0;
    BOOL ok = WriteFile(f, data.data(), (DWORD)data.size(), &written, NULL);
    CloseHandle(f);
    return ok && written == data.size();
}

static DWORD AlignUp(DWORD v, DWORD a) {
    return (v + a - 1) & ~(a - 1);
}

static IMAGE_NT_HEADERS64* GetNt64(std::vector<BYTE>& pe) {
    if (pe.size() < sizeof(IMAGE_DOS_HEADER))
        return nullptr;
    auto* dos = (IMAGE_DOS_HEADER*)pe.data();
    if (dos->e_magic != IMAGE_DOS_SIGNATURE)
        return nullptr;
    if ((size_t)dos->e_lfanew + sizeof(IMAGE_NT_HEADERS64) > pe.size())
        return nullptr;
    auto* nt = (IMAGE_NT_HEADERS64*)(pe.data() + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE || nt->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64)
        return nullptr;
    if (nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
        return nullptr;
    return nt;
}

static IMAGE_SECTION_HEADER* GetSectionHeaders(IMAGE_NT_HEADERS64* nt) {
    return (IMAGE_SECTION_HEADER*)((BYTE*)&nt->OptionalHeader + nt->FileHeader.SizeOfOptionalHeader);
}

static bool HasMagrisSection(IMAGE_NT_HEADERS64* nt) {
    auto* sec = GetSectionHeaders(nt);
    for (WORD i = 0; i < nt->FileHeader.NumberOfSections; i++) {
        if (strncmp((char*)sec[i].Name, kMagrisSectionName, IMAGE_SIZEOF_SHORT_NAME) == 0)
            return true;
    }
    return false;
}

static DWORD RvaToOffset(IMAGE_NT_HEADERS64* nt, IMAGE_SECTION_HEADER* sec, DWORD rva) {
    for (WORD i = 0; i < nt->FileHeader.NumberOfSections; i++) {
        DWORD va = sec[i].VirtualAddress;
        DWORD vs = sec[i].Misc.VirtualSize;
        if (rva >= va && rva < va + vs)
            return sec[i].PointerToRawData + (rva - va);
    }
    return 0;
}

static DWORD FindImportIatRva(const std::vector<BYTE>& pe, IMAGE_NT_HEADERS64* nt, const char* dllName, const char* funcName) {
    auto& dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (!dir.VirtualAddress || !dir.Size)
        return 0;
    auto* sec = GetSectionHeaders(nt);
    DWORD impOff = RvaToOffset(nt, sec, dir.VirtualAddress);
    if (!impOff)
        return 0;

    auto* desc = (IMAGE_IMPORT_DESCRIPTOR*)(pe.data() + impOff);
    for (; desc->Name; desc++) {
        DWORD nameOff = RvaToOffset(nt, sec, desc->Name);
        if (!nameOff)
            continue;
        if (_stricmp((const char*)(pe.data() + nameOff), dllName) != 0)
            continue;

        DWORD thunkRva = desc->OriginalFirstThunk ? desc->OriginalFirstThunk : desc->FirstThunk;
        DWORD iatRva = desc->FirstThunk;
        DWORD thunkOff = RvaToOffset(nt, sec, thunkRva);
        if (!thunkOff)
            return 0;

        auto* orig = (IMAGE_THUNK_DATA64*)(pe.data() + thunkOff);
        DWORD iatIndex = 0;
        for (; orig->u1.AddressOfData; orig++, iatIndex++) {
            if (orig->u1.Ordinal & IMAGE_ORDINAL_FLAG64)
                continue;
            DWORD ibnOff = RvaToOffset(nt, sec, (DWORD)orig->u1.AddressOfData);
            if (!ibnOff)
                continue;
            auto* ibn = (IMAGE_IMPORT_BY_NAME*)(pe.data() + ibnOff);
            if (strcmp((const char*)ibn->Name, funcName) == 0)
                return iatRva + (DWORD)(iatIndex * sizeof(IMAGE_THUNK_DATA64));
        }
    }
    return 0;
}

static void PatchU32(std::vector<BYTE>& buf, size_t pos, DWORD v) {
    if (pos + 4 <= buf.size())
        memcpy(buf.data() + pos, &v, 4);
}

static bool BuildMagrisSectionBody(std::vector<BYTE>& body, DWORD oepRva, DWORD iatCreateThread, DWORD iatLoadLibraryW, const std::wstring& dllName) {
    const DWORD kThreadProcRva = 0x80;
    const DWORD kDllPathRva = 0x120;

    std::vector<BYTE> code(0x200, 0x90);

    size_t p = 0;
    auto emit = [&](std::initializer_list<BYTE> bytes) {
        for (BYTE b : bytes) {
            if (p < code.size())
                code[p] = b;
            p++;
        }
    };

    
    emit({ 0x65, 0x48, 0x8B, 0x04, 0x25, 0x60, 0, 0, 0 });
    emit({ 0x48, 0x8B, 0x40, 0x10 });
    emit({ 0x49, 0x89, 0xC7 });
    emit({ 0x48, 0x31, 0xC9 });
    emit({ 0x48, 0x31, 0xD2 });
    size_t leaThread = p;
    emit({ 0x4D, 0x8D, 0x87, 0, 0, 0, 0 });
    size_t leaDll = p;
    emit({ 0x4D, 0x8D, 0x8F, 0, 0, 0, 0 });
    emit({ 0x48, 0x83, 0xEC, 0x28 });
    emit({ 0xC7, 0x44, 0x24, 0x20, 0, 0, 0, 0 });
    emit({ 0x48, 0xC7, 0x44, 0x24, 0x28, 0, 0, 0, 0 });
    size_t callCreate = p;
    emit({ 0x41, 0xFF, 0x97, 0, 0, 0, 0 });
    emit({ 0x48, 0x83, 0xC4, 0x28 });
    emit({ 0x4C, 0x89, 0xF8 });
    size_t addOep = p;
    emit({ 0x48, 0x05, 0, 0, 0, 0 });
    emit({ 0xFF, 0xE0 });

    
    p = kThreadProcRva;
    emit({ 0x48, 0x83, 0xEC, 0x28 });
    emit({ 0x65, 0x48, 0x8B, 0x04, 0x25, 0x60, 0, 0, 0 });
    emit({ 0x48, 0x8B, 0x40, 0x10 });
    emit({ 0x49, 0x89, 0xC7 });
    size_t callLoad = p;
    emit({ 0x41, 0xFF, 0x97, 0, 0, 0, 0 });
    emit({ 0x48, 0x83, 0xC4, 0x28 });
    emit({ 0x31, 0xC0 });
    emit({ 0xC3 });

    
    p = kDllPathRva;
    for (wchar_t ch : dllName) {
        if (p + 1 >= code.size())
            code.resize(p + 32, 0x90);
        code[p++] = (BYTE)(ch & 0xFF);
        code[p++] = (BYTE)((ch >> 8) & 0xFF);
    }
    if (p + 1 >= code.size())
        code.resize(p + 4, 0x90);
    code[p++] = 0;
    code[p++] = 0;

    PatchU32(code, leaThread + 3, kThreadProcRva);
    PatchU32(code, leaDll + 3, kDllPathRva);
    PatchU32(code, callCreate + 3, iatCreateThread);
    PatchU32(code, callLoad + 3, iatLoadLibraryW);
    PatchU32(code, addOep + 2, oepRva);

    body = std::move(code);
    return true;
}

static bool PatchPeBuffer(std::vector<BYTE>& pe) {
    IMAGE_NT_HEADERS64* nt = GetNt64(pe);
    if (!nt || HasMagrisSection(nt))
        return nt != nullptr;

    DWORD oepRva = nt->OptionalHeader.AddressOfEntryPoint;
    DWORD iatCreateThread = FindImportIatRva(pe, nt, "KERNEL32.dll", "CreateThread");
    DWORD iatLoadLibraryW = FindImportIatRva(pe, nt, "KERNEL32.dll", "LoadLibraryW");
    if (!iatCreateThread || !iatLoadLibraryW)
        return false;

    std::vector<BYTE> sectionBody;
    if (!BuildMagrisSectionBody(sectionBody, oepRva, iatCreateThread, iatLoadLibraryW, L"libcef2.dll"))
        return false;

    if (nt->FileHeader.NumberOfSections >= 95)
        return false;

    auto* sections = GetSectionHeaders(nt);
    WORD numSec = nt->FileHeader.NumberOfSections;
    auto& last = sections[numSec - 1];

    DWORD fileAlign = nt->OptionalHeader.FileAlignment ? nt->OptionalHeader.FileAlignment : 0x200;
    DWORD sectAlign = nt->OptionalHeader.SectionAlignment ? nt->OptionalHeader.SectionAlignment : 0x1000;

    DWORD newVa = AlignUp(last.VirtualAddress + last.Misc.VirtualSize, sectAlign);
    DWORD newRaw = AlignUp(last.PointerToRawData + last.SizeOfRawData, fileAlign);
    DWORD rawSize = AlignUp((DWORD)sectionBody.size(), fileAlign);
    DWORD virtSize = AlignUp((DWORD)sectionBody.size(), sectAlign);

    size_t newFileSize = (size_t)newRaw + rawSize;
    if (pe.size() < newFileSize)
        pe.resize(newFileSize, 0);

    IMAGE_SECTION_HEADER newSec = {};
    memcpy(newSec.Name, kMagrisSectionName, sizeof(kMagrisSectionName) - 1);
    newSec.Misc.VirtualSize = virtSize;
    newSec.VirtualAddress = newVa;
    newSec.SizeOfRawData = rawSize;
    newSec.PointerToRawData = newRaw;
    newSec.Characteristics = IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_CNT_CODE;

    sections[numSec] = newSec;
    nt->FileHeader.NumberOfSections = numSec + 1;
    nt->OptionalHeader.AddressOfEntryPoint = newVa;
    nt->OptionalHeader.SizeOfImage = AlignUp(newVa + virtSize, sectAlign);

    memcpy(pe.data() + newRaw, sectionBody.data(), sectionBody.size());
    return true;
}

static std::wstring CleanBackupPathForExe(const std::wstring& exePath) {
    size_t slash = exePath.find_last_of(L"\\/");
    std::wstring dir = (slash != std::wstring::npos) ? exePath.substr(0, slash + 1) : L"";
    std::wstring base = exePath.substr(slash + 1);
    size_t dot = base.find_last_of(L'.');
    std::wstring stem = (dot != std::wstring::npos) ? base.substr(0, dot) : base;
    return dir + stem + L"_clean.exe";
}

static bool IsLikelyRealSpotifyBinary(const std::wstring& path) {
    WIN32_FILE_ATTRIBUTE_DATA fad = {};
    if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &fad))
        return false;
    ULONGLONG sz = ((ULONGLONG)fad.nFileSizeHigh << 32) | fad.nFileSizeLow;
    return sz >= 512 * 1024;
}

bool RestoreSpotifyExecutable(const std::wstring& exePath) {
    std::wstring clean = CleanBackupPathForExe(exePath);
    if (GetFileAttributesW(clean.c_str()) == INVALID_FILE_ATTRIBUTES)
        return false;
    return CopyFileW(clean.c_str(), exePath.c_str(), FALSE) != FALSE;
}

bool PatchSpotifyExecutable(const std::wstring& exePath) {
    if (!IsLikelyRealSpotifyBinary(exePath))
        return false;

    std::wstring clean = CleanBackupPathForExe(exePath);
    std::vector<BYTE> current;
    if (!ReadFileBytes(exePath, current))
        return false;

    IMAGE_NT_HEADERS64* ntCur = GetNt64(current);
    if (!ntCur)
        return false;

  if (!HasMagrisSection(ntCur)) {
        if (!WriteFileBytes(clean, current))
            return false;
    }

    std::vector<BYTE> pe;
    if (!ReadFileBytes(clean, pe))
        return false;

    if (!PatchPeBuffer(pe))
        return false;
    return WriteFileBytes(exePath, pe);
}

void RemoveLegacySpotifyStub(const std::wstring& spotifyDir) {
    std::wstring realExe = spotifyDir + L"Spotify_real.exe";
    std::wstring spotifyExe = spotifyDir + L"Spotify.exe";
    if (GetFileAttributesW(realExe.c_str()) == INVALID_FILE_ATTRIBUTES)
        return;
    DeleteFileW(spotifyExe.c_str());
    MoveFileW(realExe.c_str(), spotifyExe.c_str());
}

static bool PatchIfPresent(const std::wstring& path) {
    if (GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES)
        return true;
    if (!IsLikelyRealSpotifyBinary(path))
        return true;
    return PatchSpotifyExecutable(path);
}

bool PatchSpotifyInstallDir(const std::wstring& spotifyDir) {
    RemoveLegacySpotifyStub(spotifyDir);
    bool ok = PatchIfPresent(spotifyDir + L"Spotify.exe");
    ok = PatchIfPresent(spotifyDir + L"SpotifyLauncher.exe") && ok;
    return ok;
}

bool RestoreSpotifyInstallDir(const std::wstring& spotifyDir) {
    RemoveLegacySpotifyStub(spotifyDir);
    bool ok = RestoreSpotifyExecutable(spotifyDir + L"Spotify.exe");
    std::wstring launcher = spotifyDir + L"SpotifyLauncher.exe";
    if (GetFileAttributesW(launcher.c_str()) != INVALID_FILE_ATTRIBUTES)
        ok = RestoreSpotifyExecutable(launcher) && ok;
    return ok;
}

static bool WriteSpotifyPathTxt(const std::wstring& spotifyDir, const std::wstring& realExePath) {
    std::wstring pathFile = spotifyDir + L"spotify_path.txt";
    HANDLE f = CreateFileW(pathFile.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (f == INVALID_HANDLE_VALUE)
        return false;
    std::string narrow;
    narrow.reserve(realExePath.size());
    for (wchar_t c : realExePath)
        narrow.push_back((char)(c < 256 ? c : '?'));
    DWORD w = 0;
    BOOL ok = WriteFile(f, narrow.c_str(), (DWORD)narrow.size(), &w, NULL);
    CloseHandle(f);
    return ok && w == narrow.size();
}

bool WriteSpotifyPathFile(const std::wstring& spotifyDir, const std::wstring& realExePath) {
    return WriteSpotifyPathTxt(spotifyDir, realExePath);
}

