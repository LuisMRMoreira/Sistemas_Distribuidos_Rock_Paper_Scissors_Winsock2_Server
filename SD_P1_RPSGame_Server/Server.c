#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <ctype.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma warning(disable : 4996)

#define DEFAULT_PORT "25565"
#define DEFAULT_BUFLEN 512

#define INVALID_COMMAND -3
#define INVALID_ARGUMENT -2
#define NO_ARGUMENTS -1
#define ROCK_VALUE 0
#define PAPER_VALUE 1
#define SCISSORS_VALUE 2
#define RESTART 3
#define END 4
#define STATS 5
#define HELP 6

struct addrinfo* result = NULL, * ptr = NULL, hints;

int playOrRestart(char* recvbuf) {

    printf("%d: Command Received -> ", GetCurrentThreadId());
    printf(recvbuf);
    printf("\n");


    char* context = NULL;

    char recvbufcopy[DEFAULT_BUFLEN] = "";
    strcpy(recvbufcopy, recvbuf);

    char* firstword = strtok_s(recvbufcopy, " ", &context);
    char* arguments = strtok_s(NULL, " ", &context);

    // Transforms all characters in 'firstword' to Upper Case to allow commands to be case insensitive
    if (firstword != NULL)
    {
        for (int i = 0; i < strlen(firstword); i++)
        {
            firstword[i] = toupper(firstword[i]);
        }
    }

    if (arguments != NULL)
    {
        for (int i = 0; i < strlen(arguments); i++)
        {
            arguments[i] = toupper(arguments[i]);
        }
    }

    if (strcmp(firstword, "PLAY") == 0)
    {
        if (arguments == NULL)
            return NO_ARGUMENTS;
        else if (strcmp(arguments, "ROCK") == 0)
            return ROCK_VALUE;
        else if (strcmp(arguments, "PAPER") == 0)
            return PAPER_VALUE;
        else if (strcmp(arguments, "SCISSORS") == 0)
            return SCISSORS_VALUE;
        else
            return INVALID_ARGUMENT;
    }
    else if (strcmp(firstword, "RESTART") == 0)
    {
        return RESTART;
    }
    else if (strcmp(firstword, "END") == 0)
    {
        return END;
    }
    else if (strcmp(firstword, "STATS") == 0)
    {
        return STATS;
    }
    else if (strcmp(firstword, "HELP") == 0)
    {
        return HELP;
    }
    else {
        return INVALID_COMMAND;
    }
}


