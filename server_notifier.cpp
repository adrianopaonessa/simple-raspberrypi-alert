#include <iostream>    // Provides standard input/output stream objects
#include <string>      // Provides string class and related functions
#include <WinSock2.h>  // Provides Windows Sockets API for network programming
#include <shellapi.h>  // Provides functions for Windows shell operations

// Link with the Winsock library for network functions
#pragma comment(lib, "ws2_32.lib")

const int PORT = 10709; // Server port, MUST match the one used in the client Python script (Just a random number).
const int BUFFER_SIZE = 1024; // Size of the buffer for receiving data from the socket

// Path to the PowerShell script for showing toast notifications
const std::string TOAST_NOTIFICATION_SCRIPT_PATH = "./toast_notification.ps1";

/**
 * @brief Initializes the Winsock library for network operations.
 *
 * This function calls WSAStartup to initialize the Winsock library with version 2.2.
 * If initialization fails, an error message is printed to std::cerr.
 *
 * @return true if Winsock was successfully initialized, false otherwise.
 */
bool InitializeWinsock() {
    WSADATA wsaData;

    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);

    if (iResult != 0) {
        std::cerr << "> WSAStartup failed! " << iResult << std::endl;
        return false;
    }

    return true;
}

/**
 * @brief Displays a toast notification on Windows using a PowerShell script.
 *
 * This function constructs a command to execute a PowerShell script that shows a toast notification
 * with the specified title and message. The script path is defined by TOAST_NOTIFICATION_SCRIPT_PATH.
 *
 * @param title   The title of the toast notification.
 * @param message The message content of the toast notification.
 */
void ShowToastNotification(const std::string& title, const std::string& message) {
    // Path to the PowerShell script for toast notifications
    std::string powershellScriptPath = TOAST_NOTIFICATION_SCRIPT_PATH;

    // Convert std::string to std::wstring for ShellExecuteW (which expects wide strings)
    std::wstring ws_powershellScriptPath(powershellScriptPath.begin(), powershellScriptPath.end());
    std::wstring ws_title(title.begin(), title.end());
    std::wstring ws_message(message.begin(), message.end());

    // Build the PowerShell command-line arguments to pass to ShellExecuteW.
    // -NoProfile: Prevents loading the user's profile to speed up execution.
    // -ExecutionPolicy Bypass: Allows running scripts regardless of policy.
    // -File: Specifies the script file to run.
    // -Title and -Message: Custom parameters for the toast notification.
    std::wstring args = L"-NoProfile -ExectionPolicy Bypass -File \"" +
                        ws_powershellScriptPath + L"\" -Title \"" + ws_title +
                        L"\" -Message \"" + ws_message + L"\"";
    
    // Execute the PowerShell script to show the toast notification (runs hidden)
    ShellExecuteW(NULL, L"open", L"powershell.exe", args.c_str(), NULL, SW_HIDE);
}

/**
 * @brief Entry point for the server notifier application.
 *
 * This function initializes the Winsock library and sets up a TCP server socket
 * that listens for incoming connections on a specified port. When a client connects,
 * it receives a message and checks if the message is "BTN_PRESSED". If so, it triggers
 * a toast notification to alert the user. The server handles errors gracefully, prints
 * informative messages, and cleans up resources before exiting.
 *
 * @return int Returns 0 on successful execution, or 1 if an error occurs during initialization or socket setup.
 */
int main() {
    // Initialize Winsock library; exit if initialization fails
    if (!InitializeWinsock())
        return 1;
    
    SOCKET server_fd, new_socket; // Server socket and client socket descriptors
    struct sockaddr_in address;   // Structure to hold server address information
    int addrlen = sizeof(address); // Length of the address structure
    char buffer[BUFFER_SIZE] = {0}; // Buffer for receiving data from the client

    
    // Create a TCP socket for the server
    server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (server_fd == INVALID_SOCKET) {
        // Print error if socket creation fails and clean up Winsock
        std::cerr << "> Error creating socket: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }


    // Set socket options to allow address reuse (SO_REUSEADDR)
    int opt = 1;

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) == SOCKET_ERROR) {
        // Print error if setsockopt fails, close socket, and clean up Winsock
        std::cerr << "> setsockopt failed: " << WSAGetLastError() << std::endl;
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    address.sin_family = AF_INET;           // Set address family to IPv4
    address.sin_addr.s_addr = INADDR_ANY;   // Accept connections from any network interface
    address.sin_port = htons(PORT);         // Set port number (convert to network byte order)

    // Bind the socket to the specified address and port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == SOCKET_ERROR) {
        // Print error if bind fails, close socket, and clean up Winsock
        std::cerr << "> Bind failed: " << WSAGetLastError() << ". There might be another process listening on the port: " << PORT << std::endl;
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    // Start listening for incoming connections (max 3 in the queue)
    if (listen(server_fd, 3) == SOCKET_ERROR) {
        std::cerr << "> Listen failed: " << WSAGetLastError() << std::endl;
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    std::cout << "> Server listening on port: " << PORT << std::endl;

    while (true) {
        // Accept an incoming client connection
        new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        if (new_socket == INVALID_SOCKET) {
            // Print error if accept fails and continue to wait for other connections
            std::cerr << "> Accept failed: " << WSAGetLastError() << std::endl;
            continue; // Wait for other connections
        }

        std::cout << "> Connection accepted from " << inet_ntoa(address.sin_addr) << ":" << ntohs(address.sin_port) << std::endl;

        // Reads data from the socket 'new_socket' into 'buffer', up to (BUFFER_SIZE - 1) bytes.
        // The '-1' ensures there is space left in the buffer for a null terminator.
        int valread = recv(new_socket, buffer, BUFFER_SIZE - 1, 0);
        if (valread > 0) {
            buffer[valread] = '\0'; // Add the null terminator.

            std::string received_message(buffer);
            std::cout << "> Message received: '" << received_message << "'\n";

            if (received_message == "BTN_PRESSED") {
                ShowToastNotification("Raspberry Pi Alert.", "The button has been pressed!");
            } else {
                std::cout << "> Unknown message: '" << received_message << "'\n";
            }
        } else if (valread == 0) {
            std::cout << "> Client disconnected.\n";
        } else {
            std::cout << "> Receive error: " << WSAGetLastError() << std::endl;
        }

        closesocket(new_socket); // Close the client socket
    }

    closesocket(server_fd);
    WSACleanup();
    return 0;
}