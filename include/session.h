#ifndef SESSION_H
#define SESSION_H

#include "controller.h"
#include "service.h"
#include "message.h"
#include <stdbool.h>
struct Session{
    bool (*isConnected)(Session* self);
    void (*setHandler)(Session* self, Controller* handler);
    void (*setService)(Session* self, Service* service);
    void (*sendMessage)(Session* self, Message* message);
    void (*close)(Session* self);
    
    int id;
    Controller* handler;
    Service* service;
    bool connected;
    int socket;
    void (*login)(Session* self, Message* msg);
    void (*clientRegister)(Session* self, Message* msg);
    void (*clientOk)(Session* self);


    void (*doSendMessage)(Session* self, Message* msg);
    void (*disconnect)(Session* self);
    void (*onMessage)(Session* self, Message* msg);
    void (*processMessage)(Session* self, Message* msg);

    
};

Session* createSession(int socket, int id);
void destroySession(Session* session);

#endif