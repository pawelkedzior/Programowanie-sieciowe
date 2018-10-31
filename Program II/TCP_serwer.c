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

int zmienNaCyfry(char *bajty)
{
	char *liczba=(char*)malloc(strlen(bajty));
	liczba=strcpy(liczba, bajty);
	int i;
	int zwrot=0;
	liczba[strlen(liczba)-1]='\0';

	for(i=0;i<strlen(liczba);i++)
		if(liczba[i]<48||liczba[i]>57)
			return -1;
	zwrot=atoi(liczba);
	if(bajty[strlen(bajty)-1]=='k'||bajty[strlen(bajty)-1]=='K')
	{
		zwrot*=1024;
		return zwrot;
	}
	else if(bajty[strlen(bajty)-1]=='M')
	{
		zwrot*=1024*1024;
		return zwrot;
	}
	else
		return -1;
}

int main(int argc, char *argv[])
{
	int i;
	int czyCyfry=1;
	char *buforPorcjiDanych;
	int uzyskaneGniazdo=-1;
	int gniazdoKlienta=-1;
	int bladCzySukces;
	int odebraneBajty;
	int czasOczekiwania;
	int porcjaOdbierana;
	struct sockaddr_in* strukturaAdresu;
  	struct sockaddr_in str;
	bzero((char*) &str, sizeof(str));
	strukturaAdresu=&str;
	
	socklen_t dlugoscAdresuPolaczonego;
	struct sockaddr_in* strukturaAdresuPol;
  	struct sockaddr_in str1;
	bzero((char*) &str1, sizeof(str1));
	strukturaAdresuPol=&str1;
	if(argc!=4)
		printf("Błąd użycia programu.\nPoprawne użycie: TCP_serwer [port] [czas oczekiwania(s)] [porcja danych(B)]\n");
	
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
	for(i=0;i<strlen(argv[2]);i++)
		if(argv[2][i]<48||argv[2][i]>57)
		{
			fprintf(stderr,"Niewłaściwy czas oczekiwania - podaj liczbę całkowitą.\n");
			return -1;			
		}
	czasOczekiwania=atoi(argv[2]);

	for(i=0;i<strlen(argv[3]);i++)
		if(argv[3][i]<48||argv[3][i]>57)
			czyCyfry=0;
	if(czyCyfry==1)
		porcjaOdbierana=atoi(argv[3]);
	else
	{
		porcjaOdbierana=zmienNaCyfry(argv[3]);
		if(porcjaOdbierana<0)
		{
			fprintf(stderr,"Niewłaściwa porcja danych.\nPoprawna porcja powinna być liczbą całkowitą, zakończoną ewentualnie literami: \"k\", \"K\" lub \"M\"");
			return -1;
		}
	}
	buforPorcjiDanych=(char *) malloc(porcjaOdbierana+1);
	
	dlugoscAdresuPolaczonego=sizeof(*strukturaAdresuPol);
	while(1)
	{
		printf("Czekam na program kliencki.\n");
		gniazdoKlienta=accept(uzyskaneGniazdo, (struct sockaddr *) strukturaAdresuPol, &dlugoscAdresuPolaczonego);
		if(gniazdoKlienta==-1)
			printf("Błąd połączenia z klientem: %s\n", strerror(errno));
		else 
		{
			printf("Uzyskałem połączenie z %s\n", inet_ntoa(strukturaAdresuPol->sin_addr));	
			
			do
			{
				fprintf(stderr, "Odbieram porcję danych od klienta.\n");
				odebraneBajty=read(gniazdoKlienta, buforPorcjiDanych, porcjaOdbierana);
				buforPorcjiDanych[odebraneBajty]='\0';
				if(odebraneBajty==0)
				{
					printf("Odbieranie zakończone.\n");
					break;
				}
				else if(odebraneBajty==-1)
				{
					fprintf(stderr, "Błąd odbierania danych: %s\n", strerror(errno));
					break;
				}
				fprintf(stderr, "Odebrałem %d bajtów danych.\n",odebraneBajty);
				sleep(czasOczekiwania);
			}
			while(1);

			if(shutdown (gniazdoKlienta, SHUT_RDWR)==-1)
			{
				fprintf(stderr, "Bład zamykania połączenia.\n");
				return -1;
			}
			printf("Połączenie z klientem zamknięte.\n\n");
			bzero((char*) strukturaAdresuPol, sizeof(*strukturaAdresuPol));
		}
	}
  	return 0;
}
