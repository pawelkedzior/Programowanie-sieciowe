// Możesz skompilować ten plik ze zdefiniowanym symbolem TRACE, funkcje będą
// wtedy drukować na stdout komunikaty o postępach w pracy:
//      gcc -std=c99 -pedantic -Wall -DTRACE find_mx.c -c

// Aby dowiedzieć się gdzie przesłać list zaadresowany do foo@bar.example.org
// trzeba odwołać się do DNS. Najpierw sprawdzamy czy z bar.example.org
// związane są rekordy typu MX (mail exchange). Jeśli tak, to bierzemy
// nazwy serwerów zawarte w MX-ach, sprawdzamy jakie adresy są im przypisane
// (tzn. pobieramy z DNS rekordy typu A dla IPv4 i/lub AAAA dla IPv6),
// i będziemy próbowali nawiązać połączenie z jednym z tych adresów.
// Jeśli bar.example.org nie ma rekordów MX, to w drugiej kolejności
// sprawdzamy czy ma rekordy A/AAAA, licząc że bar.example.org być może
// jest serwerem samodzielnie obsługującym swoją pocztę.
// Poniższy kod został zaprojektowany jako IPv4-only, i co za tym idzie
// nie pobiera rekordów AAAA.

// DNS używa binarnego, dość skomplikowanego protokołu komunikacyjnego. Między
// innymi używana jest kompresja nazw, wykorzystująca to że w pakiecie DNS
// zazwyczaj pojawia się wiele nazw z tą samą końcówką. Próba własnoręcznego
// zaimplementowania jego obsługi byłaby złym pomysłem -- lepiej skorzystać
// z gotowej biblioteki. Poniższy kod używa libresolv, dystrybuowanej razem
// z GNU C Library. Zaleta: nie trzeba niczego doinstalowywać, wszędzie tam
// gdzie jest glibc będzie i libresolv. Wada: kompletny brak dokumentacji,
// jedyny opis tego jak korzystać z funkcji udostępnianych przez libresolv
// można znaleźć w jednym z rozdziałów książki "DNS and BIND".

// libresolv wywodzi się z systemu BSD, i nie jest ujęta w standardzie POSIX.
// Jeśli wywołamy kompilator w trybie zgodności ze standardami, to może nie
// dać nam dostępu do niestandardowych funkcji i kompilacja zakończy się
// błędem. Na szczęście można użyć tzw. feature macros. Trzeba je zdefiniować
// zanim użyjemy dyrektyw #include, systemowe pliki nagłówkowe w oparciu o te
// makra decydują jakie definicje funkcji należy udostępnić.

// Pierwsze z poniższych makr sygnalizuje że chcemy korzystać z definicji
// opisanych standardem POSIX w wersji z 2008 roku.
// Drugie jest specyficzne dla glibc, oznacza że chcemy korzystać również
// z rzeczy dostępnych w systemach BSD.
//
#define _POSIX_C_SOURCE 200809L
#define _BSD_SOURCE
// w nowych wersjach glibc zamiast _BSD_SOURCE używa się _DEFAULT_SOURCE
// (udostępnia definicje z BSD i z System V)
#define _DEFAULT_SOURCE

// W dawnych złych czasach kolejność włączania nagłówków była istotna;
// stara dokumentacja mówi "przed <resolv.h> trzeba włączyć <sys/types.h>,
// <netinet/in.h> oraz <arpa/nameser.h>, w dokładnie takim porządku".
//
#include <sys/types.h>          // rozmaite *_t, np. u_int16_t (niestd.)
#include <netinet/in.h>
#include <arpa/nameser.h>       // niestd. nagłówek, NS_* i ns_*
#include <resolv.h>             // niestd. nagłówek, res_search
#include <netdb.h>              // h_errno i herror (niestd.)

#include <stdio.h>              // printf i pokrewne
#include <stdlib.h>             // malloc
#include <string.h>             // memset, memcpy
#include <netinet/in.h>         // struct in_addr
#include <arpa/inet.h>          // inet_ntoa (niestd.)

