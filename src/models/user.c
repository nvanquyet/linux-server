#include "user.h"
#include "session.h"
#include "service.h"
#include "sql_statement.h"
#include "database_connector.h"
#include "log.h"
#include <stdlib.h>
#include "db_statement.h"
#include <string.h>
#include "m_utils.h"
#include "server_manager.h"

void login(User *self);
void logout(User *self);
void userRegister(User *self);
void close_session_callback(void *arg);
void cleanUp(User *self);

typedef struct
{
    User *user;
    ServerManager *manager;
} CloseSessionData;

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
    self->isCleaned = false;
    self->clean_user = cleanUp;
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

void cleanUp(User *self)
{
    if (self == NULL)
    {
        return;
    }
    if(self->session != NULL){
        self->session = NULL;
    }
    if(self->service != NULL){
        self->service = NULL;
    }
    self->isCleaned = true;
    destroyUser(self);
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
    if (!validate_username_password(self->username, self->password))
    {
        self->service->server_message(self->session, "Invalid username or password");
        return;
    }

    DbStatement *stmt = db_prepare(SQL_LOGIN);

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
    db_statement_free(stmt);
    if (result == NULL)
    {
        log_message(ERROR, "Failed to execute login query");
        self->service->server_message(self->session, "Login failed, server error");
        return;
    }

    if (result->row_count == 0)
    {
        log_message(INFO, "Login failed: Invalid username or password");
        self->service->server_message(self->session, "Login failed: Invalid username or password");
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


    if (!passwordMatched)
    {
        self->service->server_message(self->session, "Invalid username or password");
        db_result_set_free(result);
        return;
    }

    self->id = userId;
    self->isOnline = true;
    time_t now = time(NULL);
    self->lastLogin = (long)now;
    self->isCleaned = false;

    db_result_set_free(result);

    ServerManager *manager = server_manager_get_instance();
    server_manager_lock();

    User *existing_user = server_manager_find_user_by_username_internal(self->username, true);
    if (existing_user != NULL && !existing_user->isCleaned)
    {
        self->service->server_message(self->session, "Account is already logged in");
        existing_user->service->server_message(existing_user->session, "Someone is trying to log in to your account!");

        CloseSessionData *existing_data = malloc(sizeof(CloseSessionData));
        if (existing_data)
        {
            existing_data->user = existing_user;
            existing_data->manager = manager;
            utils_set_timeout(close_session_callback, existing_data, 1000);
        }

        CloseSessionData *current_data = malloc(sizeof(CloseSessionData));
        if (current_data)
        {
            current_data->user = self;
            current_data->manager = manager;
            utils_set_timeout(close_session_callback, current_data, 1000);
        }
    } else{
        server_manager_add_user_internal(self, true);
        self->service->server_message(self->session, "Login success");
        self->isLoaded = true;
    }
    server_manager_unlock();
}

void logout(User *self)
{
    self->isOnline = false;
}

void userRegister(User *self)
{
    self->isOnline = false;

    if (!validate_username_password(self->username, self->password))
    {
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

void close_session_callback(void *arg)
{
    CloseSessionData *data = (CloseSessionData *)arg;
    User *user = data->user;
    ServerManager *manager = data->manager;

    if (user && !user->isCleaned)
    {
        if (user->session)
        {
            user->session->closeMessage(user->session);
        }
        server_manager_remove_user(user);
        user->isCleaned = true;
    }
    free(data);
}

User *findUserById(int id) {
    // Chuẩn bị statement với truy vấn SQL_GET_USER_BY_ID
    DbStatement *stmt = db_prepare(SQL_GET_USER_BY_ID);
    if (!stmt) {
        log_message(ERROR, "Failed to prepare statement for finding user by id");
        return NULL;
    }

    // Bind tham số ID vào vị trí 0 của câu lệnh
    if (!db_bind_int(stmt, 0, id)) {
        log_message(ERROR, "Failed to bind user id");
        db_statement_free(stmt);
        return NULL;
    }

    // Thực thi truy vấn và lấy kết quả
    DbResultSet *result = db_execute_query(stmt);
    db_statement_free(stmt);
    if (!result || result->row_count == 0) {
        log_message(INFO, "No user found with id: %d", id);
        if (result) db_result_set_free(result);
        return NULL;
    }

    // Lấy hàng đầu tiên trong kết quả
    DbResultRow *row = result->rows[0];

    // Cấp phát bộ nhớ cho đối tượng User
    User *user = (User *)malloc(sizeof(User));
    if (!user) {
        log_message(ERROR, "Memory allocation failed for user");
        db_result_set_free(result);
        return NULL;
    }

    // Khởi tạo các trường cho User
    memset(user, 0, sizeof(User));

    // Duyệt qua các trường trong hàng để lấy dữ liệu
    for (int i = 0; i < row->field_count; i++) {
        DbResultField *field = row->fields[i];
        if (field == NULL)
            continue;

        if (strcmp(field->key, "id") == 0) {
            user->id = *(int *)field->value;
        } else if (strcmp(field->key, "username") == 0) {
            user->username = strdup((char *)field->value);
        } else if (strcmp(field->key, "password") == 0) {
            user->password = strdup((char *)field->value);
        } else if (strcmp(field->key, "online") == 0) {
            user->isOnline = (*(int *)field->value) != 0;
        } else if (strcmp(field->key, "last_attendance_at") == 0) {
            user->lastLogin = *(long *)field->value;
        }
    }

    db_result_set_free(result);
    return user;
}
