#ifndef FIND_MX_H
#define FIND_MX_H

#include <netinet/in.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @file */

/** @mainpage
 * Dydaktyczna mini-biblioteka pokazująca jak wyszukiwać w DNS-ie informacje
 * przy pomocy libresolv. Oferuje tylko jedną publiczną funkcję:
 * @ref find_mail_exchanges.
 *
 * Pamiętaj aby podać kompilatorowi opcję <tt>-lresolv</tt> podczas
 * linkowania, bo libresolv jest co prawda dystrybuowana jako część GNU C
 * Library, ale gcc/g++ nie odwołują się do niej w sposób automatyczny.
 */

/**
 * Funkcja find_mail_exchanges() zwraca tablicę z adresami IPv4 serwerów
 * pocztowych obsługujących daną domenę. Element tablicy zawierający
 * <tt>INADDR_ANY</tt> oznacza koniec zwróconych wyników. Próby dostępu
 * do elementów następujących po tym z <tt>INADDR_ANY</tt> mają nieokreślony
 * efekt.
 *
 * Tablica jest alokowana przy pomocy <tt>malloc</tt>. Obowiązek zwolnienia
 * jej przy pomocy <tt>free</tt> spoczywa na wywołującym funkcję.
 *
 * W razie niemożności znalezienia adresów zwracane jest <tt>NULL</tt>, bez
 * względu na to co było przyczyną: błędy DNS, literówka w nazwie domeny, czy
 * też serwery pocztowe mające adresy IPv6 ale nie IPv4.
 */
struct in_addr * find_mail_exchanges(const char * domain_name);

#ifdef __cplusplus
}
#endif

#endif
