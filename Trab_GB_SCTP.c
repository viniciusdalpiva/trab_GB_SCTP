/*
Para compilar, precisa da bibliteca libsctp-dev
gcc nome.c -o nome -lsctp -lpthread
*/

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <netinet/sctp.h>
#define ECHOMAX 1024


int numeroPeers;
int loc_sockfd, loc_newsockfd;
char *codIP[15];
struct sctp_initmsg initmsg;
struct sockaddr_in me, target, local, destino;
pthread_t threads[2];

//--------------------------MAIN--------------------------
int main(int argc, char *argv[])
{    
	//Pede do teclado quantos peers serão conectados e seus IPs	
	char numPeers[10]; 
    printf("Digite o número de peers que serão conectados: \n");
    fgets(numPeers, 10, stdin);    

    int auxnumpeer = atoi(numPeers);	
    numeroPeers = auxnumpeer-1;
	char charIP[15];    	
	
	//For que irá alocar o IP
    for(int i = 1; i < numeroPeers +1; i++)
    {		
		printf("Digite o IP do peer %d: ", i);			
		fgets(charIP, ECHOMAX, stdin);		
        codIP[i-1] = strdup(charIP);		
    }
    system("clear");	//limpa a tela

	/* Construcao da estrutura do endereco local */
	/* Preenchendo a estrutura socket loc_addr (familia, IP, porta) */
	local.sin_family = AF_INET, /* familia do protocolo */
	local.sin_addr.s_addr = INADDR_ANY, /* endereco IP local */
	local.sin_port = htons(9999), /* porta local */	
	
	//ESTRUTURA DE INICIALIZAÇÃO DO SCTP
	/* Novas Estrutura: sctp_initmsg, contém informações para a inicialização de associação*/
	initmsg.sinit_num_ostreams = 5, /* Número de streams que se deseja mandar. */
	initmsg.sinit_max_instreams = 5, /* Número máximo de streams se deseja receber. */
	initmsg.sinit_max_attempts = 4, /* Número de tentativas até remandar INIT. */	

	//CRIAÇÃO DO SOCKET
	/* Cria o socket para enviar e receber datagramas */
	/* parametros(familia, tipo, protocolo) */
	loc_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP); // Mudança protocolo '0' => 'IPPROTO_SCTP'
	
	if (loc_sockfd < 0) {
		perror("Criando stream socket");
		exit(1);
	}

	//BIND DA ESTRUTURA - ASSOCIAÇÃO COM O SOCKET
   	/* Bind para o endereco local*/
	/* parametros(descritor socket, estrutura do endereco local, comprimento do endereco) */
	if (bind(loc_sockfd, (struct sockaddr *) &local, sizeof(struct sockaddr)) < 0) {
		perror("Ligando stream socket");
		exit(1);
	}
	
	//SETANDO AS CONFIGS DO SOCKET
	/* SCTP necessita usar setsockopt, */
	/* parametros(descritor socket, protocolo, tipo de opção(INIT, EVENTS, etc.), opções, tamanho das opções) */
  	if (setsockopt (loc_sockfd, IPPROTO_SCTP, SCTP_INITMSG, &initmsg, sizeof (initmsg)) < 0){
		perror("setsockopt(initmsg)");
		exit(1);
  	} 

//--------------------------MÉTODO DE RECEBIMENTO DE COMANDOS--------------------------
void *recebe(void *arg) { 
	char recebeLinha[ECHOMAX];
	
    do{         		
		/* parametros(descritor socket,	numeros de conexoes em espera sem serem aceites pelo accept)*/
		listen(loc_sockfd, initmsg.sinit_max_instreams);

		/* Accept permite aceitar um pedido de conexao, devolve um novo "socket" ja ligado ao emissor do pedido e o "socket" original*/
		/* parametros(descritor socket, estrutura do endereco local, comprimento do endereco)*/
		int tamanho = sizeof(struct sockaddr_in);
		loc_newsockfd = accept(loc_sockfd, (struct sockaddr *)&local, &tamanho);		           
        sctp_recvmsg(loc_newsockfd, &recebeLinha, sizeof(recebeLinha), NULL, 0, 0, 0);		
		
		//Envia o output do console p/ variável através do file
        system(recebeLinha);
        //-----------
        FILE* output = NULL;
        char buffer[ECHOMAX];		
        output = popen(recebeLinha, "r");
        if(output)
        {
            int intaux = 0;
            char charaux;
            while((charaux = fgetc(output)) != EOF)
            {
                buffer[intaux] = charaux;
                intaux++;
            } 
            buffer[intaux] = '\0';			
        }		
        //-------------------        
        sctp_sendmsg(loc_newsockfd, &buffer, sizeof(buffer), NULL, 0, 0, 0, 0, 0, 0);
    }
    while(strcmp(recebeLinha,"exit"));
}

//--------------------------MÉTODO DE ENVIO DE COMANDOS--------------------------
void *envia(void *arg) {    
	char enviaLinha[ECHOMAX], linhaRetorno[ECHOMAX];
	int rem_sockfd;
    
    do{              
		//Lê informação do teclado e executa o comando digitado		
        fgets(enviaLinha, ECHOMAX, stdin);	
		
        printf("\n==================================\n");
		printf("Retorno do comando neste computador:\n\n");        			
        system(enviaLinha);
        printf("\n==================================\n");		
        
        //For para realizar o envio para cada Peer
        for(int i = 0; i < numeroPeers; i++){            
			/* Construcao da estrutura do endereco local */
			/* Preenchendo a estrutura socket loc_addr (familia, IP, porta) */
			destino.sin_family = AF_INET, /* familia do protocolo*/
			destino.sin_addr.s_addr = inet_addr(codIP[i]), /* endereco IP local */
			destino.sin_port = htons(9999), /* porta local  */			

			/* Cria o socket para enviar e receber fluxos */
			/* parametros(familia, tipo, protocolo) */
			rem_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
			if (rem_sockfd < 0) {
				perror("Criando stream socket");
				exit(1);
			}
			
			if (connect(rem_sockfd, (struct sockaddr *) &destino, sizeof(destino)) < 0) {
				perror("Conectando stream socket");
				exit(1);
			}
				
			sctp_sendmsg(rem_sockfd, &enviaLinha,   sizeof(enviaLinha),   NULL, 0, 0, 0, 0, 0, 0);                
			sctp_recvmsg(rem_sockfd, &linhaRetorno, sizeof(linhaRetorno), NULL, 0, 0, 0);
			
            //print mensagem recebida do IP em i
			printf("\n");
			printf("\n==================================\n");
			printf("Mensagem recebida de - %s__________________________________\n", codIP[i]);    
			printf(linhaRetorno);
			printf("==================================\n");
			close(rem_sockfd);
        }
    }while(strcmp(enviaLinha,"exit"));	
}
	
//--------------------------THREADS--------------------------	
    pthread_create(&(threads[0]), NULL, recebe, NULL);
    pthread_create(&(threads[1]), NULL, envia, NULL);
	
    printf("Você está conectado! \nDigite algum comando válido: ");	
	
    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);
    close(loc_sockfd);
	close(loc_newsockfd);	
    return 0;
}