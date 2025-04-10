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
#include <regex.h>
#include <stdbool.h>

void *timeout_thread(void *arg)
{
    TimeoutData *data = (TimeoutData *)arg;

    usleep(data->milliseconds * 1000);

    data->callback(data->data);

    free(data);

    return NULL;
}

void utils_set_timeout(TimeoutCallback callback, void *data, int milliseconds)
{
    TimeoutData *timeout_data = malloc(sizeof(TimeoutData));
    if (!timeout_data)
    {
        log_message(ERROR, "Failed to allocate timeout data");
        return;
    }

    timeout_data->callback = callback;
    timeout_data->data = data;
    timeout_data->milliseconds = milliseconds;

    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, timeout_thread, timeout_data) != 0)
    {
        log_message(ERROR, "Failed to create timeout thread");
        free(timeout_data);
        return;
    }

    pthread_detach(thread_id);
}

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

void generate_aes_key_from_K(uint32_t K, unsigned char *aes_key)
{
    if (aes_key == NULL)
    {
        log_message(ERROR, "AES key buffer is NULL");
        return;
    }

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char *)&K, sizeof(K), hash);

    memcpy(aes_key, hash, 32);
}

bool validate_username_password(char *username, char *password)
{
    if (username == NULL || password == NULL)
    {
        return false;
    }

    regex_t username_regex;
    int username_result;
    char error_buffer[100];

    if (regcomp(&username_regex, "^[a-zA-Z][a-zA-Z0-9_]{2,19}$", REG_EXTENDED) != 0)
    {
        log_message(ERROR, "Failed to compile username regex");
        return false;
    }

    username_result = regexec(&username_regex, username, 0, NULL, 0);
    regfree(&username_regex);

    if (username_result != 0)
    {
        regerror(username_result, &username_regex, error_buffer, sizeof(error_buffer));
        log_message(ERROR, "Username validation failed: Must be 3-20 characters, start with a letter, and contain only letters, numbers, and underscores");
        return false;
    }

    // size_t password_len = strlen(password);
    //
    // if (password_len < 6 || password_len > 64)
    // {
    //     log_message(ERROR, "Password validation failed: Password must be between 6-64 characters");
    //     return false;
    // }
    //
    // bool has_digit = false;
    // bool has_lowercase = false;
    // bool has_uppercase = false;
    // bool has_invalid_char = false;
    //
    // for (size_t i = 0; i < password_len; i++)
    // {
    //     char c = password[i];
    //
    //     if (c >= '0' && c <= '9')
    //     {
    //         has_digit = true;
    //     }
    //     else if (c >= 'a' && c <= 'z')
    //     {
    //         has_lowercase = true;
    //     }
    //     else if (c >= 'A' && c <= 'Z')
    //     {
    //         has_uppercase = true;
    //     }
    //     else if (c == ' ')
    //     {
    //
    //         log_message(ERROR, "Password validation failed: Password cannot contain spaces");
    //         return false;
    //     }
    //     else
    //     {
    //
    //         if (strchr("!@#$%^&*()_+-=[]{};:'\",.<>/?\\|", c) == NULL)
    //         {
    //             has_invalid_char = true;
    //             break;
    //         }
    //     }
    // }
    //
    // if (!has_digit)
    // {
    //     log_message(ERROR, "Password validation failed: Password must contain at least one digit");
    //     return false;
    // }
    //
    // if (!has_lowercase)
    // {
    //     log_message(ERROR, "Password validation failed: Password must contain at least one lowercase letter");
    //     return false;
    // }
    //
    // if (!has_uppercase)
    // {
    //     log_message(ERROR, "Password validation failed: Password must contain at least one uppercase letter");
    //     return false;
    // }
    //
    // if (has_invalid_char)
    // {
    //     log_message(ERROR, "Password validation failed: Password contains invalid characters");
    //     return false;
    // }

    return true;
}
