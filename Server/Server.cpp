#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
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

vector<ClientInfo> clients; 
vector<string>History;
HANDLE hMutex;
int userCounter = 1; 

// Рассылка сообщений
void BroadcastMessage(const string& message, SOCKET senderSocket = INVALID_SOCKET) {

    WaitForSingleObject(hMutex, INFINITE);

    History.push_back(message);

    for (const ClientInfo& client : clients) {
        if (client.socket != senderSocket) {
            send(client.socket, message.c_str(), (int)message.size(), 0);
        }
    }
    ReleaseMutex(hMutex);
}

void HandleClient(SOCKET clientSocket, int userId) {
    char recvbuf[BUF_SIZE];
    int iResult;
    string userName;

    ZeroMemory(recvbuf, BUF_SIZE);
    iResult = recv(clientSocket, recvbuf, BUF_SIZE, 0);

    if (iResult > 0) {
        userName = string(recvbuf, iResult);



        WaitForSingleObject(hMutex, INFINITE);
            //string header = "-----------------";
            //send(clientSocket, header.c_str(), (int)header.size(), 0);

        for (const string& mess : History)
        {
            send(clientSocket, mess.c_str(), (int)mess.size(), 0);
        }



        clients.push_back({ clientSocket, userId, userName });
        ReleaseMutex(hMutex);
    }
    else {
        closesocket(clientSocket);
        return;
    }

    string joinMsg = "[SERVER]: " + userName + " (ID: " + to_string(userId) + ") joined\n";
    cout << joinMsg;
    BroadcastMessage(joinMsg, clientSocket);

    




    // ---------------------------------
            // ---------------------------------
            // ---------------------------------





    while (true) {
        ZeroMemory(recvbuf, BUF_SIZE);
        iResult = recv(clientSocket, recvbuf, BUF_SIZE, 0);

        if (iResult > 0) {
            string msg(recvbuf, iResult);

            if (msg.find("/exit") == 0) break;

            if (msg.find("/users") == 0) {
                string userList = "\nUSERS\n";
                {
                    WaitForSingleObject(hMutex, INFINITE);
                    for (const ClientInfo& c : clients) {
                        userList += "ID " + to_string(c.id) + ": " + c.name + "\n";
                    }
                    ReleaseMutex(hMutex);
                }
                userList += "----------------------\n";

                // тому кто просил
                send(clientSocket, userList.c_str(), (int)userList.size(), 0);
                continue;
            }

            string broadcastMsg = "[" + userName + "]: " + msg + "\n";
            cout << broadcastMsg;
            BroadcastMessage(broadcastMsg, clientSocket);
        }
        else {
            break;
        }
    }

    string leftMsg = "[SERVER]: " + userName + " left\n";
    cout << leftMsg;
    BroadcastMessage(leftMsg, clientSocket);
    {
        WaitForSingleObject(hMutex, INFINITE);
        for (auto it = clients.begin(); it != clients.end(); ++it) {
            if (it->socket == clientSocket) {
                clients.erase(it);
                break; 
            }
        }
        ReleaseMutex(hMutex);
    }
    closesocket(clientSocket);
}

int main(int argc, char* argv[]) {

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

    hMutex = CreateMutex(NULL, FALSE, NULL);

    cout << "Server started" << endl;

    while (true) {
        SOCKET ClientSocket = accept(ListenSocket, NULL, NULL);
        if (ClientSocket == INVALID_SOCKET) continue;

        thread clientThread(HandleClient, ClientSocket, userCounter);
        clientThread.detach();

        userCounter++; 
    }

    closesocket(ListenSocket);
    WSACleanup();
    return 0;
}