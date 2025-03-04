#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint8_t command;
    uint8_t *data;
    size_t length;
    size_t capacity;
} Message;

Message *message_create(uint8_t command);
Message *message_create_with_data(uint8_t command, const uint8_t *data, size_t length);
void message_write(Message *msg, const void *data, size_t length);
uint8_t *message_get_data(Message *msg, size_t *length);
void message_cleanup(Message *msg);

#endif
