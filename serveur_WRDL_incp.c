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
#define PORT_WRDL 4242
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include "mots_5_lettres.h"
#include <time.h>
#include <pthread.h>
#define LOG_DFT "WRDL_srv.log"
struct work {
	int socket;
	int logfile;
	pthread_mutex_t *mut_logfile;
};

int saisir_prop_serveur(char *prop);
ssize_t exact_read(int fd, void *buf, size_t nbytes);
ssize_t exact_write(int fd, void *buf, size_t nbytes);
int traiter_prop(const char *prop_joueur, const char *a_deviner, char * dest); 
void chaine_toupper(char *ch);
void *worker(void *work);

int main(int argc , int **argv)
{

	int log = open(argc >= 2 ? argv[1] : LOG_DFT, O_WRONLY | O_TRUNC | O_CREAT, 0644);
	if (log < 0) {
		perror("open");
		exit(2);
	}
	srand(time(NULL)); 
	pthread_mutex_t mut_logfile;
	pthread_mutex_init(&mut_logfile, NULL);

	/* 1. Création d'une socket tcp ipv4 */
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("socket");
		exit(2);
	}

	/* 2. Création de la sockaddr */
	struct sockaddr_in sa = { .sin_family = AF_INET,
				  .sin_port = htons(PORT_WRDL),
				  .sin_addr.s_addr = htonl(INADDR_ANY) };
	/* Optionnel : faire en sorte de pouvoir réutiliser l'adresse sans
	 * attendre après la fin du serveur. */
	int opt = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));

	/* 3. Attacher la socket d'écoute à l'adresse */
	if (bind(sock, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
		perror("bind");
		exit(3);
	}

	while(1){
		if (listen(sock, 10) < 0) {
			perror("listen");
			exit(2);
		}
		struct work *w = malloc(sizeof(struct work));
		if (w == NULL) {
			perror("malloc");
			exit(2);
		}
		w->logfile = log;
		w->mut_logfile = &mut_logfile;
		struct sockaddr_in addr_clt;
		socklen_t taille_addr = sizeof(struct sockaddr_in);
		w->socket = accept(sock, (struct sockaddr *) &addr_clt, &taille_addr);
		if (w->socket < 0) {
			perror("accept");
			continue;
		}
		char addr_char[INET_ADDRSTRLEN];
		if (inet_ntop(AF_INET, &(addr_clt.sin_addr), addr_char, INET_ADDRSTRLEN) == NULL) {perror("inet_ntop");
		} else {
			time_t now = time(NULL);
			char date_heure[32], log_mess[256];
			strftime(date_heure, 32, "%F:%T", localtime(&now));
			sprintf(log_mess, "%s : connection avec %s\n", date_heure, addr_char);
			pthread_mutex_lock(&mut_logfile);
			write(log, log_mess, strlen(log_mess));
			pthread_mutex_unlock(&mut_logfile);
		}
		pthread_t th;
		if (pthread_create(&th, NULL, worker, w) < 0) {
			perror("pthread_create");
			exit(2);
		}
		pthread_detach(th);
	
	}
	pthread_mutex_destroy(&mut_logfile);
	close(sock);
	return 0;
}
void *worker(void *work){
		struct work *w = work;
		char a_deviner[6];
		uint8_t  prop[6];
		uint8_t indic[6]; 
		ssize_t c,d;
		uint32_t n=0;
		int ret_val=0;
		int rej = 1;
       	        mot_alea5(a_deviner);
		printf("Reponse : %s\n",a_deviner);
		do{
			c=exact_read(w->socket,&prop,sizeof(prop));
			if((ret_val = saisir_prop_serveur(prop))==3){
				n=3;
				exact_write(w->socket,&n,sizeof(n));

			}else{

				if(ret_val==4){
					n=4;
					exact_write(w->socket,&n,sizeof(n));

				}else{

					if((traiter_prop(prop, a_deviner,indic))<5){
						n=2;
						exact_write(w->socket,&n,sizeof(n));
						exact_write(w->socket,indic,sizeof(indic));

					}
					if((traiter_prop(prop, a_deviner,indic))==5){
						n=1;
						exact_write(w->socket,&n,sizeof(n));
						exact_write(w->socket,a_deviner,sizeof(a_deviner));
						exact_read(w->socket,&rej,sizeof(rej));
						if(rej){
							mot_alea5(a_deviner);
							printf("reponse : %s\n",a_deviner);
							n=0;
						}	

					}
				}
			}
		}while(n!=1 && rej);	
					
		close(w->socket);
		free(w);
		return NULL;
	
}


void chaine_toupper(char *ch)
{
        int i;
        for (i = 0; ch[i] != '\0'; i = i + 1)
                ch[i] = toupper(ch[i]);
}

ssize_t exact_read(int fd, void *buf, size_t nbytes)
{
        ssize_t n;
        size_t total = 0;
        while (total < nbytes) {
                n = read(fd, buf + total, nbytes - total);
                if (n == -1){ /* error */
			perror("pb:\n");
                        return n;
		}
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
int traiter_prop(const char *prop_joueur, const char *a_deviner, char * dest)
{
        int i, nb_lettres_trouvees = 0;
        for (i = 0; i < 5; i = i + 1) {
                if (prop_joueur[i] == a_deviner[i]) {
                       sprintf(dest + i,"%c", prop_joueur[i]);
                        nb_lettres_trouvees = nb_lettres_trouvees + 1;
                } else if (strchr(a_deviner, prop_joueur[i])) {
                        sprintf(dest+i,"%c", tolower(prop_joueur[i]));
                } else {
                        dest[i]='_';
                }
        }
        printf("\n");
        return nb_lettres_trouvees;
}
int saisir_prop_serveur(char *prop)
{
        if(strlen(prop)<5) return 3;
        else{
          chaine_toupper(prop);
          if (!est_dans_liste_mots(prop))
                  return 4;
        }
	return 1;
}
