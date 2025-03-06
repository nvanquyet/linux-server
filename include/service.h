#ifndef SERVICE_H
#define SERVICE_H

#include "session.h"

typedef struct Service Service;
typedef struct Session Session;

struct Service {
    Session* session;
};

Service* createService();
void destroyService(Service* service);
void service_set_session(Service* service, Session* session);

#endif