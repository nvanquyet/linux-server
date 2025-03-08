#include "user.h"
#include "session.h"
#include "service.h"
#include "sql_statement.h"
#include "database_connector.h"
#include "log.h"
#include <stdlib.h>
#include "db_statement.h"

void login(User* self);
void logout(User* self);
void userRegister(User* self);

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
    free(user);
}

void user_set_session(User* user, Session* session) {
    user->session = session;
}

void user_set_service(User* user, Service* service) {
    user->service = service;
}

void login(User* self) {
    self->isOnline = true;
}

void logout(User* self) {
    self->isOnline = false;
}

void userRegister(User* self) {
    self->isOnline = true;
    
    DbStatement* stmt = db_prepare(REGISTER);
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