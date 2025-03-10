#ifndef DATABASE_CONNECTOR_H
#define DATABASE_CONNECTOR_H

#include <stdbool.h>
#include <mysql/mysql.h>
#include <pthread.h>

typedef struct
{
    MYSQL *connections[10];
    int active_connections;
    pthread_mutex_t mutex;
    bool initialized;

    char *host;
    char *user;
    char *password;
    char *database;
    int port;
} DbManager;

typedef struct
{
    char *key;
    void *value;
    int type;
} DbResultField;

typedef struct
{
    DbResultField **fields;
    int field_count;
} DbResultRow;

typedef struct
{
    DbResultRow **rows;
    int row_count;
    int capacity;
} DbResultSet;

DbManager *db_manager_get_instance();
bool db_manager_start();
void db_manager_shutdown();

MYSQL *db_manager_get_connection();
void db_manager_release_connection(MYSQL *conn);

int db_manager_update(const char *sql, ...);
int db_manager_update_with_params(const char *sql, int param_count, ...);
DbResultSet *db_manager_query(const char *sql);

void db_result_set_free(DbResultSet *result_set);

#endif
