#ifndef M_UTILS_H
#define M_UTILS_H

#include <stdbool.h>
#include <stdint.h>

bool is_port_available(int port);
int utils_next_int(int max);
int utils_mod_exp(int base, int exp, int mod);
void generate_aes_key_from_K(uint32_t K, unsigned char *aes_key);

#endif