/* librairie standard ... */
#include <stdlib.h>
/* pour getopt */
#include <unistd.h>
/* déclaration des types de base */
#include <sys/types.h>
/* constantes relatives aux domaines, types et protocoles */
#include <sys/socket.h>
/* constantes et structures propres au domaine UNIX */
#include <sys/un.h>
/* constantes et structures propres au domaine INTERNET */
#include <netinet/in.h>
/* structures retournées par les fonctions de gestion de la base de
données du réseau */
#include <netdb.h>
/* pour les entrées/sorties */
#include <stdio.h>
/* pour la gestion des erreurs */
#include <errno.h>
#include <string.h>


int sock;
struct hostent *hp;
struct sockaddr_in adr_local;
struct sockaddr_in adr_distant;
int lg_rec;
int max=10;

int lg_adr_distant = sizeof(adr_distant);
char * M;
int lg_emis;

char * pmesg;
int option=0;
struct sockaddr* padr_em;
int * plg_adr_em;
// char letters[26] = ['a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
//  'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'];
char letters[26] = "abcdefghijklmnopqrstuvwxyz";


void construire_message(char * M, int lg_M, int cas) {
    // penser à vider M
    strcpy(M, "");
    int indice = cas%26;
    char nombreMessage[5] = "";
    char caracteres[lg_M-5] = "";

    printf("Case : %d\n", indice);
    sprintf(nombreMessage, "%5d", cas);
    printf("nb message %s\n", nombreMessage);

    for (int i=0;i<lg_M-5;i++) {
        caracteres[i] = letters[indice];
    }
    // Il faut ensuite construire le message M à partir de nombreMessage et caracteres


}

void afficher_message(char * message, int lg) {
    int i;
    for (i=0;i<lg;i++) printf("%c", message[i]);printf("\n");
}

int createSocket(int source, int udp, int lg_M, int nb_messageSender, int nb_messageReceiver) {
        int mySocket, sock_bis;


        printf("lg_M socket : %d\n", lg_M);
    
        // Puit
        if (udp == 1 && source == 0) {
            mySocket = socket(AF_INET, SOCK_DGRAM, 0);
            memset((char *)& adr_local, 0, sizeof(adr_local)) ;
            adr_local.sin_family = AF_INET ; /* domaine Internet*/
            adr_local.sin_port = 9000 ; /* n° de port */
            adr_local.sin_addr.s_addr = INADDR_ANY;
            pmesg = (char*)malloc(lg_M*sizeof(char));

            int ret = bind(mySocket, (struct sockaddr *)&adr_local, sizeof(adr_local));
            
            for (int i=0;i<nb_messageReceiver;i++) {
                printf("%d\n", (int)recvfrom(mySocket, pmesg, lg_M, option, NULL, NULL));
                afficher_message(pmesg, lg_M);
            }

        // Source
        } else if (udp == 1 && source == 1) {
            mySocket = socket(AF_INET, SOCK_DGRAM, 0);
            memset((char *)&adr_distant, 0, sizeof(adr_distant)) ;
            adr_distant.sin_family = AF_INET ; /* domaine Internet*/
            adr_distant.sin_port = 9000 ; /* n° de port */
            if ((hp = gethostbyname("localhost")) == NULL) {
                printf("erreur gethostbyname\n");
                exit(1);
            }
            memcpy((char*)&(adr_distant.sin_addr.s_addr), hp->h_addr, hp->h_length);
            M = (char*)malloc(lg_M*sizeof(char));
            /* Envoi de données */
            
            for (int i=0;i<nb_messageSender;i++) {
                construire_message(M, lg_M, i);
                lg_emis = sendto(mySocket, M, lg_M, 0, (struct sockaddr*)&adr_distant, lg_adr_distant);
            }

        } else if (udp == 0 && source == 0) {
            mySocket = socket(AF_INET, SOCK_STREAM, 0);
            memset((char *)& adr_local, 0, sizeof(adr_local)) ;
            adr_local.sin_family = AF_INET ; /* domaine Internet*/
            adr_local.sin_port = 9000 ; /* n° de port */
            adr_local.sin_addr.s_addr = INADDR_ANY;
            int ret = bind(mySocket, (struct sockaddr *)&adr_local, sizeof(adr_local));

            pmesg = (char*)malloc(lg_M*sizeof(char));
            listen(mySocket, 5);
            sock_bis = accept(mySocket, NULL, NULL);

            for (int i=0;i<nb_messageReceiver;i++) {
                printf("Nombre d'octets reçus avec TCP : %d\n", (int)read(sock_bis, pmesg, lg_M));
                afficher_message(pmesg, lg_M);
            }

        } else if (udp == 0 && source == 1) {
            mySocket = socket(AF_INET, SOCK_STREAM, 0);
            memset((char *)&adr_distant, 0, sizeof(adr_distant)) ;
            adr_distant.sin_family = AF_INET ; /* domaine Internet*/
            adr_distant.sin_port = 9000 ; /* n° de port */
            if ((hp = gethostbyname("localhost")) == NULL) {
                printf("erreur gethostbyname\n");
                exit(1);
            }
            memcpy((char*)&(adr_distant.sin_addr.s_addr), hp->h_addr, hp->h_length);
            M = (char*)malloc(lg_M*sizeof(char));
            int isConnected=0;
            isConnected = (int)connect(mySocket, (struct sockaddr *)&adr_distant, sizeof(adr_distant));
            for (int i=0;i<nb_messageSender;i++) {
                construire_message(M, lg_M, i);
                afficher_message(M, lg_M);
                if(isConnected == 0) {
                    printf("Nombre d'octets envoyés avec TCP : %ld\n", write(mySocket, M, lg_M));
                } 
            }
        }

        


        

        return mySocket;
        
}

