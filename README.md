# Sistemas_Distribuidos_Rock_Paper_Scissors_Winsock2_Server
Primeiro projeto da unidade curricular Sistemas Distribuídos. Desenvolvimento do jogo Rock Paper Scissors através da biblioteca winsock e de threads. Documento do servidor.

Comandos do cliente (case insensitive):
- PLAY ROCK
- PLAY PAPER
- PLAY SCISSORS
- RESTART
- END
- STATS
- HELP

Projeto baseado no tutorial da Microsoft para Winsock: https://docs.microsoft.com/en-us/windows/win32/winsock/getting-started-with-winsock

Criação de Threads baseada no tutorial: http://www.rohitab.com/discuss/topic/26991-cc-how-to-code-a-multi-client-server-in-c-using-threads/


Para testar pode ser usado o putty com os seguintes dados: 
- Host Name (or IP address): localhost;
- Port: DEFAULT_PORT (definida no código);
- Connection type: Raw;
- Close window on exit: Never.

Antes de escrever algo no terminal do putty deve-se esperar alguns segundos até aparecer uma mensagem de conexão estabelecida.
Antes de estabelecer a ligação do putty deve pôr-se o programa a executar.

Pode também executar-se o código relativo ao cliente de forma a estabelecer comunicação com o servidor: https://github.com/LuisMRMoreira/Sistemas_Distribuidos_Rock_Paper_Scissors_Winsock2_Client

Download putty: https://www.chiark.greenend.org.uk/~sgtatham/putty/latest.html


*********************************************************************************************************
****************************************** NOVA VERSÃO **************************************************
*********************************************************************************************************

Nesta nova versão foi adicionado autenticação entre o cliente e o servidor. Desta forma, para que o cliente possa jogar, tem de se autenticar.

O novo comando adicionado foi:
- <-BACK: Indica a intenção do cliente, durante o preenchimento de um formulário, de voltar a preenceher o campo anterior.

Para já falta escrever em ficheiro a criação das contas, assim como a sua pesquisa.
Esta versão também terá que guardar em ficheiro os números (estatisticas de jogo) de cada jogador.


