#include "session.h"
#include <unistd.h>  // For close() function
#include <stdlib.h>  // For malloc

Session* createSession(int socket, int id) {
    Session* session = (Session*)malloc(sizeof(Session));
    if (session == NULL) {
        return NULL;
    }
    
    session->socket = socket;
    session->id = id;
    session->connected = true;
    
    // Initialize function pointers to NULL or default implementations
    session->isConnected = NULL;
    session->setHandler = NULL;
    session->setService = NULL;
    session->sendMessage = NULL;
    session->close = NULL;
    session->login = NULL;
    session->clientRegister = NULL;
    session->clientOk = NULL;
    session->doSendMessage = NULL;
    session->disconnect = NULL;
    session->onMessage = NULL;
    session->processMessage = NULL;
    
    // Initialize other fields
    session->handler = NULL;
    session->service = NULL;
    
    return session;
}

void destroySession(Session* session) {
    if (session != NULL) {
        close(session->socket);
        free(session);
    }
}