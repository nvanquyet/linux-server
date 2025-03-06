#include "session.h"
#include "controller.h"
#include "service.h"
#include "message.h"
#include "log.h"
#include "user.h"
#include "cmd.h"
#include "utils.h"
#include "server_manager.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include "aes_utils.h"

typedef struct
{
    Message **messages;
    int capacity;
    int size;
    pthread_mutex_t mutex;
} MessageQueue;

typedef struct
{
    Session *session;
    pthread_t thread;
    bool running;
} MessageCollector;

typedef struct
{
    Session *session;
    MessageQueue *queue;
    pthread_t thread;
    bool running;
} Sender;

typedef struct
{
    byte *key;
    MessageCollector *collector;
    Sender *sender;
    bool sendKeyComplete;
    bool isClosed;
} SessionPrivate;

void *sender_thread(void *arg);
void *collector_thread(void *arg);
Message *read_message(Session *session);
void process_message(Session *session, Message *msg);
void do_send_message(Session *session, Message *msg);
void send_key(Session *session);
void get_key(Session *session, Message *msg);
void clean_network(Session *session);

void session_login(Session *self, Message *msg);
void session_register(Session *self, Message *msg);
void session_client_ok(Session *self);
bool session_is_connected(Session *self);
void session_disconnect(Session *self);
void session_on_message(Session *self, Message *msg);
void session_process_message(Session *self, Message *msg);
void session_do_send_message(Session *self, Message *msg);
void session_close(Session *self);

MessageQueue *message_queue_create(int initial_capacity);
void message_queue_add(MessageQueue *queue, Message *message);
Message *message_queue_get(MessageQueue *queue, int index);
Message *message_queue_remove(MessageQueue *queue, int index);
void message_queue_destroy(MessageQueue *queue);

Session *createSession(int socket, int id)
{
    Session *session = (Session *)malloc(sizeof(Session));
    if (session == NULL)
    {
        return NULL;
    }

    SessionPrivate *private = (SessionPrivate *)malloc(sizeof(SessionPrivate));
    if (private == NULL)
    {
        free(session);
        return NULL;
    }

    session->socket = socket;
    session->id = id;
    session->connected = true;
    session->isLoginSuccess = false;
    session->clientOK = false;
    session->isLogin = false;
    session->IPAddress = NULL;

    session->isConnected = session_is_connected;
    session->setHandler = session_set_handler;
    session->setService = session_set_service;
    session->sendMessage = session_send_message;
    session->close = session_close;
    session->login = session_login;
    session->clientRegister = session_register;
    session->clientOk = session_client_ok;
    session->doSendMessage = session_do_send_message;
    session->disconnect = session_disconnect;
    session->onMessage = session_on_message;
    session->processMessage = session_process_message;

    private->key = NULL;
    private->sendKeyComplete = false;
    private->isClosed = false;

    private->sender = (Sender *)malloc(sizeof(Sender));
    private->sender->session = session;
    private->sender->queue = message_queue_create(10);
    private->sender->running = false;

    private->collector = (MessageCollector *)malloc(sizeof(MessageCollector));
    private->collector->session = session;
    private->collector->running = false;

    session->_private = private;

    session->handler = (Controller *)malloc(sizeof(Controller));
    session->service = (Service *)malloc(sizeof(Service));
    session->user = NULL;

    log_message(INFO, "New session: %d", id);

    private->collector->running = true;
    pthread_create(&private->collector->thread, NULL, collector_thread, private->collector);

    return session;
}

void destroySession(Session *session)
{
    if (session != NULL)
    {
        SessionPrivate *private = (SessionPrivate *)session->_private;

        if (private != NULL)
        {

            if (private->sender != NULL)
            {
                private->sender->running = false;
                pthread_join(private->sender->thread, NULL);

                if (private->sender->queue != NULL)
                {
                    message_queue_destroy(private->sender->queue);
                }
                free(private->sender);
            }

            if (private->collector != NULL)
            {
                private->collector->running = false;
                pthread_join(private->collector->thread, NULL);
                free(private->collector);
            }

            if (private->key != NULL)
            {
                free(private->key);
            }

            free(private);
        }

        if (session->socket != -1)
        {
            close(session->socket);
        }

        if (session->IPAddress != NULL)
        {
            free(session->IPAddress);
        }

        free(session);
    }
}

