#include "db_statement.h"
#include "database_connector.h"
#include <stdlib.h>
#include <string.h>
#include <mysql/mysql.h>
#include "log.h"
#include "stdbool.h"
// #if MYSQL_VERSION_ID >= 80000
//     typedef bool my_bool; 
// #endif

DbStatement *db_prepare(const char *query)
{
    DbStatement *statement = calloc(1, sizeof(DbStatement));
    if (!statement)
    {
        log_message(ERROR, "Failed to allocate statement");
        return NULL;
    }

    statement->conn = db_manager_get_connection();
    if (!statement->conn)
    {
        free(statement);
        return NULL;
    }

    statement->stmt = mysql_stmt_init(statement->conn);
    if (!statement->stmt || mysql_stmt_prepare(statement->stmt, query, strlen(query)) != 0)
    {
        log_message(ERROR, "Failed to prepare statement: %s", mysql_stmt_error(statement->stmt));
        db_manager_release_connection(statement->conn);
        if (statement->stmt)
            mysql_stmt_close(statement->stmt);
        free(statement);
        return NULL;
    }

    statement->binds = NULL;
    statement->lengths = NULL;
    statement->param_count = mysql_stmt_param_count(statement->stmt);
    statement->is_bound = false;

    return statement;
}

bool db_execute(DbStatement *stmt)
{
    bool success = false;

    if (!stmt || !stmt->stmt)
    {
        return false;
    }

    if (stmt->binds && !stmt->is_bound)
    {
        if (mysql_stmt_bind_param(stmt->stmt, stmt->binds) != 0)
        {
            goto cleanup;
        }
        stmt->is_bound = true;
    }

    success = mysql_stmt_execute(stmt->stmt) == 0;
cleanup:

    if (stmt->binds)
    {
        free(stmt->binds);
        stmt->binds = NULL;
    }

    mysql_stmt_close(stmt->stmt);
    db_manager_release_connection(stmt->conn);
    free(stmt);

    return success;
}

bool db_bind_string(DbStatement *stmt, int index, const char *value)
{
    if (!stmt || !stmt->stmt || index < 0)
    {
        log_message(ERROR, "Invalid statement or index for binding");
        return false;
    }

    if (!value)
    {
        log_message(ERROR, "Attempt to bind NULL string at index %d", index);
        return false;
    }

    unsigned long current_param_count = mysql_stmt_param_count(stmt->stmt);

    if (!stmt->binds || stmt->param_count != current_param_count)
    {

        if (stmt->binds)
        {
            free(stmt->binds);
            stmt->binds = NULL;
        }

        if (stmt->lengths)
        {
            free(stmt->lengths);
            stmt->lengths = NULL;
        }

        stmt->param_count = current_param_count;
        log_message(DEBUG, "Statement has %lu parameters", stmt->param_count);

        if (stmt->param_count == 0)
        {
            log_message(ERROR, "Statement has no parameters to bind");
            return false;
        }

        stmt->binds = calloc(stmt->param_count, sizeof(MYSQL_BIND));
        if (!stmt->binds)
        {
            log_message(ERROR, "Failed to allocate memory for parameter bindings");
            return false;
        }

        stmt->lengths = calloc(stmt->param_count, sizeof(unsigned long));
        if (!stmt->lengths)
        {
            free(stmt->binds);
            stmt->binds = NULL;
            log_message(ERROR, "Failed to allocate memory for length values");
            return false;
        }

        stmt->is_bound = false;
    }

    if (index >= stmt->param_count)
    {
        log_message(ERROR, "Parameter index %d out of bounds (max: %d)",
                    index, stmt->param_count - 1);
        return false;
    }

    stmt->lengths[index] = strlen(value);
    memset(&stmt->binds[index], 0, sizeof(MYSQL_BIND));
    stmt->binds[index].buffer_type = MYSQL_TYPE_STRING;
    stmt->binds[index].buffer = (void *)value;
    stmt->binds[index].buffer_length = stmt->lengths[index];
    stmt->binds[index].length = &stmt->lengths[index];
    stmt->binds[index].is_null = 0;

    log_message(DEBUG, "Successfully bound string value: %s (length: %lu) at index %d",
                value, stmt->lengths[index], index);
    return true;
}
bool db_return_execute(DbStatement *stmt, int *result) {
    if (!stmt || !result) {
        log_message(ERROR, "Invalid statement or result pointer");
        return false;
    }

    // Thực thi truy vấn và lấy kết quả
    DbResultSet *result_set = db_execute_query(stmt);
    if (!result_set) {
        log_message(ERROR, "Failed to execute query");
        return false;
    }

    // Kiểm tra nếu có ít nhất một dòng trong kết quả
    if (!db_fetch_row(result_set)) {
        log_message(ERROR, "Failed to fetch result row");
        db_result_set_free(result_set);  // Giải phóng tài nguyên của result_set
        return false;
    }

    // Lấy dữ liệu từ kết quả
    if (!db_get_int(result_set, 0, result)) {
        log_message(ERROR, "Failed to get result from query");
        db_result_set_free(result_set);  // Giải phóng tài nguyên của result_set
        return false;
    }
    db_result_set_free(result_set);
    return true;
}