DWORD WINAPI client_thread(SOCKET params) {
    // set our socket to the socket passed in as a parameter   
    SOCKET current_client = (SOCKET)params;

    // RECEIVING AND SENDING Data on the Server
    char recvbuf[DEFAULT_BUFLEN];
    char sendbuf[DEFAULT_BUFLEN];
    int iSendResult;
    int iRecvResult;

    int receivedMsgValue = INT_MIN;
    int randomnumber = 0;
    int gamesPlayed = 0, gamesWon = 0, gamesDraw = 0, gamesLost = 0;
    char gamesPlayedString[19], gamesWonString[16], gamesDrawString[17], gamesLostString[17];
    char auxString[5];
    bool skipCommand;

    printf("%d: Connection established\n", GetCurrentThreadId());
    strcpy_s(sendbuf, DEFAULT_BUFLEN, "100 OK: Connection established\nUse the HELP command for the list of commands available\n");
    iSendResult = send(current_client, sendbuf, strlen(sendbuf), 0);
    if (iSendResult == SOCKET_ERROR) {
        printf("%d: Send failed: %d\n", GetCurrentThreadId(), WSAGetLastError());
        closesocket(current_client);
        WSACleanup();
        return 1;
    }

    // Receive until the peer shuts down the connection
    do {
        // To prevent overflows with the stats strings, counters are reset 
        // once the user has played 1000 games in a single session
        if (gamesPlayed > 999)
        {
            gamesPlayed = 0;
            gamesWon = 0;
            gamesDraw = 0;
            gamesLost = 0;
        }

        // Convert stats values to string and prepares them to be sent, in 
        // case the client wants to see his session stats
        sprintf(auxString, "%d", gamesPlayed);
        strcat(auxString, "\n");
        strcpy(gamesPlayedString, "Games played: ");
        strcat(gamesPlayedString, auxString);

        sprintf(auxString, "%d", gamesWon);
        strcat(auxString, "\n");
        strcpy(gamesWonString, "Games won: ");
        strcat(gamesWonString, auxString);

        sprintf(auxString, "%d", gamesDraw);
        strcat(auxString, "\n");
        strcpy(gamesDrawString, "Games draw: ");
        strcat(gamesDrawString, auxString);

        sprintf(auxString, "%d", gamesLost);
        strcat(auxString, "\n");
        strcpy(gamesLostString, "Games lost: ");
        strcat(gamesLostString, auxString);
        
        ZeroMemory(recvbuf, DEFAULT_BUFLEN);
        ZeroMemory(sendbuf, DEFAULT_BUFLEN);

        skipCommand = 0;
        iRecvResult = recv(current_client, recvbuf, DEFAULT_BUFLEN, 0);

        // If the command received is a newline skip the command and do nothing
        if ((strcmp(recvbuf, "\n") == 0) || (strcmp(recvbuf, "\r") == 0) || (strcmp(recvbuf, "\r\n") == 0))
        {
            skipCommand = 1;
        }


        if ((iRecvResult > 0) && (skipCommand == 0)) {
            if (iRecvResult > 0) {
            
                // Randomly choose what the server will play
                srand(time(0));
                randomnumber = rand() % 3;

                receivedMsgValue = playOrRestart(recvbuf);

                // According to the message received, the server will do the
                // corresponding operations
                switch (receivedMsgValue)
                {
                case INVALID_COMMAND:
                    strcpy_s(sendbuf, DEFAULT_BUFLEN, "Invalid command. Valid commands <PLAY <ROCK;PAPER;SCISSORS>;RESTART;END;HELP;STATS> Try again.\n");
                    break;
                case INVALID_ARGUMENT:
                    strcpy_s(sendbuf, DEFAULT_BUFLEN, "Invalid argument to the 'PLAY' command. Valid arguments PLAY <ROCK;PAPER;SCISSORS>. Try again.\n");
                    break;
                case NO_ARGUMENTS:
                    strcpy_s(sendbuf, DEFAULT_BUFLEN, "No Arguments. Valid commands <PLAY <ROCK;PAPER;SCISSORS>> Try again.\n");
                    break;
                case HELP:
                    strcpy_s(sendbuf, DEFAULT_BUFLEN, "PLAY ROCK - Play a game and choose rock\n");
                    strcat_s(sendbuf, DEFAULT_BUFLEN, "PLAY SCISSORS - Play a game and choose scissors\n");
                    strcat_s(sendbuf, DEFAULT_BUFLEN, "PLAY PAPER - Play a game and choose paper\n");
                    strcat_s(sendbuf, DEFAULT_BUFLEN, "STATS - See your stats in the current session\n");
                    strcat_s(sendbuf, DEFAULT_BUFLEN, "RESTART - Restart the connection\n");
                    strcat_s(sendbuf, DEFAULT_BUFLEN, "END - Close the connection\n");
                    break;
                case STATS:
                    strcpy_s(sendbuf, DEFAULT_BUFLEN, gamesPlayedString);
                    strcat_s(sendbuf, DEFAULT_BUFLEN, gamesWonString);
                    strcat_s(sendbuf, DEFAULT_BUFLEN, gamesLostString);
                    strcat_s(sendbuf, DEFAULT_BUFLEN, gamesDrawString);
                    break;
                case ROCK_VALUE:
                    gamesPlayed++;
                    if (randomnumber == ROCK_VALUE) 
                    {
                        strcpy_s(sendbuf, DEFAULT_BUFLEN, "200 CLIENTPLAY ROCK <CR> SERVERPLAY ROCK <CR>\n300 RESULT Draw!\n");
                        gamesDraw++;
                    }
                    else if (randomnumber == PAPER_VALUE)
                    {
                        strcpy_s(sendbuf, DEFAULT_BUFLEN, "200 CLIENTPLAY ROCK <CR> SERVERPLAY PAPER <CR>\n300 RESULT Server Wins!\n");
                        gamesLost++;
                    }
                    else if (randomnumber == SCISSORS_VALUE)
                    {
                        strcpy_s(sendbuf, DEFAULT_BUFLEN, "200 CLIENTPLAY ROCK <CR> SERVERPLAY SCISSORS <CR>\n300 RESULT Client Wins!\n");
                        gamesWon++;
                    }
                    break;
                case PAPER_VALUE:
                    gamesPlayed++;
                    if (randomnumber == ROCK_VALUE)
                    {
                        strcpy_s(sendbuf, DEFAULT_BUFLEN, "200 CLIENTPLAY PAPER <CR> SERVERPLAY ROCK <CR>\n300 Client Wins!\n");
                        gamesWon++;
                    }
                    else if (randomnumber == PAPER_VALUE)
                    {
                        strcpy_s(sendbuf, DEFAULT_BUFLEN, "200 CLIENTPLAY PAPER <CR> SERVERPLAY PAPER <CR>\n300 RESULT Draw!\n");
                        gamesDraw++;
                    }
                    else if (randomnumber == SCISSORS_VALUE)
                    {
                        strcpy_s(sendbuf, DEFAULT_BUFLEN, "200 CLIENTPLAY PAPER <CR> SERVERPLAY SCISSORS <CR>\n300 RESULT Server Wins!\n");
                        gamesLost++;
                    }
                    break;
                case SCISSORS_VALUE:
                    gamesPlayed++;
                    if (randomnumber == ROCK_VALUE)
                    {
                        strcpy_s(sendbuf, DEFAULT_BUFLEN, "200 CLIENTPLAY SCISSORS <CR> SERVERPLAY ROCK <CR>\n300 RESULT Server Wins!\n");
                        gamesLost++;
                    }
                    else if (randomnumber == PAPER_VALUE)
                    {
                        strcpy_s(sendbuf, DEFAULT_BUFLEN, "200 CLIENTPLAY SCISSORS <CR> SERVERPLAY PAPER <CR>\n300 RESULT Client Wins!\n");
                        gamesWon++;
                    }
                    else if (randomnumber == SCISSORS_VALUE)
                    {
                        strcpy_s(sendbuf, DEFAULT_BUFLEN, "200 CLIENTPLAY SCISSORS <CR> SERVERPLAY SCISSORS <CR>\n300 RESULT Draw!\n");
                        gamesDraw++;
                    }
                    break;
                case RESTART:  
                    gamesPlayed = 0;
                    gamesWon = 0;
                    gamesLost = 0;
                    gamesDraw = 0;
                    strcpy_s(sendbuf, DEFAULT_BUFLEN, "100 OK\n");
                    break;
                case END:
                    strcpy_s(sendbuf, DEFAULT_BUFLEN, "400 BYE\n");
                    break;
                default:
                    strcpy_s(sendbuf, DEFAULT_BUFLEN, "INVALID CODE\n");
                    break;
                }

                // Send the response to the client
                iSendResult = send(current_client, sendbuf, strlen(sendbuf), 0);

                if (iSendResult == SOCKET_ERROR) {
                    printf("%d: Send failed: %d\n", GetCurrentThreadId(), WSAGetLastError());
                    closesocket(current_client);
                    WSACleanup();
                    return 1;
                }
            }

            if (receivedMsgValue == END)
            {
                printf("%d: Connection closing...\n", GetCurrentThreadId());
                break;
            }
        }
    } while (iRecvResult > 0);

    // Disconnecting from the client
    iRecvResult = shutdown(current_client, SD_SEND);
    if (iRecvResult == SOCKET_ERROR) {
        printf("%d: Shutdown failed: %d\n", GetCurrentThreadId(), WSAGetLastError());
        closesocket(current_client);
        return 1;
    }

    // Close socket
    closesocket(current_client);
    ExitThread(0);

    return 0;
}


