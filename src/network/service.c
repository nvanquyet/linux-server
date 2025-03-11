#include "service.h"
#include "session.h"
#include "log.h"
#include <stdlib.h>
#include "cmd.h"
#include "message.h"



void service_login_success(Service* service);
void service_server_message(Session* session, char *content);

Service* createService(Session* session) {
    if (session == NULL) {
        return NULL;
    }
    
    Service* service = (Service*)malloc(sizeof(Service));
    if (service == NULL) {
        return NULL;
    }
    
    service->session = session;
    service->login_success = service_login_success;
    service->server_message = service_server_message;
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
    
    Message* msg = message_create(LOGIN_SUCCESS);
    if (msg == NULL) {
        log_message(ERROR, "Failed to create message");
        return;
    }
    message_write_byte(msg, 1);
    session_send_message(session, msg);
}

void service_server_message(Session* session, char *content) {
    if (session == NULL || content == NULL) {
        return;
    }

    Message *msg = message_create(SERVER_MESSAGE);
    if (msg == NULL) {
        log_message(ERROR, "Failed to create message");
        return;
    }

    message_write_string(msg, content);
    session_send_message(session, msg);
    log_message(DEBUG, "Server message sent: %s", content);
}