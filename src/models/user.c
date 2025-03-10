#include "user.h"
#include "session.h"
#include "service.h"
#include "sql_statement.h"
#include "database_connector.h"
#include "log.h"
#include <stdlib.h>
#include "db_statement.h"
#include "message.h"
#include <string.h>

void login(User* self);
void logout(User* self, Message* msg);
void userRegister(User* self, Message* msg);

User* createUser(User* self, Session* client, char* username, char* password) {
    if (self == NULL) {
        self = (User*)malloc(sizeof(User));
        if (self == NULL) {
            return NULL;
        }
    }
    
    self->id = 1;
    self->username = strdup(username); 
    self->password = strdup(password);
    self->isOnline = false;
    self->session = client;
    self->service = NULL;
    self->ipAddr = NULL;
    self->lastLogin = 0;
    
    self->login = login;
    self->logout = logout;
    self->userRegister = userRegister;
    self->isCleaned = NULL; 
    
    return self;
}

void destroyUser(User* user) {
    if (user == NULL) {
        return;
    }
    
    if (user->username != NULL) {
        free(user->username);
    }
    
    if (user->password != NULL) {
        free(user->password);
    }
    
    if (user->ipAddr != NULL) {
        free(user->ipAddr);
    }
    
    free(user);
}

void user_set_session(User* user, Session* session) {
    user->session = session;
}

void user_set_service(User* user, Service* service) {
    user->service = service;
}

void login(User* self) {
    log_message(INFO, "User login");
    if (self->isOnline) {
        log_message(INFO, "User is already online");
        return;
    }

    DbStatement* stmt = db_prepare(SQL_LOGIN);
    diagnose_statement(stmt);
    if (stmt == NULL) {
        log_message(ERROR, "Failed to prepare login statement");
        return;
    }

    log_message(INFO, "Prepared login statement");

    // Bind username parameter
    if (!db_bind_string(stmt, 0, self->username)) {
        log_message(ERROR, "Failed to bind username parameter");
        // Cleanup is handled by execute_query even if we pass an incomplete statement
        db_execute_query(stmt);
        return;
    }

    log_message(INFO, "Bound username parameter");

    // Execute query and get results
    DbResultSet* result = db_execute_query(stmt);
    if (result == NULL) {
        log_message(ERROR, "Failed to execute login query");
        return;
    }

    // Check if user exists
    if (result->row_count == 0) {
        log_message(INFO, "Login failed: Invalid username or password");
        db_result_set_free(result);
        return;
    }

    // Get user ID from result
    DbResultRow* row = result->rows[0];
    int userId = 0;
    bool passwordMatched = false;
    
    for (int i = 0; i < row->field_count; i++) {
        DbResultField* field = row->fields[i];
        if (field && strcmp(field->key, "id") == 0) {
            if (field->type == MYSQL_TYPE_LONG || field->type == MYSQL_TYPE_LONGLONG) {
                userId = *(int*)field->value;
            }
        }
        else if (field && strcmp(field->key, "password") == 0) {
            if (field->type == MYSQL_TYPE_STRING || field->type == MYSQL_TYPE_VAR_STRING) {
                passwordMatched = (strcmp((char*)field->value, self->password) == 0);
            }
        }
    }
    
    // Verify both user ID and password
    if (userId == 0 || !passwordMatched) {
        log_message(INFO, "Login failed: Invalid username or password");
        db_result_set_free(result);
        return;
    }
    
    // Update user state
    self->id = userId;
    self->isOnline = true;
    
    // Update last login time
    time_t now = time(NULL);
    self->lastLogin = (long)now;
    
    // Free the result set
    db_result_set_free(result);
    
   /*  // Update database with online status and last login time
    char updateQuery[512];
    snprintf(updateQuery, sizeof(updateQuery), 
             "UPDATE users SET is_online=1, last_login=%ld WHERE id=%d",
             self->lastLogin, self->id);
    
    int affected = db_manager_update(updateQuery);
    if (affected <= 0) {
        log_message(ERROR, "Failed to update online status and last login");
    } */
    
    log_message(INFO, "User %s (ID: %d) logged in successfully", self->username, self->id);
    
/*     // Notify service if available
    if (self->service != NULL) {
        service_login_success(self->service);
    } */
}

void logout(User* self, Message* msg) {
    self->isOnline = false;
}

void userRegister(User* self, Message* msg) {
    self->isOnline = true;
    
    DbStatement* stmt = db_prepare(SQL_REGISTER);
    if (!stmt) {
        log_message(ERROR, "Failed to prepare statement");
        return;
    }
    
    db_bind_string(stmt, 0, self->username);
    db_bind_string(stmt, 1, self->password);
    
    if (db_execute(stmt)) {
        log_message(INFO, "User registered successfully");
    } else {
        log_message(ERROR, "Failed to register user");
    }
}