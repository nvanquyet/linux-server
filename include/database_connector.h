#ifndef DATABASE_CONNECTOR_H
#define DATABASE_CONNECTOR_H

#include <stdbool.h>
#include <mysql/mysql.h>
#include <pthread.h>

typedef struct {
    MYSQL *connections[10];  // Simple array of connections
    int active_connections;
    pthread_mutex_t mutex;
    bool initialized;
    
    // Configuration copied from Config
    char *host;
    char *user;
    char *password;
    char *database;
    int port;
} DbManager;

// Result structure for query results
typedef struct {
    char *key;
    void *value;
    int type;  // MySQL type value
} DbResultField;

typedef struct {
    DbResultField **fields;
    int field_count;
} DbResultRow;

typedef struct {
    DbResultRow **rows;
    int row_count;
    int capacity;
} DbResultSet;

// Core database functions
DbManager* db_manager_get_instance();
bool db_manager_start();
void db_manager_shutdown();

// Connection management
MYSQL* db_manager_get_connection();
void db_manager_release_connection(MYSQL *conn);

// Query execution
int db_manager_update(const char *sql, ...);
int db_manager_update_with_params(const char *sql, int param_count, ...);
DbResultSet* db_manager_query(const char *sql);

// Convenience methods
int db_manager_update_bag(const char *bag, int id);
int db_manager_update_coin(long coin, int id);

// Memory management
void db_result_set_free(DbResultSet *result_set);

#endif
