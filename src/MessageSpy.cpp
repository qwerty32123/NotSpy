#include <iostream>
#include <windows.h>
#include <csignal>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <windowsx.h>

#include "json.hpp"
#include "Shared.h"

// --- Global variables for cleanup ---
HHOOK g_hHook1 = NULL;
HHOOK g_hHook2 = NULL;
SharedData* pSharedData = nullptr;
HANDLE hMapFile = NULL;
HMODULE hinstDLL = NULL;
bool g_useJson = false;


// --- Data structure for cleanly finding the target window ---
struct EnumData {
    DWORD process_id;
    HWND window_handle;
};

// --- Callback function for EnumWindows ---
BOOL CALLBACK EnumWindowsCallback(HWND hwnd, LPARAM lParam) {
    EnumData* data = reinterpret_cast<EnumData*>(lParam);
    DWORD current_pid = 0;
    GetWindowThreadProcessId(hwnd, &current_pid);

    if (data->process_id == current_pid) {
        if (IsWindowVisible(hwnd) && GetWindowTextLength(hwnd) > 0) {
            data->window_handle = hwnd;
            return FALSE; // Stop enumerating
        }
    }
    return TRUE; // Continue enumerating
}

// (The following helper functions: message_to_string, hittest_result_to_string, json_from_message

// Function to convert a message ID to its string name for readability
std::string message_to_string(UINT msg) {
    static std::map<UINT, std::string> messageMap = {
        {WM_CREATE, "WM_CREATE"}, {WM_DESTROY, "WM_DESTROY"}, {WM_CLOSE, "WM_CLOSE"},
        {WM_SHOWWINDOW, "WM_SHOWWINDOW"}, {WM_MOVE, "WM_MOVE"}, {WM_SIZE, "WM_SIZE"},
        {WM_ACTIVATE, "WM_ACTIVATE"}, {WM_ACTIVATEAPP, "WM_ACTIVATEAPP"},
        {WM_SETFOCUS, "WM_SETFOCUS"}, {WM_KILLFOCUS, "WM_KILLFOCUS"},
        {WM_PAINT, "WM_PAINT"}, {WM_ERASEBKGND, "WM_ERASEBKGND"}, {WM_NCPAINT, "WM_NCPAINT"},
        {WM_NCHITTEST, "WM_NCHITTEST"}, {WM_NCLBUTTONDOWN, "WM_NCLBUTTONDOWN"},
        {WM_MOUSEMOVE, "WM_MOUSEMOVE"}, {WM_LBUTTONDOWN, "WM_LBUTTONDOWN"},
        {WM_LBUTTONUP, "WM_LBUTTONUP"}, {WM_LBUTTONDBLCLK, "WM_LBUTTONDBLCLK"},
        {WM_RBUTTONDOWN, "WM_RBUTTONDOWN"}, {WM_RBUTTONUP, "WM_RBUTTONUP"},
        {WM_MOUSEWHEEL, "WM_MOUSEWHEEL"}, {WM_MOUSEACTIVATE, "WM_MOUSEACTIVATE"},
        {WM_SETCURSOR, "WM_SETCURSOR"}, {WM_KEYDOWN, "WM_KEYDOWN"}, {WM_KEYUP, "WM_KEYUP"},
        {WM_CHAR, "WM_CHAR"}, {WM_SYSKEYDOWN, "WM_SYSKEYDOWN"}, {WM_SYSKEYUP, "WM_SYSKEYUP"},
        {WM_COMMAND, "WM_COMMAND"}, {WM_NOTIFY, "WM_NOTIFY"}
    };
    if (messageMap.count(msg)) {
        return messageMap[msg];
    }
    char hex_buf[32];
    snprintf(hex_buf, sizeof(hex_buf), "0x%04X", msg);
    return std::string(hex_buf);
}

