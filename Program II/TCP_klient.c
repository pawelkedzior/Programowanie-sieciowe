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
#include <time.h>

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
			strukturaAdresu=NULL;
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
				strukturaAdresu=NULL;
 				return NULL;
 			}
  	}
  	else
  			strukturaAdresu->sin_port=serwer->s_port;
  	return strukturaAdresu;
}

int uzyskajGniazdoTCP()
{
	struct protoent *protokol;
	protokol=getprotobyname("TCP");
  	return socket(PF_INET, SOCK_STREAM, protokol->p_proto);
}

int uzyskajPolaczenie(int deskryptorGniazda, struct sockaddr_in strukturaAdresu)
{
	return connect(deskryptorGniazda, (struct sockaddr *)&strukturaAdresu, sizeof(strukturaAdresu));
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

char *tworzPorcjeDanych(char *bufor, int rozmiar)
{
	int i;
	bufor=(char *)malloc(rozmiar);
	srand (time(NULL));
	for(i=0;i<rozmiar;i++)
		bufor[i]=(rand()%95)+32;
	return bufor;
}

int main(int argc, char *argv[])
{
	char *buforPorcjiDanych=NULL;
	int uzyskaneGniazdo=-1;
	int bladCzySukces;
	int wyslaneBajty;
	int i;
	struct sockaddr_in* strukturaAdresu;
  	struct sockaddr_in str;
	bzero((char*) &str, sizeof(str));
	strukturaAdresu=&str;
	
	int czyCyfry=1;
	int czasOczekiwania;
	int porcjaDanych;

	if(argc!=5)
	{
		printf("Błąd użycia programu.\nPoprawne użycie: TCP_klient [adres IPv4] [port] [czas oczekiwania(s)] [porcja danych(B)]\n");
		return 0;
	}
	
	uzyskaneGniazdo=uzyskajGniazdoTCP();
	if(uzyskaneGniazdo==-1)
	{
	    fprintf(stderr, "Nie udało się uzyskać gniazda.\n");
	    return -1;
 	}
	else
	{
	    uzyskajStruktureAdresu(strukturaAdresu, argv[1], argv[2]);	
	    if(strukturaAdresu==NULL)
	    {
		fprintf(stderr,"Nie udało mi się uzyskać struktury adresu dla %s:%s.\n", argv[1], argv[2]);
		return -1;
	    }
	    else
	    {
		bladCzySukces=uzyskajPolaczenie(uzyskaneGniazdo, *strukturaAdresu);
		if(bladCzySukces==-1)
		{
			fprintf(stderr, "Błąd uzyskiwania połączenia z %s:%s\n", argv[1], argv[2]);
			return -1;
		}
	    }
	}
	
	for(i=0;i<strlen(argv[3]);i++)
		if(argv[3][i]<48||argv[3][i]>57)
		{
			fprintf(stderr,"Niewłaściwy czas oczekiwania - podaj liczbę całkowitą.\n");
			return -1;			
		}
	czasOczekiwania=atoi(argv[3]);

	for(i=0;i<strlen(argv[4]);i++)
		if(argv[4][i]<48||argv[4][i]>57)
			czyCyfry=0;
	if(czyCyfry==1)
		porcjaDanych=atoi(argv[4]);
	else
	{
		porcjaDanych=zmienNaCyfry(argv[4]);
		if(porcjaDanych<0)
		{
			fprintf(stderr,"Niewłaściwa porcja danych.\nPoprawna porcja powinna być liczbą całkowitą, zakończoną ewentualnie literami: \"k\", \"K\" lub \"M\"");
			return -1;
		}
	}

	while(1)
	{
		buforPorcjiDanych=tworzPorcjeDanych(buforPorcjiDanych, porcjaDanych);
		fprintf(stderr,"Wysyłam porcję danych na serwer.\n");
		wyslaneBajty=write(uzyskaneGniazdo, buforPorcjiDanych, porcjaDanych);
		if(wyslaneBajty==-1)
		{
			printf("Błąd wysyłania: %s.\n", strerror(errno));
			break;
		}
		fprintf(stderr,"Wysłałem porcję danych o rozmiarze: %d\n", wyslaneBajty);
		sleep(czasOczekiwania);
	}

	if(shutdown (uzyskaneGniazdo, SHUT_RDWR)==-1)
	{
		fprintf(stderr,"Błąd zamykania połączenia.\n");
		return 1;
	}
  	return 0;
}
