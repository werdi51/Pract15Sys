#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <vector>
#include <string>

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_PORT "27015"
#define BUF_SIZE 512

using namespace std;

struct ClientInfo {
    SOCKET socket;
    int id;
    string name;
};

struct ThreadArgs {
    SOCKET s;
    int id;
};

vector<ClientInfo> clients;
vector<string> History;
HANDLE hMutex;
int userCounter = 1;

void BroadcastMessage(const string& message, SOCKET senderSocket = INVALID_SOCKET) {
    WaitForSingleObject(hMutex, INFINITE);
    History.push_back(message); 
    cout << message << endl;
    for (const auto& client : clients) {
        if (client.socket != senderSocket) {
            send(client.socket, message.c_str(), (int)message.size(), 0);
        }
    }
    ReleaseMutex(hMutex);
}

DWORD WINAPI HandleClient(LPVOID lpParam) {
    ThreadArgs* args = (ThreadArgs*)lpParam;
    SOCKET clientSocket = args->s;
    int userId = args->id;
    delete args; 

    char recvbuf[BUF_SIZE];
    int iResult;
    string userName;

    ZeroMemory(recvbuf, BUF_SIZE);
    iResult = recv(clientSocket, recvbuf, BUF_SIZE, 0);
    if (iResult > 0) {
        userName = string(recvbuf, iResult);
        userName.erase(userName.find_last_not_of(" \n\r\t") + 1);

        WaitForSingleObject(hMutex, INFINITE);


        for (const string& m : History) send(clientSocket, m.c_str(), (int)m.size(), 0);


        clients.push_back({ clientSocket, userId, userName });
        ReleaseMutex(hMutex);
    }
    else {
        closesocket(clientSocket);
        return 0;
    }

    BroadcastMessage("[SERVER]: " + userName + " (ID: " + to_string(userId) + ") joined\n");

    while (true) {
        ZeroMemory(recvbuf, BUF_SIZE);
        iResult = recv(clientSocket, recvbuf, BUF_SIZE, 0);

        if (iResult > 0) {
            string msg(recvbuf, iResult);

            if (msg.find("/exit") == 0) {
                cout << "[LOG_USERS]: User " << userName << " (" << userId << ") used command /exit" << endl;
                break;
            }

            if (msg.find("/users") == 0) {
                cout << "[LOG_USERS]: User " << userName << " (" << userId << ") used command /users" << endl;

                string userList = "\nUsers\n";
                WaitForSingleObject(hMutex, INFINITE);
                for (const auto& c : clients) {
                    userList += "ID " + to_string(c.id) + ": " + c.name + "\n";
                }
                ReleaseMutex(hMutex);

                send(clientSocket, userList.c_str(), (int)userList.size(), 0);
                continue;
            }

            string formattedMsg = "[" + userName + "]: " + msg + "\n";
            BroadcastMessage(formattedMsg, clientSocket);
        }
        else {
            break;
        }
    }

    // Уход
    BroadcastMessage("[SERVER]: " + userName + " left\n");
    WaitForSingleObject(hMutex, INFINITE);
    for (auto it = clients.begin(); it != clients.end(); ++it) {
        if (it->socket == clientSocket) {
            clients.erase(it);
            break;
        }
    }
    ReleaseMutex(hMutex);
    closesocket(clientSocket);
    return 0;
}

int main() {
    hMutex = CreateMutex(NULL, FALSE, NULL);
    WSAData wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    struct addrinfo* result = NULL, hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    SOCKET ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    listen(ListenSocket, SOMAXCONN);

    cout << "Server started" << endl;

    while (true) {
        SOCKET ClientSocket = accept(ListenSocket, NULL, NULL);
        if (ClientSocket != INVALID_SOCKET) {
            ThreadArgs* args = new ThreadArgs{ ClientSocket, userCounter++ };
            HANDLE hThread = CreateThread(NULL, 0, HandleClient, args, 0, NULL);
            if (hThread) CloseHandle(hThread);
        }
    }

    CloseHandle(hMutex);
    WSACleanup();
    return 0;
}