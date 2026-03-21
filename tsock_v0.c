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

int lg_adr_distant = sizeof(adr_distant);
char *M;
char *nom_destinataire = "localhost";
int port = 9000;

char *pmesg;
int option = 0;
struct sockaddr *padr_em;
int *plg_adr_em;
char letters[26] = "abcdefghijklmnopqrstuvwxyz";

void construire_message(char *M, int lg, int msgNumero)
{
    int indice = msgNumero % 26;
    char partieNombre[6]; // la derniere element est \0
    sprintf(partieNombre, "%5d", msgNumero + 1);
    memcpy(M, partieNombre, 5);
    memset(M + 5, letters[indice], lg - 5);
    printf("Envoie n°%d (%d) [%s]\n", msgNumero + 1, lg, M);
}

void afficher_message(char *message, int lg, int numero_msg_recu)
{
    printf("Reception n°%d (%d) [%s]\n", numero_msg_recu + 1, lg, message);
}

void afficher_params(int isSource, int lg, int nb_msg, int isUDP)
{
    if (isSource == 1)
    {
        printf("lg_msg_emis=%d, port=%d, nb_envois=%d, TP=%s, dest=%s\n", lg, port, nb_msg, (isUDP == 1) ? "udp" : "tcp", nom_destinataire);
    }
    else
    {
        printf("lg_msg_lu=%d, port=%d, nb_receptions=%d, TP=%s\n", lg, port, nb_msg, (isUDP == 1) ? "udp" : "tcp");
    }
}

int createSocket(int source, int udp, int lg_M, int nb_messageSender, int nb_messageReceiver)
{
    int mySocket, sock_bis;

    // printf("lg_M socket : %d\n", lg_M);

    // Puit (UDP)
    if (udp == 1 && source == 0)
    {
        mySocket = socket(AF_INET, SOCK_DGRAM, 0);
        memset((char *)&adr_local, 0, sizeof(adr_local));
        adr_local.sin_family = AF_INET;   /* domaine Internet*/
        adr_local.sin_port = htons(port); /* n° de port */
        adr_local.sin_addr.s_addr = INADDR_ANY;
        pmesg = (char *)malloc(lg_M * sizeof(char));

        int ret = bind(mySocket, (struct sockaddr *)&adr_local, sizeof(adr_local));
        if (ret < 0)
        {
            perror("Erreur bind (UDP puits)");
            exit(1);
        }

        afficher_params(source, lg_M, nb_messageReceiver, udp);
        for (int i = 0; i < nb_messageReceiver; i++)
        {
            // printf("%d\n", (int)recvfrom(mySocket, pmesg, lg_M, option, NULL, NULL));
            recvfrom(mySocket, pmesg, lg_M, option, NULL, NULL);
            afficher_message(pmesg, lg_M, i);
        }
    }
    // Source (UDP)
    else if (udp == 1 && source == 1)
    {
        mySocket = socket(AF_INET, SOCK_DGRAM, 0);
        memset((char *)&adr_distant, 0, sizeof(adr_distant));
        adr_distant.sin_family = AF_INET;   /* domaine Internet*/
        adr_distant.sin_port = htons(port); /* n° de port */
        if ((hp = gethostbyname(nom_destinataire)) == NULL)
        {
            printf("erreur gethostbyname\n");
            exit(1);
        }
        memcpy((char *)&(adr_distant.sin_addr.s_addr), hp->h_addr, hp->h_length);
        M = (char *)malloc(lg_M * sizeof(char));
        /* Envoi de données */

        afficher_params(source, lg_M, nb_messageSender, udp);
        for (int i = 0; i < nb_messageSender; i++)
        {
            construire_message(M, lg_M, i);
            int bytes_sent = sendto(mySocket, M, lg_M, 0, (struct sockaddr *)&adr_distant, lg_adr_distant);
            if (bytes_sent < 0)
            {
                perror("Erreur sendto");
                break;
            }
        }

        free(M);
    }
    // Puit (TCP)
    else if (udp == 0 && source == 0)
    {
        mySocket = socket(AF_INET, SOCK_STREAM, 0);
        memset((char *)&adr_local, 0, sizeof(adr_local));
        adr_local.sin_family = AF_INET;   /* domaine Internet*/
        adr_local.sin_port = htons(port); /* n° de port */
        adr_local.sin_addr.s_addr = INADDR_ANY;

        int ret = bind(mySocket, (struct sockaddr *)&adr_local, sizeof(adr_local));
        if (ret < 0)
        {
            perror("Erreur bind (TCP puits)");
            exit(1);
        }

        pmesg = (char *)malloc(lg_M * sizeof(char));
        listen(mySocket, 5);

        afficher_params(source, lg_M, nb_messageReceiver, udp);
        int total_msg = 0;
        int sock_bis;
        sock_bis = accept(mySocket, NULL, NULL);
        // while: boucle pour recevoir des messages (Accept) plusier fois
        while (total_msg < nb_messageReceiver)
        {
            if (sock_bis < 0)
            {
                perror("Erreur accept");
                break;
            }
            int bytes_read = read(sock_bis, pmesg, lg_M);
            if (bytes_read != 0)
            {
                afficher_message(pmesg, lg_M, total_msg);
                total_msg++;
            }
            else if (bytes_read == 0)
            {
                close(sock_bis);

                // Accepter une nouvelle connexion si on attend encore des messages
                if (total_msg < nb_messageReceiver)
                {
                    printf("En attente d'une nouvelle connexion...\n");
                    sock_bis = accept(mySocket, NULL, NULL);
                    if (sock_bis < 0)
                    {
                        perror("Erreur accept");
                        break;
                    }
                }
            }
            else
            {
                perror("Erreur read");
                close(sock_bis);
            }
        }

        // Fermer la dernière connexion si elle est encore ouverte
        if (sock_bis >= 0)
        {
            close(sock_bis);
        }
        free(pmesg);
    }
    // Source (TCP)
    else if (udp == 0 && source == 1)
    {
        mySocket = socket(AF_INET, SOCK_STREAM, 0);
        memset((char *)&adr_distant, 0, sizeof(adr_distant));
        adr_distant.sin_family = AF_INET;   /* domaine Internet*/
        adr_distant.sin_port = htons(port); /* n° de port */
        if ((hp = gethostbyname(nom_destinataire)) == NULL)
        {
            printf("erreur gethostbyname\n");
            exit(1);
        }
        memcpy((char *)&(adr_distant.sin_addr.s_addr), hp->h_addr, hp->h_length);
        M = (char *)malloc(lg_M * sizeof(char));
        int isConnected = 0;
        isConnected = (int)connect(mySocket, (struct sockaddr *)&adr_distant, sizeof(adr_distant));

        afficher_params(source, lg_M, nb_messageSender, udp);
        for (int i = 0; i < nb_messageSender; i++)
        {
            construire_message(M, lg_M, i);
            if (isConnected == 0)
            {
                write(mySocket, M, lg_M);
                // printf("Nombre d'octets envoyés avec TCP : %ld\n", write(mySocket, M, lg_M));
            }
        }
    }

    free(M);
    return mySocket;
}

