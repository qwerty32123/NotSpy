# NotSpy++

NotSpy++ is a lightweight, command-line Windows message spying tool for 32-bit applications. It is designed to capture the flow of `SendMessage` and `PostMessage` calls for a target process and save them in a detailed, structured JSON format for later analysis.

This tool is useful for understanding how a Windows application works, for debugging, and for gathering data to automate user input.

## Features

-   Hooks both `WH_CALLWNDPROC` and `WH_CALLWNDPROCRET` to capture messages *before* and *after* they are processed.
-   Logs detailed information, including message type ('Sent' vs. 'Returned'), window handle, message name, and raw parameters (`wParam`, `lParam`).
-   Decodes common message parameters (e.g., mouse coordinates, virtual key codes) into a human-readable format.
-   Outputs all captured data to a clean `messages.json` file.
-   Targets a specific process by its Process ID (PID).

## Prerequisites

-   Windows Operating System
-   **Visual Studio 2022** with the **"Desktop development with C++"** workload installed. This provides the necessary MSVC compiler (`cl.exe`) and Windows SDK.

## How to Build

1.  Open the **"x86 Native Tools Command Prompt for VS 2022"** and navigate to the project directory.
    ```cmd
    cl /LD src\HookDLL.cpp src\HookDLL.def user32.lib /Fe:HookDLL32.dll
    cl /EHsc /Iinclude src\MessageSpy.cpp /Fe:NotSpy32.exe user32.lib gdi32.lib
    ```

2.  Open the **"x64 Native Tools Command Prompt for VS 2022"** and navigate to the project directory.
    ```cmd
    cl /LD src\HookDLL.cpp src\HookDLL.def user32.lib /Fe:HookDLL64.dll
    cl /EHsc /Iinclude src\MessageSpy.cpp /Fe:NotSpy64.exe user32.lib gdi32.lib
    ```

This will create `NotSpy32.exe` / `HookDLL32.dll` for 32-bit targets and `NotSpy64.exe` / `HookDLL64.dll` for 64-bit targets.

## How to Use

The tool is run from the command line, specifying the Process ID (PID) of the application you wish to monitor.

### Command Syntax

```
NotSpyXX.exe <PID> [options]
```

-   **`NotSpyXX.exe`**: Use `NotSpy32.exe` for 32-bit targets and `NotSpy64.exe` for 64-bit targets.
-   **`<PID>`**: (Required) The Process ID of the target application.
-   **`[options]`**: (Optional) Additional flags to control behavior.

### Options

-   `--json`: Saves the captured messages in a detailed, human-readable `messages.json` file. If this flag is omitted, the output defaults to the high-performance `messages.dat` binary format.

### Examples

1.  **Spying on a 64-bit application (e.g., PID 12345) and saving to the default binary format:**
    ```cmd
    NotSpy64.exe 12345
    ```
    *   After pressing `Ctrl+C`, the data will be saved to `messages.dat`.

2.  **Spying on a 32-bit application (e.g., PID 9876) and saving to JSON format:**
    ```cmd
    NotSpy32.exe 9876 --json
    ```
    *   After pressing `Ctrl+C`, the data will be saved to `messages.json`.

---

After running the tool, interact with the target application to generate messages. Press `Ctrl+C` in the NotSpy terminal to stop the capture and save the log file.
## How to Use

1.  Find the Process ID (PID) of the 32-bit application you want to spy on (e.g., using Task Manager's "Details" tab).
2.  Run the spy tool from the command prompt:
    ```cmd
    MessageSpy.exe
    ```
3.  When prompted, enter the PID and press Enter.
4.  The hooks will be installed. Now, interact with the target application (move the mouse over it, click, type, etc.).
5.  When you are finished capturing, press **`Ctrl+C`** in the `MessageSpy.exe` terminal.
6.  The program will unhook, process the captured data, and save it to `messages.json` in the same directory.

## Project Structure

```
NotSpy++/
├── .gitignore         # Tells Git which files to ignore
├── README.md          # This file
├── include/           # Third-party libraries
│   └── json.hpp
└── src/               # All original source code
    ├── HookDLL.cpp
    ├── HookDLL.def
    ├── MessageSpy.cpp
    └── Shared.h
```