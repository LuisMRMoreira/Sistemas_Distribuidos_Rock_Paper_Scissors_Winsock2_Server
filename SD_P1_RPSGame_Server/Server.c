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

#define DEFAULT_PORT "25565" // N�mero da porta que comp�e o socket que establece conex�o com o serevidor
#define DEFAULT_BUFLEN 512 // Tamanho do buffer de comunica��o

// C�digos retornados pela fun��o que interpreta as mensagens recebidas.
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

// Estrutura de dados utilizada para obter a informa��o do endere�o e da porta que seram utilizados pelo servidor.
struct addrinfo* result = NULL, * ptr = NULL, hints;

// Fun��o que recebe como parametro a string enviada pelo cliente, interpreta-a e retorna um valor que servir� posteriormente para se contruir a mensagem de resposta do servidor para o cliente.
// Todos os poss�veis valores que podem ser retornados desta fun��o, est�o indicados no bloco de "#define" acima.
int playOrRestart(char* recvbuf) {

    printf("%d: Command Received -> ", GetCurrentThreadId());
    printf(recvbuf);
    printf("\n");

    // Bloco de c�digo respons�vel por fazer o "split" da mensagem recebida em poss�veis comandos interpret�veis.
    char* context = NULL;
    char recvbufcopy[DEFAULT_BUFLEN] = "";
    strcpy(recvbufcopy, recvbuf);
    char* firstword = strtok_s(recvbufcopy, " ", &context);
    char* arguments = strtok_s(NULL, " ", &context);

    // Transforma todos os caracteres do buffer que contem a mensagem recebida do servidor para "Upper Case" de forma a que os comandos sejam case insensitive.
    if (firstword != NULL)
        for (int i = 0; i < strlen(firstword); i++)
            firstword[i] = toupper(firstword[i]);
    if (arguments != NULL)
        for (int i = 0; i < strlen(arguments); i++)
            arguments[i] = toupper(arguments[i]);

    // Interpreta��o da mensagem recebida de forma a retornar um valor indicado para o posterior processamento.
    if (strcmp(firstword, "PLAY") == 0)
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
    else if (strcmp(firstword, "RESTART") == 0)
        return RESTART;
    else if (strcmp(firstword, "END") == 0)
        return END;
    else if (strcmp(firstword, "STATS") == 0)
        return STATS;
    else if (strcmp(firstword, "HELP") == 0)
        return HELP;
    else 
        return INVALID_COMMAND;
}

