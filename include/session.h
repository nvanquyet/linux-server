#ifndef ISESSION_H
#define ISESSION_H

#include <stdbool.h>

struct IMessageHandler;
struct Service;
struct Message;

typedef struct {
    bool (*isConnected)(void *self);
    void (*setHandler)(void *self, struct IMessageHandler *messageHandler);
    void (*setService)(void *self, struct Service *service);
    void (*sendMessage)(void *self, struct Message *message);
    void (*close)(void *self);
} ISession;

#endif
