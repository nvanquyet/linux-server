#ifndef DATABASE_CONNECTOR_H
#define DATABASE_CONNECTOR_H

#include <stdbool.h>
#include <mysql/mysql.h>
#include <pthread.h>

typedef struct {
    MYSQL *db;
    pthread_mutex_t lock;
} DbManager;

bool db_init(DbManager *manager, const char *host, const char *user, const char *password, const char *database);
MYSQL *db_get_connection(DbManager *manager);
void db_close(DbManager *manager);
bool db_execute(DbManager *manager, const char *sql, ...);

#endif
