#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif
    typedef struct Message Message;

    struct Message
    {
        uint8_t command;
        unsigned char *buffer;
        size_t size;
        size_t position;
    };

    Message *message_create(uint8_t command);
    void message_destroy(Message *msg);
    bool message_write(Message *msg, const void *data, size_t length);
    bool message_read(Message *msg, void *out, size_t length);
    unsigned char *message_get_data(Message *msg, size_t *length);
    void message_reset_read_position(Message *msg);
    Message* message_clone(Message *origin);

    bool message_write_byte(Message *msg, uint8_t value);
    bool message_write_bool(Message *msg, bool value);
    bool message_write_short(Message *msg, uint16_t value);
    bool message_write_int(Message *msg, uint32_t value);
    bool message_write_long(Message *msg, uint64_t value);
    bool message_write_float(Message *msg, float value);
    bool message_write_double(Message *msg, double value);
    bool message_write_utf(Message *msg, const char *str);
    bool message_write_string(Message *msg, const char *str);

    uint8_t message_read_byte(Message *msg);
    bool message_read_bool(Message *msg);
    uint16_t message_read_short(Message *msg);
    uint32_t message_read_int(Message *msg);
    uint64_t message_read_long(Message *msg);
    float message_read_float(Message *msg);
    double message_read_double(Message *msg);
    char *message_read_utf(Message *msg);
    bool message_read_string(Message *msg, char *buffer, size_t buffer_size);

    bool message_encrypt(Message *msg, const unsigned char *key, const unsigned char *iv);
    bool message_decrypt(Message *msg, const unsigned char *key, const unsigned char *iv);

#ifdef __cplusplus
}
#endif

#endif