bool session_is_connected(Session *self)
{
    return self->connected;
}

void session_set_handler(Session *session, Controller *handler)
{
    if (session != NULL)
    {
        session->handler = handler;
    }
}

void session_set_service(Session *session, Service *service)
{
    if (session != NULL)
    {
        session->service = service;
    }
}

void session_send_message(Session *session, Message *message)
{
    if (session == NULL || message == NULL)
    {
        return;
    }

    SessionPrivate *private = (SessionPrivate *)session->_private;
    if (session->connected && private->sender != NULL)
    {
        message_queue_add(private->sender->queue, message);
    }
}

void session_do_send_message(Session *session, Message *msg)
{
    if (session == NULL || msg == NULL)
    {
        return;
    }

    do_send_message(session, msg);
}

void session_close(Session *self)
{
    if (self == NULL)
    {
        return;
    }

    clean_network(self);
}

void session_disconnect(Session *self)
{
    if (self == NULL || !self->connected)
    {
        return;
    }

    close(self->socket);
    self->connected = false;
}

void session_login(Session *self, Message *msg)
{
    if (self == NULL || msg == NULL)
    {
        return;
    }

    SessionPrivate *private = (SessionPrivate *)self->_private;
    if (!self->connected || !private->sendKeyComplete)
    {
        session_disconnect(self);
        return;
    }

    if (self->isLoginSuccess || self->isLogin)
    {
        return;
    }

    self->isLogin = true;

    char username[256];
    char password[256];

    User *user = createUser(NULL, self, username, password);
    if (user != NULL)
    {

        self->isLoginSuccess = true;
        self->user = user;

        if (self->handler != NULL)
        {
            controller_set_user(self->handler, user);
            controller_set_service(self->handler, self->service);
        }

        if (self->service != NULL)
        {
            service_login_success(self->service);
        }
    }

    self->isLogin = false;
}

void send_key(Session *session)
{
    if (session == NULL)
    {
        return;
    }

    SessionPrivate *private = (SessionPrivate *)session->_private;
    if (private->sendKeyComplete)
    {
        return;
    }

    Message *msg = message_create(GET_SESSION_ID);
    if (msg != NULL)
    {
        session_send_message(session, msg);
    }
}

void session_register(Session *self, Message *msg)
{
    if (self == NULL || msg == NULL)
    {
        return;
    }
}

void session_client_ok(Session *self)
{
    if (self == NULL || self->clientOK)
    {
        return;
    }

    if (self->user == NULL)
    {
        log_message(ERROR, "Client %d: User not logged in", self->id);
        session_disconnect(self);
        return;
    }

    bool load_success = true;

    if (load_success)
    {
        self->clientOK = true;

        log_message(INFO, "Client %d: logged in successfully", self->id);
    }
    else
    {
        log_message(ERROR, "Client %d: Failed to load player data", self->id);
        session_disconnect(self);
    }
}

void session_on_message(Session *self, Message *msg)
{
    if (self == NULL || msg == NULL)
    {
        return;
    }

    SessionPrivate *private = (SessionPrivate *)self->_private;
    if (private->isClosed)
    {
        return;
    }

    if (self->handler != NULL)
    {
        (self->handler, msg);
    }
}


void clean_network(Session *session)
{
    if (session == NULL)
    {
        return;
    }

    SessionPrivate *private = (SessionPrivate *)session->_private;

    if (session->user != NULL && !session->user->isCleaned)
    {
    }

    session->connected = false;
    session->isLoginSuccess = false;

    if (session->socket != -1)
    {
        close(session->socket);
        session->socket = -1;
    }

    session->handler = NULL;
    session->service = NULL;
}

