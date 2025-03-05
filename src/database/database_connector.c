#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <mysql/mysql.h>
#include <pthread.h>
#include "database_connector.h"
#include "config.h"
#include "log.h"

// Singleton instance
static DbManager *instance = NULL;

// Helper functions
static MYSQL* create_connection() {
    Config *config = config_get_instance();
    MYSQL *conn = mysql_init(NULL);
    
    if (conn == NULL) {
        fprintf(stderr, "mysql_init() failed\n");
        return NULL;
    }
    
    // Set reconnect option
    bool reconnect = 1;
    mysql_options(conn, MYSQL_OPT_RECONNECT, &reconnect);
    
    // Connect to database server
    if (!mysql_real_connect(conn, 
                           config_get_db_host(),
                           config_get_db_user(),
                           config_get_db_password(),
                           config_get_db_name(),
                           config_get_db_port(), 
                           NULL, 0)) {
        fprintf(stderr, "Failed to connect to database: %s\n", mysql_error(conn));
        mysql_close(conn);
        return NULL;
    }
    
    return conn;
}

// Get the singleton instance
DbManager* db_manager_get_instance() {
    if (instance == NULL) {
        instance = calloc(1, sizeof(DbManager));
        if (instance == NULL) {
            fprintf(stderr, "Failed to allocate memory for DbManager\n");
            return NULL;
        }
        pthread_mutex_init(&instance->mutex, NULL);
        instance->initialized = false;
    }
    return instance;
}

// Initialize the database manager
bool db_manager_start() {
    DbManager *manager = db_manager_get_instance();
    if (manager == NULL) {
        return false;
    }
    
    pthread_mutex_lock(&manager->mutex);
    
    if (manager->initialized) {
        fprintf(stderr, "Database manager already initialized\n");
        pthread_mutex_unlock(&manager->mutex);
        return true;  // Already initialized is not an error
    }
    
    Config *config = config_get_instance();
    if (config == NULL) {
        fprintf(stderr, "Config not initialized\n");
        pthread_mutex_unlock(&manager->mutex);
        return false;
    }
    
    // Copy configuration parameters
    manager->host = strdup(config_get_db_host());
    manager->user = strdup(config_get_db_user());
    manager->password = strdup(config_get_db_password());
    manager->database = strdup(config_get_db_name());
    manager->port = config_get_db_port();
    manager->active_connections = 0;
    
    // Initialize a test connection
    MYSQL *conn = create_connection();
    if (conn == NULL) {
        fprintf(stderr, "Failed to establish test database connection\n");
        
        // Free allocated resources
        free(manager->host);
        free(manager->user);
        free(manager->password);
        free(manager->database);
        
        pthread_mutex_unlock(&manager->mutex);
        return false;
    }
    
    // Store the test connection
    manager->connections[manager->active_connections++] = conn;
    manager->initialized = true;
    
    printf("Database connection established successfully\n");
    pthread_mutex_unlock(&manager->mutex);
    return true;
}

// Clean up and shut down the database manager
void db_manager_shutdown() {
    if (instance == NULL) {
        return;
    }
    
    pthread_mutex_lock(&instance->mutex);
    
    if (instance->initialized) {
        // Close all active connections
        for (int i = 0; i < instance->active_connections; i++) {
            if (instance->connections[i]) {
                mysql_close(instance->connections[i]);
                instance->connections[i] = NULL;
            }
        }
        
        // Free allocated memory
        free(instance->host);
        free(instance->user);
        free(instance->password);
        free(instance->database);
        
        instance->active_connections = 0;
        instance->initialized = false;
        
        printf("Database connections closed\n");
    }
    
    pthread_mutex_unlock(&instance->mutex);
}

// Get a database connection
MYSQL* db_manager_get_connection() {
    DbManager *manager = db_manager_get_instance();
    if (manager == NULL || !manager->initialized) {
        fprintf(stderr, "Database manager not initialized\n");
        return NULL;
    }
    
    pthread_mutex_lock(&manager->mutex);
    
    MYSQL *conn = NULL;
    
    // Check for an available connection in the pool
    if (manager->active_connections > 0) {
        // Use the first connection (in a real pool, you'd have logic to pick an idle one)
        conn = manager->connections[0];
        
        // Test if the connection is still valid
        if (mysql_ping(conn) != 0) {
            // Connection lost, try to recreate
            mysql_close(conn);
            conn = create_connection();
            if (conn != NULL) {
                manager->connections[0] = conn;
            } else {
                // Failed to reconnect
                manager->active_connections--;
            }
        }
    } else {
        // Create a new connection if none available
        conn = create_connection();
        if (conn != NULL && manager->active_connections < 10) {
            manager->connections[manager->active_connections++] = conn;
        }
    }
    
    pthread_mutex_unlock(&manager->mutex);
    return conn;
}

// Release a database connection (in this simple implementation, we don't actually release)
void db_manager_release_connection(MYSQL *conn) {
    // In a simple implementation, we'll keep connections in the pool
    // In a more complex one, we'd return it to an available pool
}

