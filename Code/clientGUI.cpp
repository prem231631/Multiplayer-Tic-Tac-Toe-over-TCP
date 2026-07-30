#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>

#pragma comment(lib, "ws2_32.lib")

#define PORT 54000
#define BOARD_SIZE 9

// Custom Window Messages
#define WM_APP_SYMBOL     (WM_APP + 1)
#define WM_APP_BOARD      (WM_APP + 2)
#define WM_APP_YOURTURN   (WM_APP + 3)
#define WM_APP_WAIT       (WM_APP + 4)
#define WM_APP_INVALID    (WM_APP + 5)
#define WM_APP_RESULT     (WM_APP + 6)
#define WM_APP_RESTART    (WM_APP + 7)
#define WM_APP_EXIT       (WM_APP + 8)
#define WM_APP_DISCONNECT (WM_APP + 9)

// Control IDs
#define ID_CELL_BASE 100
#define ID_CONNECT   200
#define ID_IP_EDIT   201

// Global Controls
HWND g_hCells[BOARD_SIZE];
HWND g_hStatus;
HWND g_hIpEdit;
HWND g_hConnectBtn;
HWND g_hMainWnd;

// Network Variables
SOCKET g_sock = INVALID_SOCKET;
char g_mySymbol = 0;
std::string g_board(BOARD_SIZE, '_');
bool g_myTurn = false;

//------------------------------------------------------------
// sendLine()
//------------------------------------------------------------
bool sendLine(SOCKET s, const std::string& msg)
{
    std::string out = msg + "\n";

    int total = 0;
    int len = (int)out.size();

    while (total < len)
    {
        int sent = send(
            s,
            out.c_str() + total,
            len - total,
            0
        );

        if (sent == SOCKET_ERROR)
            return false;

        total += sent;
    }

    return true;
}

//------------------------------------------------------------
// recvLine()
//------------------------------------------------------------
bool recvLine(SOCKET s, std::string& outLine)
{
    outLine.clear();

    char c;

    while (true)
    {
        int r = recv(s, &c, 1, 0);

        if (r <= 0)
            return false;

        if (c == '\n')
            break;

        if (c != '\r')
            outLine += c;
    }

    return true;
}

//------------------------------------------------------------
// Background Network Thread
//------------------------------------------------------------
DWORD WINAPI networkThreadProc(LPVOID param)
{
    HWND hwnd = (HWND)param;

    std::string line;

    while (true)
    {
        if (!recvLine(g_sock, line))
        {
            PostMessage(hwnd, WM_APP_DISCONNECT, 0, 0);
            return 0;
        }

        if (line.rfind("SYMBOL:", 0) == 0)
        {
            std::string* data =
                new std::string(line.substr(7));

            PostMessage(
                hwnd,
                WM_APP_SYMBOL,
                0,
                (LPARAM)data
            );
        }

        else if (line.rfind("BOARD:", 0) == 0)
        {
            std::string* data =
                new std::string(line.substr(6));

            PostMessage(
                hwnd,
                WM_APP_BOARD,
                0,
                (LPARAM)data
            );
        }

        else if (line == "YOURTURN")
        {
            PostMessage(
                hwnd,
                WM_APP_YOURTURN,
                0,
                0
            );
        }

        else if (line == "WAIT")
        {
            PostMessage(
                hwnd,
                WM_APP_WAIT,
                0,
                0
            );
        }

        else if (line == "INVALID")
        {
            PostMessage(
                hwnd,
                WM_APP_INVALID,
                0,
                0
            );
        }

        else if (line.rfind("RESULT:", 0) == 0)
        {
            std::string* data =
                new std::string(line.substr(7));

            PostMessage(
                hwnd,
                WM_APP_RESULT,
                0,
                (LPARAM)data
            );
        }

        else if (line == "RESTART?")
        {
            PostMessage(
                hwnd,
                WM_APP_RESTART,
                0,
                0
            );
        }

        else if (line == "EXIT")
        {
            PostMessage(
                hwnd,
                WM_APP_EXIT,
                0,
                0
            );

            return 0;
        }
    }
}

//------------------------------------------------------------
// Refresh Board Buttons
//------------------------------------------------------------
void refreshCells()
{
    for (int i = 0; i < BOARD_SIZE; i++)
    {
        char c = g_board[i];

        const char* txt =
            (c == 'X') ? "X" :
            (c == 'O') ? "O" : "";

        SetWindowTextA(
            g_hCells[i],
            txt
        );

        EnableWindow(
            g_hCells[i],
            g_myTurn && c == '_'
        );
    }
}

//------------------------------------------------------------
// Status Text
//------------------------------------------------------------
void setStatus(const std::string& text)
{
    SetWindowTextA(
        g_hStatus,
        text.c_str()
    );
}

// Forward declaration
LRESULT CALLBACK WndProc(
    HWND,
    UINT,
    WPARAM,
    LPARAM
);