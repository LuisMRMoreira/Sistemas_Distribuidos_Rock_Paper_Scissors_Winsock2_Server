#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma warning(disable : 4996)

#define DEFAULT_PORT "68000"
#define DEFAULT_BUFLEN 512

#define SUCCESSFUL_MESSAGE "100 OK"

#define ARGUMENTS_STRING_SIZE 8
#define ROCK_VALUE 0
#define PAPER_VALUE 1
#define SCISSORS_VALUE 2

struct addrinfo* result = NULL, * ptr = NULL, hints;

int playOrRestart(char* recvbuf) {

    printf(recvbuf);


    char* context = NULL;

    char recvbufcopy[DEFAULT_BUFLEN] = "";
    strcpy(recvbufcopy, recvbuf);

    char* firstword = strtok_s(recvbufcopy, " ", &context);
    char* arguments = strtok_s(NULL, " ", &context);

    if (strcmp(firstword, "PLAY") == 0)
    {


        if (arguments == NULL)
            return -1;// TODO: Enviar mensagem de erro a informar que comando para jogar não tem argumentos
        else if (strcmp(arguments, "ROCK") == 0)
            return ROCK_VALUE;
        else if (strcmp(arguments, "PAPER") == 0)
            return PAPER_VALUE;
        else if (strcmp(arguments, "SCISSORS") == 0)
            return SCISSORS_VALUE;
        else
            return -2;// TODO: Enviar mensagem de erro a informar que o argumento é inválido


    }
    else if (strcmp(firstword, "RESTART") == 0)//PLAY AGAIN
    {
        return 3;
    }
    else if (strcmp(firstword, "END") == 0) //END COMUNICATION
    {
        return 4;
    }
    else {
        return -3; // INVALID COMMAND
    }


}


//Sistemas_Distribuidos_Rock_Paper_Scissors_Winsock2_Server

//Primeiro projeto da unidade curricular Sistemas Distribuidos. Desenvolvimento do jogo Rock Paper Scissors através da biblioteca winsock e de threads. 

//Desenvolvimento do servidor. Tratamento de comandos e de possíveis respostas. Sem Threads.




