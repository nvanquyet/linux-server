#include "service.h"
#include "session.h"
#include "log.h"
#include <stdlib.h>


Service* createService(Session* session) {
    if (session == NULL) {
        return NULL;
    }
    
    Service* service = (Service*)malloc(sizeof(Service));
    if (service == NULL) {
        return NULL;
    }
    
    service->session = session;
    return service;
}

void destroyService(Service* service) {
    if (service != NULL) {
        free(service);
    }
}



void service_login_success(Service* service) {
    if (service == NULL) {
        return;
    }
    
    Session* session = service->session;
    if (session == NULL) {
        return;
    }
    
    log_message(INFO, "Client %d: Login success", session->id);
}