/* fichiers de la bibliothèque standard */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* bibliothèque standard unix */
#include <unistd.h> /* close, read, write */
#include <sys/types.h>
#include <sys/socket.h>
/* spécifique à internet */
#include <arpa/inet.h> /* inet_pton */
#include "mots_5_lettres.h"
#define PORT_WRDLP 4242
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

ssize_t exact_write(int fd, void *buf, size_t nbytes);
ssize_t exact_read(int fd, void *buf, size_t nbytes);
void vider_tampon(); 
 void chaine_toupper(char *ch);
void saisir_prop_client(char *prop);
void usage(char *nom_prog)
{
	fprintf(stderr, "Usage: %s addr_ipv4\n"
			"client pour WRDLP (-----------)\n"
			"Exemple: %s 208.97.177.124\n", nom_prog, nom_prog);
}

int main(int argc, char *argv[])
{
	if (argc != 2) {
		usage(argv[0]);
		return 1;
	}
	/* 1. Création d'une socket tcp ipv4 */
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("socket");
		exit(2);
	}

	/* 2. Création de la sockaddr */
	struct sockaddr_in sa = { .sin_family = AF_INET, .sin_port = htons(PORT_WRDLP) }; //htons -> convertit le port en gros boutisme 
	if (inet_pton(AF_INET, argv[1], &sa.sin_addr) != 1) {  //converti la char en ipv4
		fprintf(stderr, "adresse ipv4 non valable\n");
		exit(1);
	}

	/* 3. Tentative de connection */
	if (connect(sock, (struct sockaddr *) &sa, sizeof(struct sockaddr_in)) < 0) {
		perror("connect");
		exit(3);
	}

	/* 4. Échange avec le serveur */
	/* 4.1 Construction de la requête INCP */
        uint8_t prop_joueur[6];
	uint8_t indic[6];
	uint8_t reponse[6];
	int rej=1;
	size_t c;
	int tentatives=1;
	uint32_t n;
		saisir_prop_client(prop_joueur);
		c=exact_write(sock,prop_joueur,sizeof(prop_joueur));
	do{
		exact_read(sock,&n,sizeof(n));
		if (n ==1){
			printf("Gagnée !!\n");
			exact_read(sock,reponse,sizeof(reponse));
			printf("Bonne reponse : %s\n",reponse);
			printf("Tentatives %d\n\n",tentatives);
			printf("souhaitez vous rejouer ? (0 : Non | 1 : Oui ) ");
			scanf("%d",&rej);
			printf("\n\n");
			exact_write(sock,&rej,sizeof(rej));
			if(rej){
				tentatives=1;
				saisir_prop_client(prop_joueur);
				c=exact_write(sock,prop_joueur,sizeof(prop_joueur));

			}

		}
		if (n==3){
			printf("Trop court ! \n");
			saisir_prop_client(prop_joueur);
			c=exact_write(sock,prop_joueur,sizeof(prop_joueur));
		}
		if (n==4){
			printf("Pas dans le dictionnaire ! \n");
			saisir_prop_client(prop_joueur);
			c=exact_write(sock,prop_joueur,sizeof(prop_joueur));
		}
		if (n==2){
			exact_read(sock,indic,sizeof(indic));
			printf("Indication : %s\n",indic);
			saisir_prop_client(prop_joueur);
			c=exact_write(sock,prop_joueur,sizeof(prop_joueur));
			tentatives++;	
			
		}
	}
	while(c>0 && rej);
	/* 4.2 Réponse du serveur */

	close(sock);
	return 0;
}
void chaine_toupper(char *ch)
{
        int i; 
        for (i = 0; ch[i] != '\0'; i = i + 1)
                ch[i] = toupper(ch[i]);
}
void vider_tampon()
{
        int c;
        while ((c = getchar()) != '\n' && c != EOF)
                ;
}
 ssize_t exact_read(int fd, void *buf, size_t nbytes)
{
	ssize_t n;
	size_t total = 0;
	while (total < nbytes) {
		n = read(fd, (char *) buf + total, nbytes - total);
		if (n == -1) /* error */
			return n;
		if (n == 0) /* end of file */
			break;
		total += n;
	}
	return total;
}
ssize_t exact_write(int fd, void *buf, size_t nbytes)
{
	ssize_t n;
	size_t total = 0;
	while (total < nbytes) {
		n = write(fd, (char *) buf + total, nbytes - total);
		if (n == -1) /* error */
			return n;
		total += n;
	}
	return total;
}
void saisir_prop_client(char *prop)
{
                printf("Votre proposition : ");
                if (scanf("%5s", prop) == EOF) {
                        printf("\n");
                        exit(1);
                }
                vider_tampon();
}
