#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <mysql/mysql.h>
#include <pthread.h>
#include "database_connector.h"
#include "config.h"
#include "log.h"

static DbManager *instance = NULL;

static MYSQL *create_connection()
{
    Config *config = config_get_instance();
    if (config == NULL)
    {
        log_message(ERROR, "Failed to get configuration instance");
        return NULL;
    }

    MYSQL *conn = mysql_init(NULL);
    if (conn == NULL)
    {
        log_message(ERROR, "Failed to initialize MySQL connection");
        return NULL;
    }

    unsigned int timeout = 5;
    mysql_options(conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);

    mysql_options(conn, MYSQL_SET_CHARSET_NAME, "utf8mb4");

    if (!mysql_real_connect(conn,
                            config_get_db_host(),
                            config_get_db_user(),
                            config_get_db_password(),
                            config_get_db_name(),
                            config_get_db_port(),
                            NULL, 0))
    {
        log_message(ERROR, "Failed to connect to database: %s", mysql_error(conn));
        mysql_close(conn);
        return NULL;
    }

    mysql_set_character_set(conn, "utf8mb4");

    return conn;
}
DbManager *db_manager_get_instance()
{
    if (instance == NULL)
    {
        instance = calloc(1, sizeof(DbManager));
        if (instance == NULL)
        {
            log_message(ERROR, "Failed to allocate database manager");
            return NULL;
        }
        pthread_mutex_init(&instance->mutex, NULL);
        instance->initialized = false;
    }
    return instance;
}

bool db_manager_start()
{
    DbManager *manager = db_manager_get_instance();
    if (manager == NULL)
    {
        return false;
    }

    pthread_mutex_lock(&manager->mutex);

    if (manager->initialized)
    {
        log_message(INFO, "Database manager already initialized");
        pthread_mutex_unlock(&manager->mutex);
        return true;
    }

    Config *config = config_get_instance();
    if (config == NULL)
    {
        log_message(ERROR, "Failed to get configuration instance");
        pthread_mutex_unlock(&manager->mutex);
        return false;
    }

    manager->host = strdup(config_get_db_host());
    manager->user = strdup(config_get_db_user());
    manager->password = strdup(config_get_db_password());
    manager->database = strdup(config_get_db_name());
    manager->port = config_get_db_port();
    manager->active_connections = 0;

    MYSQL *conn = create_connection();
    if (conn == NULL)
    {
        log_message(ERROR, "Failed to create test connection");

        free(manager->host);
        free(manager->user);
        free(manager->password);
        free(manager->database);

        pthread_mutex_unlock(&manager->mutex);
        return false;
    }

    manager->connections[manager->active_connections++] = conn;
    manager->initialized = true;

    log_message(INFO, "Database connection established successfully");
    pthread_mutex_unlock(&manager->mutex);
    return true;
}

void db_manager_shutdown()
{
    if (instance == NULL)
    {
        return;
    }

    pthread_mutex_lock(&instance->mutex);

    if (instance->initialized)
    {

        for (int i = 0; i < instance->active_connections; i++)
        {
            if (instance->connections[i])
            {
                mysql_close(instance->connections[i]);
                instance->connections[i] = NULL;
            }
        }

        free(instance->host);
        free(instance->user);
        free(instance->password);
        free(instance->database);

        instance->active_connections = 0;
        instance->initialized = false;

        log_message(INFO, "Database manager shut down");
    }

    pthread_mutex_unlock(&instance->mutex);
}

MYSQL *db_manager_get_connection()
{
    DbManager *manager = db_manager_get_instance();
    if (manager == NULL || !manager->initialized)
    {
        log_message(ERROR, "Database manager not initialized");
        return NULL;
    }

    pthread_mutex_lock(&manager->mutex);

    MYSQL *conn = NULL;

    if (manager->active_connections > 0)
    {

        conn = manager->connections[0];

        if (mysql_ping(conn) != 0)
        {

            mysql_close(conn);
            conn = create_connection();
            if (conn != NULL)
            {
                manager->connections[0] = conn;
            }
            else
            {

                manager->active_connections--;
            }
        }
    }
    else
    {

        conn = create_connection();
        if (conn != NULL && manager->active_connections < 10)
        {
            manager->connections[manager->active_connections++] = conn;
        }
    }

    pthread_mutex_unlock(&manager->mutex);
    return conn;
}

void db_manager_release_connection(MYSQL *conn)
{
    if (conn == NULL) {
        return;
    }
    
    DbManager *manager = db_manager_get_instance();
    if (manager == NULL) {
        mysql_close(conn);
        return;
    }
    
    if (mysql_ping(conn) != 0) {
        for (int i = 0; i < manager->active_connections; i++) {
            if (manager->connections[i] == conn) {
                mysql_close(conn);
                
                for (int j = i; j < manager->active_connections - 1; j++) {
                    manager->connections[j] = manager->connections[j+1];
                }
                
                manager->active_connections--;
                break;
            }
        }
    }
}