// Decodes a Hit-Test result code into its string name
std::string hittest_result_to_string(LRESULT res) {
    static std::map<LRESULT, std::string> hitTestMap = {
        {HTERROR, "HTERROR"}, {HTTRANSPARENT, "HTTRANSPARENT"}, {HTNOWHERE, "HTNOWHERE"},
        {HTCLIENT, "HTCLIENT"}, {HTCAPTION, "HTCAPTION"}, {HTSYSMENU, "HTSYSMENU"},
        {HTGROWBOX, "HTGROWBOX"}, {HTMENU, "HTMENU"}, {HTHSCROLL, "HTHSCROLL"},
        {HTVSCROLL, "HTVSCROLL"}, {HTMINBUTTON, "HTMINBUTTON"}, {HTMAXBUTTON, "HTMAXBUTTON"},
        {HTLEFT, "HTLEFT"}, {HTRIGHT, "HTRIGHT"}, {HTTOP, "HTTOP"},
        {HTTOPLEFT, "HTTOPLEFT"}, {HTTOPRIGHT, "HTTOPRIGHT"}, {HTBOTTOM, "HTBOTTOM"},
        {HTBOTTOMLEFT, "HTBOTTOMLEFT"}, {HTBOTTOMRIGHT, "HTBOTTOMRIGHT"}, {HTBORDER, "HTBORDER"},
        {HTOBJECT, "HTOBJECT"}, {HTCLOSE, "HTCLOSE"}, {HTHELP, "HTHELP"}
    };
    if (hitTestMap.count(res)) {
        return hitTestMap[res];
    }
    return "UNKNOWN_HT_RESULT_" + std::to_string(res);
}

// Creates a detailed, decoded JSON object from a MessageInfo struct
nlohmann::json json_from_message(const MessageInfo& msg) {
    nlohmann::json j;
    j["type"] = std::string(1, msg.type);
    j["hwnd"] = reinterpret_cast<uintptr_t>(msg.hwnd);
    j["message_id"] = msg.message;
    j["message_name"] = message_to_string(msg.message);
    j["wParam"] = msg.wParam;
    j["lParam"] = msg.lParam;

    nlohmann::json decoded;
    switch (msg.message) {
        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_NCLBUTTONDOWN:
            decoded["x"] = GET_X_LPARAM(msg.lParam);
            decoded["y"] = GET_Y_LPARAM(msg.lParam);
            break;
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
             decoded["virtual_key"] = msg.wParam;
             decoded["repeat_count"] = msg.lParam & 0xFFFF;
             decoded["scan_code"] = (msg.lParam >> 16) & 0xFF;
             decoded["extended_key"] = (msg.lParam >> 24) & 1;
             decoded["previous_state"] = (msg.lParam >> 30) & 1;
             break;
        case WM_CHAR:
            decoded["char_code"] = msg.wParam;
            break;
        case WM_NCHITTEST:
            decoded["x_screen"] = GET_X_LPARAM(msg.lParam);
            decoded["y_screen"] = GET_Y_LPARAM(msg.lParam);
            if (msg.type == 'R') {
                decoded["result_name"] = hittest_result_to_string(msg.result);
            }
            break;
        case WM_SETCURSOR:
            decoded["hwnd"] = (uintptr_t)msg.wParam;
            decoded["hit_test_code"] = LOWORD(msg.lParam);
            decoded["mouse_message"] = HIWORD(msg.lParam);
            if (msg.type == 'R') {
                 decoded["halted"] = (msg.result == TRUE);
            }
            break;
    }
    j["decoded_params"] = decoded;

    if (msg.type == 'R') {
        j["result"] = msg.result;
    }

    return j;
}
void cleanupAndExit(int signum) {
    std::cout << "\nStopping capture... " << std::endl;
    if (g_hHook1) UnhookWindowsHookEx(g_hHook1);
    if (g_hHook2) UnhookWindowsHookEx(g_hHook2);
    if (hinstDLL) FreeLibrary(hinstDLL);

    if (pSharedData && pSharedData->count > 0) {
        std::cout << "Processing " << pSharedData->count << " captured messages." << std::endl;

        if (g_useJson) {
            // JSON Output
            nlohmann::json messagesJson = nlohmann::json::array();
            for (int i = 0; i < pSharedData->count; ++i) {
                messagesJson.push_back(json_from_message(pSharedData->messages[i]));
            }

            std::ofstream o("messages.json");
            o << std::setw(4) << messagesJson << std::endl;
            o.close();
            std::cout << "Saved JSON data to messages.json" << std::endl;
        } else {
            // Binary Output (Default)
            std::ofstream o("messages.dat", std::ios::binary);
            if (o) {
                o.write(reinterpret_cast<const char*>(pSharedData->messages), pSharedData->count * sizeof(MessageInfo));
                o.close();
                std::cout << "Saved binary data to messages.dat" << std::endl;
            }
        }
    } else {
        std::cout << "No messages were captured." << std::endl;
    }

    if (pSharedData) UnmapViewOfFile(pSharedData);
    if (hMapFile) CloseHandle(hMapFile);

    exit(signum);
}
void printUsage(const char* progName) {
    std::cerr << "Usage: " << progName << " <PID> [options]\n";
    std::cerr << "Options:\n";
    std::cerr << "  --json    Save output in human-readable JSON format (messages.json).\n";
    std::cerr << "            Default is high-performance binary format (messages.dat).\n";
}

