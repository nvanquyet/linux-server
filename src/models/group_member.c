#include "../../include/group_member.h"
#include "../../include/db_statement.h"
#include "../../include/sql_statement.h"
#include "../../include/log.h"
#include "../../include/user.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "group.h"

bool add_group_member(int group_id, int user_id, char *error_message) {
    // Kiểm tra xem thành viên đã tồn tại trong nhóm chưa
    if (check_member_exists(group_id, user_id)) {
        if (error_message != NULL) {
            snprintf(error_message, ERROR_MESSAGE_SIZE, "User %d is already a member of group %d", user_id, group_id);
        }
        return false;
    }

    // Tiến hành thêm thành viên vào nhóm như bình thường
    DbStatement *stmt = db_prepare(SQL_ADD_GROUP_MEMBER);
    if (!stmt) {
        snprintf(error_message, ERROR_MESSAGE_SIZE, "Failed to prepare statement for adding group member");
        log_message(ERROR, "%s", error_message);
        return false;
    }

    if (!db_bind_int(stmt, 0, group_id) || !db_bind_int(stmt, 1, user_id) ||
        !db_bind_long(stmt, 2, (long)time(NULL)) || !db_bind_int(stmt, 3, 0)) {
        snprintf(error_message, ERROR_MESSAGE_SIZE, "Failed to bind parameters");
        log_message(ERROR, "%s", error_message);
        db_statement_free(stmt);
        return false;
        }

    if (!db_execute(stmt)) {
        snprintf(error_message, ERROR_MESSAGE_SIZE, "Failed to execute query to add group member");
        log_message(ERROR, "%s", error_message);
        db_statement_free(stmt);
        return false;
    }
    stmt = NULL;
    db_statement_free(stmt);
    log_message(INFO, "Successfully added user %d to group %d", user_id, group_id);
    return true;
}

bool remove_group_member(int group_id, int user_id, char *error_message) {
    // Kiểm tra xem thành viên có tồn tại trong nhóm không
    if (!check_member_exists(group_id, user_id)) {
        if (error_message != NULL) {
            snprintf(error_message, ERROR_MESSAGE_SIZE, "User %d is not a member of group %d", user_id, group_id);
        }
        return false;
    }
    DbStatement *stmt = db_prepare(SQL_REMOVE_GROUP_MEMBER);
    if (!stmt) {
        snprintf(error_message, ERROR_MESSAGE_SIZE, "Failed to prepare statement for removing group member");
        log_message(ERROR, "%s", error_message);
        return false;
    }

    if (!db_bind_int(stmt, 0, group_id) || !db_bind_int(stmt, 1, user_id)) {
        snprintf(error_message, ERROR_MESSAGE_SIZE, "Failed to bind parameters");
        log_message(ERROR, "%s", error_message);
        db_statement_free(stmt);
        return false;
    }

    if (!db_execute(stmt)) {
        snprintf(error_message, ERROR_MESSAGE_SIZE, "Failed to execute query to remove group member");
        log_message(ERROR, "%s", error_message);
        db_statement_free(stmt);
        return false;
    }
    stmt = NULL;
    db_statement_free(stmt);
    log_message(INFO, "Successfully removed user %d from group %d", user_id, group_id);
    return true;
}

