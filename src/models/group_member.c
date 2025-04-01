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

bool add_group_member(int group_id, int user_id) {
    DbStatement *stmt = db_prepare(SQL_ADD_GROUP_MEMBER);
    if (!stmt) {
        log_message(ERROR, "Failed to prepare statement for adding group member");
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
    // Bind joined_at (thời gian hiện tại)
    long joined_at = (long)time(NULL);
    if (!db_bind_long(stmt, 2, joined_at)) {
        log_message(ERROR, "Failed to bind joined_at parameter");
        db_statement_free(stmt);
        return false;
    }
    int role = 0;
    if (!db_bind_int(stmt, 3, role)) {
        log_message(ERROR, "Failed to bind role parameter");
        db_statement_free(stmt);
        return false;
    }

    // Thực thi câu lệnh INSERT
    if (!db_execute_update(stmt)) {
        log_message(ERROR, "Failed to execute query to add group member");
        db_statement_free(stmt);
        return false;
    }

    db_statement_free(stmt);
    log_message(INFO, "Successfully added user %d to group %d", user_id, group_id);
    return true;
}

bool remove_group_member(int group_id, int user_id) {
    // Chuẩn bị câu lệnh DELETE cho bảng group_members
    DbStatement *stmt = db_prepare(SQL_REMOVE_GROUP_MEMBER);
    if (!stmt) {
        log_message(ERROR, "Failed to prepare statement for removing group member");
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

    // Thực thi câu lệnh DELETE
    if (!db_execute_update(stmt)) {
        log_message(ERROR, "Failed to execute query to remove group member");
        db_statement_free(stmt);
        return false;
    }

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
