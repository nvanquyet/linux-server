#include "controller.h"
#include "session.h"
#include "user.h"
#include "service.h"
#include "message.h"
#include "log.h"
#include "cmd.h"

Controller* createController(Session* client){
    Controller* controller = (Controller*)malloc(sizeof(Controller));
    if (controller == NULL) {
        return NULL;
    }
    controller->client = client;
    controller->service = NULL;
    controller->user = NULL;
    controller->onMessage = controller_on_message;
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
        return;
    }
    uint8_t command = message->command;
    
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


