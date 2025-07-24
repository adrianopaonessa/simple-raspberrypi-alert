#define UNICODE     // Enable Unicode support for Windows API functions
#define _UNICODE    // Enable Unicode support for C runtime functions

#include <WinSock2.h>      // Provides Windows Sockets API for TCP/IP networking
#include <windows.h>       // Core Windows API functions and types
#include <shellapi.h>      // Shell API for ShellExecute and tray icon management
#include <WS2tcpip.h>      // Additional TCP/IP utilities for Windows Sockets
#include <string>          // Standard C++ string class
#include <thread>          // C++11 threading support
#include <iostream>        // Standard input/output stream objects

// Link with the Winsock library for network functions
#pragma comment(lib, "ws2_32.lib")


#define WM_APP_TRAYICON (WM_APP + 1)    // Custom Windows message identifier for tray icon events
#define ID_TRAY_EXIT 1001               // Menu command ID for "Exit" option in tray menu
#define ID_TRAY_SHOWHIDE 1002           // Menu command ID for "Show/Hide" option in tray menu

// Unique identifier for the tray icon
const UINT TRAY_UID = 1;

// TCP server port number, MUST be the same as the one used in the Python script
const int PORT = 10709;

const std::string TOAST_SCRIPT = "toast_notification.ps1";  // Path to the PowerShell script for toast notifications
const std::string TRIGGER_MSG = "DEFAULT MESSAGE";          // Message that triggers the notification

NOTIFYICONDATA nid = {}; // Structure for tray icon data (initializes to zero)
HMENU trayMenu;          // Handle for the tray icon's context menu
HWND hwnd;               // Handle for the hidden window used for tray icon events

bool showConsole = false; // Tracks whether the console window is currently shown
bool running = true;      // Controls the main server loop; set to false to stop the server



// -------------------- Utility functions --------------------

void ShowToast(const std::string &title, const std::string &msg) {
    // Build the PowerShell command line to execute the toast notification script with parameters
    std::wstring command = L"-NoProfile -ExecutionPolicy Bypass -File \"" +
                           std::wstring(TOAST_SCRIPT.begin(), TOAST_SCRIPT.end()) + L"\" " + // Add script path
                           L"-Title \"" + std::wstring(title.begin(), title.end()) + L"\" " + // Add title argument
                           L"-Message \"" + std::wstring(msg.begin(), msg.end()) + L"\"";    // Add message argument

    // Launch PowerShell in hidden mode to run the toast notification script
    ShellExecuteW(NULL, L"open", L"powershell.exe", command.c_str(), NULL, SW_HIDE);
}

void ShowConsole() {
    // If the console is not currently shown
    if (!showConsole) {
        AllocConsole(); // Allocate a new console for the calling process
        FILE* dummy;
        freopen_s(&dummy, "CONOUT$", "w", stdout); // Redirect stdout to the new console
        showConsole = true; // Mark that the console is now shown
    }

    ShowWindow(GetConsoleWindow(), SW_SHOW); // Make the console window visible
    SetForegroundWindow(GetConsoleWindow()); // Bring the console window to the foreground
}

void HideConsole() {
    // Hide the console window (if it exists)
    ShowWindow(GetConsoleWindow(), SW_HIDE);
}

void CreateTrayIcon(HINSTANCE hInstance) {
    nid.cbSize = sizeof(nid); // Set the size of the NOTIFYICONDATA structure
    nid.hWnd = hwnd; // Assign the window handle to receive tray icon messages
    nid.uID = TRAY_UID; // Set a unique identifier for the tray icon
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_INFO; // Specify which fields are valid (icon, message, tooltip, info)
    nid.uCallbackMessage = WM_APP_TRAYICON; // Set the custom message for tray icon events
    nid.hIcon = LoadIcon(NULL, IDI_INFORMATION); // Load a standard information icon for the tray
    wcscpy_s(nid.szTip, ARRAYSIZE(nid.szTip), L"Raspberry Pi Server Notifier"); // Set the tooltip text for the tray icon
    Shell_NotifyIcon(NIM_ADD, &nid); // Add the tray icon to the system tray

    // Menu
    trayMenu = CreatePopupMenu(); // Create a popup menu for the tray icon
    AppendMenuW(trayMenu, MF_STRING, ID_TRAY_SHOWHIDE, L"Show/Hide console window"); // Add "Show/Hide" option to the tray menu
    AppendMenuW(trayMenu, MF_STRING, ID_TRAY_EXIT, L"Stop server notifier"); // Add an "Exit" option to the tray menu
}

void RemoveTrayIcon() {
    // Remove the tray icon from the system tray
    Shell_NotifyIcon(NIM_DELETE, &nid);

    // Destroy the tray icon's context menu to free resources
    DestroyMenu(trayMenu);
}


// -------------------- TCP Server --------------------

