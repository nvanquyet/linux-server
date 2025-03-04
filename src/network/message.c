#include "message.h"
#include <stdio.h>

#define INITIAL_CAPACITY 128

Message *message_create(uint8_t command) {
    Message *msg = (Message *)malloc(sizeof(Message));
    if (!msg) return NULL;
    
    msg->command = command;
    msg->capacity = INITIAL_CAPACITY;
    msg->length = 0;
    msg->data = (uint8_t *)malloc(msg->capacity);
    
    if (!msg->data) {
        free(msg);
        return NULL;
    }
    
    return msg;
}

Message *message_create_with_data(uint8_t command, const uint8_t *data, size_t length) {
    Message *msg = (Message *)malloc(sizeof(Message));
    if (!msg) return NULL;

    msg->command = command;
    msg->capacity = length;
    msg->length = length;
    msg->data = (uint8_t *)malloc(length);
    
    if (!msg->data) {
        free(msg);
        return NULL;
    }
    
    memcpy(msg->data, data, length);
    return msg;
}

void message_write(Message *msg, const void *data, size_t length) {
    if (msg->length + length > msg->capacity) {
        msg->capacity *= 2;
        uint8_t *new_data = (uint8_t *)realloc(msg->data, msg->capacity);
        if (!new_data) return;
        msg->data = new_data;
    }
    
    memcpy(msg->data + msg->length, data, length);
    msg->length += length;
}

uint8_t *message_get_data(Message *msg, size_t *length) {
    *length = msg->length;
    return msg->data;
}

void message_cleanup(Message *msg) {
    if (!msg) return;
    free(msg->data);
    free(msg);
}
