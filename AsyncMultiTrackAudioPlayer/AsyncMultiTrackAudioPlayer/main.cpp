#include <windows.h>
#include <mmsystem.h>
#include <shellapi.h>
#include <string>
#include <vector>
#include <algorithm>
#include <cwctype>

#define IDC_PLAY_BUTTON 101
#define IDC_STOP_BUTTON 102
#define IDC_PATH_EDIT   103

HWND hEditPath;
std::vector<std::wstring> openAliases; // Tracks open sounds

// Helper to convert extension to lowercase for safe checking
std::wstring GetExtensionLower(const std::wstring& path) {
    size_t dotIdx = path.find_last_of(L".");
    if (dotIdx == std::wstring::npos) return L"";
    std::wstring ext = path.substr(dotIdx);
    std::transform(ext.begin(), ext.end(), ext.begin(), [](wchar_t c) {
        return std::towlower(c);
    });
    return ext;
}

void PlayMCI(HWND hwndNotify, std::wstring path) {
    if (path.empty()) return;

    // Clean up surrounding quotes safely
    if (path.length() >= 2 && path.front() == L'\"' && path.back() == L'\"') {
        path = path.substr(1, path.length() - 2);
    }

    // Determine the correct MCI device type based on extension
    std::wstring ext = GetExtensionLower(path);
    std::wstring deviceType = L"waveaudio"; // Default for .wav
    
    if (ext == L".mp3") {
        deviceType = L"MPEGVideo";          // Driver used for MP3 playback
    }

    // Unique alias name
    std::wstring alias = L"snd_" + std::to_wstring(GetTickCount64()) + std::to_wstring(rand());
    
    // Open the file with the appropriate device type
    std::wstring openCmd = L"open \"" + path + L"\" type " + deviceType + L" alias " + alias;
    if (mciSendStringW(openCmd.c_str(), NULL, 0, NULL) == 0) {
        // Play the file and request MM_MCINOTIFY when finished
        std::wstring playCmd = L"play " + alias + L" from 0 notify";
      
mciSendStringW(playCmd.c_str(), NULL, 0, (HWND)hwndNotify);
        openAliases.push_back(alias);
    }
}

void StopAllMCI() {
    for (const auto& alias : openAliases) {
        std::wstring stopCmd = L"stop " + alias;
        std::wstring closeCmd = L"close " + alias;
        mciSendStringW(stopCmd.c_str(), NULL, 0, NULL);
        mciSendStringW(closeCmd.c_str(), NULL, 0, NULL);
    }
    openAliases.clear();
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            DragAcceptFiles(hwnd, TRUE);
            hEditPath = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", 
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 20, 20, 440, 25, hwnd, (HMENU)IDC_PATH_EDIT, NULL, NULL);
            CreateWindowW(L"BUTTON", L"PLAY", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 
                20, 60, 215, 40, hwnd, (HMENU)IDC_PLAY_BUTTON, NULL, NULL);
            CreateWindowW(L"BUTTON", L"STOP ALL", WS_VISIBLE | WS_CHILD, 
                245, 60, 215, 40, hwnd, (HMENU)IDC_STOP_BUTTON, NULL, NULL);
            break;

        case WM_DROPFILES: {
            HDROP hDrop = (HDROP)wParam;
            wchar_t filePath[MAX_PATH];
            UINT fileCount = DragQueryFileW(hDrop, 0xFFFFFFFF, NULL, 0);
            for (UINT i = 0; i < fileCount; i++) {
                DragQueryFileW(hDrop, i, filePath, MAX_PATH);
                PlayMCI(hwnd, filePath);
            }
            DragFinish(hDrop);
            break;
        }

        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_PLAY_BUTTON) {
                wchar_t buffer[MAX_PATH];
                GetWindowTextW(hEditPath, buffer, MAX_PATH);
                PlayMCI(hwnd, buffer);
            } else if (LOWORD(wParam) == IDC_STOP_BUTTON) {
                StopAllMCI();
            }
            break;

        case MM_MCINOTIFY: {
            MCIDEVICEID deviceID = (MCIDEVICEID)lParam;
            mciSendCommandW(deviceID, MCI_CLOSE, 0, 0);
            break;
        }

        case WM_DESTROY:
            StopAllMCI();
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"MCIPlayer";
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(0, L"MCIPlayer", L"Win32 Multi-Audio (MCI)", 
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, 
        CW_USEDEFAULT, CW_USEDEFAULT, 500, 160, NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, nCmdShow);
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}