bool db_fetch_row(DbResultSet *result_set) {
    if (!result_set) {
        log_message(ERROR, "Invalid result set is NULL");
        return false;
    }

    // Kiểm tra xem có dòng nào trong result_set để lấy không
    if (result_set->current_row < result_set->row_count - 1) {
        result_set->current_row++;  // Di chuyển đến dòng kế tiếp
        return true;
    }

    // Nếu không còn dòng nào để lấy
    log_message(ERROR, "No more rows in result set");
    return false;
}

bool db_get_int(DbResultSet *result_set, int column_index, int *result) {
    if (!result_set || !result) {
        log_message(ERROR, "Invalid result set or result pointer");
        return false;
    }

    // Kiểm tra chỉ số cột hợp lệ
    if (column_index < 0 || column_index >= result_set->rows[result_set->current_row]->field_count) {
        log_message(ERROR, "Invalid column index");
        return false;
    }

    // Lấy giá trị từ cột tại column_index của dòng hiện tại
    *result = *((int*)result_set->rows[result_set->current_row]->fields[column_index]->value);

    return true;
}


DbResultSet *db_execute_query(DbStatement *stmt)
{
    if (!stmt || !stmt->stmt)
    {
        log_message(ERROR, "Invalid statement for execute_query");
        return NULL;
    }

    log_message(DEBUG, "Executing prepared query");

    if (stmt->binds && !stmt->is_bound)
    {
        log_message(DEBUG, "Binding parameters to statement");
        if (mysql_stmt_bind_param(stmt->stmt, stmt->binds) != 0)
        {
            log_message(ERROR, "Failed to bind parameters: %s",
                        mysql_stmt_error(stmt->stmt));
            goto cleanup_no_result;
        }
        stmt->is_bound = true;
        log_message(DEBUG, "Parameters bound successfully");
    }

    if (mysql_stmt_execute(stmt->stmt) != 0)
    {
        log_message(ERROR, "Failed to execute statement: %s",
                    mysql_stmt_error(stmt->stmt));
        goto cleanup_no_result;
    }

    MYSQL_RES *meta = mysql_stmt_result_metadata(stmt->stmt);
    if (!meta)
    {
        log_message(ERROR, "No result metadata available: %s",
                    mysql_stmt_error(stmt->stmt));
        goto cleanup_no_result;
    }
    log_message(DEBUG, "Got result metadata");

    if (mysql_stmt_store_result(stmt->stmt) != 0)
    {
        log_message(ERROR, "Failed to store result: %s",
                    mysql_stmt_error(stmt->stmt));
        mysql_free_result(meta);
        goto cleanup_no_result;
    }
    log_message(DEBUG, "Stored result");

    unsigned int num_fields = mysql_num_fields(meta);
    MYSQL_FIELD *fields = mysql_fetch_fields(meta);
    my_ulonglong num_rows = mysql_stmt_num_rows(stmt->stmt);

    DbResultSet *result_set = calloc(1, sizeof(DbResultSet));
    if (!result_set)
    {
        log_message(ERROR, "Failed to allocate memory for result set");
        mysql_free_result(meta);
        goto cleanup_no_result;
    }

    result_set->capacity = num_rows;
    result_set->row_count = 0;
    result_set->rows = calloc(num_rows, sizeof(DbResultRow *));
    if (!result_set->rows)
    {
        log_message(ERROR, "Failed to allocate memory for result rows");
        free(result_set);
        mysql_free_result(meta);
        goto cleanup_no_result;
    }

    MYSQL_BIND *result_binds = calloc(num_fields, sizeof(MYSQL_BIND));
    if (!result_binds)
    {
        log_message(ERROR, "Failed to allocate memory for result bindings");
        free(result_set->rows);
        free(result_set);
        mysql_free_result(meta);
        goto cleanup_no_result;
    }

    my_bool *is_null = calloc(num_fields, sizeof(my_bool));
    if (!is_null)
    {
        log_message(ERROR, "Failed to allocate memory for NULL indicators");
        free(result_binds);
        free(result_set->rows);
        free(result_set);
        mysql_free_result(meta);
        goto cleanup_no_result;
    }

    my_bool *error = calloc(num_fields, sizeof(my_bool));
    if (!error)
    {
        log_message(ERROR, "Failed to allocate memory for error indicators");
        free(is_null);
        free(result_binds);
        free(result_set->rows);
        free(result_set);
        mysql_free_result(meta);
        goto cleanup_no_result;
    }

    unsigned long *lengths = calloc(num_fields, sizeof(unsigned long));
    if (!lengths)
    {
        log_message(ERROR, "Failed to allocate memory for length indicators");
        free(error);
        free(is_null);
        free(result_binds);
        free(result_set->rows);
        free(result_set);
        mysql_free_result(meta);
        goto cleanup_no_result;
    }

    char **string_buffers = calloc(num_fields, sizeof(char *));
    if (!string_buffers)
    {
        log_message(ERROR, "Failed to allocate memory for string buffers");
        free(lengths);
        free(error);
        free(is_null);
        free(result_binds);
        free(result_set->rows);
        free(result_set);
        mysql_free_result(meta);
        goto cleanup_no_result;
    }

    for (unsigned int i = 0; i < num_fields; i++)
    {

        string_buffers[i] = calloc(1024, sizeof(char));
        if (!string_buffers[i])
        {

            for (unsigned int j = 0; j < i; j++)
                free(string_buffers[j]);
            free(string_buffers);
            free(lengths);
            free(error);
            free(is_null);
            free(result_binds);
            free(result_set->rows);
            free(result_set);
            mysql_free_result(meta);
            goto cleanup_no_result;
        }

        memset(&result_binds[i], 0, sizeof(MYSQL_BIND));
        result_binds[i].buffer_type = MYSQL_TYPE_STRING;
        result_binds[i].buffer = string_buffers[i];
        result_binds[i].buffer_length = 1024;
        result_binds[i].is_null = &is_null[i];
        result_binds[i].length = &lengths[i];
        result_binds[i].error = &error[i];
    }

    if (mysql_stmt_bind_result(stmt->stmt, result_binds) != 0)
    {
        log_message(ERROR, "Failed to bind result: %s", mysql_stmt_error(stmt->stmt));

        for (unsigned int i = 0; i < num_fields; i++)
            free(string_buffers[i]);
        free(string_buffers);
        free(lengths);
        free(error);
        free(is_null);
        free(result_binds);
        free(result_set->rows);
        free(result_set);
        mysql_free_result(meta);
        goto cleanup_no_result;
    }

    int row_idx = 0;
    while (mysql_stmt_fetch(stmt->stmt) == 0 && row_idx < num_rows)
    {
        DbResultRow *row = calloc(1, sizeof(DbResultRow));
        if (!row)
        {
            log_message(ERROR, "Failed to allocate memory for row");
            continue;
        }

        row->field_count = num_fields;
        row->fields = calloc(num_fields, sizeof(DbResultField *));
        if (!row->fields)
        {
            log_message(ERROR, "Failed to allocate memory for fields");
            free(row);
            continue;
        }

        for (unsigned int i = 0; i < num_fields; i++)
        {
            if (!is_null[i])
            {
                DbResultField *field = calloc(1, sizeof(DbResultField));
                if (!field)
                {
                    log_message(ERROR, "Failed to allocate memory for field");
                    continue;
                }

                field->key = strdup(fields[i].name);
                field->type = fields[i].type;

                log_message(DEBUG, "Field %d: %s = %s",
                            i, field->key, string_buffers[i]);

                switch (fields[i].type)
                {
                case MYSQL_TYPE_LONGLONG:
                {
                    long long *val = malloc(sizeof(long long));
                    if (val)
                    {
                        *val = strtoll(string_buffers[i], NULL, 10);
                        field->value = val;
                    }
                    break;
                }
                case MYSQL_TYPE_FLOAT:
                {
                    float *val = malloc(sizeof(float));
                    if (val)
                    {
                        *val = strtof(string_buffers[i], NULL);
                        field->value = val;
                    }
                    break;
                }
                case MYSQL_TYPE_STRING:
                case MYSQL_TYPE_VAR_STRING:
                    field->value = strdup(string_buffers[i]);
                    break;
                case MYSQL_TYPE_LONG:
                {
                    int *val = malloc(sizeof(int));
                    if (val)
                    {
                        *val = atoi(string_buffers[i]);
                        field->value = val;
                    }
                    break;
                }
                case MYSQL_TYPE_TINY:
                {
                    int *val = malloc(sizeof(int));
                    if (val)
                    {
                        *val = (atoi(string_buffers[i]) != 0);
                        field->value = val;
                    }
                    break;
                }
                default:
                    field->value = strdup(string_buffers[i]);
                    break;
                }
                row->fields[i] = field;
            }
        }

        result_set->rows[result_set->row_count++] = row;
        row_idx++;
    }

    for (unsigned int i = 0; i < num_fields; i++)
        free(string_buffers[i]);
    free(string_buffers);
    free(lengths);
    free(error);
    free(is_null);
    free(result_binds);
    mysql_free_result(meta);

    if (stmt->binds)
    {
        free(stmt->binds);
        stmt->binds = NULL;
    }
    if (stmt->lengths)
    {
        free(stmt->lengths);
        stmt->lengths = NULL;
    }

    mysql_stmt_close(stmt->stmt);
    stmt->stmt = NULL;

    return result_set;

cleanup_no_result:
    log_message(ERROR, "Cleanup after error");
    if (stmt->binds)
    {
        free(stmt->binds);
        stmt->binds = NULL;
    }
    if (stmt->lengths)
    {
        free(stmt->lengths);
        stmt->lengths = NULL;
    }
    return NULL;
}
void db_statement_free(DbStatement *stmt) {
    if (!stmt) return;  // ✅ Kiểm tra NULL tránh lỗi

    if (stmt->binds) {
        free(stmt->binds);
        stmt->binds = NULL;
    }

    if (stmt->lengths) {
        free(stmt->lengths);
        stmt->lengths = NULL;
    }

    if (stmt->stmt) {
        mysql_stmt_close(stmt->stmt);
        stmt->stmt = NULL;
    }

    if (stmt->conn) {
        db_manager_release_connection(stmt->conn);
        stmt->conn = NULL;
    }

    free(stmt);
    stmt = NULL;  // ✅ Tránh truy cập sau khi giải phóng
}


