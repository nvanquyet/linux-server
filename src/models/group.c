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
Group *create_group(const char *group_name, const char *password, User *creator, char *error_message) {
    if (!group_name || !password || !creator) {
        snprintf(error_message, 256, "Invalid input for group creation");
        return NULL;
    }

    Group* group = create_new_group(group_name, password);
    if (!group) {
        snprintf(error_message, 256, "Memory allocation failed for group");
        return NULL;
    }

    group->created_by = creator;
    group->member_count = 1;

    DbStatement *stmt = db_prepare(SQL_CREATE_GROUP);
    if (stmt == NULL) {
        snprintf(error_message, 256, "Failed to prepare create group statement");
        free(group);
        return NULL;
    }

    if (!db_bind_string(stmt, 0, group->name)) {
        snprintf(error_message, 256, "Failed to bind group_name");
        db_statement_free(stmt);
        free(group);
        return NULL;
    }

    if (!db_bind_int(stmt, 1, creator->id)) {
        snprintf(error_message, 256, "Failed to bind created_by");
        db_statement_free(stmt);
        free(group);
        return NULL;
    }

    if (!db_bind_long(stmt, 2, group->created_at)) {
        snprintf(error_message, 256, "Failed to bind created_at");
        db_statement_free(stmt);
        free(group);
        return NULL;
    }

    if (!db_bind_string(stmt, 3, group->password)) {
        snprintf(error_message, 256, "Failed to bind password");
        db_statement_free(stmt);
        free(group);
        return NULL;
    }

    if (!db_execute(stmt)) {
        snprintf(error_message, 256, "Failed to execute create group query");
        db_statement_free(stmt);
        free(group);
        return NULL;
    }

    group->id = db_get_insert_id(stmt);
    stmt = NULL;
    db_statement_free(stmt);

    if (!add_group_member(group->id, creator->id, "")) {
        snprintf(error_message, 256, "Failed to add creator to group");
        free(group);
        return NULL;
    }

    snprintf(error_message, 256, "Group created successfully");
    return group;
}

bool delete_group(Group *self, User *user, char *error_message) {
    if (!self) {
        snprintf(error_message, 256, "Group pointer is NULL");
        return false;
    }
    if (!user) {
        snprintf(error_message, 256, "User pointer is NULL");
        return false;
    }

    if (self->created_by == NULL || self->created_by->id != user->id) {
        snprintf(error_message, 256, "User %d is not the owner of group %d", user->id, self->id);
        return false;
    }

    DbStatement *stmt = db_prepare(SQL_DELETE_GROUP);
    if (!stmt) {
        snprintf(error_message, 256, "Failed to prepare delete group statement");
        return false;
    }

    if (!db_bind_int(stmt, 0, self->id)) {
        snprintf(error_message, 256, "Failed to bind group_id parameter");
        db_statement_free(stmt);
        return false;
    }

    if (!db_execute(stmt)) {
        snprintf(error_message, 256, "Failed to execute group deletion query");
        db_statement_free(stmt);
        return false;
    }

    db_statement_free(stmt);
    snprintf(error_message, 256, "Group '%s' (ID: %d) deleted successfully", self->name, self->id);
    return true;
}

Group *get_group_by_id(int group_id) {
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

Group *create_new_group(char* group_name, char *password) {
    if (group_name == NULL || password == NULL || strlen(group_name) == 0) {
        log_message(ERROR, "Group name or password is invalid");
        return NULL;
    }

    Group* group = malloc(sizeof(Group));
    if (group == NULL) {
        log_message(ERROR, "Failed to allocate memory for group");
        return NULL;
    }

    memset(group, 0, sizeof(Group));
    strncpy(group->name, group_name, sizeof(group->name));
    strncpy(group->password, password, sizeof(group->password));
    group->created_at = (long)time(NULL);
    return group;
}

Group *get_group(Group *self, char *errorMsg, size_t errorSize) {
    DbStatement *stmt = db_prepare(SQL_FIND_GROUP_BY_NAME);
    if (!stmt) {
        snprintf(errorMsg, errorSize, "Failed to prepare group lookup query");
        return NULL;
    }

    if (!db_bind_string(stmt, 0, self->name)) {
        snprintf(errorMsg, errorSize, "Failed to bind group_name parameter");
        db_statement_free(stmt);
        return NULL;
    }

    DbResultSet *result = db_execute_query(stmt);
    db_statement_free(stmt);

    if (!result || result->row_count == 0) {
        snprintf(errorMsg, errorSize, "Group not found");
        if (result) db_result_set_free(result);
        return NULL;
    }

    for (int rowIndex = 0; rowIndex < result->row_count; rowIndex++) {
        DbResultRow *row = result->rows[rowIndex];
        int groupId = -1;
        char *dbPassword = NULL;
        long createdAt = 0;
        int createdBy = -1;

        for (int i = 0; i < row->field_count; i++) {
            DbResultField *field = row->fields[i];
            if (!field) continue;

            if (strcmp(field->key, "group_id") == 0 && (field->type == MYSQL_TYPE_LONG || field->type == MYSQL_TYPE_LONGLONG)) {
                groupId = *(int *)field->value;
            } else if (strcmp(field->key, "password") == 0 &&
                       (field->type == MYSQL_TYPE_STRING || field->type == MYSQL_TYPE_VAR_STRING)) {
                dbPassword = (char *)field->value;
            } else if (strcmp(field->key, "created_at") == 0) {
                createdAt = *(long *)field->value;
            } else if (strcmp(field->key, "created_by") == 0) {
                createdBy = *(int *)field->value;
            }
        }

        if (dbPassword && strcmp(dbPassword, self->password) == 0) {
            Group *group = create_new_group(self->name, "");
            if (!group) {
                snprintf(errorMsg, errorSize, "Memory allocation failed");
                db_result_set_free(result);
                return NULL;
            }

            group->id = groupId;
            group->created_at = createdAt;

            db_result_set_free(result);
            return group;
        }
    }

    snprintf(errorMsg, errorSize, "Group name or password incorrect");
    db_result_set_free(result);
    return NULL;
}

void broad_cast_group_noti(int group_id, Message *msg) {
    int count = 0;
    int* member_ids = get_group_members(group_id, &count);

    if (!member_ids || count == 0) {
        log_message(WARN, "Không tìm thấy thành viên nào trong group_id = %d", group_id);
        return;
    }
    broadcast_message(member_ids, count, msg);
    free(member_ids);
}
