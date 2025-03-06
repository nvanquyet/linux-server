#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct Message Message;
struct Message {
    unsigned char command;
    
    // Output stream members (tương đương ByteArrayOutputStream và DataOutputStream)
    unsigned char* os_buffer;   // Buffer lưu trữ data
    size_t os_size;             // Kích thước hiện tại của data
    size_t os_capacity;         // Sức chứa tối đa của buffer
    size_t os_position;         // Vị trí hiện tại khi ghi
    
    // Input stream members (tương đương ByteArrayInputStream và DataInputStream)
    unsigned char* is_buffer;   // Buffer cho input
    size_t is_size;             // Kích thước của input data
    size_t is_position;         // Vị trí hiện tại khi đọc
    
    bool is_externally_allocated; // Flag để biết buffer có được cấp phát từ bên ngoài không
};

Message* message_create(unsigned char command);
void message_destroy(Message* msg);
Message* message_create_empty();
Message* message_create_with_data(unsigned char command, unsigned char* data, size_t data_length) ;
unsigned char message_read_byte(Message* msg);
short message_read_short(Message* msg);
bool message_write_byte(Message* msg, unsigned char value);
bool message_write_short(Message* msg, short value);
bool message_write_utf(Message* msg, const char* str);
char* message_read_utf(Message* msg);
unsigned char message_get_command(Message* msg);
void message_set_command(Message* msg, unsigned char command);
unsigned char* message_get_data(Message* msg, size_t* length);
Message* message_reader(Message* msg);
Message* message_writer(Message* msg);
void message_cleanup(Message* msg);

#endif
