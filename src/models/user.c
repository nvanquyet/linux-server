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
#include "m_utils.h"

void login(User *self);
void logout(User *self);
void userRegister(User *self);
DbResultSet *get_user_data_map(User *self);

User *createUser(User *self, Session *client, char *username, char *password)
{
    if (self == NULL)
    {
        self = (User *)malloc(sizeof(User));
        if (self == NULL)
        {
            return NULL;
        }
    }

    self->id = 0;
    self->username = strdup(username);
    self->password = strdup(password);
    self->isOnline = false;
    self->session = client;
    self->service = client->service;
    self->ipAddr = NULL;
    self->lastLogin = 0;

    self->login = login;
    self->logout = logout;
    self->userRegister = userRegister;
    self->isCleaned = NULL;
    self->isLoaded = false;

    return self;
}

void destroyUser(User *user)
{
    if (user == NULL)
    {
        return;
    }

    if (user->username != NULL)
    {
        free(user->username);
    }

    if (user->password != NULL)
    {
        free(user->password);
    }

    if (user->ipAddr != NULL)
    {
        free(user->ipAddr);
    }

    free(user);
}

void user_set_session(User *user, Session *session)
{
    user->session = session;
}

void user_set_service(User *user, Service *service)
{
    user->service = service;
}

void login(User *self)
{
    if(!validate_username_password(self->username, self->password)){
        self->service->server_message(self->session, "Invalid username or password");
        return;
    }

    if (self->isOnline)
    {
        log_message(INFO, "User is already online");
        return;
    }

    DbStatement *stmt = db_prepare(SQL_LOGIN);
    // diagnose_statement(stmt);
    if (stmt == NULL)
    {
        log_message(ERROR, "Failed to prepare login statement");
        return;
    }

    if (!db_bind_string(stmt, 0, self->username))
    {
        log_message(ERROR, "Failed to bind username parameter");
        db_execute_query(stmt);
        return;
    }
    DbResultSet *result = db_execute_query(stmt);

    if (result == NULL)
    {
        log_message(ERROR, "Failed to execute login query");
        return;
    }

    if (result->row_count == 0)
    {
        log_message(INFO, "Login failed: Invalid username or password");
        db_result_set_free(result);
        return;
    }

    DbResultRow *row = result->rows[0];
    int userId = 0;
    bool passwordMatched = false;

    for (int i = 0; i < row->field_count; i++)
    {
        DbResultField *field = row->fields[i];
        if (field && strcmp(field->key, "id") == 0)
        {
            if (field->type == MYSQL_TYPE_LONG || field->type == MYSQL_TYPE_LONGLONG)
            {
                userId = *(int *)field->value;
            }
        }
        else if (field && strcmp(field->key, "password") == 0)
        {
            if (field->type == MYSQL_TYPE_STRING || field->type == MYSQL_TYPE_VAR_STRING)
            {
                passwordMatched = (strcmp((char *)field->value, self->password) == 0);
            }
        }
    }

    if (userId == 0 || !passwordMatched)
    {
        log_message(INFO, "Login failed: Invalid username or password");
        db_result_set_free(result);
        return;
    }

    self->id = userId;
    self->isOnline = true;
    time_t now = time(NULL);
    self->lastLogin = (long)now;

    db_result_set_free(result);

    
    DbStatement *updateQuery = db_prepare(SQL_UPDATE_USER_LOGIN);
    if (!updateQuery)
    {
        log_message(ERROR, "Failed to prepare update query");
        return;
    }
    service_server_message(self->session, "Login success");
    self->isLoaded = true;
}


void logout(User *self)
{
    self->isOnline = false;
}

void userRegister(User *self)
{
    self->isOnline = false;

    if(!validate_username_password(self->username, self->password)){
        self->service->server_message(self->session, "Invalid username or password");
        return;
    }

    DbStatement *check_stmt = db_prepare(SQL_LOGIN);
    if (!check_stmt)
    {
        log_message(ERROR, "Failed to prepare check statement");
        return;
    }

    if (!db_bind_string(check_stmt, 0, self->username))
    {
        log_message(ERROR, "Failed to bind username for check");
        db_statement_free(check_stmt);
        return;
    }
    
    DbResultSet *result = db_execute_query(check_stmt);
    db_statement_free(check_stmt);
    
    if (result)
    {
        if (result->row_count > 0)
        {
            self->service->server_message(self->session, "User already exists");
            db_result_set_free(result);
            return;
        }
        db_result_set_free(result);
    }

    DbStatement *reg_stmt = db_prepare(SQL_REGISTER);
    if (!reg_stmt)
    {
        log_message(ERROR, "Failed to prepare registration statement");
        return;
    }

    if (!db_bind_string(reg_stmt, 0, self->username))
    {
        log_message(ERROR, "Failed to bind username for registration");
        db_statement_free(reg_stmt);
        return;
    }
    
    if (!db_bind_string(reg_stmt, 1, self->password))
    {
        log_message(ERROR, "Failed to bind password");
        db_statement_free(reg_stmt);
        return;
    }

    if (db_execute(reg_stmt))
    {
        log_message(INFO, "User registered successfully");
        self->service->server_message(self->session, "Registration successful");
    }
    else
    {
        log_message(ERROR, "Failed to register user");
        self->service->server_message(self->session, "Registration failed");
    }
}