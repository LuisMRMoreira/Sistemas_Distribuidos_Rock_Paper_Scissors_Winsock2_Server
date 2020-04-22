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
// UPDATE: 
#define AUTHENTICATION_CREATE_ACCOUNT 10 // Representa a inten��o por parte do cliente de criar uma conta.
#define AUTHENTICATION_AUTHENTICATE 11 // Representa a inten��o por parte do cliente de se autenticar.
#define AUTHENTICATION_BACK_FIELD 12 // Voltar atr�s no preenchimento de um campo.
#define GARBAGE 13
#define ACCEPTED_ENTRY 14 // Valor durante a autentica��o ou cria��o de conta que pode ser aceite uma vez que pode representar os valores dos campos.
#define MAX_NUM_CREATE_ACCOUNT_FIELDS 4 // De forma a que seja mais din�mico alterar o n�mero de campos de preenchimento, estabelece-se aqui o n�mero m�ximo.
#define MAX_NUM_AUTHENTICATION_FIELDS 2

// Retornos das fun��es usadas para guardar dados em ficheiros
#define ERROR_RELEASING_MUTEX -4
#define ERROR_SAVING -3
#define USER_ALREADY_EXISTS -2
#define EMAIL_ALREADY_USED -1
#define SUCCESS_SAVING 0


// C�digos usados para designar fun��es para guardar ou ler dados em ficheiros
#define REGISTER_USER 0

// Estrutura de dados utilizada para obter a informa��o do endere�o e da porta que seram utilizados pelo servidor.
struct addrinfo* result = NULL, * ptr = NULL, hints;

// UPDATE: Este enum foi criado de forma a que o interpretador passe receba a informa��o necess�ria para interpretar o buffer que veio do cliente. Quando um cliente "isAuthenticating" e "isCreatingAccount" n�o est� autenticado, no entanto foi adicionado o "isNotAuthenticated" para o caso de n�o se saber gual a inten��o do cliente quando n�o est� autenticado.
enum Authentication { isNotAuthenticated = -1, isAuthenticated = 1, isAuthenticating = 2, isCreatingAccount = 3 };

// Campos necess�rios para criar um perfil.
enum AccountFields { username = 1, password = 2, email = 3 };

// Defini��o do mutex usado para escrever em ficheiros
HANDLE ghMutex;


