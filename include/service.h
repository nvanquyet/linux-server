#ifndef SERVICE_H
#define SERVICE_H

#include "session.h"

typedef struct Service Service;
typedef struct Session Session;
typedef struct Message Message;

struct Service {
    Session* session;
    
    void (*login_success)(Service* self);
    void (*server_message)(Session* session, char *content);
    void (*broadcast_message)(int user_id[], int num_users, Message *msg);
    void (*direct_message)(int user_id, Message *msg);
};

Service* createService(Session* session);
void destroyService(Service* service);
void service_login_success(Service* service);
void service_server_message(Session* session, char *content);
void broadcast_message(int user_id[], int num_users, Message *msg);
void broadcast_message_except(int user_id[], int num_users, Message *msg, int excep_id);
void direct_message(int user_id, Message *msg);

#endif