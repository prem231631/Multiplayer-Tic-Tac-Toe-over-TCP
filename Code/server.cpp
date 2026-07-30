#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>

#define BOARD_SIZE 9

#pragma comment(lib,"ws2_32.lib")

//sendLine()
bool sendLine(SOCKET s, const std::string& msg)
{
    std::string out = msg + "\n";
    int total = 0;
    int len = (int)out.size();

    while(total < len)
    {
        int sent = send(
            s,
            out.c_str() + total,
            len - total,
            0
        );

        if(sent == SOCKET_ERROR)
            return false;
        total += sent;
    }
    return true;
};

//recvLine()
bool recvLine(SOCKET s, std::string& outLine)
{
    outLine.clear();
    char c;

    while(true)
    {
        int r = recv(s, &c, 1, 0);
        if(r <= 0)
            return false;

        if(c == '\n')
            break;

        if(c != '\r')
            outLine += c;
    }
    return true;
};

//
char board[9];
void resetBoard(char board[])
{
    for(int i=0; i<BOARD_SIZE; i++)
    {
        board[i]= '_';
    }
}
std::string boardToString(const char board[])
{
    return std::string(board, BOARD_SIZE);
}

int main()
{
    WSADATA wsaData;

    int result = WSAStartup(MAKEWORD(2,2), &wsaData);

    if(result != 0)
    {
        std::cout<<"WSAStartup Failed"<<std::endl;
        return 1;
    }

    std::cout<<"Winsock Initialized Successfully"<<std::endl;

    //Socket Creation
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (serverSocket == INVALID_SOCKET)
    {
        std::cout << "Socket creation failed." << std::endl;
        WSACleanup();
        return 1;
    }

    std::cout << "Socket created successfully." << std::endl;

    //Bind
    sockaddr_in serverAddr;

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(54000);

    int bindResult = bind(
        serverSocket,
        (sockaddr*)&serverAddr,
        sizeof(serverAddr)
    );

    if (bindResult == SOCKET_ERROR)
    {
        std::cout << "Bind failed." << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Bind successful." << std::endl;

    //Listening
    int listenResult = listen(serverSocket, 2);

    if (listenResult == SOCKET_ERROR)
    {
        std::cout << "Listen failed." << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Server is listening..." << std::endl;

    //Accepting a client
    std::cout << "Waiting for client..." << std::endl;

    SOCKET clientSocket = accept(serverSocket, NULL, NULL);

    if (clientSocket == INVALID_SOCKET)
    {
        std::cout << "Accept failed." << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Client connected!" << std::endl;

    //Succeed Message
    const char* message = "Hello Client";

    send(
        clientSocket,
        message,
        strlen(message),
        0
    );



    WSACleanup();

    return 0;
}