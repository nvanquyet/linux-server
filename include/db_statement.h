#ifndef DB_STATEMENT_H
#define DB_STATEMENT_H

#include <mysql/mysql.h>
#include <stdbool.h>
#include "database_connector.h"

typedef struct DbStatement DbStatement;

struct DbStatement{
    MYSQL *conn;
    MYSQL_STMT *stmt;
    MYSQL_BIND *binds;
    unsigned long *lengths;
    unsigned long param_count;
    bool is_bound;
};

DbStatement* db_prepare(const char* query);
bool db_bind_string(DbStatement* stmt, int index, const char* value);
bool db_execute(DbStatement* stmt);
bool db_execute_update(DbStatement *stmt);
DbResultSet *db_execute_query(DbStatement *stmt);
void diagnose_statement(DbStatement *stmt);
void db_statement_free(DbStatement *stmt);
bool db_bind_int(DbStatement *stmt, int index, int value);
bool db_bind_long(DbStatement *stmt, int index, long value);
int db_get_insert_id(DbStatement *stmt) ;
#endif