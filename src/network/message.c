#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "message.h"
#include "log.h"

#define MAX_BUFFER_SIZE 4096



// Hàm khởi tạo Message với command
Message* message_create(unsigned char command) {
    Message* msg = (Message*)malloc(sizeof(Message));
    if (msg == NULL) {
        log_message(ERROR, "Failed to allocate memory for Message");
        return NULL;
    }
    
    msg->command = command;
    
    // Khởi tạo output stream
    msg->os_capacity = MAX_BUFFER_SIZE;
    msg->os_buffer = (unsigned char*)malloc(msg->os_capacity);
    if (msg->os_buffer == NULL) {
        log_message(ERROR, "Failed to allocate memory for output buffer");
        free(msg);
        return NULL;
    }
    msg->os_size = 0;
    msg->os_position = 0;
    
    // Input stream chưa được khởi tạo
    msg->is_buffer = NULL;
    msg->is_size = 0;
    msg->is_position = 0;
    
    msg->is_externally_allocated = false;
    
    return msg;
}

// Hàm khởi tạo Message không có command
Message* message_create_empty() {
    Message* msg = (Message*)malloc(sizeof(Message));
    if (msg == NULL) {
        log_message(ERROR, "Failed to allocate memory for Message");
        return NULL;
    }
    
    // Khởi tạo output stream
    msg->os_capacity = MAX_BUFFER_SIZE;
    msg->os_buffer = (unsigned char*)malloc(msg->os_capacity);
    if (msg->os_buffer == NULL) {
        log_message(ERROR, "Failed to allocate memory for output buffer");
        free(msg);
        return NULL;
    }
    msg->os_size = 0;
    msg->os_position = 0;
    
    // Input stream chưa được khởi tạo
    msg->is_buffer = NULL;
    msg->is_size = 0;
    msg->is_position = 0;
    
    msg->is_externally_allocated = false;
    
    return msg;
}

// Hàm khởi tạo Message với data có sẵn
Message* message_create_with_data(unsigned char command, unsigned char* data, size_t data_length) {
    Message* msg = (Message*)malloc(sizeof(Message));
    if (msg == NULL) {
        log_message(ERROR, "Failed to allocate memory for Message");
        return NULL;
    }
    
    msg->command = command;
    
    // Output stream không được sử dụng
    msg->os_buffer = NULL;
    msg->os_size = 0;
    msg->os_capacity = 0;
    msg->os_position = 0;
    
    // Khởi tạo input stream
    msg->is_buffer = (unsigned char*)malloc(data_length);
    if (msg->is_buffer == NULL) {
        log_message(ERROR, "Failed to allocate memory for input buffer");
        free(msg);
        return NULL;
    }
    memcpy(msg->is_buffer, data, data_length);
    msg->is_size = data_length;
    msg->is_position = 0;
    
    msg->is_externally_allocated = false;
    
    return msg;
}

// Hàm giải phóng bộ nhớ
void message_destroy(Message* msg) {
    if (msg == NULL) {
        return;
    }
    
    if (msg->os_buffer != NULL) {
        free(msg->os_buffer);
        msg->os_buffer = NULL;
    }
    
    if (msg->is_buffer != NULL && !msg->is_externally_allocated) {
        free(msg->is_buffer);
        msg->is_buffer = NULL;
    }
    
    free(msg);
}

// Getter cho command
unsigned char message_get_command(Message* msg) {
    if (msg == NULL) {
        log_message(ERROR, "Message is NULL");
        return 0;
    }
    return msg->command;
}

// Setter cho command
void message_set_command(Message* msg, unsigned char command) {
    if (msg == NULL) {
        log_message(ERROR, "Message is NULL");
        return;
    }
    msg->command = command;
}

// Hàm lấy data từ output stream
unsigned char* message_get_data(Message* msg, size_t* length) {
    if (msg == NULL || msg->os_buffer == NULL) {
        log_message(ERROR, "Message or output buffer is NULL");
        if (length != NULL) {
            *length = 0;
        }
        return NULL;
    }
    
    if (length != NULL) {
        *length = msg->os_size;
    }
    return msg->os_buffer;
}

// Writer functions (tương đương DataOutputStream)

// Kiểm tra và mở rộng buffer nếu cần
bool ensure_capacity(Message* msg, size_t additional_bytes) {
    if (msg->os_position + additional_bytes > msg->os_capacity) {
        size_t new_capacity = msg->os_capacity * 2;
        if (new_capacity < msg->os_position + additional_bytes) {
            new_capacity = msg->os_position + additional_bytes;
        }
        
        unsigned char* new_buffer = (unsigned char*)realloc(msg->os_buffer, new_capacity);
        if (new_buffer == NULL) {
            log_message(ERROR, "Failed to reallocate output buffer");
            return false;
        }
        
        msg->os_buffer = new_buffer;
        msg->os_capacity = new_capacity;
    }
    return true;
}