#include "find_mx.h"

// deklaracje wyprzedzajace dla pomocniczych funkcji
static int query(const char * domain_name, int type,
        struct in_addr * results);
static int process_mx_record(ns_msg * handle, ns_rr * record,
        struct in_addr * results);
static int process_a_record(ns_msg * handle, ns_rr * record,
        struct in_addr * results);
static void add(struct in_addr * results, struct in_addr address);

// rozmiar tablicy, w której zapamiętywane będą znalezione adresy IP
#define ARRAY_LENGTH 32

// patrz opis w find_mx.h
//
struct in_addr * find_mail_exchanges(const char * domain_name)
{
#ifdef TRACE
    printf("finding MX-es for %s\n", domain_name);
#endif

    // zaalokuj i zainicjuj tablicę, w której będą zapamiętywane wyniki
    struct in_addr * results = (struct in_addr *)
                malloc(ARRAY_LENGTH * sizeof(struct in_addr));
    if (results == NULL) {
        fputs("malloc: out of memory\n", stderr);
        return NULL;
    }
    for (int i = 0; i < ARRAY_LENGTH; ++i) {
        results[i].s_addr = INADDR_ANY;
    }

    // najpierw spróbuj pobrać rekordy MX, a jeśli ich brak -- rekordy A
    int status;
    status = query(domain_name, ns_t_mx, results);
    if (status == 0) {
#ifdef TRACE
        printf("no MX records found, trying A records\n");
#endif
        status = query(domain_name, ns_t_a, results);
    }
    if (status == -1 || results[0].s_addr == INADDR_ANY) {
        fputs("find_mail_exchanges: none found\n", stderr);
        free(results);
        return NULL;
    }

#ifdef TRACE
    printf("search done, found some IPv4 adresses\n");
#endif

    return results;
}

static void add(struct in_addr * results, struct in_addr address)
{
    for (int i = 0; i < ARRAY_LENGTH - 1; ++i) {
        if (results[i].s_addr == INADDR_ANY) {
            results[i] = address;
            return;
        }
    }
    fputs("too many results to remember them all\n", stderr);
}

// zwraca liczbę znalezionych rekordów (nie adresów! jeśli znajdzie jeden MX,
// a ten MX będzie miał przypisane trzy adresy IPv4, to zwróci 1; tak samo
// jeśli nie będzie żadnego, bo ten MX jest IPv6-only), albo -1 jeśli błąd
//
// rekordy MX należałoby sortować wg. ich priorytetów ale to za dużo roboty,
// przetwarzane są więc w takiej kolejności w jakiej zwrócił je serwer DNS
//
static int query(const char * domain_name, int type, struct in_addr * results)
{
    int status;

#ifdef TRACE
    printf("querying DNS for type %i records for %s\n", type, domain_name);
#endif

    // bufor na pakiet z odpowiedzią zwróconą przez nameserver
    unsigned char reply[NS_PACKETSZ];
    int reply_len;

    reply_len = res_search(domain_name, ns_c_in, type, reply, sizeof(reply));
    if (reply_len == -1) {
        if (h_errno == NO_DATA) {
            // ta nazwa nie ma rekordów tego typu
            return 0;
        } else {
            herror("res_search");
            return -1;
        }
    }

    // pomocnicza struktura danych, funkcje biblioteczne zapisują w niej
    // różne informacje dotyczące analizowanego pakietu DNS
    ns_msg handle;

    // zrób wstępną analizę pakietu
    status = ns_initparse(reply, reply_len, &handle);
    if (status == -1) {
        fputs("ns_initparse: truncated or malformed packet\n", stderr);
        return -1;
    }

    // liczba rekordów w sekcji "answer" pakietu
    u_int16_t answers_count = ns_msg_count(handle, ns_s_an);

    // druga pomocnicza struktura danych, w niej zapisywane są informacje
    // o właśnie analizowanym rekordzie
    ns_rr record;

    for (int i = 0; i < answers_count; ++i) {
        // przeanalizuj i-ty rekord w sekcji "answer"
        status = ns_parserr(&handle, ns_s_an, i, &record);
        if (status == -1) {
            fputs("ns_parserr: truncated or malformed record\n", stderr);
            return -1;
        }

#ifdef TRACE
        char buf[1024];
        ns_sprintrr(&handle, &record, NULL, NULL, buf, sizeof(buf));
        printf("%s\n", buf);
#endif

        if (type == ns_t_mx) {
            status = process_mx_record(&handle, &record, results);
        } else if (type == ns_t_a) {
            status = process_a_record(&handle, &record, results);
        } else {
            // type niby musi być równy MX albo A, ale Murphy nie śpi
            return -1;
        }
        if (status == -1) {
            return -1;
        }
    }

    return answers_count;
}

