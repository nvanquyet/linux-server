#include "user.h"
#include "session.h"
#include "service.h"

User* createUser(User* self, Session* client, char* username, char* password) {
    self->id = 0;
    self->username = username;
    self->password = password;
    self->isOnline = false;
    self->session = client;
    self->service = NULL;
    self->ipAddr = NULL;
    self->lastLogin = 0;
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
}