void *sender_thread(void *arg)
{
    Sender *sender = (Sender *)arg;
    Session *session = sender->session;
    MessageQueue *queue = sender->queue;

    while (session->connected && sender->running)
    {
        SessionPrivate *private = (SessionPrivate *)session->_private;

        if (private->sendKeyComplete)
        {

            while (queue->size > 0)
            {
                Message *msg = message_queue_remove(queue, 0);
                if (msg != NULL)
                {
                    do_send_message(session, msg);
                }
            }
        }

        usleep(10000);
    }

    return NULL;
}

void *collector_thread(void *arg)
{
    MessageCollector *collector = (MessageCollector *)arg;
    Session *session = collector->session;
    SessionPrivate *private = (SessionPrivate *)session->_private;

    while (session->connected && collector->running)
    {
        Message *message = read_message(session);
        if (message != NULL)
        {
            if (!private->sendKeyComplete)
            {
                send_key(session);
            }
            else
            {
                process_message(session, message);
            }
        }
        else
        {
            break;
        }
    }

    return NULL;
}



Message *read_message(Session *session)
{
    if (session == NULL)
    {
        return NULL;
    }

    return NULL;
}

void process_message(Session *session, Message *msg)
{
    if (session == NULL || msg == NULL)
    {
        return;
    }

    SessionPrivate *private = (SessionPrivate *)session->_private;
    if (!private->isClosed)
    {
        session->processMessage(session, msg);
    }
}

void do_send_message(Session *session, Message *msg)
{
    if (session == NULL || msg == NULL)
    {
        return;
    }
    

}

MessageQueue *message_queue_create(int initial_capacity)
{
    MessageQueue *queue = (MessageQueue *)malloc(sizeof(MessageQueue));
    if (queue == NULL)
    {
        return NULL;
    }

    queue->messages = (Message **)malloc(sizeof(Message *) * initial_capacity);
    queue->capacity = initial_capacity;
    queue->size = 0;
    pthread_mutex_init(&queue->mutex, NULL);

    return queue;
}

void message_queue_add(MessageQueue *queue, Message *message)
{
    if (queue == NULL || message == NULL)
    {
        return;
    }

    pthread_mutex_lock(&queue->mutex);

    if (queue->size >= queue->capacity)
    {
        int new_capacity = queue->capacity * 2;
        Message **new_messages = (Message **)realloc(queue->messages, sizeof(Message *) * new_capacity);
        if (new_messages == NULL)
        {
            pthread_mutex_unlock(&queue->mutex);
            return;
        }
        queue->messages = new_messages;
        queue->capacity = new_capacity;
    }

    queue->messages[queue->size++] = message;

    pthread_mutex_unlock(&queue->mutex);
}

Message *message_queue_get(MessageQueue *queue, int index)
{
    if (queue == NULL || index < 0 || index >= queue->size)
    {
        return NULL;
    }

    pthread_mutex_lock(&queue->mutex);
    Message *msg = queue->messages[index];
    pthread_mutex_unlock(&queue->mutex);

    return msg;
}

Message *message_queue_remove(MessageQueue *queue, int index)
{
    if (queue == NULL || index < 0 || index >= queue->size)
    {
        return NULL;
    }

    pthread_mutex_lock(&queue->mutex);

    Message *msg = queue->messages[index];

    for (int i = index; i < queue->size - 1; i++)
    {
        queue->messages[i] = queue->messages[i + 1];
    }

    queue->size--;

    pthread_mutex_unlock(&queue->mutex);

    return msg;
}

void message_queue_destroy(MessageQueue *queue)
{
    if (queue == NULL)
    {
        return;
    }

    pthread_mutex_lock(&queue->mutex);

    free(queue->messages);

    pthread_mutex_unlock(&queue->mutex);
    pthread_mutex_destroy(&queue->mutex);

    free(queue);
}

void session_process_message(Session *self, Message *msg)
{
    if (self == NULL || msg == NULL)
    {
        return;
    }

    SessionPrivate *private = (SessionPrivate *)self->_private;
    if (private->isClosed)
    {
        return;
    }

    self->onMessage(self, msg);
}