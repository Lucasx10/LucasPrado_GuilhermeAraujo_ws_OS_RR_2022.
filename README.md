# Servidor De Jogo Multithreading para match
## Descrição
 Servidor Game Multithreading feito na linguagem C. Consiste em um programa de servidor que lida com conexões e comandos de vários programas cliente, e pode lidar com esses clientes conexões simultaneamente.
Pode ser usado por diferentes usuários em um sistema Unix compartilhado no terminal para jogar uma versão do "batalha naval" multiplayer que consiste em jogadores criam seus navios em localizações X e Y e tentam bombardear o navio dos outros jogadores. 

## Implementação 
- [battleship_client.c](Server-Game-Multithreading/battleship_client.c), - Leva argumentos de linha de comando que especificam uma operação especificada por um player para que o servidor processe.
- [battleship_server.c](Server-Game-Multithreading/battleship_server.c), - É o código mestre executável que é capaz de aceitar múltiplas conexões de clientes simultaneamente.
- [linkedlist.c](Server-Game-Multithreading/linkedlist.c), - Para armazenar os identificadores exclusivos do cliente e com quem eles estão vinculados. 
- [makefile](Server-Game-Multithreading/makefile), - Compilará os battleship_server e battleship_client executáveis e suas dependências, cria os players e simula uma situação de jogo.

**Instruções de teste**

Para testar a execução, comandos linux:

   `make clean`

   `make all`

   `make test`

## Resultado do teste
![teste]