// Ghi một byte vào output stream
bool message_write_byte(Message* msg, unsigned char value) {
    if (msg == NULL || msg->os_buffer == NULL) {
        log_message(ERROR, "Message or output buffer is NULL");
        return false;
    }
    
    if (!ensure_capacity(msg, 1)) {
        return false;
    }
    
    msg->os_buffer[msg->os_position++] = value;
    if (msg->os_position > msg->os_size) {
        msg->os_size = msg->os_position;
    }
    
    return true;
}

// Ghi một short (2 bytes) vào output stream
bool message_write_short(Message* msg, short value) {
    if (msg == NULL || msg->os_buffer == NULL) {
        log_message(ERROR, "Message or output buffer is NULL");
        return false;
    }
    
    if (!ensure_capacity(msg, 2)) {
        return false;
    }
    
    // Big-endian encoding
    msg->os_buffer[msg->os_position++] = (value >> 8) & 0xFF;
    msg->os_buffer[msg->os_position++] = value & 0xFF;
    
    if (msg->os_position > msg->os_size) {
        msg->os_size = msg->os_position;
    }
    
    return true;
}

// Ghi một int (4 bytes) vào output stream
bool message_write_int(Message* msg, int value) {
    if (msg == NULL || msg->os_buffer == NULL) {
        log_message(ERROR, "Message or output buffer is NULL");
        return false;
    }
    
    if (!ensure_capacity(msg, 4)) {
        return false;
    }
    
    // Big-endian encoding
    msg->os_buffer[msg->os_position++] = (value >> 24) & 0xFF;
    msg->os_buffer[msg->os_position++] = (value >> 16) & 0xFF;
    msg->os_buffer[msg->os_position++] = (value >> 8) & 0xFF;
    msg->os_buffer[msg->os_position++] = value & 0xFF;
    
    if (msg->os_position > msg->os_size) {
        msg->os_size = msg->os_position;
    }
    
    return true;
}

// Ghi một long (8 bytes) vào output stream
bool message_write_long(Message* msg, long long value) {
    if (msg == NULL || msg->os_buffer == NULL) {
        log_message(ERROR, "Message or output buffer is NULL");
        return false;
    }
    
    if (!ensure_capacity(msg, 8)) {
        return false;
    }
    
    // Big-endian encoding
    msg->os_buffer[msg->os_position++] = (value >> 56) & 0xFF;
    msg->os_buffer[msg->os_position++] = (value >> 48) & 0xFF;
    msg->os_buffer[msg->os_position++] = (value >> 40) & 0xFF;
    msg->os_buffer[msg->os_position++] = (value >> 32) & 0xFF;
    msg->os_buffer[msg->os_position++] = (value >> 24) & 0xFF;
    msg->os_buffer[msg->os_position++] = (value >> 16) & 0xFF;
    msg->os_buffer[msg->os_position++] = (value >> 8) & 0xFF;
    msg->os_buffer[msg->os_position++] = value & 0xFF;
    
    if (msg->os_position > msg->os_size) {
        msg->os_size = msg->os_position;
    }
    
    return true;
}

// Ghi một string vào output stream (UTF-8)
bool message_write_utf(Message* msg, const char* str) {
    if (msg == NULL || msg->os_buffer == NULL) {
        log_message(ERROR, "Message or output buffer is NULL");
        return false;
    }
    
    if (str == NULL) {
        // Nếu string là NULL, ghi độ dài là 0
        return message_write_short(msg, 0);
    }
    
    size_t str_len = strlen(str);
    if (str_len > 65535) {
        log_message(ERROR, "String is too long: %zu", str_len);
        return false;
    }
    
    // Ghi độ dài của string trước (2 bytes)
    if (!message_write_short(msg, (short)str_len)) {
        return false;
    }
    
    // Ghi nội dung string
    if (!ensure_capacity(msg, str_len)) {
        return false;
    }
    
    memcpy(msg->os_buffer + msg->os_position, str, str_len);
    msg->os_position += str_len;
    
    if (msg->os_position > msg->os_size) {
        msg->os_size = msg->os_position;
    }
    
    return true;
}

// Reader functions (tương đương DataInputStream)

// Đọc một byte từ input stream
unsigned char message_read_byte(Message* msg) {
    if (msg == NULL || msg->is_buffer == NULL || msg->is_position >= msg->is_size) {
        log_message(ERROR, "Failed to read byte: invalid message or buffer or EOF");
        return 0;
    }
    
    return msg->is_buffer[msg->is_position++];
}

