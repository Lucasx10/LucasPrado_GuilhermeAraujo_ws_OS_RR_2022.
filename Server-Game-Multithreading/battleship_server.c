#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>

#include "linkedlist.h"


pthread_mutex_t list_lock;	//atomicidade da lista encadeada

pthread_mutex_t thread_lock;  //atomicidade das op threads

pthread_mutex_t srv_cli_lock; //para evitar data races envolvendo dados passados 
							  //de threads principais para filhos

pthread_mutex_t log_file_lock; //atomicidade das op log file


extern struct node *head;
extern struct node *current;

//arquivo de log para gravar structs necessarias para gerar time preciso
FILE *log_file;
time_t ltime;
struct tm result;
char stime[32];

// Struct usada para enviar dados junto com threads criados
// para client_handler()
typedef struct {
	int *srv;
	int *cli;
	char *client_identifier;
} sock_info;

// Rotina de interrupção SIGINT - deve sair com segurança se encontrar sigint
static void sigint_handler(int signal, siginfo_t * t, void *arg)
{
	fprintf(log_file,
		"\nSIGINT signal encountered, exiting gracefully...\n");
	pthread_mutex_unlock(&srv_cli_lock);
	while (head != NULL) {
		deleteNode(head);
		head = head->next;
	}
	fclose(log_file);
	exit(0);
}

