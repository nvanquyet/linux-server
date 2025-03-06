#ifndef SERVICE_H
#define SERVICE_H

#include "session.h"

typedef struct Service Service;
typedef struct Session Session;

struct Service {
    Session* session;
};

Service* createService(Session* session);
void destroyService(Service* service);
void service_set_session(Service* service, Session* session);
void service_login_success(Service* service);


#endif