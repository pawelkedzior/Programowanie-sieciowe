// gcc -std=c99 -pedantic -Wall find_mx.c -c
// g++ -std=c++98 -pedantic -Wall demo_mx.cpp find_mx.o -lresolv

#include <cstdio>
#include <cstdlib>
#include <iostream>

#include <arpa/inet.h>

#include "find_mx.h"

int main(int argc, char * argv[])
{
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " domain_or_host_name\n";
        return 1;
    }

    struct in_addr * adresses = find_mail_exchanges(argv[1]);
    if (adresses == NULL) {
        return 1;
    }

    char buf[INET_ADDRSTRLEN];
    for (int i = 0; adresses[i].s_addr != INADDR_ANY; ++i) {
        if (inet_ntop(AF_INET, adresses + i, buf, sizeof(buf)) == NULL) {
            std::perror("inet_ntop");
            return 1;
        }
        std::cout << buf << '\n';
    }

    std::free(adresses);
    return 0;
}
