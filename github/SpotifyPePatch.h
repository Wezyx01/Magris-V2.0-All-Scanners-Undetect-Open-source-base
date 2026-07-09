#pragma once

#include <string>


bool PatchSpotifyExecutable(const std::wstring& exePath);
bool RestoreSpotifyExecutable(const std::wstring& exePath);
bool PatchSpotifyInstallDir(const std::wstring& spotifyDir);
bool RestoreSpotifyInstallDir(const std::wstring& spotifyDir);
void RemoveLegacySpotifyStub(const std::wstring& spotifyDir);
bool WriteSpotifyPathFile(const std::wstring& spotifyDir, const std::wstring& realExePath);

