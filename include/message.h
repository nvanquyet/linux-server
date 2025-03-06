#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct Message Message;

struct Message {
    unsigned char command;
    unsigned char *buffer;
    size_t size;
    size_t position;
};

Message *message_create(unsigned char command);
void message_destroy(Message *msg);
bool message_write(Message *msg, const void *data, size_t length);
bool message_read(Message *msg, void *out, size_t length);
unsigned char *message_get_data(Message *msg, size_t *length);

#endif