// Đọc một short (2 bytes) từ input stream
short message_read_short(Message* msg) {
    if (msg == NULL || msg->is_buffer == NULL || msg->is_position + 1 >= msg->is_size) {
        log_message(ERROR, "Failed to read short: invalid message or buffer or EOF");
        return 0;
    }
    
    // Big-endian decoding
    short value = ((short)(msg->is_buffer[msg->is_position++]) << 8) & 0xFF00;
    value |= ((short)(msg->is_buffer[msg->is_position++])) & 0xFF;
    
    return value;
}

// Đọc một int (4 bytes) từ input stream
int message_read_int(Message* msg) {
    if (msg == NULL || msg->is_buffer == NULL || msg->is_position + 3 >= msg->is_size) {
        log_message(ERROR, "Failed to read int: invalid message or buffer or EOF");
        return 0;
    }
    
    // Big-endian decoding
    int value = ((int)(msg->is_buffer[msg->is_position++]) << 24) & 0xFF000000;
    value |= ((int)(msg->is_buffer[msg->is_position++]) << 16) & 0x00FF0000;
    value |= ((int)(msg->is_buffer[msg->is_position++]) << 8) & 0x0000FF00;
    value |= ((int)(msg->is_buffer[msg->is_position++])) & 0x000000FF;
    
    return value;
}

// Đọc một long (8 bytes) từ input stream
long long message_read_long(Message* msg) {
    if (msg == NULL || msg->is_buffer == NULL || msg->is_position + 7 >= msg->is_size) {
        log_message(ERROR, "Failed to read long: invalid message or buffer or EOF");
        return 0;
    }
    
    // Big-endian decoding
    long long value = ((long long)(msg->is_buffer[msg->is_position++]) << 56) & 0xFF00000000000000LL;
    value |= ((long long)(msg->is_buffer[msg->is_position++]) << 48) & 0x00FF000000000000LL;
    value |= ((long long)(msg->is_buffer[msg->is_position++]) << 40) & 0x0000FF0000000000LL;
    value |= ((long long)(msg->is_buffer[msg->is_position++]) << 32) & 0x000000FF00000000LL;
    value |= ((long long)(msg->is_buffer[msg->is_position++]) << 24) & 0x00000000FF000000LL;
    value |= ((long long)(msg->is_buffer[msg->is_position++]) << 16) & 0x0000000000FF0000LL;
    value |= ((long long)(msg->is_buffer[msg->is_position++]) << 8) & 0x000000000000FF00LL;
    value |= ((long long)(msg->is_buffer[msg->is_position++])) & 0x00000000000000FFLL;
    
    return value;
}

// Đọc một string (UTF-8) từ input stream
char* message_read_utf(Message* msg) {
    if (msg == NULL || msg->is_buffer == NULL) {
        log_message(ERROR, "Message or input buffer is NULL");
        return NULL;
    }
    
    // Đọc độ dài string
    short length = message_read_short(msg);
    if (length <= 0) {
        // Trả về empty string nếu độ dài là 0
        char* empty_str = (char*)malloc(1);
        if (empty_str != NULL) {
            empty_str[0] = '\0';
        }
        return empty_str;
    }
    
    if (msg->is_position + length > msg->is_size) {
        log_message(ERROR, "Failed to read UTF string: invalid length");
        return NULL;
    }
    
    // Cấp phát bộ nhớ cho string (thêm 1 byte cho null terminator)
    char* str = (char*)malloc(length + 1);
    if (str == NULL) {
        log_message(ERROR, "Failed to allocate memory for UTF string");
        return NULL;
    }
    
    // Copy dữ liệu từ buffer
    memcpy(str, msg->is_buffer + msg->is_position, length);
    str[length] = '\0';  // Thêm null terminator
    
    msg->is_position += length;
    
    return str;
}

// Truy cập trực tiếp đến reader (không cần trong C, nhưng giữ lại cho tương thích API)
Message* message_reader(Message* msg) {
    return msg;  // Trong C, chúng ta trả về message để thực hiện các hàm đọc trực tiếp
}

// Truy cập trực tiếp đến writer (không cần trong C, nhưng giữ lại cho tương thích API)
Message* message_writer(Message* msg) {
    return msg;  // Trong C, chúng ta trả về message để thực hiện các hàm ghi trực tiếp
}

// Dọn dẹp tài nguyên (tương đương cleanup trong Java)
void message_cleanup(Message* msg) {
    // Trong C, không cần đóng stream, nhưng chúng ta có thể reset các vị trí
    if (msg != NULL) {
        msg->os_position = 0;
        msg->is_position = 0;
    }
}