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

int main(int argc, char *argv[])
{
	const int maksDlugWizytowki=4096;
	char buforWizytowki[maksDlugWizytowki+1];
	int uzyskaneGniazdo=-1;
	int bladCzySukces;
	int przeczytaneBajty;
	int i;
	struct sockaddr_in* strukturaAdresu;
  	struct sockaddr_in str;
	bzero((char*) &str, sizeof(str));
	strukturaAdresu=&str;

	if(argc!=3)
		printf("Błąd użycia programu.\nPoprawne użycie: klient_wizytowek [adres IPv4] [port]\n");
	
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

	przeczytaneBajty=read(uzyskaneGniazdo, buforWizytowki, maksDlugWizytowki);
	buforWizytowki[przeczytaneBajty]='\0';
	for(i=0;i<przeczytaneBajty;i++)
	{
		if((buforWizytowki[i]>=0&&buforWizytowki[i]<32&&buforWizytowki[i]!=13&&buforWizytowki[i]!=10)||buforWizytowki[i]==127)
		{
			printf("Błąd czytania z serwera - nieprawidłowy znak(%d).\n",buforWizytowki[i]);
			break;
		}
	}
	if(i==przeczytaneBajty)
		printf("%s",buforWizytowki);

	if(shutdown (uzyskaneGniazdo, SHUT_RDWR)==-1)
	{
		fprintf(stderr,"Bład zamykania połączenia.\n");
		return 1;
	}
  	return 0;
}