// Execute an update query (INSERT, UPDATE, DELETE)
int db_manager_update(const char *sql, ...) {
    MYSQL *conn = db_manager_get_connection();
    if (conn == NULL) {
        fprintf(stderr, "Failed to get database connection\n");
        return -1;
    }
    
    // Execute query directly (no parameter binding for simplicity)
    if (mysql_query(conn, sql) != 0) {
        fprintf(stderr, "Query failed: %s\n", mysql_error(conn));
        return -1;
    }
    
    int affected_rows = mysql_affected_rows(conn);
    return affected_rows;
}

// Create a result set from a MySQL result
DbResultSet* create_result_set(MYSQL_RES *mysql_result) {
    if (mysql_result == NULL) {
        return NULL;
    }
    
    my_ulonglong num_rows = mysql_num_rows(mysql_result);
    unsigned int num_fields = mysql_num_fields(mysql_result);
    
    // Allocate the result set
    DbResultSet *result_set = calloc(1, sizeof(DbResultSet));
    if (result_set == NULL) {
        mysql_free_result(mysql_result);
        return NULL;
    }
    
    // Allocate rows array
    result_set->capacity = num_rows;
    result_set->row_count = 0;
    result_set->rows = calloc(num_rows, sizeof(DbResultRow*));
    if (result_set->rows == NULL) {
        free(result_set);
        mysql_free_result(mysql_result);
        return NULL;
    }
    
    // Get field information
    MYSQL_FIELD *fields = mysql_fetch_fields(mysql_result);
    
    // Process each row
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(mysql_result))) {
        // Create a row
        DbResultRow *result_row = calloc(1, sizeof(DbResultRow));
        if (result_row == NULL) continue;
        
        // Allocate fields array
        result_row->field_count = num_fields;
        result_row->fields = calloc(num_fields, sizeof(DbResultField*));
        if (result_row->fields == NULL) {
            free(result_row);
            continue;
        }
        
        // Process each field
        for (unsigned int i = 0; i < num_fields; i++) {
            if (row[i] != NULL) {
                // Create field
                DbResultField *field = calloc(1, sizeof(DbResultField));
                if (field == NULL) continue;
                
                // Copy field name
                field->key = strdup(fields[i].name);
                field->type = fields[i].type;
                
                // Handle different MySQL data types
                switch (field->type) {
                    case MYSQL_TYPE_LONGLONG: // BIGINT
                        {
                            long long *val = malloc(sizeof(long long));
                            if (val) {
                                *val = strtoll(row[i], NULL, 10);
                                field->value = val;
                            }
                            break;
                        }
                    case MYSQL_TYPE_FLOAT: // FLOAT
                        {
                            float *val = malloc(sizeof(float));
                            if (val) {
                                *val = strtof(row[i], NULL);
                                field->value = val;
                            }
                            break;
                        }
                    case MYSQL_TYPE_STRING: // VARCHAR/TEXT
                    case MYSQL_TYPE_VAR_STRING:
                        field->value = strdup(row[i]);
                        break;
                    case MYSQL_TYPE_LONG: // INT
                        {
                            int *val = malloc(sizeof(int));
                            if (val) {
                                *val = atoi(row[i]);
                                field->value = val;
                            }
                            break;
                        }
                    case MYSQL_TYPE_TINY: // TINYINT (for boolean)
                        {
                            int *val = malloc(sizeof(int));
                            if (val) {
                                *val = (atoi(row[i]) != 0);
                                field->value = val;
                            }
                            break;
                        }
                    default:
                        field->value = strdup(row[i]);
                        break;
                }
                
                // Add to the row
                result_row->fields[i] = field;
            }
        }
        
        // Add the row to the result set
        result_set->rows[result_set->row_count++] = result_row;
    }
    
    mysql_free_result(mysql_result);
    return result_set;
}

// Execute a query and return results
DbResultSet* db_manager_query(const char *sql) {
    MYSQL *conn = db_manager_get_connection();
    if (conn == NULL) {
        fprintf(stderr, "Failed to get database connection\n");
        return NULL;
    }
    
    // Execute query
    if (mysql_query(conn, sql) != 0) {
        fprintf(stderr, "Query failed: %s\n", mysql_error(conn));
        return NULL;
    }
    
    // Get result
    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL) {
        if (mysql_field_count(conn) == 0) {
            // Query was not supposed to return data
            return NULL;
        } else {
            // Error occurred
            fprintf(stderr, "Failed to retrieve result set: %s\n", mysql_error(conn));
            return NULL;
        }
    }
    
    // Convert to our result set structure
    return create_result_set(result);
}

// Free a result set
void db_result_set_free(DbResultSet *result_set) {
    if (result_set == NULL) {
        return;
    }
    
    // Free all rows
    for (int i = 0; i < result_set->row_count; i++) {
        DbResultRow *row = result_set->rows[i];
        if (row) {
            // Free all fields
            for (int j = 0; j < row->field_count; j++) {
                DbResultField *field = row->fields[j];
                if (field) {
                    free(field->key);
                    free(field->value);
                    free(field);
                }
            }
            free(row->fields);
            free(row);
        }
    }
    
    // Free the rows array
    free(result_set->rows);
    
    // Free the result set itself
    free(result_set);
}
