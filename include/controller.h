#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "session.h"
#include "user.h"
#include "service.h"
#include "message.h"

typedef struct Controller Controller;
typedef struct Message Message;
typedef struct Session Session;
typedef struct User User;

struct Controller {
    void (*onMessage)(Controller* self, Message* message);
    void (*onConnectionFail)(Controller* self);
    void (*onDisconnected)(Controller* self);
    void (*onConnectOK)(Controller* self);
    void (*messageInChat)(Controller* self, Message* ms);
    void (*messageNotInChat)(Controller* self, Message* ms);
    void (*newMessage)(Controller* self, Message* ms);
    
    Session* client;
    Service* service;
    User* user;
};

Controller* createController(Session* client);
void destroyController(Controller* controller);
void controller_set_service(Controller* controller, Service* service);
void controller_set_user(Controller* controller, User* user);
void controller_on_message(Controller* controller, Message* msg);


void get_users(Session* session);
void get_joined_groups(Session* session, Message* msg);
void server_handle_join_group(Session* session, Message* msg);
void server_handle_leave_group(Session* session, Message* msg);
void server_delete_group(Session* session, Message* msg);
void server_receive_message(Session* session, Message* msg);
void server_receive_group_message(Session* session, Message* msg);
void get_chat_history(Session* session, Message* msg);
void server_create_group(Session* session, Message* msg);
void get_user_message(Session* session, Message* msg);
void get_group_message(Session* session, Message* msg);
#endif