void diagnose_statement(DbStatement *stmt)
{
    if (!stmt || !stmt->stmt)
    {
        log_message(ERROR, "Statement is NULL");
        return;
    }

    log_message(INFO, "Statement diagnostic:");
    log_message(INFO, "  Parameter count: %lu", mysql_stmt_param_count(stmt->stmt));
    log_message(INFO, "  Field count: %lu", mysql_stmt_field_count(stmt->stmt));

    const char *stmt_info = mysql_stmt_sqlstate(stmt->stmt);
    log_message(INFO, "  SQL state: %s", stmt_info ? stmt_info : "NULL");

    if (stmt->binds)
    {
        log_message(INFO, "  Binds allocated: Yes");
        log_message(INFO, "  Is bound: %s", stmt->is_bound ? "Yes" : "No");

        for (int i = 0; i < stmt->param_count; i++)
        {
            log_message(INFO, "  Parameter %d:", i);
            log_message(INFO, "    Buffer type: %d", stmt->binds[i].buffer_type);
            log_message(INFO, "    Buffer: %p", stmt->binds[i].buffer);
            if (stmt->binds[i].buffer_type == MYSQL_TYPE_STRING && stmt->binds[i].buffer)
            {
                log_message(INFO, "    String value: %s", (char *)stmt->binds[i].buffer);
                log_message(INFO, "    Buffer length: %lu", stmt->binds[i].buffer_length);
            }
        }
    }
    else
    {
        log_message(INFO, "  No binds allocated");
    }
}

