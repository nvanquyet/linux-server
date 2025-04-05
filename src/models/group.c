// group.c
// Created by vawnwuyest on 4/1/25

#include "../../include/group.h"
#include <db_statement.h>
#include "../../include/log.h"
#include "../../include/sql_statement.h"
#include "../../include/user.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "group_member.h"

bool create_group(Group *self, const char *group_name, User *creator) {
    if (!self || !group_name || !creator) {
        log_message(ERROR, "Invalid input for group creation");
        return false;
    }

    // Gán tên cho nhóm
    strncpy(self->name, group_name, sizeof(self->name) - 1);
    self->name[sizeof(self->name) - 1] = '\0';
    // Lấy thời gian hiện tại
    self->created_at = (long)time(NULL);
    self->created_by = creator;
    self->member_count = 0;

    // Chuẩn bị câu lệnh SQL để chèn nhóm vào DB.
    // SQL_CREATE_GROUP được định nghĩa dưới dạng:
    // "INSERT INTO groups (group_name, created_by, created_at) VALUES (?, ?, ?)"
    DbStatement *stmt = db_prepare(SQL_CREATE_GROUP);
    if (stmt == NULL) {
        log_message(ERROR, "Failed to prepare create group statement");
        creator->service->server_message(creator->session, "Group creation failed, server error");
         return false;
    }

    // Bind các tham số: Tên nhóm, ID người tạo và thời gian tạo.
    if (!db_bind_string(stmt, 0, self->name)) {
        log_message(ERROR, "Failed to bind group_name parameter");
        db_statement_free(stmt);
         return false;
    }
    if (!db_bind_int(stmt, 1, creator->id)) {
        log_message(ERROR, "Failed to bind created_by parameter");
        db_statement_free(stmt);
         return false;
    }
    if (!db_bind_long(stmt, 2, self->created_at)) {
        log_message(ERROR, "Failed to bind created_at parameter");
        db_statement_free(stmt);
         return false;
    }
    if (!db_execute(stmt))
    {
        log_message(ERROR, "Failed to execute group creation query");
        db_statement_free(stmt);
        creator->service->server_message(creator->session, "Group creation failed, server error");
         return false;
    }
    self->id = db_get_insert_id(stmt);
    //free memory
    stmt = NULL;
    db_statement_free(stmt);

    //Add to member
    if (!add_group_member(self->id, creator->id, "")) {
        log_message(ERROR, "Failed to add creator to group");
        return false;
    }
    log_message(INFO, "Success to add creator to group");
    return true;
}

bool delete_group(Group *self, User *user) {
    if (!self) {
        log_message(ERROR, "Group pointer is NULL");
        return false;
    }
    if (!user) {
        log_message(ERROR, "User pointer is NULL");
        return false;
    }


    if (self->created_by == NULL || self->created_by->id != user->id) {
        log_message(WARN, "User %d is not the owner of group %d, deletion denied", user->id, self->id);
        return false;
    }

    // Chuẩn bị câu lệnh SQL để xóa nhóm khỏi DB
    DbStatement *stmt = db_prepare(SQL_DELETE_GROUP);
    if (!stmt) {
        log_message(ERROR, "Failed to prepare delete group statement");
        return false;
    }

    if (!db_bind_int(stmt, 0, self->id)) {
        log_message(ERROR, "Failed to bind group_id parameter");
        db_statement_free(stmt);
        return false;
    }

    if (!db_execute(stmt)) {
        log_message(ERROR, "Failed to execute group deletion query");
        db_statement_free(stmt);
        return false;
    }

    stmt = NULL;
    db_statement_free(stmt);
    log_message(INFO, "Group '%s' (ID: %d) deleted successfully", self->name, self->id);
    return true;
}

Group *get_group(int group_id) {
    if (group_id <= 0) {
        log_message(ERROR, "Invalid group ID");
        return NULL;
    }

    // Chuẩn bị câu lệnh SQL để lấy thông tin nhóm
    DbStatement *stmt = db_prepare(SQL_GET_GROUP);
    if (!stmt) {
        log_message(ERROR, "Failed to prepare statement for retrieving group");
        return NULL;
    }

    // Bind ID của nhóm vào câu lệnh SQL
    if (!db_bind_int(stmt, 0, group_id)) {
        log_message(ERROR, "Failed to bind group ID");
        db_statement_free(stmt);
        return NULL;
    }

    // Thực thi truy vấn và lấy kết quả
    DbResultSet *result = db_execute_query(stmt);
    db_statement_free(stmt);
    if (!result || result->row_count == 0) {
        log_message(WARN, "Group ID %d not found", group_id);
        db_result_set_free(result);
        return NULL;
    }

    // Cấp phát bộ nhớ cho đối tượng Group
    Group *group = (Group *)malloc(sizeof(Group));
    if (!group) {
        log_message(ERROR, "Failed to allocate memory for Group");
        db_result_set_free(result);
        return NULL;
    }
    memset(group, 0, sizeof(Group));

    // Lấy dữ liệu từ hàng đầu tiên của kết quả truy vấn
    DbResultRow *row = result->rows[0];
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
            group->created_by = findUserById(creator_id); // Hàm này lấy thông tin User
        }
    }

    db_result_set_free(result);
    log_message(INFO, "Retrieved group %d: %s", group->id, group->name);
    return group;
}
