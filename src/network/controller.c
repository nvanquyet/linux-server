#include "controller.h"

Controller* createController(Session* client){
    Controller* controller = (Controller*)malloc(sizeof(Controller));
    if (controller == NULL) {
        return NULL;
    }
    controller->client = client;
    controller->service = NULL;
    controller->user = NULL;
    return controller;
}
void destroyController(Controller* controller){

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