int __cdecl main(int argc, char** argv) {

    //INITIALIZING Winsock
    WSADATA wsaData;
    int iResult;
    // Initialize Winsock: Call WSAStartup and check for errors.
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }

    //CREATING a Socket for the Server
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;
    // Resolve the local address and port to be used by the server: The getaddrinfo function is used to determine the values in the sockaddr structure.
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    //Create a SOCKET object called ListenSocket for the server to listen for client connections.
    SOCKET ListenSocket = INVALID_SOCKET;
    // Create a SOCKET for the server to listen for client connections
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    //Check for errors to ensure that the socket is a valid socket.
    if (ListenSocket == INVALID_SOCKET) {
        printf("Error at socket(): %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // BINDING a Socket to an IP address and a port on the system
    // Setup the TCP listening socket
    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }
    freeaddrinfo(result); // Free the memory allocated by the getaddrinfo function for this address information.


    // LISTENING on a Socket for incoming connection requests
    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) { // SOMAXCONN: backlog that specifies the maximum length of the queue of pending connections to accept
        printf("Listen failed with error: %ld\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }


    // ACCEPTING a Connection
    // Create a temporary SOCKET object called ClientSocket for accepting connections from clients.
    SOCKET ClientSocket;
    DWORD thread;

    int tempInteger = INT_MIN;

    printf("Server is up and running.\n");
    printf("Waiting for new connections...\n");

    while (1)
    {
        ClientSocket = INVALID_SOCKET;
   
        // Accept a client socket
        ClientSocket = accept(ListenSocket, NULL, NULL);
        if (ClientSocket == INVALID_SOCKET) {
            printf("accept failed: %d\n", WSAGetLastError());
            closesocket(ListenSocket);
            WSACleanup();
            return 1;
        }

        printf("100 OK: Client connected!\n");

        // create our recv_cmds thread and parse client socket as a parameter
        tempInteger = CreateThread(NULL, 0, client_thread, ClientSocket, 0, &thread);
        printf("Thread created: Thread id -> %d\n", tempInteger);
    }
}