void main(int argc, char **argv)
{
    int c;
    extern char *optarg;
    extern int optind;
    int nb_messageSender = 10;          /* default: 10 */
    int nb_messageReceiver = 100000000; // default: infini
    int source = -1;                    /* 0=puits, 1=source */
    int udp = 0;                        /* 0 = TCP, 1 = UDP */
    int n = 0;
    int l = 0;
    int nValue;
    int lValue;
    int sock;
    int lg_M = 30;

    while ((c = getopt(argc, argv, "pn:sul:")) != -1)
    {
        switch (c)
        {
        case 'p':
            if (source == 1)
            {
                printf("usage: cmd [-p|-s][-n ##]\n");
            }
            source = 0;

            break;

        case 's':
            if (source == 0)
            {
                printf("usage: cmd [-p|-s][-n ##]\n");
                exit(1);
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
            udp = 1;
            break;

        case 'l':
            if (atoi(optarg) >= 0)
            {
                lg_M = atoi(optarg) + 5;
            }

            break;

        default:
            printf("usage: cmd [-p|-s][-n ##]\n");
            break;
        }
    }

    // Parse non-option arguments (destination et port)
    // Si il y a 2 arguments non options (argc - 2) => destionation + port
    if (optind == argc - 2)
    {
        char *arg = argv[optind];
        port = atoi(argv[argc - 1]);
        nom_destinataire = arg;
        // printf("port: %d", port);
        // printf("dest: %s\n", nom_destinataire);
    }
    // S'il y a que 1 argument non-option => port
    if (optind == argc - 1)
    {
        port = atoi(argv[argc - 1]);
        // printf("port: %d\n", port);
    }

    sock = createSocket(source, udp, lg_M, nb_messageSender, nb_messageReceiver);
    close(sock);
    if (source == -1)
    {
        printf("usage: cmd [-p|-s][-n ##]\n");
        exit(1);
    }

    if (source == 1)
        printf("on est dans le source\n");
    else
        printf("on est dans le puits\n");

    if (nb_messageSender != -1)
    {
        if (source == 1)
            printf("nb de tampons à envoyer : %d\n", nb_messageSender);
        else
            printf("nb de tampons à recevoir : %d\n", nb_messageSender);
    }
    else
    {
        if (source == 1)
        {
            nb_messageSender = 10;
            printf("nb de tampons à envoyer = 10 par défaut\n");
        }
        else
            printf("nb de tampons à envoyer = infini\n");
    }
}