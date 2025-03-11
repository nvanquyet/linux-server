#include "controller.h"
#include "session.h"
#include "user.h"
#include "service.h"
#include "message.h"
#include "log.h"
#include "cmd.h"
#include <stdlib.h>
#include "server_manager.h"


void controller_on_message(Controller* self, Message* message);
void controller_on_connection_fail(Controller* self);
void controller_on_disconnected(Controller* self);
void controller_on_connect_ok(Controller* self);
void controller_message_in_chat(Controller* self, Message* ms);
void controller_message_not_in_chat(Controller* self, Message* ms);
void controller_new_message(Controller* self, Message* ms);


void get_online_users(Session* session);

Controller* createController(Session* client){
    Controller* controller = (Controller*)malloc(sizeof(Controller));
    if (controller == NULL) {
        return NULL;
    }
    controller->client = client;
    controller->service = NULL;
    controller->user = NULL;
    controller->onMessage = controller_on_message;
    controller->onConnectionFail = controller_on_connection_fail;
    controller->onDisconnected = controller_on_disconnected;
    controller->onConnectOK = controller_on_connect_ok;
    controller->messageInChat = controller_message_in_chat;
    controller->messageNotInChat = controller_message_not_in_chat;
    controller->newMessage = controller_new_message;

    return controller;
}
void destroyController(Controller* controller){
    if(controller != NULL){
        free(controller);
    }
}
void controller_set_service(Controller* controller, Service* service){
    if(controller != NULL){
        controller->service = service;
    }
}
void controller_set_user(Controller* controller, User* user){
    if(controller != NULL){
        controller->user = user;
    }
}

void controller_on_message(Controller* self, Message* message){
    if(self == NULL || message == NULL){
        log_message(ERROR, "Client %d: message is NULL", self->client->id);
        return;
    }

    uint8_t command = message->command;
    switch (command)
    {
    case LOGIN:
        self->client->login(self->client, message);
        break;
    case REGISTER:
        self->client->clientRegister(self->client, message);
        break;
    case LOGOUT:
        log_message(INFO, "Client %d: logout", self->client->id);
        break;
    case GET_ONLINE_USERS:
        get_online_users(self->client);
        break;
    default:
        log_message(ERROR, "Client %d: unknown command %d", self->client->id, command);
        break;
    }
}

void controller_on_connection_fail(Controller* self){
    if(self == NULL){
        return;
    }
    log_message(ERROR, "Client %d: connection fail", self->client->id);
}

void controller_on_disconnected(Controller* self){
    if(self == NULL){
        return;
    }
    log_message(ERROR, "Client %d: disconnected", self->client->id);
}

void controller_on_connect_ok(Controller* self){
    if(self == NULL){
        return;
    }
    log_message(INFO, "Client %d: connected", self->client->id);
}

void controller_message_in_chat(Controller* self, Message* ms){
    if(self == NULL || ms == NULL){
        return;
    }
    log_message(INFO, "Client %d: message in chat", self->client->id);
}

void controller_message_not_in_chat(Controller* self, Message* ms){
    if(self == NULL || ms == NULL){
        return;
    }
    log_message(INFO, "Client %d: message not in chat", self->client->id);
}

void controller_new_message(Controller* self, Message* ms){
    if(self == NULL || ms == NULL){
        return;
    }
    log_message(INFO, "Client %d: new message", self->client->id);
}

void get_online_users(Session* session){
    ServerManager *manager = server_manager_get_instance();
    if(manager == NULL){
        return;
    }
    if(session == NULL){
        return;
    }

    User *users[MAX_USERS];
    int count = 0;
    server_manager_get_users(users, &count);

    Message *msg = message_create(GET_ONLINE_USERS);
    if(msg == NULL){
        log_message(ERROR, "Failed to create message");
        return;
    }
    message_write_int(msg, count);
    for(int i = 0; i < count; i++){
        message_write_string(msg, users[i]->username);
    }

    session_send_message(session, msg);
}
