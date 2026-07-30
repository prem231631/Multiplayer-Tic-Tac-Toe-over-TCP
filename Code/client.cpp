#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>

#pragma comment(lib,"ws2_32.lib")

int main()
{
    WSADATA wsaData;

    if(WSAStartup(MAKEWORD(2,2), &wsaData)!=0)
    {
        std::cout<<"WSAStartup Failed"<<std::endl;
        return 1;
    }

    std::cout<<"Winsock Initialized"<<std::endl;

    //Socket Creation
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if(clientSocket == INVALID_SOCKET)
    {
        std::cout<<"Socket creation failed"<<std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr;

    serverAddr.sin_family = AF_INET;

    serverAddr.sin_port = htons(54000);

    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    //Connection Request
    int result = connect(
                    clientSocket,
                    (sockaddr*)&serverAddr,
                    sizeof(serverAddr)
                 );

    if(result == SOCKET_ERROR)
    {
        std::cout<<"Connection Failed"<<std::endl;

        closesocket(clientSocket);
        WSACleanup();

        return 1;
    }

    std::cout<<"Connected to Server"<<std::endl;

    //Succeed Message
    char buffer[100];

    int bytesReceived = recv(
        clientSocket,
        buffer,
        sizeof(buffer),
        0
    );

    buffer[bytesReceived] = '\0';

    std::cout << buffer << std::endl;

    closesocket(clientSocket);

    WSACleanup();

    return 0;
}