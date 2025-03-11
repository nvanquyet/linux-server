#ifndef M_UTILS_H
#define M_UTILS_H

#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

typedef void (*TimeoutCallback)(void* data);

typedef struct {
    TimeoutCallback callback;
    void* data;
    int milliseconds;
} TimeoutData;

void utils_set_timeout(TimeoutCallback callback, void* data, int milliseconds);

bool is_port_available(int port);
int utils_next_int(int max);
int utils_mod_exp(int base, int exp, int mod);
void generate_aes_key_from_K(uint32_t K, unsigned char *aes_key);
bool validate_username_password(char *username, char *password);

#endif