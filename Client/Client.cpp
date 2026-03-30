#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <string>
#include <thread>

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_PORT "27015"
#define BUF_SIZE 512

using namespace std;

//  прослушиваниe сообщений 
void ReceiveThread(SOCKET ConnectSocket) {
    char recvbuf[BUF_SIZE];
    int iResult;

    while (true) {
        ZeroMemory(recvbuf, BUF_SIZE);
        iResult = recv(ConnectSocket, recvbuf, BUF_SIZE, 0);
        if (iResult > 0) {
            cout << string(recvbuf, iResult);
        }
        else {
            cout << "\n[Disconnected from server]" << endl;
            break;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cout << "Usage: " << argv[0] << " <server_ip_address>\nExample: " << argv[0] << " 127.0.0.1" << endl;
        return 1;
    }

    WSAData wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        cout << "WSAStartup failed: " << iResult << endl;
        return 1;
    }

    struct addrinfo* result = NULL, hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    iResult = getaddrinfo(argv[1], DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        cout << "getaddrinfo error: " << iResult << endl;
        WSACleanup();
        return 1;
    }

    SOCKET ConnectSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ConnectSocket == INVALID_SOCKET) {
        cout << "Error at socket(): " << WSAGetLastError() << endl;
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    iResult = connect(ConnectSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        cout << "Cannot connect to server." << endl;
        closesocket(ConnectSocket);
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }
    freeaddrinfo(result);

    cout << "Connected to the chat! Type '/exit' to leave." << endl;

    thread recvThread(ReceiveThread, ConnectSocket);
    recvThread.detach();

    // читаем 
    string userInput;

    cout << "Type your name" << endl;

    getline(cin, userInput);

    iResult = send(ConnectSocket, userInput.c_str(), (int)userInput.size(), 0);
    if (iResult == SOCKET_ERROR) {
        cout << "send failed: " << WSAGetLastError() << endl;
    }

    Sleep(500);

    while (true) {
        getline(cin, userInput);
        if (userInput.empty()) continue;

        if (userInput[0] == '/') {

            if (userInput.length() > 1 && userInput[1] == '/') {
                cout << "Only one slash agreed" << endl;
                continue;  
            }

            string command = userInput.substr(1);

            if (command == "exit") {
                break;
            }
            
            if (command == "users")
            {
                send(ConnectSocket, userInput.c_str(), (int)userInput.size(), 0);
                continue;
            }
            else {
                cout << "command '" << command << "' doesn't exist" << endl;
                continue; 
            }
        }

        iResult = send(ConnectSocket, userInput.c_str(), (int)userInput.size(), 0);
        if (iResult == SOCKET_ERROR) {
            cout << "send failed: " << WSAGetLastError() << endl;
            break;
        }
    }

    shutdown(ConnectSocket, SD_SEND);
    closesocket(ConnectSocket);
    WSACleanup();

    return 0;
}