bool db_bind_int(DbStatement *stmt, int index, int value) {
    if (!stmt || !stmt->stmt || index < 0) {
        log_message(ERROR, "Invalid statement or index for binding int");
        return false;
    }

    unsigned long current_param_count = mysql_stmt_param_count(stmt->stmt);
    if (index >= current_param_count) {
        log_message(ERROR, "Parameter index %d out of bounds (max: %lu)", index, current_param_count - 1);
        return false;
    }

    // Nếu mảng binds chưa được khởi tạo hoặc số tham số không khớp, cần khởi tạo lại
    if (!stmt->binds || stmt->param_count != current_param_count) {
        if (stmt->binds) {
            free(stmt->binds);
            stmt->binds = NULL;
        }
        if (stmt->lengths) {
            free(stmt->lengths);
            stmt->lengths = NULL;
        }
        stmt->param_count = current_param_count;
        stmt->binds = calloc(stmt->param_count, sizeof(MYSQL_BIND));
        if (!stmt->binds) {
            log_message(ERROR, "Failed to allocate memory for parameter bindings");
            return false;
        }
        stmt->lengths = calloc(stmt->param_count, sizeof(unsigned long));
        if (!stmt->lengths) {
            free(stmt->binds);
            stmt->binds = NULL;
            log_message(ERROR, "Failed to allocate memory for length values");
            return false;
        }
        stmt->is_bound = false;
    }

    // Cấp phát bộ nhớ cho giá trị int (và lưu lại để dùng sau khi statement thực thi)
    int *pValue = malloc(sizeof(int));
    if (!pValue) {
        log_message(ERROR, "Failed to allocate memory for int parameter at index %d", index);
        return false;
    }
    *pValue = value;

    memset(&stmt->binds[index], 0, sizeof(MYSQL_BIND));
    stmt->binds[index].buffer_type = MYSQL_TYPE_LONG;
    stmt->binds[index].buffer = pValue;
    stmt->binds[index].buffer_length = sizeof(int);

    unsigned long *len = malloc(sizeof(unsigned long));
    if (!len) {
        log_message(ERROR, "Failed to allocate memory for length at index %d", index);
        free(pValue);
        return false;
    }
    *len = sizeof(int);
    stmt->binds[index].length = len;

    stmt->binds[index].is_null = 0;

    log_message(DEBUG, "Successfully bound int value %d at index %d", value, index);
    return true;
}

