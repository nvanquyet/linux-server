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
ChatHistory* get_chat_histories_by_user(int user_id, int* out_count) {
    DbStatement* stmt = db_prepare(SQL_GET_CHAT_HISTORIES_BY_USER);
    if (!stmt) {
        log_message(ERROR, "Failed to prepare chat history statement");
        return NULL;
    }

    // Bind đúng thứ tự các dấu `?` trong SQL
    db_bind_int(stmt, 1, user_id); // dùng trong CASE
    db_bind_int(stmt, 2, user_id); // sender_id
    db_bind_int(stmt, 3, user_id); // receiver_id
    db_bind_int(stmt, 4, user_id); // group_members

    DbResultSet* result = db_execute_query(stmt);
    db_statement_free(stmt);
    if (!result || result->row_count == 0) {
        *out_count = 0;
        db_result_set_free(result);
        return NULL;
    }

    // Cấp phát bộ nhớ cho lịch sử trò chuyện
    ChatHistory* histories = (ChatHistory*)malloc(sizeof(ChatHistory) * result->row_count);
    if (!histories) {
        db_result_set_free(result);
        return NULL;
    }

    for (int i = 0; i < result->row_count; i++) {
        DbResultRow* row = result->rows[i];
        ChatHistory* history = &histories[i];
        memset(history, 0, sizeof(ChatHistory));

        for (int j = 0; j < row->field_count; j++) {
            DbResultField* field = row->fields[j];
            if (!field) continue;

            if (strcmp(field->key, "chat_id") == 0) {
                history->id = *(int*)field->value;

                // Format `chat_with` = "4" (user) hoặc "G5" (group)
                if (history->id < 0) {
                    snprintf(history->chat_with, sizeof(history->chat_with), "G%d", -history->id);
                } else {
                    snprintf(history->chat_with, sizeof(history->chat_with), "%d", history->id);
                }
            } else if (strcmp(field->key, "last_message") == 0) {
                // Kiểm tra kích thước để tránh lỗi tràn bộ nhớ
                strncpy(history->last_message, (char*)field->value, sizeof(history->last_message) - 1);
            } else if (strcmp(field->key, "last_time") == 0) {
                history->last_time = *(long*)field->value;
            } else if (strcmp(field->key, "sender_name") == 0) {
                // Thêm tên người gửi
                strncpy(history->sender_name, (char*)field->value, sizeof(history->sender_name) - 1);
            }
        }
    }

    *out_count = result->row_count;
    db_result_set_free(result);

    // Trả về kết quả
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