void TcpServerThread() {
    std::cout << "- TCP Server thread started." << std::endl;

    // Structure to hold information about the Windows Sockets implementation
    WSADATA wsaData;

    // Socket handles for the server and client, initialized to invalid value
    SOCKET serverSocket = INVALID_SOCKET, clientSocket = INVALID_SOCKET;

    // Initialize Winsock version 2.2 for network communication
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    // Create a new socket for the server using IPv4 (AF_INET), TCP (SOCK_STREAM), and TCP protocol (IPPROTO_TCP)
    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    // Create and initialize the sockaddr_in structure for the server address
    sockaddr_in addr{};                  // Zero-initialize the address structure
    addr.sin_family = AF_INET;           // Set address family to IPv4
    addr.sin_port = htons(PORT);         // Set the port number (convert to network byte order)
    addr.sin_addr.s_addr = INADDR_ANY;   // Accept connections from any network interface

    // Bind the server socket to the specified address and port
    bind(serverSocket, (sockaddr*)&addr, sizeof(addr));

    // Start listening for incoming connections (backlog set to 1)
    listen(serverSocket, 1);

    std::cout << "- Server listening on port: " << PORT << std::endl;

    while (running) {
        sockaddr_in clientAddr; // Structure to hold the client's address information
        int len = sizeof(clientAddr); // Variable to store the size of the client address structure
        clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &len); // Accept an incoming connection and fill clientAddr with the client's info

        // Check if the accepted client socket is invalid (error occurred)
        if (clientSocket == INVALID_SOCKET) {
            // Print an error message to the console
            std::cout << "> Error accepting connection." << std::endl;

            // Continue to the next iteration to accept another connection
            continue;
        }

        std::cout << "> Connection from " << inet_ntoa(clientAddr.sin_addr) << std::endl;

        char buffer[1024];   // Buffer to store incoming data from the client
        int bytesRead;       // Variable to hold the number of bytes read from the client socket

        // Continuously receive data from the connected client while running
        while ((bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0)) > 0 && running) {
            buffer[bytesRead] = '\0'; // Null-terminate the received data to make it a valid C-string
            std::string msg(buffer);   // Convert the buffer to a C++ string

            // Check if the received message matches the trigger message
            if (msg == TRIGGER_MSG) {
                std::cout << "> Message received: " << msg << std::endl; // Log the received message to the console
                ShowToast("Raspberry Pi", "Button pressed!");            // Show a toast notification
            }
        }

        std::cout << "> Connection closed!" << std::endl;
        closesocket(clientSocket); // Close the client socket after the connection ends
        }

        closesocket(serverSocket); // Close the server socket when the server loop ends
        WSACleanup();              // Clean up Winsock resources before exiting
}


// -------------------- Invisible window + tray --------------------

// Window procedure to handle messages for the hidden window
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_APP_TRAYICON: // Custom message for tray icon events
            switch (lParam) {
                case WM_LBUTTONUP: // Left mouse button released on tray icon
                    ShowConsole(); // Show the console window
                    break;
                case WM_RBUTTONUP: // Right mouse button released on tray icon
                    POINT pt; // Structure to hold cursor position
                    GetCursorPos(&pt); // Get current cursor position
                    SetForegroundWindow(hwnd); // Bring the hidden window to foreground (required for menu)

                    // Show the tray icon's context menu at the cursor position
                    TrackPopupMenu(trayMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);
                    break;
            }
            break;
        
        case WM_COMMAND: // Menu command received
            if (LOWORD(wParam) == ID_TRAY_EXIT) { // If "Exit" menu item selected
                // Signal to quit the application (posts WM_QUIT to the message loop)
                PostQuitMessage(0); 
            } else if (LOWORD(wParam) == ID_TRAY_SHOWHIDE) {
                // Toggle the visibility of the console window
                if (!showConsole) {
                    HideConsole(); // Hide the console if currently shown
                    showConsole = true;
                } else {
                    ShowConsole(); // Show the console if currently hidden
                    showConsole = false;
                }
            }
            break;
        
        case WM_CLOSE: // Window close event
            HideConsole(); // Hide the console window (does not exit the app)
            return 0; // Prevent default close behavior
    }

    // Default message handling for unprocessed messages
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void RunTrayApp(HINSTANCE hInstance) {
    ShowConsole(); // Show the console window (for debugging/logging)
    HideConsole(); // Immediately hide the console window (start hidden)

    std::cout << "- Program starting up..." << std::endl; // Log startup message

    const wchar_t CLASS_NAME[] = L"HiddenWindowClass"; // Name for the hidden window class

    WNDCLASS wc = {}; // Structure to define window class properties
    wc.lpfnWndProc = WindowProc; // Set the window procedure callback
    wc.hInstance = hInstance;    // Set the application instance handle
    wc.lpszClassName = CLASS_NAME; // Set the window class name

    RegisterClass(&wc); // Register the window class with the OS

    // Create the hidden window (used for tray icon events)
    hwnd = CreateWindowExW(0, CLASS_NAME, L"", 0, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);

    CreateTrayIcon(hInstance); // Add the tray icon to the system tray

    std::thread serverThread(TcpServerThread); // Start the TCP server in a separate thread
    serverThread.detach(); // Detach the thread so it runs independently

    MSG msg; // Message structure for the Windows message loop

    // Main message loop: process messages for the hidden window
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg); // Translate messages
        DispatchMessage(&msg);  // Dispatch messages to the window procedure
    }

    running = false; // Signal the server thread to stop
    RemoveTrayIcon(); // Remove the tray icon from the system tray
}


// -------------------- Entry point --------------------

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    // Entry point for Windows applications (WinMain)
    RunTrayApp(hInstance); // Start the tray application and TCP server
    return 0; // Return exit code 0 (success)
}