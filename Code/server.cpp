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

//Convert board into string
std::string boardToString(const char board[])
{
    return std::string(board, BOARD_SIZE);
}

//Reset Board
void resetBoard(char board[])
{
    for(int i=0; i<BOARD_SIZE; i++)
    {
        board[i]= '_';
    }
}

//checkWinner
char checkWinner(const char board[])
{
    static const int lines[8][3] =
    {
        {0,1,2},
        {3,4,5},
        {6,7,8},

        {0,3,6},
        {1,4,7},
        {2,5,8},

        {0,4,8},
        {2,4,6}
    };

    for(int i = 0; i < 8; i++)
    {
        int a = lines[i][0];
        int b = lines[i][1];
        int c = lines[i][2];

        if(board[a] != '_' &&
           board[a] == board[b] &&
           board[b] == board[c])
        {
            return board[a];
        }
    }

    return 0;
}

//Detect a Draw
bool isBoardFull(const char board[])
{
    for(int i = 0; i < BOARD_SIZE; i++)
    {
        if(board[i] == '_')
        {
            return false;
        }
    }
    return true;
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

    //Accepting two players
    std::cout << "Waiting for Player X..." << std::endl;

    SOCKET clientX = accept(serverSocket, NULL, NULL);

    if (clientX == INVALID_SOCKET)
    {
        std::cout << "Accept failed." << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Player X connected." << std::endl;

    std::cout << "Waiting for Player O..." << std::endl;

    SOCKET clientO = accept(serverSocket, NULL, NULL);

    if (clientO == INVALID_SOCKET)
    {
        std::cout << "Accept failed." << std::endl;
        closesocket(clientX);
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Player O connected." << std::endl;

    sendLine(clientX, "SYMBOL: X");
    sendLine(clientO, "SYMBOL: O");
    
    //Game Loop
    char board[BOARD_SIZE];
    bool keepPlaying = true;
    while (keepPlaying)
    {
        resetBoard(board);

        SOCKET current = clientX;
        SOCKET other = clientO;

        char currentSymbol = 'X';

        sendLine(clientX, "BOARD:" + boardToString(board));
        sendLine(clientO, "BOARD:" + boardToString(board));

        bool gameOver = false;

        while (!gameOver)
        {
            // Tell players whose turn it is
            sendLine(current, "YOURTURN");
            sendLine(other, "WAIT");

            // Receive move
            std::string line;

            if (!recvLine(current, line))
            {
                keepPlaying = false;
                gameOver = true;
                break;
            }

            // Parse MOVE:x
            int pos = -1;

            if (line.rfind("MOVE:", 0) == 0)
            {
                try
                {
                    pos = std::stoi(line.substr(5));
                }
                catch (...)
                {
                    pos = -1;
                }
            }

            // Validate move
            if (pos < 0 || pos >= BOARD_SIZE || board[pos] != '_')
            {
                sendLine(current, "INVALID");
                continue;
            }

            // Update board
            board[pos] = currentSymbol;

            // Send updated board
            std::string boardMsg = "BOARD:" + boardToString(board);

            sendLine(clientX, boardMsg);
            sendLine(clientO, boardMsg);

            // Check winner
            char winner = checkWinner(board);

            if (winner)
            {
                sendLine(current, "RESULT:WIN");
                sendLine(other, "RESULT:LOSE");

                gameOver = true;
            }
            else if (isBoardFull(board))
            {
                sendLine(clientX, "RESULT:DRAW");
                sendLine(clientO, "RESULT:DRAW");

                gameOver = true;
            }
            else
            {
                // Switch turns
                std::swap(current, other);

                currentSymbol = (currentSymbol == 'X') ? 'O' : 'X';
            }
        }

        // Ask both players if they want to play again
        sendLine(clientX, "RESTART?");
        sendLine(clientO, "RESTART?");

        std::string respX;
        std::string respO;

        bool okX = recvLine(clientX, respX);
        bool okO = recvLine(clientO, respO);

        if (!okX || !okO || respX != "Y" || respO != "Y")
        {
            sendLine(clientX, "EXIT");
            sendLine(clientO, "EXIT");

            keepPlaying = false;
        }
    }



    WSACleanup();
    closesocket(clientX);
    closesocket(clientO);
    closesocket(serverSocket);

    return 0;
}