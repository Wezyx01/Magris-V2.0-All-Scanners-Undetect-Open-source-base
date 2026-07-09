#pragma once

#include <string>

template<unsigned N>
struct _EA {
    char d[N];
    constexpr _EA(const char(&s)[N]) : d{} {
        for (unsigned i = 0; i < N; i++)
            d[i] = s[i] ^ static_cast<char>(0x55 + (i & 7));
    }
};
template<unsigned N>
struct _EW {
    wchar_t d[N];
    constexpr _EW(const wchar_t(&s)[N]) : d{} {
        for (unsigned i = 0; i < N; i++)
            d[i] = s[i] ^ static_cast<wchar_t>(0x55 + (i & 7));
    }
};
static __forceinline std::string _DA(const char* e, unsigned n) {
    std::string r(n, 0);
    for (unsigned i = 0; i < n; i++) r[i] = e[i] ^ static_cast<char>(0x55 + (i & 7));
    return r;
}
static __forceinline std::wstring _DW(const wchar_t* e, unsigned n) {
    std::wstring r(n, 0);
    for (unsigned i = 0; i < n; i++) r[i] = e[i] ^ static_cast<wchar_t>(0x55 + (i & 7));
    return r;
}
#define XS(s)   _DA(_EA<sizeof(s)>(s).d, sizeof(s) - 1)
#define XSF(s)  XS(s)
#define XW(s)   _DW(_EW<sizeof(s) / sizeof(wchar_t)>(s).d, sizeof(s) / sizeof(wchar_t) - 1)
#define XWC(s)  ([]() -> const wchar_t* { static std::wstring _c = XW(s); return _c.c_str(); }())
#define XSC(s)  ([]() -> const char* { static std::string _c = XS(s); return _c.c_str(); }())

