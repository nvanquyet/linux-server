#include "message.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "aes_utils.h"
#include "log.h"

#define INITIAL_BUFFER_SIZE 64

/**
 * Creates a new message with the specified command.
 * @param command The command code for this message
 * @return A pointer to the newly created Message, or NULL if allocation failed
 */
Message *message_create(uint8_t command)
{
    Message *msg = (Message *)malloc(sizeof(Message));
    if (msg == NULL)
    {
        return NULL;
    }

    msg->command = command;
    msg->buffer = (unsigned char *)malloc(INITIAL_BUFFER_SIZE);
    if (msg->buffer == NULL)
    {
        free(msg);
        return NULL;
    }

    msg->size = INITIAL_BUFFER_SIZE;
    msg->position = 0;

    return msg;
}

/**
 * Frees all resources associated with a message.
 * @param msg The message to destroy
 */
void message_destroy(Message *msg)
{
    if (msg != NULL)
    {
        if (msg->buffer != NULL)
        {
            free(msg->buffer);
        }
        free(msg);
    }
}

/**
 * Ensures the message buffer has enough space for additional data.
 * @param msg The message to resize
 * @param additional_size The number of additional bytes needed
 * @return true if successful, false if memory allocation failed
 */
static bool message_ensure_capacity(Message *msg, size_t additional_size)
{
    if (msg == NULL || additional_size == 0)
    {
        return false;
    }

    size_t required_size = msg->position + additional_size;

    if (required_size > msg->size)
    {

        size_t new_size = msg->size;
        while (new_size < required_size)
        {
            new_size *= 2;
        }

        unsigned char *new_buffer = (unsigned char *)realloc(msg->buffer, new_size);
        if (new_buffer == NULL)
        {
            return false;
        }

        msg->buffer = new_buffer;
        msg->size = new_size;
    }

    return true;
}

/**
 * Writes data to the message buffer.
 * @param msg The message to write to
 * @param data The data to write
 * @param length The length of the data in bytes
 * @return true if successful, false if memory allocation failed
 */
bool message_write(Message *msg, const void *data, size_t length)
{
    if (msg == NULL || data == NULL || length == 0)
    {
        return false;
    }

    if (!message_ensure_capacity(msg, length))
    {
        return false;
    }

    memcpy(msg->buffer + msg->position, data, length);
    msg->position += length;

    return true;
}

/**
 * Reads data from the message buffer.
 * @param msg The message to read from
 * @param out The buffer to read into
 * @param length The number of bytes to read
 * @return true if successful, false if there is not enough data
 */
bool message_read(Message *msg, void *out, size_t length)
{
    if (msg == NULL || out == NULL || length == 0)
    {
        return false;
    }

    if (msg->position + length > msg->size)
    {
        return false;
    }

    memcpy(out, msg->buffer + msg->position, length);
    msg->position += length;

    return true;
}

/**
 * Gets the raw data buffer from the message.
 * @param msg The message to get data from
 * @param length Pointer to a variable that will receive the data length
 * @return A pointer to the message data buffer
 */
unsigned char *message_get_data(Message *msg, size_t *length)
{
    if (msg == NULL || length == NULL)
    {
        return NULL;
    }

    *length = msg->position;
    return msg->buffer;
}

/**
 * Encrypts the message buffer using AES encryption
 * @param msg The message to encrypt
 * @param key The encryption key (must be 32 bytes for AES-256)
 * @param iv The initialization vector (must be 16 bytes)
 * @return true if encryption was successful, false otherwise
 */
bool message_encrypt(Message *msg, const unsigned char *key, const unsigned char *iv)
{
    if (msg == NULL || key == NULL || iv == NULL || msg->position == 0)
    {
        return false;
    }

    size_t max_ciphertext_len = msg->position + EVP_MAX_BLOCK_LENGTH;
    unsigned char *ciphertext = (unsigned char *)malloc(max_ciphertext_len);
    if (ciphertext == NULL)
    {
        return false;
    }

    size_t ciphertext_len = 0;

    aes_encrypt(msg->buffer, msg->position, (unsigned char *)key, (unsigned char *)iv,
                ciphertext, &ciphertext_len);

    free(msg->buffer);
    msg->buffer = ciphertext;
    msg->size = max_ciphertext_len;
    msg->position = ciphertext_len;

    return true;
}

/**
 * Decrypts the message buffer using AES decryption
 * @param msg The message to decrypt
 * @param key The encryption key (must be 32 bytes for AES-256)
 * @param iv The initialization vector (must be 16 bytes)
 * @return true if decryption was successful, false otherwise
 */
bool message_decrypt(Message *msg, const unsigned char *key, const unsigned char *iv)
{
    if (msg == NULL || key == NULL || iv == NULL || msg->position == 0)
    {
        log_message(ERROR, "Failed to decrypt message, invalid parameters");
        return false;
    }

    unsigned char *plaintext = (unsigned char *)malloc(msg->position);
    if (plaintext == NULL)
    {
        log_message(ERROR, "Failed to allocate memory for decryption");
        return false;
    }

    size_t plaintext_len = 0;

    aes_decrypt(msg->buffer, msg->position, (unsigned char *)key, (unsigned char *)iv,
                plaintext, &plaintext_len);

    if (plaintext_len == 0)
    {
        log_message(ERROR, "Decryption failed - produced zero-length output");
        free(plaintext);
        return false;
    }

    char *safe_text = malloc(plaintext_len + 1);
    if (safe_text)
    {
        memcpy(safe_text, plaintext, plaintext_len);
        safe_text[plaintext_len] = '\0';
        free(safe_text);
    }

    free(msg->buffer);
    msg->buffer = plaintext;
    msg->size = msg->position;
    msg->position = plaintext_len;

    return true;
}