// Fun��o que representa a parte do c�digo que ser� poss�velmente executada em multiplas threads.
// Esta funcionalidade permite que multiplos clientes estejam conectados e a comunicar com o mesmo servidor simultaneamente atrav�s da cria��o de uma thread para cada novo cliente que se conecta ao servidor.
// Esta fun��o utilizada o resultado da fun��o anterior (int playOrRestart(char* recvbuf)) de forma a criar/ escolher a mensagem para o cliente.
DWORD WINAPI client_thread(SOCKET params) {
    
    // Criar um socket com os valores do socket criado ap�s se ter aceite a conex�o entre o cliente e o servidor.
    SOCKET current_client = (SOCKET)params;

    // Variaveis que cont�m informa��o das mensagens entre o servidor e o cliente.
    char recvbuf[DEFAULT_BUFLEN]; // Buffer com a string recebida pelo servidor
    char sendbuf[DEFAULT_BUFLEN]; // Buffer com a string que ser� enviada pelo servidor
    int iSendResult; // Resultado da fun��o send (envio de mensagem para o servidor).
    int iRecvResult; // Resultado da fun��o recv (recebe mensagens do cliente no servidor). N�mero de caracteres recebidos.

    int receivedMsgValue = INT_MIN; // Variavel que ir� guardar o resultado da interpreta��o da mensagem recebida atrav�s da fun��o "playOrRestart(recvbuf);".
    
    // Vari�veis utilizadas para a l�gica do comando "STATS"
    int randomnumber = 0;
    int gamesPlayed = 0, gamesWon = 0, gamesDraw = 0, gamesLost = 0;
    char gamesPlayedString[19], gamesWonString[16], gamesDrawString[17], gamesLostString[17];
    char auxString[5];
    
    bool skipCommand; // Vari�vel que permite ignorar poss�veis "lixos" que venham no buffer recebido.

    printf("%d: Connection established\n", GetCurrentThreadId());
    
    // Envio de uma mensagem informativa sobre o estado de conex�o com o servidor, assim como a interpreta��o do resultado do envio de forma a evitar poss�veis erros.
    strcpy_s(sendbuf, DEFAULT_BUFLEN, "100 OK: Connection established\nUse the HELP command for the list of commands available\n"); // Preenchimento do buffer de envio com a mensagem que se pretende enviar.
    iSendResult = send(current_client, sendbuf, strlen(sendbuf), 0); // Envio da mensagem contida no array de caracteres "sendbuf" para o cliente com o socket "current_client".
    if (iSendResult == SOCKET_ERROR) { //Interpreta��o do resultado do envio da mensagem de forma a informar poss�veis erros.
        // Em caso de erro:
        printf("%d: Send failed: %d\n", GetCurrentThreadId(), WSAGetLastError());
        closesocket(current_client); // Fechar o socket.
        WSACleanup(); // Limpeza de possiveis recursos alocados.
        return 1;
    }

    // Recebe informa��o at� que a conex�o entre o servidor e o cliente seja fechada, ou seja, enqaundo o cliente enviar informa��o para o servidor (iRecvResult > 0).
    do {

        // De forma a prevenir poss�veis "overflows" das strings que representam as estatisticas, os contadores s�o reinicializados a cada 1000 jogos em cada sess�o.
        if (gamesPlayed > 999)
        {
            gamesPlayed = 0;
            gamesWon = 0;
            gamesDraw = 0;
            gamesLost = 0;
        }

        // Convers�o dos valores para strins e prepar�-las para serem enviadas, no caso do cliente querer ver as suas estat�sticas.
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
        
        // Faz o reset do conteudo das strings para remover poss�veis caracteres indesejados.
        ZeroMemory(recvbuf, DEFAULT_BUFLEN);
        ZeroMemory(sendbuf, DEFAULT_BUFLEN);

        skipCommand = 0;
        
        // Recebe uma mensagem do cliente com o soccket "current_client" e guarda-a na string "recvbuf".
        iRecvResult = recv(current_client, recvbuf, DEFAULT_BUFLEN, 0);

        // Permite ignorar poss�veis caracteres que representem "lixo" para o nosso processamento ao passar � frente o processamento do mesmo.
        // Normalmente estes caracteres s�o os "\n", "\r" e "\r\n".
        if ((strcmp(recvbuf, "\n") == 0) || (strcmp(recvbuf, "\r") == 0) || (strcmp(recvbuf, "\r\n") == 0))
            skipCommand = 1;

        // No caso de se ter recebido mais que zero caracteres e da string recebida n�o representar caractres que devam ser ignorados:
        if ((iRecvResult > 0) && (skipCommand == 0)) { 
            if (iRecvResult > 0) {
            
                // Escolha aleat�ria da jogada do servidor.
                srand(time(0));
                randomnumber = rand() % 3;

                // Utilizar a fun��o criada em cima para interpretar a mensagem recebida.
                receivedMsgValue = playOrRestart(recvbuf);




                // De acordo com o valor obtido na fun��o "playOrRestart(recvbuf);", ser� criada/ escolhida uma mensagem que posteriormente ser� enviada como mensagem de resposta pelo servidor, ao cliente.
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
                    gamesPlayed++; // No caso da mensagem recebida ser de uma jogada, o n�mero de jogos � incrementado.
                    if (randomnumber == ROCK_VALUE) // De acordo com o resultado da jogada, os contadores de resultados � interpretado.
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
                case RESTART:  // Comando de reinicializa��o dos contadores das estatisticas.
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

                // Chegando a este ponto, o buffer "sendbuf" j� cont�m a mensagem que se quer enviar para o cliente "current_client", portanto a mensagem � enviada.
                iSendResult = send(current_client, sendbuf, strlen(sendbuf), 0);
                if (iSendResult == SOCKET_ERROR) {
                    printf("%d: Send failed: %d\n", GetCurrentThreadId(), WSAGetLastError());
                    closesocket(current_client);
                    WSACleanup();
                    return 1;
                }
            }

            if (receivedMsgValue == END)
            { //No caso da mensagem recebida pelo servidor ser de termino de conex�o, sai-se do ciclo e  faz-se os passos necess�rios para terminar a conex�o.
                printf("%d: Connection closing...\n", GetCurrentThreadId());
                break;
            }
        }
    } while (iRecvResult > 0);

    // Desconectar o servidor do cliente
    iRecvResult = shutdown(current_client, SD_SEND);
    if (iRecvResult == SOCKET_ERROR) {
        printf("%d: Shutdown failed: %d\n", GetCurrentThreadId(), WSAGetLastError());
        closesocket(current_client);
        return 1;
    }

    // Fechar o socket
    closesocket(current_client);

    // Fechar a thread e limpar o espa�o de mem�ria que esta a ocupar, uma vez que j� n�o pode servir o se proposito.
    ExitThread(0);

    return 0;
}


