//
// Created by vawnwuyest on 4/5/25.
//
#include "../../include/db_message.h"

#include <database_connector.h>
#include <db_statement.h>
#include <log.h>
#include <sql_statement.h>
#include <stdlib.h>
#include <string.h>

#include "group.h"
#include "group_member.h"
ChatHistory* get_chat_histories_by_user(int user_id, int* out_count) {
    DbStatement* stmt = db_prepare(SQL_GET_CHAT_HISTORIES_BY_USER);
    if (!stmt) {
        log_message(ERROR, "Failed to prepare chat history statement");
        *out_count = 0;
        return NULL;
    }

    // Bind đúng thứ tự các dấu `?` trong SQL
    db_bind_int(stmt, 0, user_id); // CASE WHEN sender_id = ?
    db_bind_int(stmt, 1, user_id); // WHERE sender_id = ?
    db_bind_int(stmt, 2, user_id); // WHERE receiver_id = ?
    db_bind_int(stmt, 3, user_id); // WHERE group_id IN ...
    db_bind_int(stmt, 4, user_id); // JOIN CASE WHEN m.sender_id = ?

    DbResultSet* result = db_execute_query(stmt);
    db_statement_free(stmt);

    if (!result || result->row_count == 0) {
        *out_count = 0;
        if (result) db_result_set_free(result);
        return NULL;
    }

    // First pass to count actual valid entries
    int valid_count = 0;
    for (int i = 0; i < result->row_count; i++) {
        DbResultRow* row = result->rows[i];
        for (int j = 0; j < row->field_count; j++) {
            DbResultField* field = row->fields[j];
            if (!field) continue;
            if (strcmp(field->key, "chat_id") == 0) {
                int chat_id = *(int*)field->value;
                if (chat_id < 0) {
                    // This is a group chat, check if user is still a member
                    bool is_member = check_member_exists(-chat_id, user_id);
                    if (is_member) {
                        valid_count++;
                    }
                } else {
                    // This is a direct message, always valid
                    valid_count++;
                }
                break;
            }
        }
    }

    // Cấp phát bộ nhớ cho lịch sử trò chuyện (only for valid entries)
    ChatHistory* histories = (ChatHistory*)malloc(sizeof(ChatHistory) * valid_count);
    if (!histories) {
        db_result_set_free(result);
        *out_count = 0;
        return NULL;
    }

    // Second pass to fill the array
    int valid_index = 0;
    for (int i = 0; i < result->row_count; i++) {
        DbResultRow* row = result->rows[i];
        bool skip_entry = false;
        int chat_id = 0;

        // First get the chat_id to check membership
        for (int j = 0; j < row->field_count; j++) {
            DbResultField* field = row->fields[j];
            if (!field) continue;
            if (strcmp(field->key, "chat_id") == 0) {
                chat_id = *(int*)field->value;
                if (chat_id < 0) {
                    // Check if user is still a member
                    bool is_member = check_member_exists(-chat_id, user_id);
                    if (!is_member) {
                        skip_entry = true;
                    }
                }
                break;
            }
        }

        if (skip_entry) continue;

        // Now fill in the data
        ChatHistory* history = &histories[valid_index];
        memset(history, 0, sizeof(ChatHistory));

        for (int j = 0; j < row->field_count; j++) {
            DbResultField* field = row->fields[j];
            if (!field) continue;

            if (strcmp(field->key, "chat_id") == 0) {
                history->id = chat_id;
                // Format chat_with information
                if (history->id < 0) {
                    Group* g = get_group_by_id(-history->id);
                    if (g) {
                        snprintf(history->chat_with, sizeof(history->chat_with), "%s", g->name);
                        free(g);
                    } else {
                        snprintf(history->chat_with, sizeof(history->chat_with), "Unknown Group");
                    }
                } else if (history->id != user_id) {
                    User* u = findUserById(history->id);
                    if (u) {
                        snprintf(history->chat_with, sizeof(history->chat_with), "%s", u->username);
                        free(u);
                    } else {
                        snprintf(history->chat_with, sizeof(history->chat_with), "Unknown User");
                    }
                }
            } else if (strcmp(field->key, "last_message") == 0) {
                strncpy(history->last_message, (char*)field->value, sizeof(history->last_message) - 1);
                history->last_message[sizeof(history->last_message) - 1] = '\0'; // Ensure null termination
            } else if (strcmp(field->key, "last_time") == 0) {
                history->last_time = *(long*)field->value;
            } else if (strcmp(field->key, "sender_name") == 0) {
                strncpy(history->sender_name, (char*)field->value, sizeof(history->sender_name) - 1);
                history->sender_name[sizeof(history->sender_name) - 1] = '\0'; // Ensure null termination
            }
        }

        valid_index++;
    }

    *out_count = valid_count;
    db_result_set_free(result);

    return histories;
}