// Fun��o que recebe como parametro a string enviada pelo cliente, interpreta-a e retorna um valor que servir� posteriormente para se contruir a mensagem de resposta do servidor para o cliente.
// Todos os poss�veis valores que podem ser retornados desta fun��o, est�o indicados no bloco de "#define" acima.
// UPDATE: Nesta nova vers�o foi adicionado aos parametros, um enum que informa o interpretador, do estado de autentica��o. Esta fun��o foi usada para permitir a funcionalidade do utilizador reverter o preenchimento de um ou v�rios campo durante a autentica��o ou cria��o de conta.
int interpreter(enum Authentication value, char* recvbuf) {

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


    // UPDATE: No caso do jogador ainda n�o estar autenticado.
    // De acordo com o estado de autentica��o da sess�o, o processamento do buffer que veio do cliente vai ser dferente
    if (value != isAuthenticated) {

        if (value == isAuthenticating || value == isCreatingAccount) // No caso do jogador estar a autenticar-se ou a criar conta, interpreta o buffer recebido. No caso desse buffer ter o conteudo "BACK", significa que o utilizador pretende corrigir o campo anterior.
            if (strcmp(firstword, "<-BACK") == 0)
                return AUTHENTICATION_BACK_FIELD;
            else return ACCEPTED_ENTRY; // No caso do cliente se estar a autenticar ou a criar uma conta, se o utilizador n�o enviou o comando para voltar para o campo anterior, ent�o � porque temos que aceitar esse campo, esteja ele bem ou mal.

        if (strcmp(firstword, "Y") == 0) // J� tem conta e por isso, pretende autenticar-se.
            return AUTHENTICATION_AUTHENTICATE;
        else if (strcmp(firstword, "N") == 0) // Ainda n�o tem conta e por isso vai cri�-la.
            return AUTHENTICATION_CREATE_ACCOUNT;
        else return GARBAGE;
    }
    else {    
        // Interpreta��o da mensagem recebida de forma a retornar um valor indicado para o posterior processamento. Esta mensagem s� interpretada com as normas do jogo no caso do cliete estar autenticado
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
}


// UPDATE: 
bool startAuthentication(SOCKET current_client) {
    char recvbuf[DEFAULT_BUFLEN]; // Buffer com a string recebida pelo servidor
    ZeroMemory(recvbuf, DEFAULT_BUFLEN);
    char sendbuf[DEFAULT_BUFLEN];
    ZeroMemory(recvbuf, DEFAULT_BUFLEN);
    int iSendResult;
    int iRecvResult;

    // Esta vari�vel ser� utilizada para se saber em que t�pico de recolha de informa��o para autentica��o (Username, email, password) � que o utilizador est�.
    enum Authentication authentication = isNotAuthenticated;
    int fields = -1;
    char fieldsEntries[MAX_NUM_CREATE_ACCOUNT_FIELDS-1][DEFAULT_BUFLEN];
    // Este array multidimensional � preenchido de forma inversa. No caso da autentica��o, na linha com o index 1 � armazenado o valor do username e na 0, a password. No caso da cria��o da conta, no index 2 est� o username, no 1 a email e no 0 a password. 
        // Cria��o de conta:
            // 0 -> Password; (O valor do campo de confrma��o de password nunca � armazenado, apenas � comparado no elemento 0 deste campo, assim que for recebido no buffer recvbuf)
            // 1 -> Email;
            // 2 -> Username.
        // Autentica��o:
            // 0 -> Password;
            // 1 -> Username.
    // Neste �ltimo caso, apenas dois elementos do array s�o preenchidos, permanecendo o �ltimo inalterado.



    strcpy_s(sendbuf, DEFAULT_BUFLEN, "Before we start! Do you have any account already created?\nPress \"Y\" if you do, or \"N\" if you don't and then press enter: ");
    iSendResult = send(current_client, sendbuf, strlen(sendbuf), 0); // Envio da mensagem contida no array de caracteres "sendbuf" para o cliente com o socket "current_client".
    if (iSendResult == SOCKET_ERROR) { // No caso do envio dar erro.
        printf("%d: Send failed: %d\n", GetCurrentThreadId(), WSAGetLastError());
        ExitThread(0); // Fecha-se a thread e limpa-se o espa�o de mem�ria que alocou.
        closesocket(current_client); // Fecha-se o socket.
        WSACleanup(); // Limpa-se o espa�o de mem�ria alocado pelo socket.
        return false;
    }

    do
    {
        // receber buffer do cliente.
        iRecvResult = recv(current_client, recvbuf, DEFAULT_BUFLEN, 0);

        // De acordo com o que o cliente envia para o servidor, a fun��o interpreter interpreta esse buffer e retorna um resultado. Na primeira itera��o, o primeiro parametro que � passado para esta fun��o ser� a indica��o de que o utilizador n�o est� autenticado para que seja facil processar a informa��o recebida de acordo com o estado em que a sess�o est�.
        int interpretedResult =  interpreter(authentication, recvbuf);

        // O c�digo recebido da interpreta��o � tratado neste switch. Trata os casos do utilizador:
            // J� tem conta e por isso tem a inten��o de se autenticar (AUTHENTICATION_AUTHENTICATE, colocou "Y" na primeira pergunta)
            // N�o ter conta e por isso tem a inten��o de criar uma para jogar (AUTHENTICATION_CREATE_ACCOUNT, colocou "N" na primeira pergunta)
            // Durante a autentica��o ou cria��o de conta, o utilizador pode querer voltar a preeencher um campo, para isso usa o comando "<-back" e no caso de ser poss�vel, volta a preencher o �ltimo campo.
            // No caso do c�digo recebido ser do tipo accept entry, verifica-se se j� estamos no �ltimo campo. Se sim, no caso de estarmos a criar conta, comparamos o valor dos campos "password" e "confirm password" e, se forem iguais, guarda-se em ficheiro a nova conta e pede-se ao utilizador para se autenticar com os dados que acabou de criar. No caso da autentica��o, depois de se preencher os campos username e password, verifica-se se no ficheiro se encontra alguma conta com esses dados (caso n�o exista, envia-se essa menagem e o utilizador volta a inserir os dados de autentica��o). Caso ainda n�o se esteja no �ltimo campo, o campo atual � gauradado no index indicado do aray fieldsEntries (ver informa��es do array em cima).
            // No caso do valor ser identificado como GARBAGE � porque n�o tem significado nenhum e por isso deve pedir-se novamente ao utilizador a sua inten��o.
                
        switch (interpretedResult)
        {
        case AUTHENTICATION_AUTHENTICATE: // Caso a inten��o do cliente seja autenticar-se.
            fields = MAX_NUM_AUTHENTICATION_FIELDS; // O n�mero de campos a preencher passa a ser MAX_NUM_AUTHENTICATION_FIELDS.
            authentication = isAuthenticating; // E a vari�vel authentication passa a indicar que o cliente est� a autenticar-se (isAuthenticating).
            break;
        case AUTHENTICATION_CREATE_ACCOUNT: // Caso a inten��o do cliente seja criar conta.
            fields = MAX_NUM_CREATE_ACCOUNT_FIELDS; // O n�mero de campos a preencher passa a ser MAX_NUM_CREATE_ACCOUNT_FIELDS.
            authentication = isCreatingAccount;// E a vari�vel authentication passa a indicar que o cliente est� a criar uma conta (isCreatingAccount).
            break;
        case AUTHENTICATION_BACK_FIELD:
            if (authentication == isAuthenticating) // Se o utilizador se estiver a autenticar
                if (fields >= MAX_NUM_AUTHENTICATION_FIELDS - 1) // e o n�mero de campos for maior que o n�mero maximo de campos - 1
                    fields++; // Ent�o � porque o utilizador n�o pode voltar mais para traz e por isso s� se adiciona 1 aos fields (adiciona-se porque na �ltima itera��o removeu-se 1)
                else
                    fields += 2; // Caso contr�rio, ent�o o utilizador pode voltar a preencher o �ltimo campo e por isso adiciona-se 2.
            else if (authentication == isCreatingAccount) // A cria��o de conta funciona de forma an�loga
                if (fields < MAX_NUM_CREATE_ACCOUNT_FIELDS - 1)
                    fields += 2;
                else
                    fields++;
            break;
        case ACCEPTED_ENTRY:

            if (fields == 0) { // Caso se tenha alcan�ado o �ltimo campo de preenchimento
                
                if (authentication == isCreatingAccount)
                {
                    // Comparar se a password inserida � igual � password de confirma��o
                    if (!strcmp(fieldsEntries[MAX_NUM_CREATE_ACCOUNT_FIELDS - 2 - 1], recvbuf)) // No caso dos valores n�o serem iguais, envia-se uma mensagem de erro e volta-se a pedir para introduzir a password de novo.
                    {
                        strcpy_s(sendbuf, DEFAULT_BUFLEN, "Passwords doen't match! Try again\nPassword: ");
                        iSendResult = send(current_client, sendbuf, strlen(sendbuf), 0);
                        if (iSendResult == SOCKET_ERROR) {
                            printf("%d: Send failed: %d\n", GetCurrentThreadId(), WSAGetLastError());
                            closesocket(current_client);
                            WSACleanup();
                            return false;
                        }
                        ZeroMemory(fieldsEntries[MAX_NUM_CREATE_ACCOUNT_FIELDS - 2 - 1], DEFAULT_BUFLEN); // Uma vez que vamos voltar a pedir ao utilizador para inserir os dados da password, temos de os limpar do array.
                        fields += 2;
                        iRecvResult = recv(current_client, recvbuf, DEFAULT_BUFLEN, 0); // Importante: Limpar lixo que est� no buffer.
                        continue; // Voltar ao inicio do while sem que o resto do c�digo seja executado.
                    }
                    else // No caso do valor dos dois campos da password serem iguais, armazena-se os dados, limpa-se o ecra, faz-se o reset das vari�veis e pede-se ao utilizador para se autenticar.
                    {
                        ChangeFileData(*fieldsEntries, REGISTER_USER);
                    
                        memset(fieldsEntries, 0, sizeof(fieldsEntries[MAX_NUM_CREATE_ACCOUNT_FIELDS-1][DEFAULT_BUFLEN]) * ( MAX_NUM_CREATE_ACCOUNT_FIELDS -1 ) * ( DEFAULT_BUFLEN ) ); // zerar array
                        authentication = isAuthenticating;
                        fields = MAX_NUM_AUTHENTICATION_FIELDS-1;
                        system("cls"); // Limpar todo o ecra.
                        strcpy_s(sendbuf, DEFAULT_BUFLEN, "Authentication:\nUsername: ");
                        iSendResult = send(current_client, sendbuf, strlen(sendbuf), 0);
                        if (iSendResult == SOCKET_ERROR) {
                            printf("%d: Send failed: %d\n", GetCurrentThreadId(), WSAGetLastError());
                            closesocket(current_client);
                            WSACleanup();
                            return false;
                        }
                        iRecvResult = recv(current_client, recvbuf, DEFAULT_BUFLEN, 0); // Importante: Limpar lixo que est� no buffer.
                        continue; // Voltar ao inicio do while sem que o resto do c�digo seja executado.
                    }
                }
                else if (authentication == isAuthenticating)
                {
                    strcpy_s(fieldsEntries[fields], DEFAULT_BUFLEN, recvbuf);

                    // TODO: Verificar se o que est� no ficheiro est� de acordo com os dados do array fieldsEntries.
                    if (false) // No caso dos dados introduzidos estarem de acordo com os que est�o em ficheiro
                    {
                        return true;
                    }
                    else // Caso contr�rio, limpam-se todos os dados e pede-se ao utilizador para colocar os dados de autentica��o novamente.
                    {
                        fields = MAX_NUM_AUTHENTICATION_FIELDS-1;
                        memset(fieldsEntries, 0, sizeof(fieldsEntries[MAX_NUM_CREATE_ACCOUNT_FIELDS -1][DEFAULT_BUFLEN]) * (MAX_NUM_CREATE_ACCOUNT_FIELDS -1 ) * (DEFAULT_BUFLEN)); // zerar array
                        system("cls"); // Limpar todo o ecra.
                        strcpy_s(sendbuf, DEFAULT_BUFLEN, "There is no such account! Try again.\nUsername: ");
                        iSendResult = send(current_client, sendbuf, strlen(sendbuf), 0);
                        if (iSendResult == SOCKET_ERROR) {
                            printf("%d: Send failed: %d\n", GetCurrentThreadId(), WSAGetLastError());
                            closesocket(current_client);
                            WSACleanup();
                            return false;
                        }
                        iRecvResult = recv(current_client, recvbuf, DEFAULT_BUFLEN, 0); // Importante: Limpar lixo que est� no buffer.
                        continue;
                    }
                    
                    
                }
            }
            else // Caso contr�rio, este array � preenchido com os valores dos campos de preenchiento para a cria��o de uma conta. No final este array vai ser utilizado para se escrever em ficheiro os dados das contas.
                if (authentication == isAuthenticating)
                    strcpy_s(fieldsEntries[fields], DEFAULT_BUFLEN, recvbuf); // Esta array � para o caso da autentica��o e � preenchido na ordem iversa.
                else 
                    strcpy_s(fieldsEntries[fields-1], DEFAULT_BUFLEN, recvbuf); // Esta array � para o caso da cria��o de conta e � preenchido na ordem iversa.

            
            break;
        case GARBAGE:
            // Informa o cliente que os dados que inseriu n�o est�o dentro do protocolo de comunica��o e que por isso deve voltar a submeter o comando.
            strcpy_s(sendbuf, DEFAULT_BUFLEN, "Invalid command! please select one of the mentioned options!\n");
            iSendResult = send(current_client, sendbuf, strlen(sendbuf), 0);
            if (iSendResult == SOCKET_ERROR) {
                printf("%d: Send failed: %d\n", GetCurrentThreadId(), WSAGetLastError());
                closesocket(current_client);
                WSACleanup();
                return false;
            }
            //recv(current_client, recvbuf, DEFAULT_BUFLEN, 0); // Limpar lixo do buffer que recebe dados do cliente.
            continue; // Voltar ao inicio do while sem que o resto do c�digo seja executado.
            break;
        default:
            break;
        }


        // Depois de ser interpretar e tratar a mensagem do cliente para o servidor, cria-se a mensagem de resposta do servidor para o cliente
        switch (authentication)
        {
        case isAuthenticating:
            switch (fields)
            {
            case MAX_NUM_AUTHENTICATION_FIELDS: // Valor do array: Username -> 1 ; Valor do case: 2
                strcpy_s(sendbuf, DEFAULT_BUFLEN, "\nUsername: ");
                fields--; // Elimina-se um campo poque este j� est� a ser tratado e assim a variavel fields pode ser utilizada para aceder como index ao array fieldsEntries.
                break;
            case MAX_NUM_AUTHENTICATION_FIELDS - 1: // Valor do array: Password -> 0 ; Valor do case: 1
                strcpy_s(sendbuf, DEFAULT_BUFLEN, "\nPassword: ");
                fields--;
                break;
            default:
                break;
            }
            break;
        case isCreatingAccount:
            switch (fields)
            {
            case MAX_NUM_CREATE_ACCOUNT_FIELDS: // Valor do array: Username -> 2 ; Valor do case: 4
                strcpy_s(sendbuf, DEFAULT_BUFLEN, "\nUsername: ");
                fields--;
                break;
            case MAX_NUM_CREATE_ACCOUNT_FIELDS - 1: // Valor do array: Email -> 1 ; Valor do case: 3
                strcpy_s(sendbuf, DEFAULT_BUFLEN, "\nEmail: ");
                fields--;
                break;
            case MAX_NUM_CREATE_ACCOUNT_FIELDS - 2: // Valor do array: Password -> 0 ; Valor do case: 2
                strcpy_s(sendbuf, DEFAULT_BUFLEN, "\nPassword: ");
                fields--;
                break;
            case MAX_NUM_CREATE_ACCOUNT_FIELDS - 3: // Valor do array: Confirm Password -> n�o tem ; Valor do case: 1
                strcpy_s(sendbuf, DEFAULT_BUFLEN, "\nConfirm password: ");
                fields--;
                break;
            default:
                break;
            }
            break;
        default:
            break;
        }


        // ENvio da mensagem de resposta do servidor para o cliente.
        iSendResult = send(current_client, sendbuf, strlen(sendbuf), 0);
        if (iSendResult == SOCKET_ERROR) { // Tratamento de poss�vel erro.
            printf("%d: Send failed: %d\n", GetCurrentThreadId(), WSAGetLastError());
            closesocket(current_client);
            WSACleanup();
            return false;
        }


        // Limpeza dos buffers
        ZeroMemory(recvbuf, DEFAULT_BUFLEN);
        ZeroMemory(sendbuf, DEFAULT_BUFLEN);

        //iRecvResult = recv(current_client, recvbuf, DEFAULT_BUFLEN, 0); // IMPORTANTE: Limpar lixo que est� no buffer.

    } while (iRecvResult > 0);

}

int RegisterUserDataOnFile(char (*data)[DEFAULT_BUFLEN])
{
    FILE* file;

    // Verifica��o se o ficheiro existe
    if ((file = fopen("users.txt", "r")) == NULL)
    {
        // Se o ficheiro n�o existir � criado um novo e aberto
        if ((file = fopen("users.txt", "w")) == NULL)
        {
            return ERROR_SAVING;
        }

        // � guardado o novo utilizador logo no in�cio do ficheiro e � fechado
        fprintf(file, "%s;%s;%s\n", data[2], data[1], data[0]);
        fclose(file);

        return SUCCESS_SAVING;
    }
    else
    {
        // Se o ficheiro j� existir carregamos, de cada vez, um utilizador para a mem�ria, para verificar se j� existe um com o mesmo nome
        // ou email

        // Contamos as linhas do ficheiro para saber quantos utilizadores j� existem
        int numUsers = 0;
        char c;
        while ((c = fgetc(file)) != EOF)
        {
            if (c == '\n')
            {
                numUsers++;
            }
        }

        // Array tempor�rio para guardar os dados de um utilizador
        char dataUser[3][20];

        // Guarda os dados de uma linha do ficheiro
        char buffer[3 * 20];    

        // Usadas para dividir as linhas do ficheiro em campos (nome, email e password)
        char* strings;
        int contador = 0;
  
        for (int i = 0; i < numUsers; i++)
        {
            // Obt�m os dados de �nica linha
            fgets(buffer, sizeof(buffer), file);

            // Divide os dados em campis (nome, email e password)
            strings = strtok(buffer, ";");
            while ((strings != NULL) && (contador < 3))
            {
                strcpy(dataUser[contador], strings);
                contador++;
            }

            // Verifica��o se algum utilizador no ficheiro tem o mesmo username que o usado para registar
            if (strcmp(dataUser[0], data[2]) == 0)
            {
                return USER_ALREADY_EXISTS;
            }

            // Verifica��o se algum utilizador no ficheiro tem o mesmo email que o usado para registar
            if (strcmp(dataUser[1], data[1]) == 0)
            {
                return EMAIL_ALREADY_USED;
            }
        }

        // Se todas as verifica��es passarem, fechamos o ficheiro, abrimos de novo em modo 'append' e adicionamos o novo utilizador ao ficheiro
        fclose(file);

        // Se o ficheiro n�o existir � criado um novo e aberto
        if ((file = fopen("users.txt", "a")) == NULL)
        {
            return ERROR_SAVING;
        }

        // � guardado o novo utilizador no ficheiro e � fechado o ficheiro
        fprintf(file, "%s;%s;%s\n", data[2], data[1], data[0]);
        fclose(file);

        return SUCCESS_SAVING;
    }
}

int ChangeFileData(char* data, int operation)
{
    // Pedido do thread para usar o mutex
    DWORD dwWaitResult;
    dwWaitResult = WaitForSingleObject(ghMutex, INFINITE);

    bool fileResult;
    switch (dwWaitResult)
    {
        // O thread est� a usar o mutex
        case WAIT_OBJECT_0:
            __try 
            {
                
                switch (operation)
                {
                    case REGISTER_USER:
                        fileResult = RegisterUserDataOnFile(data);
                        break;
                }

            }
            __finally 
            {
                // O thread para de usar o mutex, deixando-o dispon�vel para outro thread. 
                if (!ReleaseMutex(ghMutex))
                {
                    return ERROR_RELEASING_MUTEX;
                }
               
                // Retorna ERROR_SAVING ou SUCCESS_SAVING consoante o sucesso das opera��es feitas no ficheiro
                return fileResult;
            }
            break;

        // O thread est� a usar o mutex, mas o mutex encontra-se abandonado (ainda est� a ser usado por um thread que j� n�o existe)
        case WAIT_ABANDONED:
            return ERROR_SAVING;
    }
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
    strcpy_s(sendbuf, DEFAULT_BUFLEN, "100 OK: Connection established\n"); // Preenchimento do buffer de envio com a mensagem que se pretende enviar.
    iSendResult = send(current_client, sendbuf, strlen(sendbuf), 0); // Envio da mensagem contida no array de caracteres "sendbuf" para o cliente com o socket "current_client".
    if (iSendResult == SOCKET_ERROR) { //Interpreta��o do resultado do envio da mensagem de forma a informar poss�veis erros.
        // Em caso de erro:
        printf("%d: Send failed: %d\n", GetCurrentThreadId(), WSAGetLastError());
        closesocket(current_client); // Fechar o socket.
        WSACleanup(); // Limpeza de possiveis recursos alocados.
        return 1;
    }

    // UPDATE: Esta fun��o vai tratar da autentica��o do servidor. Caso n�o tenha conta, cria-a, caso contr�rio autentica-o.
    startAuthentication(current_client);


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

        // Convers�o dos valores para strings e prepar�-las para serem enviadas, no caso do cliente querer ver as suas estat�sticas.
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
                receivedMsgValue = interpreter(1, -1, -1, recvbuf);




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

    // Cria��o do mutex sem dono (n�o est� associado a nenhum thread) e verifica��o de erros
    ghMutex = CreateMutex(NULL, FALSE, NULL);
    if (ghMutex == NULL)
    {
        printf("CreateMutex error: %d\n", GetLastError());
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