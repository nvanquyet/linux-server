#ifndef AES_UTILS_H
#define AES_UTILS_H

#include <openssl/evp.h>

void aes_encrypt(const unsigned char *plaintext, size_t plaintext_len, unsigned char *key, unsigned char *iv, unsigned char *ciphertext, size_t *ciphertext_len);
void aes_decrypt(const unsigned char *ciphertext, size_t ciphertext_len, unsigned char *key, unsigned char *iv, unsigned char *plaintext, size_t *plaintext_len);

#endif
