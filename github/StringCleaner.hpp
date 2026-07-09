#pragma once

#include <Windows.h>
#include <string>
#include <vector>
#include "XorStr.h"

namespace StringCleanerUtil {

    inline void SetDebugPrivileges() {
        HANDLE hToken = NULL;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
            return;

        TOKEN_PRIVILEGES tp = {};
        tp.PrivilegeCount = 1;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        if (LookupPrivilegeValueW(NULL, XWC(L"SeDebugPrivilege"), &tp.Privileges[0].Luid)) {
            AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);
        }
        CloseHandle(hToken);
    }

} 

class StringCleaner {
public:
    
    StringCleaner(DWORD processId, std::vector<std::string> strings) : _pid(processId), _strings(strings), _regionBases() {
        SYSTEM_INFO sys_info;
        GetSystemInfo(&sys_info);
        _min_address = (size_t)sys_info.lpMinimumApplicationAddress;
        _max_address = (size_t)sys_info.lpMaximumApplicationAddress;
    }

    
    StringCleaner(DWORD processId, std::vector<std::string> strings, std::vector<size_t> regionBases)
        : _pid(processId), _strings(strings), _regionBases(regionBases) {
        SYSTEM_INFO sys_info;
        GetSystemInfo(&sys_info);
        _min_address = (size_t)sys_info.lpMinimumApplicationAddress;
        _max_address = (size_t)sys_info.lpMaximumApplicationAddress;
    }

    void clean(HANDLE processHandle) {
        StringCleanerUtil::SetDebugPrivileges();
        _cleanStrings(processHandle);
        CloseHandle(processHandle);
    }

private:
    size_t _min_address;
    size_t _max_address;
    std::vector<std::string> _strings;
    std::vector<size_t> _regionBases;
    DWORD _pid;

    std::vector<size_t> _find_substr_indexes(std::string& string, const std::string& substring) {
        std::vector<size_t> indexes = {};
        size_t index = 0;
        while (true) {
            size_t x = string.find(substring, index);
            if (x == std::string::npos) break;
            indexes.push_back(x);
            index = x + substring.length();
        }
        return indexes;
    }

    void _cleanStrings(HANDLE processHandle) {
        const size_t kMaxRegionSize = 50 * 1024 * 1024;
        std::vector<std::string> fixed_strings;
        for (const std::string& raw_string : _strings) {
            fixed_strings.push_back(raw_string);
            std::string wide;
            for (const char c : raw_string) {
                wide.push_back(c);
                wide.push_back(0);
            }
            fixed_strings.push_back(wide);
        }

        if (!_regionBases.empty()) {
            for (size_t base : _regionBases) {
                MEMORY_BASIC_INFORMATION mbi = {};
                if (VirtualQueryEx(processHandle, (void*)base, &mbi, sizeof(mbi)) == 0) continue;
                if ((mbi.State & MEM_COMMIT) && (mbi.Protect & PAGE_READWRITE) && mbi.RegionSize > 0 && mbi.RegionSize <= kMaxRegionSize) {
                    std::string buffer(mbi.RegionSize, 0);
                    if (ReadProcessMemory(processHandle, (void*)base, &buffer[0], mbi.RegionSize, 0)) {
                        for (const std::string& string_to_remove : fixed_strings) {
                            std::vector<BYTE> replace(string_to_remove.size(), 0);
                            std::vector<size_t> indexes = _find_substr_indexes(buffer, string_to_remove);
                            for (size_t index : indexes) {
                                WriteProcessMemory(processHandle, (void*)(base + index), replace.data(), replace.size(), 0);
                            }
                        }
                    }
                }
            }
            return;
        }

        size_t current_address = _min_address;
        MEMORY_BASIC_INFORMATION mbi = {};
        while (current_address < _max_address) {
            if (VirtualQueryEx(processHandle, (void*)current_address, &mbi, sizeof(mbi)) == 0)
                break;

            if ((mbi.State & MEM_COMMIT) && (mbi.Protect & PAGE_READWRITE) && mbi.RegionSize > 0 && mbi.RegionSize <= kMaxRegionSize) {
                std::string buffer(mbi.RegionSize, 0);
                if (ReadProcessMemory(processHandle, (void*)current_address, &buffer[0], mbi.RegionSize, 0)) {
                    for (const std::string& string_to_remove : fixed_strings) {
                        std::vector<BYTE> replace(string_to_remove.size(), 0);
                        std::vector<size_t> indexes = _find_substr_indexes(buffer, string_to_remove);
                        for (size_t index : indexes) {
                            WriteProcessMemory(processHandle, (void*)(current_address + index), replace.data(), replace.size(), 0);
                        }
                    }
                }
            }
            current_address += mbi.RegionSize;
        }
    }
};