void save_private_message(int sender_id, int receiver_id, const char* content) {
    DbStatement* stmt = db_prepare(SQL_INSERT_PRIVATE_MESSAGE);
    if (!stmt) return;

    time_t now = time(NULL);

    db_bind_int(stmt, 0, sender_id);
    db_bind_int(stmt, 1, receiver_id);
    db_bind_string(stmt, 2, content);
    char datetime_str[20];
    struct tm* tm_info = localtime(&now);
    strftime(datetime_str, sizeof(datetime_str), "%Y-%m-%d %H:%M:%S", tm_info);

    if (!db_bind_string(stmt, 3, datetime_str)) {
        log_message(ERROR, "Failed to bind time parameter");
        db_statement_free(stmt);
        return;
    }


    if (!db_execute(stmt)) {
        log_message(ERROR, "Failed to execute statement");
        db_statement_free(stmt);
        return;
    }
    stmt = NULL;
    db_statement_free(stmt);
    log_message(INFO, "Saved private message from %d to %d: %s", sender_id, receiver_id, content);
}

void save_group_message(int sender_id, int group_id, const char* content) {
    DbStatement* stmt = db_prepare(SQL_INSERT_GROUP_MESSAGE);
    if (!stmt) return;

    if (!stmt) {
        log_message(ERROR, "Failed to prepare delete group statement");
        return;
    }
    time_t now = time(NULL);
    if (!db_bind_int(stmt, 0, sender_id)) {
        log_message(ERROR, "Failed to bind sender_id parameter");
        db_statement_free(stmt);
        return;
    }
    if (!db_bind_int(stmt, 1, group_id)) {
        log_message(ERROR, "Failed to bind group_id parameter");
        db_statement_free(stmt);
        return;
    }
    if (!db_bind_string(stmt, 2, content)) {
        log_message(ERROR, "Failed to bind content parameter");
        db_statement_free(stmt);
        return;
    }

    char datetime_str[20];
    struct tm* tm_info = localtime(&now);
    strftime(datetime_str, sizeof(datetime_str), "%Y-%m-%d %H:%M:%S", tm_info);

    if (!db_bind_string(stmt, 3, datetime_str)) {
        log_message(ERROR, "Failed to bind time parameter");
        db_statement_free(stmt);
        return;
    }

    if (!db_execute(stmt)) {
        log_message(ERROR, "Failed to execute statement");
        db_statement_free(stmt);
        return;
    }
    stmt = NULL;
    db_statement_free(stmt);
    log_message(INFO, "Saved group message from %d to group %d: %s", sender_id, group_id, content);
}

MessageData* get_chat_messages(int user_id, int chat_with_id, int group_id, int* count) {
    *count = 0;
    DbStatement* stmt = NULL;

    if (group_id > 0) {
        stmt = db_prepare(SQL_GET_MESSAGES_WITH_GROUP);
        if (!stmt) {
            log_message(ERROR, "Failed to prepare statement for group messages");
            return NULL;
        }
        db_bind_int(stmt, 0, group_id);
    } else {
        stmt = db_prepare(SQL_GET_MESSAGES_WITH_USER);
        if (!stmt) {
            log_message(ERROR, "Failed to prepare statement for user messages");
            return NULL;
        }
        db_bind_int(stmt, 0, user_id);
        db_bind_int(stmt, 1, chat_with_id);
        db_bind_int(stmt, 2, chat_with_id);
        db_bind_int(stmt, 3, user_id);
    }

    DbResultSet* result = db_execute_query(stmt);
    db_statement_free(stmt);

    if (!result || result->row_count == 0) {
        if (result) db_result_set_free(result);
        return NULL;
    }

    MessageData* messages = (MessageData*)malloc(sizeof(MessageData) * result->row_count);
    if (!messages) {
        db_result_set_free(result);
        return NULL;
    }

    for (int i = 0; i < result->row_count; i++) {
        DbResultRow* row = result->rows[i];
        MessageData* m = &messages[i];

        for (int j = 0; j < row->field_count; j++) {
            DbResultField* field = row->fields[j];
            if (!field) continue;

            if (strcmp(field->key, "sender_id") == 0) {
                m->sender_id = *(int*)field->value;
            } else if (strcmp(field->key, "sender_name") == 0) {
                m->sender_name = strdup((char*)field->value);
            }else if (strcmp(field->key, "message_content") == 0) {
                m->content = strdup((char*)field->value);
            } else if (strcmp(field->key, "timestamp") == 0) {
                m->timestamp = *(long*)field->value;
            }
        }
    }
    log_message(INFO, "Load history success");
    *count = result->row_count;
    db_result_set_free(result);
    return messages;
}
