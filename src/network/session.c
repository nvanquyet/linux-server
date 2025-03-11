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
#include <openssl/rand.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "m_utils.h"

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

typedef struct
{
    uint8_t p;
    uint8_t g;
    uint16_t aa;
    uint16_t A;
    uint16_t B;
    uint16_t K;
} Key;

void *sender_thread(void *arg);
void *collector_thread(void *arg);
Message *read_message(Session *session);
void process_message(Session *session, Message *msg);
bool do_send_message(Session *session, Message *msg);
void trade_key(Session *session, Message *msg);
void send_public_key(Session *session, Message *msg);
void send_dh_params(Session *session, Message *msg);
void send_dh_params(Session *session, Message *msg);
void clean_network(Session *session);
void session_close_message(Session *session);

void session_login(Session *self, Message *msg);
void session_register(Session *self, Message *msg);
void session_client_ok(Session *self);
bool session_is_connected(Session *self);
void session_disconnect(Session *self);
void session_on_message(Session *self, Message *msg);
void session_process_message(Session *self, Message *msg);
bool session_do_send_message(Session *self, Message *msg);
void session_close(Session *self);
void session_send_message(Session *session, Message *msg);
Message *session_read_message(Session *self);

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

    Key *key = (Key *)malloc(sizeof(Key));
    if (key == NULL)
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
    session->readMessage = session_read_message;
    session->closeMessage = session_close_message;

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

    key->A = 0;
    key->B = 0;
    key->K = 0;
    key->g = 0;
    key->p = 0;
    key->aa = 0;
    session->_key = key;

    session->handler = (Controller *)malloc(sizeof(Controller));
    session->service = (Service *)malloc(sizeof(Service));
    session->handler->service = session->service;
    session->user = NULL;

    private->collector->running = true;
    pthread_create(&private->collector->thread, NULL, collector_thread, private->collector);

    private->sender->running = true;
    pthread_create(&private->sender->thread, NULL, sender_thread, private->sender);

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
    if (session->connected && private->sender != NULL && private->sender->running)
    {
        log_message(DEBUG, "Adding message to queue 0");
        message_queue_add(private->sender->queue, message);
    }
}

bool session_do_send_message(Session *session, Message *msg)
{
    if (session == NULL || msg == NULL)
    {
        return false;
    }
    if (!do_send_message(session, msg))
    {
        log_message(ERROR, "Failed to send message");
        return false;
    }
    return true;
}

void session_close_message(Session *self)
{
    if (self == NULL)
    {
        return;
    }

    SessionPrivate *private = (SessionPrivate *)self->_private;
    if (!private || private->isClosed)
    {
        return;
    }

    private->isClosed = true;

    if (self->IPAddress != NULL)
    {
        server_manager_remove_ip(self->IPAddress);
    }
    else
    {
        log_message(ERROR, "Failed to remove IP address");
    }

    if (self->user != NULL)
    {
        server_manager_remove_user(self->user);
        destroyUser(self->user);
        self->user = NULL;
    }

    if (self->handler != NULL)
    {
        self->handler->onDisconnected(self->handler);
    }
    else
    {
        log_message(ERROR, "Failed to call onDisconnected");
    }
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

void session_login(Session *self, Message *msg) {
    if (self == NULL || msg == NULL) {
        return;
    }

    SessionPrivate *private = (SessionPrivate *)self->_private;
    if (!self->connected || !private->sendKeyComplete) {
        session_disconnect(self);
        return;
    }

    if (self->isLoginSuccess || self->isLogin) {
        return;
    }

    self->isLogin = true;

    msg->position = 0; 
    
    char username[256] = {0};
    char password[256] = {0};
    
    if (!message_read_string(msg, username, sizeof(username)) ||
        !message_read_string(msg, password, sizeof(password))) {
        log_message(ERROR, "Failed to read login data");
        return;
    }
    
    User *user = createUser(NULL, self, username, password);
    if (user != NULL) {
        user->login(user);
        if(user->isLoaded){
            self->isLoginSuccess = true;
            self->user = user;
    
            if (self->handler != NULL) {
                controller_set_user(self->handler, user);
                controller_set_service(self->handler, self->service);
            }
        } else {
            self->isLoginSuccess = false;
            self->isLogin = false;
            log_message(INFO, "Login failed: Invalid username or password");
            destroyUser(user);
        }
    }

    self->isLogin = false;
}
void trade_key(Session *session, Message *msg)
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

    switch (msg->command)
    {
    case GET_SESSION_ID:
        send_dh_params(session, msg);
        break;
    case TRADE_DH_PARAMS:
        send_public_key(session, msg);
        break;
    default:
        log_message(ERROR, "Unknown command %d", msg->command);
        break;
    }
}

