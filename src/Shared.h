// Shared.h

#ifndef NOTSPY___SHARED_H
#define NOTSPY___SHARED_H

#include <windows.h>
#include <ctime>

#define MAX_MESSAGES 10000

struct MessageInfo {
    char type;
    LRESULT result;
    HWND hwnd;
    UINT message;
    WPARAM wParam;
    LPARAM lParam;
    time_t timestamp;
};

struct SharedData {
    int count;
    MessageInfo messages[MAX_MESSAGES];
};

#endif //NOTSPY___SHARED_H