// zwraca -1 w przypadku wystąpienia błędu
//
static int process_mx_record(ns_msg * handle, ns_rr * record,
        struct in_addr * results)
{
    // dla pewności sprawdźmy czy to na pewno rekord typu MX
    if (ns_rr_type(*record) != ns_t_mx) {
        fputs("unexpected resource record type\n", stderr);
        return -1;
    }

    // wskaźniki na pierwszy bajt całego pakietu, tuż za ostatnim bajtem
    // pakietu, pierwszy bajt właśnie przetwarzanego rekordu, i do tego
    // jeszcze długość rekordu w bajtach
    const u_char * msg_begin = ns_msg_base(*handle);
    const u_char * msg_end = msg_begin + ns_msg_size(*handle);
    const u_char * data = ns_rr_rdata(*record);
    u_int16_t data_len = ns_rr_rdlen(*record);

    // rekordy MX zawierają 16-bitowy priorytet i skompresowaną nazwę serwera
    if (data_len < 3) {
        fputs("malformed (too short) resource record\n", stderr);
        return -1;
    }

    u_int prio = ns_get16(data);

    char mx_name[MAXDNAME];
    int status = ns_name_uncompress(msg_begin, msg_end, data + 2,
                mx_name, sizeof(mx_name));

    // jeśli nazwę udało się poprawnie zdekompresować, to w status jest liczba
    // bajtów zajmowanych przez tę nazwę w oryginalnej, spakowanej postaci
    if (status == -1 || data_len != 2 + status) {
        fputs("malformed resource record\n", stderr);
        return -1;
    }

#ifdef TRACE
    printf("resolving MX = %s (priority %u)\n", mx_name, prio);
#endif

    status = query(mx_name, ns_t_a, results);
    if (status == 0) {
        // to jeszcze nie katastrofa, być może inne MX-y będą miały
        // adresy IPv4, więc nie robimy tutaj "return -1"
        fputs("failed to resolve MX name to IPv4 address\n", stderr);
    }

    return status;
}

// zwraca -1 w przypadku wystąpienia błędu
//
static int process_a_record(ns_msg * handle, ns_rr * record,
        struct in_addr * results)
{
    if (ns_rr_type(*record) != ns_t_a) {
        fputs("unexpected resource record type\n", stderr);
        return -1;
    }

    const u_char * data = ns_rr_rdata(*record);
    u_int16_t data_len = ns_rr_rdlen(*record);

    // rekordy A powinny zawierać dokładnie 4 bajty danych
    if (data_len != 4) {
        fputs("malformed resource record\n", stderr);
        return -1;
    }

    // te bajty to adres IPv4 w sieciowej kolejności bajtów, można go
    // bezpośrednio skopiować do struktury in_addr
    struct in_addr address;
    memset(&address, 0, sizeof(address));
    memcpy(&address.s_addr, data, 4);

#ifdef TRACE
    printf("adding A = %s\n", inet_ntoa(address));
#endif

    add(results, address);

    return 0;
}
