#include "message.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "aes_utils.h"
#include "log.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <openssl/evp.h>

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
 * Writes a string to the message buffer, prefixed with its length.
 * @param msg The message to write to
 * @param str The null-terminated string to write
 * @return true if successful, false if memory allocation failed
 */
bool message_write_string(Message *msg, const char *str)
{
    if (msg == NULL || str == NULL)
    {
        return false;
    }

    size_t length = strlen(str);

    if (!message_write(msg, &length, sizeof(length)))
    {
        return false;
    }

    if (length > 0 && !message_write(msg, str, length))
    {
        return false;
    }

    return true;
}

/**
 * Reads a string from the message buffer that was written with message_write_string.
 * @param msg The message to read from
 * @param buffer The buffer to store the string in
 * @param buffer_size The size of the buffer
 * @return true if successful, false if there is not enough data or buffer is too small
 */
bool message_read_string(Message *msg, char *buffer, size_t buffer_size)
{
    if (msg == NULL || buffer == NULL || buffer_size == 0)
    {
        return false;
    }

    size_t length;

    if (!message_read(msg, &length, sizeof(length)))
    {
        return false;
    }

    if (length >= buffer_size)
    {
        return false;
    }

    if (length > 0 && !message_read(msg, buffer, length))
    {
        return false;
    }

    buffer[length] = '\0';

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

/**
 * Resets the read position to the beginning of the buffer
 * @param msg The message to reset
 */
void message_reset_read_position(Message *msg)
{
    if (msg != NULL)
    {
        msg->position = 0;
    }
}

/**
 * Writes a byte value to the message buffer
 * @param msg The message to write to
 * @param value The byte value to write
 * @return true if successful, false if memory allocation failed
 */
bool message_write_byte(Message *msg, uint8_t value)
{
    return message_write(msg, &value, sizeof(value));
}

/**
 * Reads a byte from the message buffer
 * @param msg The message to read from
 * @return The byte value
 */
uint8_t message_read_byte(Message *msg)
{
    uint8_t value = 0;
    message_read(msg, &value, sizeof(value));
    return value;
}

/**
 * Writes a boolean value to the message buffer (1 byte)
 * @param msg The message to write to
 * @param value The boolean value to write
 * @return true if successful, false if memory allocation failed
 */
bool message_write_bool(Message *msg, bool value)
{
    uint8_t byte_val = value ? 1 : 0;
    return message_write_byte(msg, byte_val);
}

/**
 * Reads a boolean from the message buffer
 * @param msg The message to read from
 * @return The boolean value
 */
bool message_read_bool(Message *msg)
{
    return message_read_byte(msg) != 0;
}

/**
 * Writes a short (2 bytes) in network byte order
 * @param msg The message to write to
 * @param value The short value to write
 * @return true if successful, false if memory allocation failed
 */
bool message_write_short(Message *msg, uint16_t value)
{
    uint16_t net_value = htons(value);
    return message_write(msg, &net_value, sizeof(net_value));
}

/**
 * Reads a short (2 bytes) in network byte order
 * @param msg The message to read from
 * @return The short value
 */
uint16_t message_read_short(Message *msg)
{
    uint16_t net_value = 0;
    message_read(msg, &net_value, sizeof(net_value));
    return ntohs(net_value);
}

/**
 * Writes an int (4 bytes) in network byte order
 * @param msg The message to write to
 * @param value The int value to write
 * @return true if successful, false if memory allocation failed
 */
bool message_write_int(Message *msg, uint32_t value)
{
    uint32_t net_value = htonl(value);
    return message_write(msg, &net_value, sizeof(net_value));
}

/**
 * Reads an int (4 bytes) in network byte order
 * @param msg The message to read from
 * @return The int value
 */
uint32_t message_read_int(Message *msg)
{
    uint32_t net_value = 0;
    message_read(msg, &net_value, sizeof(net_value));
    return ntohl(net_value);
}

/**
 * Writes a long (8 bytes) in network byte order
 * @param msg The message to write to
 * @param value The long value to write
 * @return true if successful, false if memory allocation failed
 */
bool message_write_long(Message *msg, uint64_t value)
{
    // Write high 4 bytes
    bool success = message_write_int(msg, (uint32_t)(value >> 32));
    // Write low 4 bytes
    success = success && message_write_int(msg, (uint32_t)(value & 0xFFFFFFFF));
    return success;
}

/**
 * Reads a long (8 bytes) in network byte order
 * @param msg The message to read from
 * @return The long value
 */
uint64_t message_read_long(Message *msg)
{
    // Read high 4 bytes
    uint64_t high = message_read_int(msg);
    // Read low 4 bytes
    uint64_t low = message_read_int(msg);
    return (high << 32) | low;
}

/**
 * Writes a float (4 bytes)
 * @param msg The message to write to
 * @param value The float value to write
 * @return true if successful, false if memory allocation failed
 */
bool message_write_float(Message *msg, float value)
{
    uint32_t int_val;
    memcpy(&int_val, &value, sizeof(value));
    return message_write_int(msg, int_val);
}

/**
 * Reads a float (4 bytes)
 * @param msg The message to read from
 * @return The float value
 */
float message_read_float(Message *msg)
{
    uint32_t int_val = message_read_int(msg);
    float value;
    memcpy(&value, &int_val, sizeof(value));
    return value;
}

/**
 * Writes a double (8 bytes)
 * @param msg The message to write to
 * @param value The double value to write
 * @return true if successful, false if memory allocation failed
 */
bool message_write_double(Message *msg, double value)
{
    uint64_t long_val;
    memcpy(&long_val, &value, sizeof(value));
    return message_write_long(msg, long_val);
}

/**
 * Reads a double (8 bytes)
 * @param msg The message to read from
 * @return The double value
 */
double message_read_double(Message *msg)
{
    uint64_t long_val = message_read_long(msg);
    double value;
    memcpy(&value, &long_val, sizeof(long_val));
    return value;
}

/**
 * Writes a UTF-encoded string (Java style with 2-byte length prefix)
 * @param msg The message to write to
 * @param str The string to write
 * @return true if successful, false if string is too long or memory allocation failed
 */
bool message_write_utf(Message *msg, const char *str)
{
    if (msg == NULL || str == NULL)
    {
        return false;
    }

    size_t str_len = strlen(str);
    if (str_len > 65535)  // Max size representable in 2 bytes
    {
        log_message(ERROR, "UTF string too long: %zu bytes", str_len);
        return false;
    }

    // Write 2-byte length prefix
    if (!message_write_short(msg, (uint16_t)str_len))
    {
        return false;
    }

    // Write string bytes (without null terminator)
    if (str_len > 0 && !message_write(msg, str, str_len))
    {
        return false;
    }

    return true;
}

/**
 * Reads a UTF-encoded string (Java style with 2-byte length prefix)
 * @param msg The message to read from
 * @return The string (caller is responsible for freeing it) or NULL on error
 */
char *message_read_utf(Message *msg)
{
    if (msg == NULL)
    {
        return NULL;
    }

    // Read 2-byte length prefix
    uint16_t str_len = message_read_short(msg);
    
    if (str_len == 0)
    {
        // Empty string
        return strdup("");
    }

    // Check if there's enough data
    size_t orig_pos = msg->position;
    if (orig_pos + str_len > msg->size)
    {
        log_message(ERROR, "Not enough data to read UTF string of length %u", str_len);
        return NULL;
    }

    // Allocate space for string plus null terminator
    char *str = malloc(str_len + 1);
    if (str == NULL)
    {
        log_message(ERROR, "Failed to allocate memory for UTF string");
        return NULL;
    }

    // Read string bytes
    if (!message_read(msg, str, str_len))
    {
        free(str);
        return NULL;
    }

    // Add null terminator
    str[str_len] = '\0';
    return str;
}