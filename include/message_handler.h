#ifndef MESSAGE_HANDLER_H
#define MESSAGE_HANDLER_H

#include "message.h"

typedef struct {
    void (*onMessage)(struct MessageHandler* handler, Message* message);
    void (*onConnectionFail)(struct MessageHandler* handler);
    void (*onDisconnected)(struct MessageHandler* handler);
    void (*onConnectOK)(struct MessageHandler* handler);
    void (*messageInGame)(struct MessageHandler* handler, Message* ms);
    void (*messageNotInGame)(struct MessageHandler* handler, Message* ms);
    void (*newMessage)(struct MessageHandler* handler, Message* ms);

} MessageHandler;

#endif