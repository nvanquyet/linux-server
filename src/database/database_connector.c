#include "database_connector.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

bool db_init(DbManager *manager, const char *host, const char *user, const char *password, const char *database) {
    manager->db = mysql_init(NULL);
    if (manager->db == NULL) {
        fprintf(stderr, "mysql_init() failed\n");
        return false;
    }
    
    if (mysql_real_connect(manager->db, host, user, password, 
                          database, 0, NULL, 0) == NULL) {
        fprintf(stderr, "Cannot connect to database: %s\n", 
                mysql_error(manager->db));
        return false;
    }
    
    pthread_mutex_init(&manager->lock, NULL);
    return true;
}

MYSQL *db_get_connection(DbManager *manager) {
    return manager->db;
}

bool db_execute(DbManager *manager, const char *sql, ...) {
    pthread_mutex_lock(&manager->lock);
    
    
    char query[4096] = {0};
    va_list args;
    va_start(args, sql);
    
    char *pos = (char*)sql;
    char *dest = query;
    int remaining = sizeof(query) - 1;
    
    while (*pos && remaining > 0) {
        if (*pos == '?' && *(pos+1) != '?') {
            int type = va_arg(args, int);
            if (type == 0) break; 
            
            if (type == 1) { 
                int value = va_arg(args, int);
                int len = snprintf(dest, remaining, "%d", value);
                dest += len;
                remaining -= len;
            }
            else if (type == 2) { 
                char *text = va_arg(args, char*);
                int len = 0;
                
                
                char *escaped = malloc(strlen(text) * 2 + 1);
                mysql_real_escape_string(manager->db, escaped, text, strlen(text));
                
                len = snprintf(dest, remaining, "'%s'", escaped);
                free(escaped);
                
                dest += len;
                remaining -= len;
            }
            else if (type == 3) { 
                double value = va_arg(args, double);
                int len = snprintf(dest, remaining, "%f", value);
                dest += len;
                remaining -= len;
            }
            pos++;
        } else {
            *dest++ = *pos++;
            remaining--;
        }
    }
    *dest = '\0';
    va_end(args);
    
    
    bool success = (mysql_query(manager->db, query) == 0);
    if (!success) {
        fprintf(stderr, "SQL Error: %s\n", mysql_error(manager->db));
    }
    
    pthread_mutex_unlock(&manager->lock);
    return success;
}

void db_close(DbManager *manager) {
    mysql_close(manager->db);
    pthread_mutex_destroy(&manager->lock);
}