bool db_bind_long(DbStatement *stmt, int index, long value) {
    if (!stmt || !stmt->stmt || index < 0) {
        log_message(ERROR, "Invalid statement or index for binding long");
        return false;
    }

    unsigned long current_param_count = mysql_stmt_param_count(stmt->stmt);
    if (index >= current_param_count) {
        log_message(ERROR, "Parameter index %d out of bounds (max: %lu)", index, current_param_count - 1);
        return false;
    }

    if (!stmt->binds || stmt->param_count != current_param_count) {
        if (stmt->binds) {
            free(stmt->binds);
            stmt->binds = NULL;
        }
        if (stmt->lengths) {
            free(stmt->lengths);
            stmt->lengths = NULL;
        }
        stmt->param_count = current_param_count;
        stmt->binds = calloc(stmt->param_count, sizeof(MYSQL_BIND));
        if (!stmt->binds) {
            log_message(ERROR, "Failed to allocate memory for parameter bindings");
            return false;
        }
        stmt->lengths = calloc(stmt->param_count, sizeof(unsigned long));
        if (!stmt->lengths) {
            free(stmt->binds);
            stmt->binds = NULL;
            log_message(ERROR, "Failed to allocate memory for length values");
            return false;
        }
        stmt->is_bound = false;
    }

    // Cấp phát bộ nhớ cho giá trị long
    long *pValue = malloc(sizeof(long));
    if (!pValue) {
        log_message(ERROR, "Failed to allocate memory for long parameter at index %d", index);
        return false;
    }
    *pValue = value;

    memset(&stmt->binds[index], 0, sizeof(MYSQL_BIND));
    stmt->binds[index].buffer_type = MYSQL_TYPE_LONGLONG;  // Sử dụng MYSQL_TYPE_LONGLONG cho giá trị long
    stmt->binds[index].buffer = pValue;
    stmt->binds[index].buffer_length = sizeof(long);

    unsigned long *len = malloc(sizeof(unsigned long));
    if (!len) {
        log_message(ERROR, "Failed to allocate memory for length at index %d", index);
        free(pValue);
        return false;
    }
    *len = sizeof(long);
    stmt->binds[index].length = len;

    stmt->binds[index].is_null = 0;

    log_message(DEBUG, "Successfully bound long value %ld at index %d", value, index);
    return true;
}
int db_get_insert_id(DbStatement *stmt) {
    if (!stmt || !stmt->stmt) return -1;
    // mysql_stmt_insert_id trả về giá trị auto-generated ID
    return (int)mysql_stmt_insert_id(stmt->stmt);
}