int __cdecl main(int argc, char** argv) {
    int receivedMsgValue = -3;
    int randomnumber = 0;



//INITIALIZING Winsock
    WSADATA wsaData;//Create a WSADATA object called wsaData.
    int iResult;
    // Initialize Winsock: Call WSAStartup and return its value as an integer and check for errors.
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
    freeaddrinfo(result);// Free the memory allocated by the getaddrinfo function for this address information.



// LISTENING on a Socket for incoming connection requests
    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) { // SOMAXCONN: backlog that specifies the maximum length of the queue of pending connections to accept
        printf("Listen failed with error: %ld\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }


    // ACCEPTING a Connection
       //Create a temporary SOCKET object called ClientSocket for accepting connections from clients.
    SOCKET ClientSocket;
    ClientSocket = INVALID_SOCKET;

    // Accept a client socket
    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET) {
        printf("accept failed: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // No longer need server socket
    //closesocket(ListenSocket);

// RECEIVING AND SENDING Data on the Server
    char recvbuf[DEFAULT_BUFLEN];
    char sendbuf[DEFAULT_BUFLEN];
    int iSendResult;


    // Receive until the peer shuts down the connection
    do {
        
        ZeroMemory(recvbuf, DEFAULT_BUFLEN);
        ZeroMemory(sendbuf, DEFAULT_BUFLEN);

        iResult = recv(ClientSocket, recvbuf, DEFAULT_BUFLEN, 0);
        if (iResult > 0){

            srand(time(0));
            randomnumber = rand() % 3;

            int receivedMsgValue = playOrRestart(recvbuf);
            //De acordo com a mensagem que o servidor receve, trata-a 
            switch (receivedMsgValue)
            {
            case -3:
                strcpy_s(sendbuf, DEFAULT_BUFLEN, "Invalid command. Valid commands <PLAY <ROCK;PAPER;SCISSORS>;RESTART> Try again.\n");
                break;
            case -2:
                strcpy_s(sendbuf, DEFAULT_BUFLEN, "Invalid argument to the 'PLAY' command. Valid arguments PLAY <ROCK;PAPER;SCISSORS>. Try again.\n");
                break;
            case -1:
                strcpy_s(sendbuf, DEFAULT_BUFLEN, "No Arguments. Valid commands <PLAY <ROCK;PAPER;SCISSORS>;RESTART> Try again.\n");
                break;
            case ROCK_VALUE:

                if (randomnumber == ROCK_VALUE)// Jogada do cliente
                {
                    strcpy_s(sendbuf, DEFAULT_BUFLEN, "200 CLIENTPLAY ROCK <CR> SERVERPLAY ROCK < CR>\n300 RESULT Draw!\n");

                }
                else if (randomnumber == PAPER_VALUE)
                {
                    strcpy_s(sendbuf, DEFAULT_BUFLEN, "200 CLIENTPLAY PAPER <CR> SERVERPLAY ROCK < CR>\n300 RESULT Client Wins!\n");
                }
                else if (randomnumber == SCISSORS_VALUE)
                {
                    strcpy_s(sendbuf, DEFAULT_BUFLEN, "200 CLIENTPLAY SCISSORS <CR> SERVERPLAY ROCK < CR>\n300 RESULT Server Wins!\n");
                }

                break;

            case PAPER_VALUE:

                if (randomnumber == ROCK_VALUE)// Jogada do cliente
                {
                    strcpy_s(sendbuf, DEFAULT_BUFLEN, "200 CLIENTPLAY ROCK <CR> SERVERPLAY PAPER < CR>\n300 Server Wins!");

                }
                else if (randomnumber == PAPER_VALUE)
                {
                    strcpy_s(sendbuf, DEFAULT_BUFLEN, "200 CLIENTPLAY PAPER <CR> SERVERPLAY PAPER < CR>\n300 RESULT Draw!\n");
                }
                else if (randomnumber == SCISSORS_VALUE)
                {
                    strcpy_s(sendbuf, DEFAULT_BUFLEN, "200 CLIENTPLAY SCISSORS <CR> SERVERPLAY PAPER < CR>\n300 RESULT Client Wins!\n");
                }

                break;

            case SCISSORS_VALUE:

                if (randomnumber == ROCK_VALUE)// Jogada do cliente
                {
                    strcpy_s(sendbuf, DEFAULT_BUFLEN, "200 CLIENTPLAY ROCK <CR> SERVERPLAY SCISSORS < CR>\n300 RESULT Client Wins!\n");

                }
                else if (randomnumber == PAPER_VALUE)
                {
                    strcpy_s(sendbuf, DEFAULT_BUFLEN, "200 CLIENTPLAY PAPER <CR> SERVERPLAY SCISSORS < CR>\n300 RESULT Server Wins!\n");
                }
                else if (randomnumber == SCISSORS_VALUE)
                {
                    strcpy_s(sendbuf, DEFAULT_BUFLEN, "200 CLIENTPLAY SCISSORS <CR> SERVERPLAY SCISSORS < CR>\n300 RESULT Draw!\n");
                }

                break;

            case 3:// RESTART                
                strcpy_s(sendbuf, DEFAULT_BUFLEN, "100 OK\n");
                break;
            case 4:// END
                strcpy_s(sendbuf, DEFAULT_BUFLEN, "400 BYE\n");
                break;
            default:
                strcpy_s(sendbuf, DEFAULT_BUFLEN, "INVALID CODE\n");
                break;
            }

            // Echo the buffer back to the sender
            iSendResult = send(ClientSocket, sendbuf, strlen(sendbuf), 0);
            if (iSendResult == SOCKET_ERROR) {
                printf("send failed: %d\n", WSAGetLastError());
                closesocket(ClientSocket);
                WSACleanup();
                return 1;
            }

            if (receivedMsgValue==4)//END
            {
                printf("Connection closing...\n");
                break;                
                // Close socket
                //closesocket(ClientSocket);
                //WSACleanup();
            }

            iResult = recv(ClientSocket, recvbuf, DEFAULT_BUFLEN, 0);// Clear garbage


        }

    } while (iResult > 0);

    // Disconnecting the Server
        // shutdown the send half of the connection since no more data will be sent
    iResult = shutdown(ClientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }

    // cleanup
    closesocket(ClientSocket);
    WSACleanup();

    return 0;

}


