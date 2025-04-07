#ifndef SESSION_H
#define SESSION_H

#include "controller.h"
#include "service.h"
#include "message.h"
#include <stdbool.h>

// Forward declarations
typedef struct User User;
typedef struct Session Session;
typedef struct Controller Controller;
typedef struct Service Service;
typedef struct Message Message;

typedef unsigned char byte;

struct Session {
    int id;
    int socket;
    User* user;
    Controller* handler;
    Service* service;
    bool connected;
    bool clientOK;
    bool isLogin;
    char* IPAddress;
    

    bool (*isConnected)(Session* self);
    void (*setHandler)(Session* self, Controller* handler);
    void (*setService)(Session* self, Service* service);
    void (*sendMessage)(Session* self, Message* message);
    void (*close)(Session* self);
    int (*login)(Session *self, Message *msg, char *errorMessage, size_t errorSize);
    bool (*clientRegister)(Session* self, Message* msg, char *errorMessage, size_t errorSize);
    void (*clientOk)(Session* self);
    bool (*doSendMessage)(Session* self, Message* msg);
    void (*disconnect)(Session* self);
    void (*onMessage)(Session* self, Message* msg);
    void (*processMessage)(Session* self, Message* msg);
    Message* (*readMessage)(Session* self);
    void (*closeMessage)(Session* self);
    
    void* _private;
    void* _key;
};

Session* createSession(int socket, int id);
void destroySession(Session* session);
void session_set_handler(Session* session, Controller* handler);
void session_set_service(Session* session, Service* service);
void session_send_message(Session* session, Message* message);
void session_close(Session* session);
int session_login(Session *self, Message *msg, char *errorMessage, size_t errorSize);
bool session_register(Session *self, Message *msg, char *errorMessage, size_t errorSize);
void session_client_ok(Session* session);
bool session_do_send_message(Session* session, Message* message);
void session_disconnect(Session* session);
void session_on_message(Session* session, Message* message);
void session_process_message(Session* session, Message* message);
Message* session_read_message(Session* session);
void session_close_message(Session* session);


#endif