void main (int argc, char **argv)
{
	int c;
	extern char *optarg;
	extern int optind;
	int nb_messageSender = 10; /* Nb de messages à envoyer ou à recevoir, par défaut : 10 en émission, infini en réception */
	int nb_messageReceiver = 100000000;
    int source = -1 ; /* 0=puits, 1=source */
    int udp = 0; /* 0 = pas UDP, 1 = UDP */
    int n=0;
    int l=0;
    int nValue;
    int lValue;

    int sock;
    struct hostent *hp;
    struct sockaddr_in adr_local;

    int lg_M = 30;


	while ((c = getopt(argc, argv, "pn:sul:")) != -1) {
		switch (c) {
		case 'p':
			if (source == 1) {
				printf("usage: cmd [-p|-s][-n ##]\n");
                

			}
			source = 0;
            
			break;

		case 's':
			if (source == 0) {
				printf("usage: cmd [-p|-s][-n ##]\n");
				exit(1) ;
			}
			source = 1;
            


			break;

		case 'n':
			nb_messageSender = atoi(optarg);
            nb_messageReceiver = atoi(optarg);
            printf("Nombre messages envoyés : %d\n", nb_messageSender);
            printf("Nombre messages reçus : %d\n", nb_messageReceiver);
			break;

        case 'u':
            udp=1;
        break;

        case 'l':
            if(atoi(optarg) >= 0) {
                lg_M = atoi(optarg) + 5;
                printf("lg_M : %d\n", lg_M);
            }

			break;
    
		default:
			printf("usage: cmd [-p|-s][-n ##]\n");
			break;
		}
	}


    sock = createSocket(source, udp, lg_M, nb_messageSender, nb_messageReceiver);

	if (source == -1) {
		printf("usage: cmd [-p|-s][-n ##]\n");
		exit(1) ;
	}

	if (source == 1)
		printf("on est dans le source\n");
	else
		printf("on est dans le puits\n");

	if (nb_messageSender != -1) {
		if (source == 1)
			printf("nb de tampons à envoyer : %d\n", nb_messageSender);
		else
			printf("nb de tampons à recevoir : %d\n", nb_messageSender);
	} else {
		if (source == 1) {
			nb_messageSender = 10 ;
			printf("nb de tampons à envoyer = 10 par défaut\n");
		} else
		printf("nb de tampons à envoyer = infini\n");

	}
}