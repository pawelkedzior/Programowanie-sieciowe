#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

struct sockaddr_in *uzyskajStruktureAdresu(struct sockaddr_in *strukturaAdresu, char *adresHosta, char *usluga)
{	
  	struct hostent *host;
  	struct servent *serwer;

  	strukturaAdresu->sin_family=AF_INET;

	strukturaAdresu->sin_addr.s_addr=inet_addr(adresHosta);
 	if (strukturaAdresu->sin_addr.s_addr==INADDR_NONE)
 	{
 		host=gethostbyname(adresHosta);
		if(host==NULL)
  		{
    		fprintf(stderr, "Nie rozpoznano serwera: %s.\n", adresHosta);
		   return NULL;
  		}
		#define h_addr h_addr_list[0]
  		memcpy(host->h_addr, (char *) &strukturaAdresu->sin_addr, host->h_length);
 	}

  	if(!(serwer=getservbyname(usluga, "TCP")))
  	{
  			strukturaAdresu->sin_port=htons((uint16_t)atoi(usluga));
  			if(strukturaAdresu->sin_port==0)
  			{
	  			fprintf(stderr, "Nie mogę odnaleźć usługi: %s\n", usluga);
 				return NULL;
 			}
  	}
  	else
  			strukturaAdresu->sin_port=serwer->s_port;
  	return strukturaAdresu;
}

struct sockaddr_in *uzyskajStruktureAdresuSerwera(struct sockaddr_in *strukturaAdresu, char *port)
{
	uzyskajStruktureAdresu(strukturaAdresu, "127.0.0.1", port);
	strukturaAdresu->sin_family=AF_INET;
	strukturaAdresu->sin_port=htons((uint16_t)atoi(port));
   strukturaAdresu->sin_addr.s_addr=INADDR_ANY;
	return strukturaAdresu;
}

int uzyskajGniazdoTCP()
{
  	return socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);//protokol->p_proto);
}

int dowiazGniazdo(struct sockaddr_in *strukturaAdresu, int deskryptorGniazda)
{
	return bind(deskryptorGniazda, (struct sockaddr *) strukturaAdresu, sizeof(*strukturaAdresu));
}

int nasluchujPort(int deskryptorGniazda)
{
	return listen(deskryptorGniazda, 16);
}

int main(int argc, char *argv[])
{
	const char *buforWizytowki="Witaj, świecie!\r\n";
	int uzyskaneGniazdo=-1;
	int gniazdoKlienta=-1;
	int bladCzySukces;
	int wyslaneBajty;
	struct sockaddr_in* strukturaAdresu;
  	struct sockaddr_in str;
	bzero((char*) &str, sizeof(str));
	strukturaAdresu=&str;
	
	socklen_t dlugoscAdresuPolaczonego;
	struct sockaddr_in* strukturaAdresuPol;
  	struct sockaddr_in str1;
	bzero((char*) &str1, sizeof(str1));
	strukturaAdresuPol=&str1;

	if(argc!=2)
		printf("Błąd użycia programu.\nPoprawne użycie: serwer_wizytowek [port]\n");
	
	uzyskaneGniazdo=uzyskajGniazdoTCP();
	if(uzyskaneGniazdo==-1)
	{
	    fprintf(stderr, "Nie udało się uzyskać gniazda.\n");
	    return -1;
 	}
 	
	uzyskajStruktureAdresuSerwera(strukturaAdresu, argv[1]);
	if(strukturaAdresu==NULL)
	{
		fprintf(stderr, "Błąd tworzenia struktury adresu dla portu %s\n", argv[1]);
		return -1;
	}
	
	bladCzySukces=dowiazGniazdo(strukturaAdresu, uzyskaneGniazdo);
	if(bladCzySukces==-1)
	{
		fprintf(stderr, "Błąd dowiązywania gniazda: %s.\n", strerror(errno));
		return -1;
	}
	
	bladCzySukces=nasluchujPort(uzyskaneGniazdo);
	if(bladCzySukces==-1)
	{
		fprintf(stderr, "Błąd nasłuchiwania gniazda.\n");
		return -1;
	}
	
	dlugoscAdresuPolaczonego=sizeof(*strukturaAdresuPol);
	while(1)
	{
		printf("Czekam na kolejny program kliencki.\n");
		gniazdoKlienta=accept(uzyskaneGniazdo, (struct sockaddr *) strukturaAdresuPol, &dlugoscAdresuPolaczonego);
		if(gniazdoKlienta==-1)
			printf("Błąd połączenia z klientem: %s\n", strerror(errno));
		else 
		{
			printf("Uzyskałem połączenie z %s\n", inet_ntoa(strukturaAdresuPol->sin_addr));
			
			printf("Wysyłam wizytówkę: \"%s\" na adres klienta\n", buforWizytowki);
			wyslaneBajty=write(gniazdoKlienta, buforWizytowki, strlen(buforWizytowki)*sizeof(*buforWizytowki));
			if(wyslaneBajty!=sizeof(*buforWizytowki)*strlen(buforWizytowki))
				printf("Błąd wysyłania wizytówki.\n");
			printf("Wizytowka wyslana!\n");

			if(shutdown (gniazdoKlienta, SHUT_RDWR)==-1)
			{
				fprintf(stderr,"Bład zamykania połączenia.\n");
				return -1;
			}
			printf("Połączenie z klientem zamknięte.\n\n");
			bzero((char*) strukturaAdresuPol, sizeof(*strukturaAdresuPol));
		}
	}
  	return 0;
}