// Rotina de interrupção SIGHUP - deve sair com segurança se encontrar sighup
static void sighup_handler(int signal, siginfo_t * t, void *arg)
{
	fprintf(log_file,
		"\nSIGHUP signal encountered, exiting gracefully...\n");
	pthread_mutex_unlock(&srv_cli_lock);
	fclose(log_file);
	exit(0);
}
// Retorna um timestamp preciso e seguro para threads para uso quando
// gravando no arquivo de log.
char *getTime()
{
	ltime = time(NULL);
	localtime_r(&ltime, &result);
	return asctime_r(&result, stime);
}
/*
Função de thread de destino para threads filhos gerados pelo processo principal
para lidar com conexões de clientes de entrada. Conexões de cliente de entrada
terá instruções JOIN ou BOMB que devem ser analisadas
e, em seguida, aplicado ao servidor do jogo.
*/
void *handle_client(void *package)
{
	char identifier[5];	
	char x_val[9];		
	char y_val[9];		
	int x_val_int;		
	int y_val_int;		
	char bbuffer[21];	

	// Desempacota a estrutura sock_info contendo informações do main
	// em relação aos sockets do servidor e do cliente
	sock_info *info = package;
	int client = *info->cli;
	strncpy(identifier, info->client_identifier, 4);
	recv(client, bbuffer, sizeof(bbuffer), 0);

	pthread_mutex_unlock(&srv_cli_lock);	

	if (strncmp(bbuffer, "JOIN", 4) == 0) {

		pthread_mutex_lock(&list_lock);
		
		//verificação do buffer e recuperação dos valores x e y do navio
		int j;	
		int x = 0;
		for (j = 4; j < 12; j++) {
			x_val[x] = bbuffer[j];
			x++;
		}
		x = 0;
		for (j = 12; j < 20; j++) {
			y_val[x] = bbuffer[j];
		}
		x_val_int = atoi(x_val);
		y_val_int = atoi(y_val);

		// Se nenhum valor de solução foi especificado pelo jogador que entrou, atribua
		// valores aleatorios de solução x e y
	
		if (x_val_int == 0 && y_val_int == 0) {
			x_val_int = rand() % (10 + 1 - 1) + 1;
			y_val_int = rand() % (10 + 1 - 1) + 1;
		}
		insertHead(identifier, x_val_int, y_val_int);

		pthread_mutex_lock(&log_file_lock);
		fprintf(log_file,
			"%s\t: => %s entrou no jogo. Seu navio está localizado em x = %d and y = %d.\n",
			getTime(), identifier, x_val_int, y_val_int);
		pthread_mutex_unlock(&log_file_lock);

		if (head->next != head && head->next->next == head) {
			// neste caso passamos de 1 jogador para 2, então o jogo é iniciado
			current = head->last;
			pthread_mutex_lock(&log_file_lock);
			fprintf(log_file,
				"%s\t: => O jogo agora atingiu o mínimo de dois jogadores. Seu status agora está ativo.\n",
				getTime());
			pthread_mutex_unlock(&log_file_lock);
		}
		pthread_mutex_unlock(&list_lock);

	} else if (strncmp(bbuffer, "BOMB", 4) == 0) {
		pthread_mutex_lock(&list_lock);
	
		if (head == NULL) {
			pthread_mutex_lock(&log_file_lock);
			fprintf(log_file, "%s\t:=> %s tentei usar o comando bomb, mas ninguém entrou no jogo ainda!\n", getTime(), identifier);
			pthread_mutex_unlock(&log_file_lock);
		} else if (head->next == NULL) {
			pthread_mutex_lock(&log_file_lock);
			fprintf(log_file, "%s\t:=> %s tentei usar o comando bomb, mas apenas uma pessoa está no jogo! É necessário um mínimo de 2 pessoas.\n", getTime(), identifier);
			pthread_mutex_unlock(&log_file_lock);
		} else if (head->next != head) {
			//Verifica se é a vez do cliente, se não, repassa essa informação
			if (strncmp(current->identifier, identifier, 4) == 0) {

				// Pega as coordenadas x e y enviadas pelo cliente para bombardear
				int j;	
				int x = 0;	
				for (j = 4; j < 12; j++) {	
					x_val[x] = bbuffer[j];
					x++;
				}
				x = 0;
				for (j = 12; j < 20; j++) {	
					y_val[x] = bbuffer[j];
				}
				x_val_int = atoi(x_val);
				y_val_int = atoi(y_val);

				// Determina se o tiro foi um acerto e toma a ação correspondente
				int hit = 0;
				if (x_val_int == current->next->x_solution
				    && y_val_int == current->next->y_solution)
					hit = 1;

				fprintf(log_file,
					"%s\t: => É a vez de %s e ele bombardeou %s com valores x = %d e y = %d.\n",
					getTime(), current->identifier,
					current->next->identifier, x_val_int,
					y_val_int);

				// Se não for um acerto, simplesmente mude a vez atual para a próxima pessoa no jogo
				if (hit == 0) {
					pthread_mutex_lock(&log_file_lock);
					fprintf(log_file,
						"%s\t: => %s perdeu %s. Agora é a vez de %s.\n",
						getTime(), current->identifier,
						current->next->identifier,
						current->next->identifier);
					pthread_mutex_unlock(&log_file_lock);
					current = current->next;

				// Se for um acerto e houver apenas uma pessoa restante, então o jogador venceu!
				// Caso contrário, remova o jogador atingido do jogo e passe para a vez do outro jogador
				// depois daquele que morreu.
				} else if (hit == 1) {
				
					if (current->next->next == current) {
						pthread_mutex_lock
						    (&log_file_lock);
						fprintf(log_file,
							"%s\t: => %s atingiu o navio de %s! %s vence o jogo, pois é o único sobrevivente restante! Aguardando mais desafiantes...",
							getTime(),
							current->identifier,
							current->next->
							identifier,
							current->identifier);
						pthread_mutex_unlock
						    (&log_file_lock);
					} else {
						pthread_mutex_lock
						    (&log_file_lock);
						fprintf(log_file,
							"%s\t: => %s atingiu o navio de %s! %s está fora do jogo. %s agora está atirando em %s, e agora é a vez de %s.\n",
							getTime(),
							current->identifier,
							current->next->
							identifier,
							current->next->
							identifier,
							current->identifier,
							current->next->next->
							identifier,
							current->next->next->
							identifier);
						pthread_mutex_unlock
						    (&log_file_lock);
						current = current->next->next;
						deleteNode(current->last);
					}
				}
			
			} else {
				pthread_mutex_lock(&log_file_lock);
				fprintf(log_file,
					"%s\t:=> %s tentou jogar a sua vez, mas não é a vez dele.\n",
					getTime(), identifier);
				pthread_mutex_unlock(&log_file_lock);
			}
		} else {
			pthread_mutex_lock(&log_file_lock);
			fprintf(log_file,
				"%s\t:=> %s tentou dar uma volta, mas não há outros jogadores no jogo. Pelo menos 2 devem se juntar primeiro.\n",
				getTime(), identifier);
		}
		pthread_mutex_unlock(&list_lock);
	}

	close(client);		

	free(info);
	return NULL;
}

