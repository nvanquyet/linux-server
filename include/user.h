#ifndef USER_H
#define USER_H

#include "session.h"
#include "stdbool.h"
#include "service.h"

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

    void (*login)(User *self);
    void (*logout)(User *self, Message *msg);
    void (*userRegister)(User *self, Message *msg);
    bool (*isCleaned)(User *self);
};

User *createUser(User *user, Session *client, char *username, char *password);
void destroyUser(User *user);
void user_set_session(User *user, Session *session);
void user_set_service(User *user, Service *service);

#endif


