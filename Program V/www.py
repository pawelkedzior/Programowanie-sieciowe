# coding=utf-8

#program zwraca None, jeśli wszystko jest w porządku (strona poprawnie wczytana i jest typu html/text oraz zgodna treść), lub tekst błędu
import http.client
import sys
pol=http.client.HTTPConnection("www.fais.uj.edu.pl")
pol.request("HEAD","/dla-studentow/ogloszenia")
odp=pol.getresponse()
if (odp.status!=200):
    pol.close()
    sys.exit(("Błąd wczytywania strony: "+odp.status+" "+odp.reason))
if (odp.getheader("Content-Type").find("text/html")==-1):
    pol.close()    
    sys.exit("Błąd - strona nie zawiera kodu html lub tekstu.")
tekst=odp.read()

pol.request("GET","/dla-studentow/ogloszenia")
odp=pol.getresponse()
tekst=odp.read()
tekst=str(tekst)
tekst=tekst.lower()

if((tekst.find("internal error")!=-1) or (tekst.find("błąd wewnętrzny")!=-1) or (tekst.find("wewnętrzny błąd")!=-1)):
    pol.close()
    sys.exit("Błąd wewnętrzny serwera.")
if((tekst.find("nie można załadować strony")!=-1) or (tekst.find("nie odnaleziono strony")!=-1)):
    pol.close()
    sys.exit("Nie wczytano właściwej treści strony.")
if((tekst.find("oszenia")==-1) or (tekst.find("nast")==-1) or (tekst.find("na skr")==-1) or (tekst.find("organizacja roku akademickiego")==-1)):
    pol.close()
    sys.exit("Niewłaściwa treść strony.")

pol.close()
print("Wczytywanie strony zakończone sukcesem.")
sys.exit(None)