int main(int argc, char* argv[]) {
    #ifdef _WIN64
        const char* arch = "64-bit";
        const char* dllNameToLoad = "HookDLL64.dll";
    #else
        const char* arch = "32-bit";
        const char* dllNameToLoad = "HookDLL32.dll";
    #endif

    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    DWORD dwProcessId = 0;
    try {
        dwProcessId = std::stoul(argv[1]);
    } catch (const std::invalid_argument& e) {
        std::cerr << "Error: Invalid PID '" << argv[1] << "'. PID must be a number.\n";
        printUsage(argv[0]);
        return 1;
    } catch (const std::out_of_range& e) {
        std::cerr << "Error: PID '" << argv[1] << "' is out of range.\n";
        return 1;
    }

    if (argc > 2 && std::string(argv[2]) == "--json") {
        g_useJson = true;
    }

    std::cout << "--- MsgMon Controller (" << arch << ") ---" << std::endl;
    std::cout << "This version can only spy on " << arch << " applications." << std::endl;
    std::cout << "Output format: " << (g_useJson ? "JSON" : "Binary (default)") << std::endl;


    EnumData data = { dwProcessId, NULL };

    EnumWindows(EnumWindowsCallback, reinterpret_cast<LPARAM>(&data));
    HWND hTargetWnd = data.window_handle;

    if (!hTargetWnd) {
        std::cerr << "Warning: Could not find a visible window for PID " << dwProcessId << ". Hooking may fail." << std::endl;
    } else {
         std::cout << "Found target window handle: " << hTargetWnd << std::endl;
    }

    DWORD dwThreadId = GetWindowThreadProcessId(hTargetWnd, NULL);
    if (dwThreadId == 0) {
        std::cerr << "Error: Could not get Thread ID for the window. GetLastError: " << GetLastError() << std::endl;
        return 1;
    }

    hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(SharedData), TEXT("MessageSpySharedMemory"));
    pSharedData = (SharedData*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedData));
    pSharedData->count = 0;

    hinstDLL = LoadLibraryA(dllNameToLoad);
    if (!hinstDLL) {
        std::cerr << "Error: Could not load " << dllNameToLoad << "! GetLastError: " << GetLastError() << std::endl;
        cleanupAndExit(1);
    }

    HOOKPROC CallWndProc = (HOOKPROC)GetProcAddress(hinstDLL, "CallWndProc");
    HOOKPROC CallWndProcRet = (HOOKPROC)GetProcAddress(hinstDLL, "CallWndProcRet");

    if (!CallWndProc || !CallWndProcRet) {
        std::cerr << "Error: Could not find hook procedures in DLL! GetLastError: " << GetLastError() << std::endl;
        cleanupAndExit(1);
    }

    g_hHook1 = SetWindowsHookEx(WH_CALLWNDPROC, CallWndProc, hinstDLL, dwThreadId);
    g_hHook2 = SetWindowsHookEx(WH_CALLWNDPROCRET, CallWndProcRet, hinstDLL, dwThreadId);

    if (!g_hHook1 || !g_hHook2) {
        std::cerr << "Error: SetWindowsHookEx failed! GetLastError: " << GetLastError() << std::endl;
        cleanupAndExit(1);
    }

    std::cout << "Hooks installed successfully. Capturing messages..." << std::endl;
    std::cout << "Press Ctrl+C in this window to stop and save the data." << std::endl;

    signal(SIGINT, cleanupAndExit);
    while (true) {
        Sleep(100);
    }

    return 0;
}