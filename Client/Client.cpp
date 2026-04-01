#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <string>

#pragma comment(lib, "Ws2_32.lib")
#define DEFAULT_PORT "27015"
#define BUF_SIZE 512
using namespace std;

DWORD WINAPI ReceiveThread(LPVOID lpParam) {
    SOCKET ConnectSocket = (SOCKET)lpParam;
    char recvbuf[BUF_SIZE];
    int iResult;

    while (true) {
        ZeroMemory(recvbuf, BUF_SIZE);
        iResult = recv(ConnectSocket, recvbuf, BUF_SIZE, 0);
        if (iResult > 0) cout << string(recvbuf, iResult);
        else {
            cout << "\n[Server connection exit]" << endl;
            break;
        }
    }
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cout << "erreor." << endl;
        return 1;
    }

    WSAData wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    struct addrinfo* result = NULL, hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    getaddrinfo(argv[1], DEFAULT_PORT, &hints, &result);
    SOCKET ConnectSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    connect(ConnectSocket, result->ai_addr, (int)result->ai_addrlen);
    freeaddrinfo(result);

    cout << "Connected Type your name: ";
    string name;
    getline(cin, name);
    send(ConnectSocket, name.c_str(), (int)name.size(), 0);

    HANDLE hThread = CreateThread(NULL, 0, ReceiveThread, (LPVOID)ConnectSocket, 0, NULL);
    if (hThread) CloseHandle(hThread);

    string userInput;

    ULONGLONG lastMessageTime = 0;  
     int SPAM_DELAY = 1000;   

    while (true) {
        getline(cin, userInput);
        if (userInput.empty()) continue;

        ULONGLONG currentTime = GetTickCount64();
        if (currentTime - lastMessageTime < SPAM_DELAY) {
            cout << "[SYSTEM]: Do not spam" << endl;
            continue; 
        }
        lastMessageTime = currentTime; 

        if (userInput[0] == '/') {
            string command = userInput.substr(1);
            if (command == "exit")
                break;
            if (command == "users") {
                send(ConnectSocket, userInput.c_str(), (int)userInput.size(), 0);
                continue;
            }
        }

        send(ConnectSocket, userInput.c_str(), (int)userInput.size(), 0);
    }

    closesocket(ConnectSocket);
    WSACleanup();
    return 0;
}