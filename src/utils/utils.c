#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "m_utils.h"
#include "log.h"
#include <openssl/sha.h>
#include <stdint.h>

bool is_port_available(int port)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        return false;
    }

    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        close(sock);
        return false;
    }

    close(sock);
    return true;
}

int utils_next_int(int max)
{
    return rand() % max;
}

int utils_mod_exp(int base, int exp, int mod)
{
    int result = 1;
    base = base % mod;

    while (exp > 0)
    {
        if (exp % 2 == 1)
        {
            result = (result * base) % mod;
        }

        exp = exp >> 1;
        base = (base * base) % mod;
    }

    return result;
}


void generate_aes_key_from_K(uint32_t K, unsigned char *aes_key) {
    if (aes_key == NULL) {
        log_message(ERROR, "AES key buffer is NULL");
        return;
    }
    
    unsigned char hash[SHA256_DIGEST_LENGTH]; 
    SHA256((unsigned char *)&K, sizeof(K), hash);
    
    memcpy(aes_key, hash, 32);
}
