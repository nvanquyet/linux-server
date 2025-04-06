//
// Created by vawnwuyest on 4/5/25.
//

#ifndef ONLINE_MESSAGE_H
#define ONLINE_MESSAGE_H
typedef struct {
    int id;                   // nếu âm là group, dương là user
    char sender_name[64];       // Ví dụ: "4" hoặc "G5"
    char chat_with[64];       // Ví dụ: "4" hoặc "G5"
    char last_message[256];   // Tin nhắn cuối
    long last_time;           // Thời gian
} ChatHistory;
typedef struct {
    int sender_id;
    char* sender_name;
    char* content;
    long timestamp;
} MessageData;


ChatHistory* get_chat_histories_by_user(int user_id, int* out_count);

void save_private_message(int sender_id, int receiver_id, const char* content);

void save_group_message(int sender_id, int group_id, const char* content);

MessageData* get_chat_messages(int user_id, int chat_with_id, int group_id, int* count);

#endif //ONLINE_MESSAGE_H
