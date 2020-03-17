# Sistemas_Distribuidos_Rock_Paper_Scissors_Winsock2_Server
Primeiro projeto da unidade curricular Sistemas Distribuídos. Desenvolvimento do jogo Rock Paper Scissors através da biblioteca winsock e de threads. Documento do servidor.

Comandos do cliente (case insensitive):
- PLAY ROCK
- PLAY PAPER
- PLAY SCISSORS
- RESTART
- END
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
