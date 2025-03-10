#include "controller.h"
#include "session.h"
#include "user.h"
#include "service.h"
#include "message.h"
#include "log.h"
#include "cmd.h"


void controller_on_message(Controller* self, Message* message);
void controller_on_connection_fail(Controller* self);
void controller_on_disconnected(Controller* self);
void controller_on_connect_ok(Controller* self);
void controller_message_in_chat(Controller* self, Message* ms);
void controller_message_not_in_chat(Controller* self, Message* ms);
void controller_new_message(Controller* self, Message* ms);

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
        log_message(INFO, "Client %d: logged in successfully", controller->client->id);
    }
}

void controller_on_message(Controller* self, Message* message){
    if(self == NULL || message == NULL){
        log_message(ERROR, "Client %d: message is NULL", self->client->id);
        return;
    }

    log_message(INFO, "here");
    uint8_t command = message->command;
    log_message(INFO, "Command %d", command);
    switch (command)
    {
    case LOGIN:
        log_message(INFO, "Client %d: login", self->client->id);
        break;
    case REGISTER:
        log_message(INFO, "Client %d: register", self->client->id);
        break;
    case LOGOUT:
        log_message(INFO, "Client %d: logout", self->client->id);
        break;
    case GET_SESSION_ID:
        log_message(INFO, "Client %d: get session id", self->client->id);
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


