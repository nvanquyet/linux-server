#ifndef SERVICE_H
#define SERVICE_H

#include "session.h"

typedef struct Service Service;
typedef struct Session Session;

struct Service {
    Session* session;
    
    void (*login_success)(Service* self);
    void (*server_message)(Session* session, char *content);
};

Service* createService(Session* session);
void destroyService(Service* service);
void service_login_success(Service* service);
void service_server_message(Session* session, char *content);


#endif