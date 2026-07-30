#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>

#pragma comment(lib,"ws2_32.lib")

#define PORT 54000
#define BOARD_SIZE 9

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

//Printing the Board
void printBoard(const std::string& board)
{
    auto cell = [&](int i)
    {
        return board[i] == '_'
            ? std::to_string(i)
            : std::string(1, board[i]);
    };

    std::cout << "\n";

    std::cout << " "
              << cell(0)
              << " | "
              << cell(1)
              << " | "
              << cell(2)
              << "\n";

    std::cout << "---+---+---\n";

    std::cout << " "
              << cell(3)
              << " | "
              << cell(4)
              << " | "
              << cell(5)
              << "\n";

    std::cout << "---+---+---\n";

    std::cout << " "
              << cell(6)
              << " | "
              << cell(7)
              << " | "
              << cell(8)
              << "\n\n";
}

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

    //
    char mySymbol = 0;
    std::string line;
    std::string currentBoard(9, '_');
    bool playing = true;

    while (playing)
    {
        if (!recvLine(clientSocket, line))
        {
            std::cout << "Disconnected from server.\n";
            break;
        }

        if (line.rfind("SYMBOL:", 0) == 0)
        {
            mySymbol = line[7];
            std::cout << "You are Player "
                    << mySymbol
                    << std::endl;
        }

        else if (line.rfind("BOARD:", 0) == 0)
        {
            currentBoard = line.substr(6);
            printBoard(currentBoard);
        }

        //Handle YOURTURN
        else if (line == "YOURTURN")
        {
            int pos = -1;
            while (true)
            {
                std::cout << "Your turn (" << mySymbol << ").\n";
                std::cout << "Enter cell number (0-8): ";

                std::cin >> pos;

                if (std::cin.fail())
                {
                    std::cin.clear();
                    std::cin.ignore(10000, '\n');

                    std::cout << "Please enter a valid number.\n";
                    continue;
                }
                break;
            }
            sendLine(clientSocket, "MOVE:" + std::to_string(pos));
        }

        //Handle WAIT
        else if (line == "WAIT")
        {
            std::cout << "Waiting for opponent...\n";
        }

        //Handle INVALID
        else if (line == "INVALID")
        {
            std::cout << "Invalid move.\n";
            std::cout << "Please choose another cell.\n";
        }

        //Handle RESULT:WIN
        else if (line == "RESULT:WIN")
        {
            std::cout << "\n************************\n";
            std::cout << "      YOU WIN!\n";
            std::cout << "************************\n";
        }

        //Handle RESULT:LOSE
        else if (line == "RESULT:LOSE")
        {
            std::cout << "\n************************\n";
            std::cout << "      YOU LOSE!\n";
            std::cout << "************************\n";
        }

        //Handle RESULT:DRAW
        else if (line == "RESULT:DRAW")
        {
            std::cout << "\n************************\n";
            std::cout << "     MATCH DRAW!\n";
            std::cout << "************************\n";
        }

        //Hangle Restart
        else if (line == "RESTART?")
        {
            char choice;

            std::cout << "\nPlay Again? (Y/N): ";
            std::cin >> choice;

            choice = toupper(choice);

            std::string response(1, choice);

            sendLine(clientSocket, response);

            if(choice != 'Y')
            {
                playing = false;
            }
        }

        //Handle EXIT
        else if (line == "EXIT")
        {
            std::cout << "\nGame Finished.\n";
            std::cout << "Thank you for playing.\n";

            playing = false;
        }
    }


    WSACleanup();
    return 0;
}