# Aby uruchomić przy pomocy funkcji make uruchom należy ustawić zmienną systemową ADRESY.
# Wartością tej zmiennej powinny być argumenty przekazywane do programu, czyli adresy do sprawdzenia.
# Można też wywołać make przygotuj lub make all, w celu przygotowania programu do uruchomienia.

all: przygotuj

find_mx.o: find_mx.c
	gcc -std=c99 -pedantic -Wall -fPIC find_mx.c -c

cotrzymajAdresy.o: cotrzymajAdresy.c DNS.h
	gcc -pedantic -Wall -fPIC cotrzymajAdresy.c -c

DNS.class: DNS.java
	javac DNS.java

DNS.h: DNS.class
	javah DNS

biblioteka: find_mx.o cotrzymajAdresy.o 
	gcc -shared -o libotrzymajAdresy.so  -L/usr/local/lib -lresolv -Wl,-rpath,/usr/local/lib -I /etc-java-config-2/current-system-vm/include -I /etc-java-config-2/current-system-vm/include/linux -fPIC cotrzymajAdresy.o find_mx.o /usr/lib/x86_64-linux-gnu/libresolv.so

przygotuj: biblioteka DNS.class DNS.h cotrzymajAdresy.o find_mx.o libresolv.so

uruchom: przygotuj
	java -Djava.library.path=. DNS $(ADRESY)

clean:
	rm -f *.o libotrzymajAdresy.so *.class DNS.h *~

.PHONY: all clean uruchom przygotuj
