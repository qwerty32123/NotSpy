// HookDLL.cpp

#include <windows.h>
#include <cstdio>
#include "Shared.h"

// Shared memory pointers
#pragma data_seg(".shared")
HHOOK hHookCallWndProc = NULL;
HHOOK hHookCallWndProcRet = NULL;
#pragma data_seg()
#pragma comment(linker, "/section:.shared,RWS")

SharedData* pSharedData = nullptr;
HANDLE hMapFile = NULL;

// Helper function to add a message to our shared buffer
void AddMessageToLog(const MessageInfo& msg) {
    if (pSharedData && pSharedData->count < MAX_MESSAGES) {
        pSharedData->messages[pSharedData->count] = msg;
        pSharedData->count++;
    }
}

extern "C" {
    // This is our "S" hook for SENT messages
    __declspec(dllexport) LRESULT CALLBACK CallWndProc(int nCode, WPARAM wParam, LPARAM lParam) {
        if (nCode >= 0) {
            CWPSTRUCT* cwp = (CWPSTRUCT*)lParam;
            MessageInfo msg = {};
            msg.type = 'S';
            msg.result = 0; // Not applicable for 'S'
            msg.hwnd = cwp->hwnd;
            msg.message = cwp->message;
            msg.wParam = cwp->wParam;
            msg.lParam = cwp->lParam;
            msg.timestamp = time(nullptr);
            AddMessageToLog(msg);
        }
        return CallNextHookEx(hHookCallWndProc, nCode, wParam, lParam);
    }

    // This is our "R" hook for RETURNED messages
    __declspec(dllexport) LRESULT CALLBACK CallWndProcRet(int nCode, WPARAM wParam, LPARAM lParam) {
        if (nCode >= 0) {
            CWPRETSTRUCT* cwpr = (CWPRETSTRUCT*)lParam;
            MessageInfo msg = {};
            msg.type = 'R';
            msg.result = cwpr->lResult; // We have the result now!
            msg.hwnd = cwpr->hwnd;
            msg.message = cwpr->message;
            msg.wParam = cwpr->wParam;
            msg.lParam = cwpr->lParam;
            msg.timestamp = time(nullptr);
            AddMessageToLog(msg);
        }
        return CallNextHookEx(hHookCallWndProcRet, nCode, wParam, lParam);
    }
}

// DllMain needs to open the shared memory, same as before
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, TEXT("MessageSpySharedMemory"));
            if (hMapFile) {
                pSharedData = (SharedData*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedData));
            }
            break;
        case DLL_PROCESS_DETACH:
            if (pSharedData) UnmapViewOfFile(pSharedData);
            if (hMapFile) CloseHandle(hMapFile);
            break;
    }
    return TRUE;
}