void send_dh_params(Session *session, Message *msg)
{
    Key *key = (Key *)session->_key;
    SessionPrivate *private = (SessionPrivate *)session->_private;
    key->p = utils_next_int(255);
    key->g = utils_next_int(255);
    if (key->p == 0 || key->g == 0)
    {
        log_message(ERROR, "Invalid DH params");
        return;
    }

    Message *message = message_create(TRADE_DH_PARAMS);
    message_write(message, &key->p, sizeof(key->p));
    message_write(message, &key->g, sizeof(key->g));
    session->doSendMessage(session, message);
    message_destroy(message);

    private->sendKeyComplete = false;
}

void send_public_key(Session *session, Message *msg)
{
    Key *key = (Key *)session->_key;
    SessionPrivate *private = (SessionPrivate *)session->_private;

    key->B = msg->buffer[0];

    if (key->B == 0)
    {
        log_message(ERROR, "Invalid public key %d", key->B);
        return;
    }

    key->aa = utils_next_int(65535);
    key->A = utils_mod_exp(key->g, key->aa, key->p);
    key->K = utils_mod_exp(key->B, key->aa, key->p);
    if (key->K == 0)
    {
        log_message(ERROR, "Invalid key");
        return;
    }

    Message *message = message_create(TRADE_KEY);
    message_write(message, &key->A, sizeof(key->A));
    session->doSendMessage(session, message);
    message_destroy(message);
    private->key = (unsigned char *)malloc(32);
    generate_aes_key_from_K(key->K, private->key);
    private->sendKeyComplete = true;
}

