#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "find_mx.h"
#include <sys/types.h>
#include <netinet/in.h>
#include <resolv.h>      
#include <netdb.h>       
#include <string.h>
#include <arpa/nameser.h>     

JNIEXPORT jobjectArray JNICALL Java_DNS_adresyIP
  (JNIEnv *srodowisko, jobject jobj, jstring adres)
{
	const char *argumentFunkcji=(*srodowisko)->GetStringUTFChars(srodowisko, adres, 0);
	int liczbaAdresow=0;
	int i;
	char bufor[INET_ADDRSTRLEN];
	
	struct in_addr *adresy=find_mail_exchanges((char *)argumentFunkcji);
   if (adresy==NULL) {
   	return NULL;
   }

	for (i=0; adresy[i].s_addr!=INADDR_ANY; ++i)
		liczbaAdresow++;

	if(liczbaAdresow==0) 
		return NULL;

	jstring adresyJava[liczbaAdresow];

	for (i=0; i<liczbaAdresow; ++i)
	{
		if (inet_ntop(AF_INET, adresy+i, bufor, sizeof(bufor))==NULL) 
		{
			return NULL;
		}
		adresyJava[i]=(*srodowisko)->NewStringUTF(srodowisko, bufor);
	}

	jclass klasaString=(*srodowisko)->FindClass(srodowisko, "Ljava/lang/String;");
	jobjectArray zwracane=(*srodowisko)->NewObjectArray(srodowisko, liczbaAdresow, klasaString, NULL);
	jmethodID konstruktorString=(*srodowisko)->GetMethodID(srodowisko, klasaString, "<init>", "(Ljava/lang/String;)V");
	if (konstruktorString==NULL) 
		return NULL;
		
	for (i=0;i<liczbaAdresow;i++)
	{
		jobject kolejnyAdres=(*srodowisko)->NewObject(srodowisko, klasaString, konstruktorString, adresyJava[i]);
		(*srodowisko)->SetObjectArrayElement(srodowisko, zwracane, i, kolejnyAdres);
	}
	
	free(adresy);
	(*srodowisko)->ReleaseStringUTFChars(srodowisko, adres, argumentFunkcji);    
	
	return zwracane;
}