int db_manager_update(const char *sql, ...)
{
    MYSQL *conn = db_manager_get_connection();
    if (conn == NULL)
    {
        log_message(ERROR, "Failed to get database connection");
        return -1;
    }

    if (mysql_query(conn, sql) != 0)
    {
        log_message(ERROR, "Query failed: %s", mysql_error(conn));
        return -1;
    }

    int affected_rows = mysql_affected_rows(conn);
    return affected_rows;
}

DbResultSet *create_result_set(MYSQL_RES *mysql_result)
{
    if (mysql_result == NULL)
    {
        return NULL;
    }

    my_ulonglong num_rows = mysql_num_rows(mysql_result);
    unsigned int num_fields = mysql_num_fields(mysql_result);

    DbResultSet *result_set = calloc(1, sizeof(DbResultSet));
    if (result_set == NULL)
    {
        mysql_free_result(mysql_result);
        return NULL;
    }

    result_set->capacity = num_rows;
    result_set->row_count = 0;
    result_set->rows = calloc(num_rows, sizeof(DbResultRow *));
    if (result_set->rows == NULL)
    {
        free(result_set);
        mysql_free_result(mysql_result);
        return NULL;
    }

    MYSQL_FIELD *fields = mysql_fetch_fields(mysql_result);

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(mysql_result)))
    {

        DbResultRow *result_row = calloc(1, sizeof(DbResultRow));
        if (result_row == NULL)
            continue;

        result_row->field_count = num_fields;
        result_row->fields = calloc(num_fields, sizeof(DbResultField *));
        if (result_row->fields == NULL)
        {
            free(result_row);
            continue;
        }

        for (unsigned int i = 0; i < num_fields; i++)
        {
            if (row[i] != NULL)
            {

                DbResultField *field = calloc(1, sizeof(DbResultField));
                if (field == NULL)
                    continue;

                field->key = strdup(fields[i].name);
                field->type = fields[i].type;

                switch (field->type)
                {
                case MYSQL_TYPE_LONGLONG:
                {
                    long long *val = malloc(sizeof(long long));
                    if (val)
                    {
                        *val = strtoll(row[i], NULL, 10);
                        field->value = val;
                    }
                    break;
                }
                case MYSQL_TYPE_FLOAT:
                {
                    float *val = malloc(sizeof(float));
                    if (val)
                    {
                        *val = strtof(row[i], NULL);
                        field->value = val;
                    }
                    break;
                }
                case MYSQL_TYPE_STRING:
                case MYSQL_TYPE_VAR_STRING:
                    field->value = strdup(row[i]);
                    break;
                case MYSQL_TYPE_LONG:
                {
                    int *val = malloc(sizeof(int));
                    if (val)
                    {
                        *val = atoi(row[i]);
                        field->value = val;
                    }
                    break;
                }
                case MYSQL_TYPE_TINY:
                {
                    int *val = malloc(sizeof(int));
                    if (val)
                    {
                        *val = (atoi(row[i]) != 0);
                        field->value = val;
                    }
                    break;
                }
                default:
                    field->value = strdup(row[i]);
                    break;
                }

                result_row->fields[i] = field;
            }
        }

        result_set->rows[result_set->row_count++] = result_row;
    }

    mysql_free_result(mysql_result);
    return result_set;
}

DbResultSet *db_manager_query(const char *sql)
{
    MYSQL *conn = db_manager_get_connection();
    if (conn == NULL)
    {
        log_message(ERROR, "Failed to get database connection");
        return NULL;
    }

    if (mysql_query(conn, sql) != 0)
    {
        log_message(ERROR, "Query failed: %s", mysql_error(conn));
        return NULL;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL)
    {
        if (mysql_field_count(conn) == 0)
        {

            return NULL;
        }
        else
        {

            log_message(ERROR, "Failed to store result: %s", mysql_error(conn));
            return NULL;
        }
    }

    return create_result_set(result);
}

void db_result_set_free(DbResultSet *result_set)
{
    if (result_set == NULL)
    {
        return;
    }

    for (int i = 0; i < result_set->row_count; i++)
    {
        DbResultRow *row = result_set->rows[i];
        if (row)
        {

            for (int j = 0; j < row->field_count; j++)
            {
                DbResultField *field = row->fields[j];
                if (field)
                {
                    free(field->key);
                    free(field->value);
                    free(field);
                }
            }
            free(row->fields);
            free(row);
        }
    }

    free(result_set->rows);

    free(result_set);
}