int main(void)
{

	socklen_t size;
	int srv, cli;

	struct sockaddr_un srvaddr, cliaddr;

	int num_threads = 0;
	pthread_t threads[1024];

	
	srand(time(0));

	
	pthread_mutex_init(&list_lock, NULL);
	pthread_mutex_init(&thread_lock, NULL);
	pthread_mutex_init(&srv_cli_lock, NULL);

	// Cria uma struct para o tratamento de sinais de sinal
	// (deve chamar sigint_handler para sair normalmente)
	struct sigaction sa_sigint;
	memset(&sa_sigint, 0, sizeof(sa_sigint));
	sigemptyset(&sa_sigint.sa_mask);
	sa_sigint.sa_sigaction = sigint_handler;
	sa_sigint.sa_flags = SA_SIGINFO;
	sigaction(SIGINT, &sa_sigint, NULL);

	// Cria um struct para o tratamento de sinais de sighup
	// (deve chamar sighup_handler para imprimir os números armazenados
	// na ordem)	
	struct sigaction sa_sighup;
	memset(&sa_sighup, 0, sizeof(sa_sighup));
	sigemptyset(&sa_sighup.sa_mask);
	sa_sighup.sa_sigaction = sighup_handler;
	sa_sighup.sa_flags = SA_SIGINFO;
	sigaction(SIGHUP, &sa_sighup, NULL);

	log_file = fopen("battleship_server.log", "a+");	
	if (log_file == NULL) {
		fprintf(stderr, "There was an error opening the log file.\n");
		exit(1);
	}
	
	srv = socket(AF_UNIX, SOCK_STREAM, 0);

	strcpy(srvaddr.sun_path, "./srv_socket");
	srvaddr.sun_family = AF_UNIX;
	size = sizeof(srvaddr);

	
	unlink("./srv_socket");	
	if (bind(srv, (struct sockaddr *)&srvaddr, size) != 0) {
		fprintf(log_file,
			"%s\t: => Error binding server socket. Exiting with non-zero exit status 1.",
			getTime());
		exit(1);
	}
	
	listen(srv, 100);
	fprintf(log_file,
		"%s\t: => Began listening on ./srv_socket for incoming client connections.\n",
		getTime());

	fprintf(stdout, "listening on ./srv_socket\n");

	
	while (1) {

		
		cli = accept(srv, (struct sockaddr *)&cliaddr, &size);

		pthread_mutex_lock(&srv_cli_lock);

		// Informações do pacote necessárias para a thread handle_client
		//função do trabalhador em uma estrutura que pode aceitar como um
		// argumento
		sock_info *package = malloc(sizeof *package);
		int srv_copy = srv;
		int cli_copy = cli;
		package->srv = &srv_copy;
		package->cli = &cli_copy;
		package->client_identifier = malloc(5);
		
		package->client_identifier =
		    cliaddr.sun_path + strlen(cliaddr.sun_path) - 4;
		// Cria um novo thread e inicia-o handle_client para lidar com o
		// cliente entrante
		pthread_mutex_lock(&thread_lock);
		pthread_create(&threads[num_threads], NULL,
			       handle_client, package);
		num_threads++;
		pthread_mutex_unlock(&thread_lock);
	}
}
