#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct Message Message;
struct Message
{
    unsigned char command;

    unsigned char *os_buffer;
    size_t os_size;
    size_t os_capacity;
    size_t os_position;

    unsigned char *is_buffer;
    size_t is_size;
    size_t is_position;

    bool is_externally_allocated;
};

Message *message_create(unsigned char command);
void message_destroy(Message *msg);
Message *message_create_empty();
Message *message_create_with_data(unsigned char command, unsigned char *data, size_t data_length);
unsigned char message_read_byte(Message *msg);
short message_read_short(Message *msg);
bool message_write_byte(Message *msg, unsigned char value);
bool message_write_short(Message *msg, short value);
bool message_write_utf(Message *msg, const char *str);
char *message_read_utf(Message *msg);
unsigned char message_get_command(Message *msg);
void message_set_command(Message *msg, unsigned char command);
unsigned char *message_get_data(Message *msg, size_t *length);
Message *message_reader(Message *msg);
Message *message_writer(Message *msg);
void message_cleanup(Message *msg);

#endif
