#ifndef USER_H
#define USER_H

#include "session.h"
#include "stdbool.h"
#include "service.h"
#include <stddef.h>

typedef struct User User;
typedef struct Session Session;
typedef struct Service Service;
typedef struct Message Message;
struct User
{
    int id;
    char *username;
    char *password;
    bool isOnline;
    Session *session;
    Service *service;
    char *ipAddr;
    long lastLogin;
    bool isLoaded;
    bool isCleaned;
    bool messageSent;

    void (*login)(User *self);
    int (*loginResult)(User *self, char *errorMessage, size_t errorSize);
    bool (*registerResult)(User *self, char *errorMessage, size_t errorSize);
    void (*logout)(User *self);
    void (*userRegister)(User *self);
    void (*clean_user)(User *self);
};

User *createUser(User *user, Session *client, char *username, char *password);
void destroyUser(User *user);
void user_set_session(User *user, Session *session);
void user_set_service(User *user, Service *service);
void close_session_callback(void *arg);
User *findUserById(int id);

User* get_all_users_except(User* current_user, int* count);
User* get_all_users(int* count);

User* search_user(char *username, int *count);

#endif