Group **find_groups_by_user(int user_id, int *out_count) {
    if (!out_count) {
        log_message(ERROR, "Invalid output count pointer");
        return NULL;
    }
    *out_count = 0;

    // Chuẩn bị câu lệnh SQL để lấy các Group mà user tham gia
    DbStatement *stmt = db_prepare(SQL_GET_GROUPS_BY_USER);
    if (!stmt) {
        log_message(ERROR, "Failed to prepare statement for retrieving groups by user");
        return NULL;
    }

    // Bind user_id vào tham số đầu tiên
    if (!db_bind_int(stmt, 0, user_id)) {
        log_message(ERROR, "Failed to bind user_id parameter");
        db_statement_free(stmt);
        return NULL;
    }

    // Thực thi câu lệnh truy vấn
    DbResultSet *result = db_execute_query(stmt);
    db_statement_free(stmt);
    if (!result) {
        log_message(ERROR, "Failed to execute query for retrieving groups by user");
        return NULL;
    }

    // Cấp phát mảng các con trỏ Group với kích thước bằng số dòng trả về
    Group **group_array = calloc(result->row_count, sizeof(Group *));
    if (!group_array) {
        log_message(ERROR, "Failed to allocate memory for group array");
        db_result_set_free(result);
        return NULL;
    }

    int count = 0;
    // Duyệt qua từng hàng kết quả và tạo đối tượng Group cho mỗi hàng
    for (int i = 0; i < result->row_count; i++) {
        DbResultRow *row = result->rows[i];
        Group *group = (Group *)malloc(sizeof(Group));
        if (!group) {
            log_message(ERROR, "Memory allocation failed for group at row %d", i);
            continue;
        }
        memset(group, 0, sizeof(Group));

        // Giả sử các trường trả về từ truy vấn: group_id, group_name, created_at, created_by
        for (int j = 0; j < row->field_count; j++) {
            DbResultField *field = row->fields[j];
            if (!field) continue;
            if (strcmp(field->key, "group_id") == 0) {
                group->id = *(int *)field->value;
            } else if (strcmp(field->key, "group_name") == 0) {
                strncpy(group->name, (char *)field->value, sizeof(group->name) - 1);
                group->name[sizeof(group->name) - 1] = '\0';
            } else if (strcmp(field->key, "created_at") == 0) {
                group->created_at = *(long *)field->value;
            } else if (strcmp(field->key, "created_by") == 0) {
                int creator_id = *(int *)field->value;
                group->created_by = findUserById(creator_id); // Hàm này cần được định nghĩa trong module user
            }
        }
        group->member_count = 0; // Thành viên có thể được tải riêng nếu cần
        group_array[count++] = group;
    }

    *out_count = count;
    db_result_set_free(result);
    log_message(INFO, "Retrieved %d groups for user %d", count, user_id);
    return group_array;
}
int* get_group_members(int group_id, int* out_count) {
    if (!out_count) {
        log_message(ERROR, "Invalid output count pointer");
        return NULL;
    }
    *out_count = 0;

    DbStatement* stmt = db_prepare("SELECT user_id FROM group_members WHERE group_id = ?");
    if (!stmt) {
        log_message(ERROR, "Failed to prepare group members statement");
        return NULL;
    }

    if (!db_bind_int(stmt, 0, group_id)) {
        log_message(ERROR, "Failed to bind group_id parameter");
        db_statement_free(stmt);
        return NULL;
    }

    DbResultSet* result = db_execute_query(stmt);
    db_statement_free(stmt);

    if (!result) {
        log_message(ERROR, "Failed to execute group members query");
        return NULL;
    }

    int capacity = 16;
    int* user_ids = malloc(capacity * sizeof(int));
    if (!user_ids) {
        log_message(ERROR, "Memory allocation failed for user_ids");
        db_result_set_free(result);
        return NULL;
    }

    int count = 0;
    DbResultRow* row;

    while ((row = db_result_next_row(result))) {
        DbResultField* field = db_result_get_field(row, "user_id");
        if (!field || !field->value) {
            log_message(WARN, "Missing user_id field in row");
            continue;
        }

        if (count >= capacity) {
            capacity *= 2;
            int* new_arr = realloc(user_ids, capacity * sizeof(int));
            if (!new_arr) {
                log_message(ERROR, "Failed to expand user_ids array");
                free(user_ids);
                db_result_set_free(result);
                return NULL;
            }
            user_ids = new_arr;
        }

        user_ids[count++] = *(int*)field->value;
    }

    db_result_set_free(result);
    *out_count = count;
    return user_ids;
}

bool check_member_exists(int group_id, int user_id) {
    DbStatement *stmt = db_prepare(SQL_CHECK_MEMBER_EXISTENCE);
    if (!stmt) {
        log_message(ERROR, "Failed to prepare statement for checking member existence");
        return false;
    }

    // Bind group_id vào tham số thứ 0
    if (!db_bind_int(stmt, 0, group_id)) {
        log_message(ERROR, "Failed to bind group_id parameter");
        db_statement_free(stmt);
        return false;
    }

    // Bind user_id vào tham số thứ 1
    if (!db_bind_int(stmt, 1, user_id)) {
        log_message(ERROR, "Failed to bind user_id parameter");
        db_statement_free(stmt);
        return false;
    }

    // Thực thi câu lệnh SELECT để kiểm tra số lượng bản ghi
    int count = 0;
    if (!db_return_execute(stmt, &count)) {
        log_message(ERROR, "Failed to execute query to check member existence");
        db_statement_free(stmt);
        return false;
    }
    stmt = NULL;
    db_statement_free(stmt);
    // Nếu count > 0, thành viên đã tồn tại trong nhóm
    return count > 0;
}
