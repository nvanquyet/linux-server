#include "message.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_BUFFER_SIZE 64

/**
 * Creates a new message with the specified command.
 * @param command The command code for this message
 * @return A pointer to the newly created Message, or NULL if allocation failed
 */
Message *message_create(unsigned char command) {
    Message *msg = (Message *)malloc(sizeof(Message));
    if (msg == NULL) {
        return NULL;
    }
    
    msg->command = command;
    msg->buffer = (unsigned char *)malloc(INITIAL_BUFFER_SIZE);
    if (msg->buffer == NULL) {
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
void message_destroy(Message *msg) {
    if (msg != NULL) {
        if (msg->buffer != NULL) {
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
static bool message_ensure_capacity(Message *msg, size_t additional_size) {
    if (msg == NULL || additional_size == 0) {
        return false;
    }
    
    size_t required_size = msg->position + additional_size;
    
    // Check if we need to expand the buffer
    if (required_size > msg->size) {
        // Calculate new size (double the current size until it's large enough)
        size_t new_size = msg->size;
        while (new_size < required_size) {
            new_size *= 2;
        }
        
        // Reallocate the buffer
        unsigned char *new_buffer = (unsigned char *)realloc(msg->buffer, new_size);
        if (new_buffer == NULL) {
            return false;
        }
        
        // Update message with new buffer
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
bool message_write(Message *msg, const void *data, size_t length) {
    if (msg == NULL || data == NULL || length == 0) {
        return false;
    }
    
    // Ensure buffer has enough space
    if (!message_ensure_capacity(msg, length)) {
        return false;
    }
    
    // Copy data into buffer
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
bool message_read(Message *msg, void *out, size_t length) {
    if (msg == NULL || out == NULL || length == 0) {
        return false;
    }
    
    // Check if there is enough data to read
    if (msg->position + length > msg->size) {
        return false;
    }
    
    // Copy data from buffer to output
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
unsigned char *message_get_data(Message *msg, size_t *length) {
    if (msg == NULL || length == NULL) {
        return NULL;
    }
    
    *length = msg->position;
    return msg->buffer;
}