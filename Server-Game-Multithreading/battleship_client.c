#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <pthread.h>

void getXAndY(char *bbuffer, char *xVal, char *yVal) {
	int offset = 8 - strlen(xVal);
	int i = 0;
	for (i = 0; i < offset; i++) {
		strcat(bbuffer, "0");
	}
	strcat(bbuffer, xVal);
	offset = 8 - strlen(yVal);
	i = 0;
	for (i = 0; i < offset; i++) {
		strcat(bbuffer, "0");
	}
	strcat(bbuffer, yVal);
}

int main(int argc, char **argv)
{
	
	int iFlag=0, jFlag=0, bFlag=0, xFlag=0, yFlag=0;
	
	int o;
	
	int cli;
	
	int testval; // Resultado das chamadas para a função socket, testado quando a erros
	int size;
	
	char path[19] = "./cli_socket_";
	
	char *identifier = NULL;
	
	char *xVal = NULL;
	char *yVal = NULL;
	
	char bbuffer[21];	
	
	struct sockaddr_un srvaddr;
	struct sockaddr_un cliaddr;

	// verificação rápida de erros para ver se a chamada está dentro do número esperado de argumentos
	// para tornar a análise arg do comando getopt mais fácil

	if (!(argc >= 3)) {
		fprintf(stderr,
			"Erro: poucos argumentos dados a num_client\n");
		return 1;
	} else if (!(argc <= 8)) {
		fprintf(stderr,
			"Erro: muitos argumentos dados a num_client\n");
		return 1;
	}

	while ((o = getopt(argc, argv, "i:jbx:y:")) != -1) {

		switch (o) {
		case 'i':
			iFlag = 1;
			identifier = optarg;
			break;

		case 'j':
			jFlag = 1;
			break;

		case 'b':
			bFlag = 1;
			break;

		case 'x':
			xFlag = 1;
			xVal = optarg;
			break;

		case 'y':
			yFlag = 1;
			yVal = optarg;
			break;

		case '?':
			if (optopt == 'o')
				fprintf(stderr, "-%c needs an argument\n",
					optopt);
			else if (isprint(optopt))
				fprintf(stderr, "Invalid option: -%c\n",
					optopt);
			else
				fprintf(stderr,
					"Error parsing cmdline arguments given\n");
			return 1;
		}
	}		

	if (!iFlag) {
		fprintf(stderr,
			"Error: must specify your unique identifier with -i flag and \
			optarg\n");
	} else if (jFlag && bFlag) {
		fprintf(stderr,
			"Error: cannot use both -j and -b simultaneously\n");
		return 1;
	} else if (bFlag && (!xFlag || !yFlag)) {
		fprintf(stderr,
			"Error: cannot use -b without specifying both the -x and -y \
			argument and their optargs\n");
	}
	// fim do loop while para analisar os argumentos da linha de comando
	// ----------------------------------------------------------------


	// Inicializa o socket do cliente e configura a estrutura

	cli = socket(AF_UNIX, SOCK_STREAM, 0);
	cliaddr.sun_family = AF_UNIX;
	strcpy(cliaddr.sun_path, strcat(path, identifier));
	size = sizeof(cliaddr);

	// Vincula o socket do cliente a ./cli_socket
	unlink(cliaddr.sun_path);
	testval = bind(cli, (struct sockaddr *)&cliaddr, size);
	if (testval == -1) {
		fprintf(stderr, "Error binding client socket\n");
		close(cli);
		return 1;
	}
	
	srvaddr.sun_family = AF_UNIX;
	strcpy(srvaddr.sun_path, "./srv_socket");

	// Conecta ao soquete do servidor
	testval = connect(cli, (struct sockaddr *)&srvaddr, size);
	if (testval == -1) {
		fprintf(stderr, "Error connecting to server socket\n");
		close(cli);
		return 1;
	}

	//Envia dados para o servidor
	if (jFlag == 1) {
		strcpy(bbuffer, "JOIN");

	
		if (xVal != NULL && yVal != NULL) {
			getXAndY(bbuffer, xVal, yVal);

		// Se a solução opcional x e y para ingressar no player não foi especificada, defina como 0.
		// o servidor então atribuirá ao jogador uma solução aleatória.
		} else {
			int i = 0;
			for (i = 0; i < 16; i++) {
				strcat(bbuffer, "0");
			}
		}

	} else if (bFlag == 1) {
		strcpy(bbuffer, "BOMB");


		getXAndY(bbuffer, xVal, yVal);
	}

	//Envia requisição para o servidor
	testval = send(cli, bbuffer, strlen(bbuffer), 0);
	if (testval == -1) {
		fprintf(stderr, "Error sending request to server socket\n");
		close(cli);
		return 1;
	}
	// Terminada a interação com o servidor; fechar soquete do cliente
	close(cli);

	return 0;
}