int __cdecl main(int argc, char** argv) {

    WSADATA wsaData;
    int iResult;

    // INICIALIZAR a biblioteca winsock utilizada para estabelecer comunica��o entre um cliente e um servidor. Execu��o da fun��o WSAStartup e determina��o de poss�veis erros.
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }

    // CRIA��O de um socket para o servidor.
    ZeroMemory(&hints, sizeof(hints));

    // Configura��es dos dados necess�rios para estabelecer comunica��o
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Obter o endere�o e a porta (sockaddr) que o servidor ir� usar. Esta informa��o ser� guardada na estrutura de dados criada "addrinfo".
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Cria��o de um objeto do tipo SOCKET que ser� utilizado para "ouvir" a tentativa de criar novas liga��es por parte de clientes.
    SOCKET ListenSocket = INVALID_SOCKET;
    // Cria��o do SOCKET atrav�s da fun��o "socket".
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    // Verifica��o se o socket criado � v�lido.
    if (ListenSocket == INVALID_SOCKET) {
        printf("Error at socket(): %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Fazer o BINDING de um socket para um endere�o IP e uma porta no sistema.
    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // Libertar a mem�ria alocada pela fun��o getaddrinfo para a informa��o deste endere�o.
    freeaddrinfo(result); 

    // LISTENING: Estar atento a poss�veis novos pedidos de conex�o por parte de clientes.
    iResult = listen(ListenSocket, SOMAXCONN); // SOMAXCONN: Valor que especifica o tamanho m�ximo da fila de conex�es pendentes.
    if (iResult == SOCKET_ERROR) { 
        printf("Listen failed with error: %ld\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }


    // ACCEPTING a Connection
    // ACEITAR uma conex�o: criar um socket tempor�rio chamado "ClientSocket" que sevir� exclusivamente para guardar informa��o do socket de cada cliente que se tenta ligar ao servidor.
    SOCKET ClientSocket;
    DWORD thread;

    int tempInteger = INT_MIN; // Variavel utilizada para obter o valor da thread criada.

    printf("Server is up and running.\n");
    printf("Waiting for new connections...\n");

    while (1)
    {
        ClientSocket = INVALID_SOCKET;
   
        // Aceitar a conex�o de um cliente e estabelecer um socket de comunica��o para o novo cliente na vari�vel "ClientSocket"
        ClientSocket = accept(ListenSocket, NULL, NULL);
        if (ClientSocket == INVALID_SOCKET) {
            printf("accept failed: %d\n", WSAGetLastError());
            closesocket(ListenSocket);
            WSACleanup();
            return 1;
        }

        printf("100 OK: Client connected!\n");

        // Cada vez que um cliente se tente ligar ao servidor, ser� criada uma nova thread. Cada thread ir� executar o c�digo correspondente � fun��o "client_thread" a quem ser� passado como parametro o socket da nova conex�o "ClientSocket".
        tempInteger = CreateThread(NULL, 0, client_thread, ClientSocket, 0, &thread);
        printf("Thread created: Thread id -> %d\n", tempInteger);
    }
}