void session_register(Session *self, Message *msg)
{
    if (self == NULL || msg == NULL)
    {
        return;
    }

    msg->position = 0;
    char username[256] = {0};
    char password[256] = {0};

    if (!message_read_string(msg, username, sizeof(username)) ||
        !message_read_string(msg, password, sizeof(password)))
    {
        log_message(ERROR, "Failed to read register data");
        return;
    }

    User *user = createUser(NULL, self, username, password);
    if (user != NULL)
    {
        user->userRegister(user);
        self->user = user;
    }
    else
    {
        log_message(ERROR, "Failed to create user");
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
                    message_destroy(msg);
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
        Message *message = session_read_message(session);
        if (message != NULL)
        {

            if (!private->sendKeyComplete)
            {
                trade_key(session, message);
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
    session_close_message(session);

    return NULL;
}

Message *session_read_message(Session *session)
{
    if (session == NULL)
    {
        return NULL;
    }

    SessionPrivate *private = (SessionPrivate *)session->_private;

    uint8_t command;
    if (recv(session->socket, &command, sizeof(command), 0) <= 0)
    {
        log_message(ERROR, "Failed to receive command, closing session");
        return NULL;
    }
    log_message(INFO, "Received command: %d", command);

    if (command == GET_SESSION_ID || command == TRADE_KEY || command == TRADE_DH_PARAMS)
    {
        uint32_t size_network;
        if (recv(session->socket, &size_network, sizeof(size_network), 0) <= 0)
        {
            log_message(ERROR, "Failed to receive unencrypted message size");
            return NULL;
        }
        uint32_t size = ntohl(size_network);

        Message *msg = message_create(command);
        if (msg == NULL)
        {
            log_message(ERROR, "Failed to create message");
            return NULL;
        }

        if (size > 0)
        {

            free(msg->buffer);
            msg->buffer = (unsigned char *)malloc(size);
            if (msg->buffer == NULL)
            {
                log_message(ERROR, "Failed to allocate buffer for unencrypted message");
                message_destroy(msg);
                return NULL;
            }
            msg->size = size;

            size_t total_read = 0;
            while (total_read < size)
            {
                ssize_t bytes_read = recv(session->socket, msg->buffer + total_read,
                                          size - total_read, 0);
                if (bytes_read <= 0)
                {
                    log_message(ERROR, "Failed to receive unencrypted data");
                    message_destroy(msg);
                    return NULL;
                }
                total_read += bytes_read;
            }
            msg->position = size;
        }

        return msg;
    }

    unsigned char iv[16];
    if (recv(session->socket, iv, sizeof(iv), 0) <= 0)
    {
        log_message(ERROR, "Failed to receive IV");
        return NULL;
    }

    uint32_t original_size;
    if (recv(session->socket, &original_size, sizeof(original_size), 0) <= 0)
    {
        log_message(ERROR, "Failed to receive original size");
        return NULL;
    }
    original_size = ntohl(original_size);

    uint32_t encrypted_size;
    if (recv(session->socket, &encrypted_size, sizeof(encrypted_size), 0) <= 0)
    {
        log_message(ERROR, "Failed to receive encrypted size");
        return NULL;
    }
    encrypted_size = ntohl(encrypted_size);

    Message *msg = message_create(command);
    if (msg == NULL)
    {
        log_message(ERROR, "Failed to create message");
        return NULL;
    }

    free(msg->buffer);
    msg->buffer = (unsigned char *)malloc(encrypted_size);
    if (msg->buffer == NULL)
    {
        log_message(ERROR, "Failed to allocate buffer");
        message_destroy(msg);
        return NULL;
    }
    msg->size = encrypted_size;

    size_t total_read = 0;
    while (total_read < encrypted_size)
    {
        ssize_t bytes_read = recv(session->socket, msg->buffer + total_read,
                                  encrypted_size - total_read, 0);
        if (bytes_read <= 0)
        {
            log_message(ERROR, "Failed to receive encrypted data");
            message_destroy(msg);
            return NULL;
        }
        total_read += bytes_read;
    }
    msg->position = encrypted_size;

    if (private->key == NULL)
    {
        private->key = (unsigned char *)malloc(32);
        if (private->key == NULL)
        {
            log_message(ERROR, "Failed to allocate key");
            message_destroy(msg);
            return NULL;
        }
        memcpy(private->key, "test_secret_key_for_aes_256_cipher", 32);
    }

    if (!message_decrypt(msg, private->key, iv))
    {
        log_message(ERROR, "Failed to decrypt message");
        message_destroy(msg);
        return NULL;
    }

    msg->position = 0;
    return msg;
}

bool do_send_message(Session *session, Message *msg)
{
    log_message(INFO, "Sending message command: %d", msg->command);

    if (session == NULL || msg == NULL)
    {
        return false;
    }

    if (msg->command == GET_SESSION_ID || msg->command == TRADE_KEY || msg->command == TRADE_DH_PARAMS)
    {

        if (send(session->socket, &msg->command, sizeof(uint8_t), 0) < 0)
        {
            log_message(ERROR, "Failed to send unencrypted message command");
            return false;
        }

        uint32_t net_size = htonl((uint32_t)msg->position);
        if (send(session->socket, &net_size, sizeof(net_size), 0) < 0)
        {
            log_message(ERROR, "Failed to send unencrypted message size");
            return false;
        }

        if (msg->position > 0)
        {
            if (send(session->socket, msg->buffer, msg->position, 0) < 0)
            {
                log_message(ERROR, "Failed to send unencrypted message data");
                return false;
            }
        }
        return true;
    }

    SessionPrivate *private = (SessionPrivate *)session->_private;

    unsigned char iv[16];
    if (RAND_bytes(iv, sizeof(iv)) != 1)
    {
        log_message(ERROR, "Failed to generate random IV");
        return false;
    }

    size_t original_size = msg->position;

    if (!message_encrypt(msg, private->key, iv))
    {
        log_message(ERROR, "Failed to encrypt message");
        return false;
    }

    if (send(session->socket, &msg->command, sizeof(uint8_t), 0) < 0)
    {
        log_message(ERROR, "Failed to send message command");
        return false;
    }

    if (send(session->socket, iv, sizeof(iv), 0) < 0)
    {
        log_message(ERROR, "Failed to send message IV");
        return false;
    }

    uint32_t net_original_size = htonl((uint32_t)original_size);
    if (send(session->socket, &net_original_size, sizeof(net_original_size), 0) < 0)
    {
        log_message(ERROR, "Failed to send original size");
        return false;
    }

    uint32_t net_encrypted_size = htonl((uint32_t)msg->position);
    if (send(session->socket, &net_encrypted_size, sizeof(net_encrypted_size), 0) < 0)
    {
        log_message(ERROR, "Failed to send encrypted size");
        return false;
    }

    if (send(session->socket, msg->buffer, msg->position, 0) < 0)
    {
        log_message(ERROR, "Failed to send encrypted data");
        return false;
    }

    return true;
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
        Controller *handler = session->handler;
        if (handler != NULL)
        {
            handler->onMessage(handler, msg);
        }
        else
        {
            log_message(ERROR, "Failed to call onMessage");
        }
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