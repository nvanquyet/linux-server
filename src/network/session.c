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

typedef struct {
    Message** messages;
    int capacity;
    int size;
    pthread_mutex_t mutex;
} MessageQueue;

typedef struct {
    Session* session;
    pthread_t thread;
    bool running;
} MessageCollector;

typedef struct {
    Session* session;
    MessageQueue* queue;
    pthread_t thread;
    bool running;
} Sender;

typedef struct {
    byte* key;
    int key_length;
    byte curR;
    byte curW;
    MessageCollector* collector;
    Sender* sender;
    bool sendKeyComplete;
    bool isClosed;
} SessionPrivate;

void* sender_thread(void* arg);
void* collector_thread(void* arg);
Message* read_message(Session* session);
void process_message(Session* session, Message* msg);
void do_send_message(Session* session, Message* msg);
void generate_key(Session* session);
byte read_key(Session* session, byte b);
byte write_key(Session* session, byte b);
void clean_network(Session* session);

void session_login(Session* self, Message* msg);
void session_register(Session* self, Message* msg);
void session_client_ok(Session* self);
bool session_is_connected(Session* self);
void session_disconnect(Session* self);
void session_on_message(Session* self, Message* msg);
void session_process_message(Session* self, Message* msg);
void session_do_send_message(Session* self, Message* msg);
void session_close(Session* self);

// Message queue functions
MessageQueue* message_queue_create(int initial_capacity);
void message_queue_add(MessageQueue* queue, Message* message);
Message* message_queue_get(MessageQueue* queue, int index);
Message* message_queue_remove(MessageQueue* queue, int index);
void message_queue_destroy(MessageQueue* queue);

Session* createSession(int socket, int id) {
    Session* session = (Session*)malloc(sizeof(Session));
    if (session == NULL) {
        return NULL;
    }
    
    // Allocate private data
    SessionPrivate* private = (SessionPrivate*)malloc(sizeof(SessionPrivate));
    if (private == NULL) {
        free(session);
        return NULL;
    }
    
    // Initialize socket and basic info
    session->socket = socket;
    session->id = id;
    session->connected = true;
    session->isLoginSuccess = false;
    session->clientOK = false;
    session->isLogin = false;
    session->IPAddress = NULL;
    
    // Initialize function pointers
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
    
    // Initialize private data
    private->key = NULL;
    private->key_length = 0;
    private->curR = 0;
    private->curW = 0;
    private->sendKeyComplete = false;
    private->isClosed = false;
    
    // Create message sender
    private->sender = (Sender*)malloc(sizeof(Sender));
    private->sender->session = session;
    private->sender->queue = message_queue_create(10);
    private->sender->running = false;
    
    // Create message collector
    private->collector = (MessageCollector*)malloc(sizeof(MessageCollector));
    private->collector->session = session;
    private->collector->running = false;
    
    // Store private data
    session->_private = private;
    
    // Initialize fields
    session->handler = (Controller*)malloc(sizeof(Controller));
    session->service = (Service*)malloc(sizeof(Service));
    session->user = NULL;
    
    log_message(INFO, "New session: %d", id);
    
    // Start collector thread
    private->collector->running = true;
    pthread_create(&private->collector->thread, NULL, collector_thread, private->collector);
    
    return session;
}

void destroySession(Session* session) {
    if (session != NULL) {
        SessionPrivate* private = (SessionPrivate*)session->_private;
        
        // Clean up private data
        if (private != NULL) {
            // Stop threads
            if (private->sender != NULL) {
                private->sender->running = false;
                pthread_join(private->sender->thread, NULL);
                
                // Destroy message queue
                if (private->sender->queue != NULL) {
                    message_queue_destroy(private->sender->queue);
                }
                free(private->sender);
            }
            
            if (private->collector != NULL) {
                private->collector->running = false;
                pthread_join(private->collector->thread, NULL);
                free(private->collector);
            }
            
            // Free encryption key
            if (private->key != NULL) {
                free(private->key);
            }
            
            free(private);
        }
        
        // Close socket
        if (session->socket != -1) {
            close(session->socket);
        }
        
        // Free IP address
        if (session->IPAddress != NULL) {
            free(session->IPAddress);
        }
        
        free(session);
    }
}

bool session_is_connected(Session* self) {
    return self->connected;
}

void session_set_handler(Session* session, Controller* handler) {
    if (session != NULL) {
        session->handler = handler;
    }
}

void session_set_service(Session* session, Service* service) {
    if (session != NULL) {
        session->service = service;
    }
}

void session_send_message(Session* session, Message* message) {
    if (session == NULL || message == NULL) {
        return;
    }
    
    SessionPrivate* private = (SessionPrivate*)session->_private;
    if (session->connected && private->sender != NULL) {
        message_queue_add(private->sender->queue, message);
    }
}

void session_do_send_message(Session* session, Message* msg) {
    if (session == NULL || msg == NULL) {
        return;
    }
    
    do_send_message(session, msg);
}

void session_close(Session* self) {
    if (self == NULL) {
        return;
    }
    
    clean_network(self);
}

void session_disconnect(Session* self) {
    if (self == NULL || !self->connected) {
        return;
    }
    
    // Close the socket to trigger disconnection
    close(self->socket);
    self->connected = false;
}

void session_login(Session* self, Message* msg) {
    if (self == NULL || msg == NULL) {
        return;
    }
    
    // Implementation of login logic
    // This would typically read data from the message and authenticate the user
    // For brevity, this is simplified
    
    SessionPrivate* private = (SessionPrivate*)self->_private;
    if (!self->connected || !private->sendKeyComplete) {
        session_disconnect(self);
        return;
    }
    
    if (self->isLoginSuccess || self->isLogin) {
        return;
    }
    
    self->isLogin = true;
    
    // Extract credentials from message
    char username[256];
    char password[256];
    // Read from message data...
    
    // Create and authenticate user
    User* user = createUser(NULL, self, username, password);
    if (user != NULL) {
        // Assume login is successful
        self->isLoginSuccess = true;
        self->user = user;
        
        // Set up controller and service
        if (self->handler != NULL) {
            controller_set_user(self->handler, user);
            controller_set_service(self->handler, self->service);
        }
        
        // Notify service
        if (self->service != NULL) {
            service_login_success(self->service);
        }
    }
    
    self->isLogin = false;
}

void session_register(Session* self, Message* msg) {
    if (self == NULL || msg == NULL) {
        return;
    }
    
    // Implementation of registration logic
    // This would typically read data from the message and create a new user
    // For brevity, this is simplified
}

void session_client_ok(Session* self) {
    if (self == NULL || self->clientOK) {
        return;
    }
    
    if (self->user == NULL) {
        log_message(ERROR, "Client %d: User not logged in", self->id);
        session_disconnect(self);
        return;
    }
    
    // Load player data, create character if needed
    bool load_success = true; // user_load_player_data(self->user);
    
    if (load_success) {
        self->clientOK = true;
        // service_player_load_all(self->service);
        log_message(INFO, "Client %d: logged in successfully", self->id);
    } else {
        log_message(ERROR, "Client %d: Failed to load player data", self->id);
        session_disconnect(self);
    }
}

void session_on_message(Session* self, Message* msg) {
    if (self == NULL || msg == NULL) {
        return;
    }
    
    // Process the message based on its command
    SessionPrivate* private = (SessionPrivate*)self->_private;
    if (private->isClosed) {
        return;
    }
    
    // If we have a handler, delegate to it
    if (self->handler != NULL) {
        (self->handler, msg);
    }
}

void generate_key(Session* session) {
    if (session == NULL) {
        return;
    }
    
    SessionPrivate* private = (SessionPrivate*)session->_private;
    
    // Generate a random key
    char key_prefix[] = "game_";
    char random_part[10];
    sprintf(random_part, "%d", utils_next_int(10000));
    
    int prefix_len = strlen(key_prefix);
    int random_len = strlen(random_part);
    int key_len = prefix_len + random_len;
    
    if (private->key != NULL) {
        free(private->key);
    }
    
    private->key = (byte*)malloc(key_len);
    private->key_length = key_len;
    
    // Copy the key components
    memcpy(private->key, key_prefix, prefix_len);
    memcpy(private->key + prefix_len, random_part, random_len);
}

void send_key(Session* session) {
    if (session == NULL) {
        return;
    }
    
    SessionPrivate* private = (SessionPrivate*)session->_private;
    if (private->sendKeyComplete) {
        return;
    }
    
    // Generate encryption key
    generate_key(session);
    
    // Create message with key information
    Message* msg = message_create(GET_SESSION);
    // Write key data to message...
    
    // Send the key message
    do_send_message(session, msg);
    
    // Mark key as sent
    private->sendKeyComplete = true;
    
    // Start sender thread now that key is established
    private->sender->running = true;
    pthread_create(&private->sender->thread, NULL, sender_thread, private->sender);
}

byte read_key(Session* session, byte b) {
    if (session == NULL) {
        return b;
    }
    
    SessionPrivate* private = (SessionPrivate*)session->_private;
    byte curR = private->curR;
    private->curR = (byte)(curR + 1);
    
    byte result = (byte)((private->key[curR] & 255) ^ (b & 255));
    
    if (private->curR >= private->key_length) {
        private->curR %= private->key_length;
    }
    
    return result;
}

byte write_key(Session* session, byte b) {
    if (session == NULL) {
        return b;
    }
    
    SessionPrivate* private = (SessionPrivate*)session->_private;
    byte curW = private->curW;
    private->curW = (byte)(curW + 1);
    
    byte result = (byte)((private->key[curW] & 255) ^ (b & 255));
    
    if (private->curW >= private->key_length) {
        private->curW %= private->key_length;
    }
    
    return result;
}

void clean_network(Session* session) {
    if (session == NULL) {
        return;
    }
    
    SessionPrivate* private = (SessionPrivate*)session->_private;
    
    // Clean up user if available
    if (session->user != NULL && !session->user->isCleaned) {
        // user_clean_up(session->user);
    }
    
    // Reset crypto state
    private->curR = 0;
    private->curW = 0;
    
    // Mark as disconnected
    session->connected = false;
    session->isLoginSuccess = false;
    
    // Close the socket
    if (session->socket != -1) {
        close(session->socket);
        session->socket = -1;
    }
    
    // Clear handlers
    session->handler = NULL;
    session->service = NULL;
}

void* sender_thread(void* arg) {
    Sender* sender = (Sender*)arg;
    Session* session = sender->session;
    MessageQueue* queue = sender->queue;
    
    while (session->connected && sender->running) {
        SessionPrivate* private = (SessionPrivate*)session->_private;
        
        if (private->sendKeyComplete) {
            // Process all pending messages
            while (queue->size > 0) {
                Message* msg = message_queue_remove(queue, 0);
                if (msg != NULL) {
                    do_send_message(session, msg);
                    // message_destroy(msg);
                }
            }
        }
        
        // Sleep a bit to prevent CPU hogging
        usleep(10000); // 10ms
    }
    
    return NULL;
}

void* collector_thread(void* arg) {
    MessageCollector* collector = (MessageCollector*)arg;
    Session* session = collector->session;
    SessionPrivate* private = (SessionPrivate*)session->_private;
    
    while (session->connected && collector->running) {
        Message* message = read_message(session);
        if (message != NULL) {
            if (!private->sendKeyComplete) {
                send_key(session);
            } else {
                process_message(session, message);
            }
            // message_destroy(message);
        } else {
            break; // Connection closed
        }
    }
    
    // Handle disconnection
    // session_close_message(session);
    
    return NULL;
}

Message* read_message(Session* session) {
    if (session == NULL) {
        return NULL;
    }
    
    // This is a simplified version - in reality you'd need to handle all the
    // DataInputStream-equivalent functionality and error handling
    
    // Read bytes from socket and construct a message
    // For now, just return NULL to indicate no message or error
    return NULL;
}

void process_message(Session* session, Message* msg) {
    if (session == NULL || msg == NULL) {
        return;
    }
    
    SessionPrivate* private = (SessionPrivate*)session->_private;
    if (!private->isClosed) {
        session->processMessage(session, msg);
    }
}

void do_send_message(Session* session, Message* msg) {
    if (session == NULL || msg == NULL) {
        return;
    }
    
    // This would normally write the message to the socket
    // using dos.write() equivalent operations
    // For brevity, implementation details are omitted
}

MessageQueue* message_queue_create(int initial_capacity) {
    MessageQueue* queue = (MessageQueue*)malloc(sizeof(MessageQueue));
    if (queue == NULL) {
        return NULL;
    }
    
    queue->messages = (Message**)malloc(sizeof(Message*) * initial_capacity);
    queue->capacity = initial_capacity;
    queue->size = 0;
    pthread_mutex_init(&queue->mutex, NULL);
    
    return queue;
}

void message_queue_add(MessageQueue* queue, Message* message) {
    if (queue == NULL || message == NULL) {
        return;
    }
    
    pthread_mutex_lock(&queue->mutex);
    
    // Resize if necessary
    if (queue->size >= queue->capacity) {
        int new_capacity = queue->capacity * 2;
        Message** new_messages = (Message**)realloc(queue->messages, sizeof(Message*) * new_capacity);
        if (new_messages == NULL) {
            pthread_mutex_unlock(&queue->mutex);
            return;
        }
        queue->messages = new_messages;
        queue->capacity = new_capacity;
    }
    
    // Add message to queue
    queue->messages[queue->size++] = message;
    
    pthread_mutex_unlock(&queue->mutex);
}

Message* message_queue_get(MessageQueue* queue, int index) {
    if (queue == NULL || index < 0 || index >= queue->size) {
        return NULL;
    }
    
    pthread_mutex_lock(&queue->mutex);
    Message* msg = queue->messages[index];
    pthread_mutex_unlock(&queue->mutex);
    
    return msg;
}

Message* message_queue_remove(MessageQueue* queue, int index) {
    if (queue == NULL || index < 0 || index >= queue->size) {
        return NULL;
    }
    
    pthread_mutex_lock(&queue->mutex);
    
    // Get the message at the index
    Message* msg = queue->messages[index];
    
    // Shift remaining elements
    for (int i = index; i < queue->size - 1; i++) {
        queue->messages[i] = queue->messages[i + 1];
    }
    
    // Decrement size
    queue->size--;
    
    pthread_mutex_unlock(&queue->mutex);
    
    return msg;
}

void message_queue_destroy(MessageQueue* queue) {
    if (queue == NULL) {
        return;
    }
    
    pthread_mutex_lock(&queue->mutex);
    
    // Free all messages (ideally they should be destroyed properly)
    free(queue->messages);
    
    pthread_mutex_unlock(&queue->mutex);
    pthread_mutex_destroy(&queue->mutex);
    
    free(queue);
}

void session_process_message(Session* self, Message* msg) {
    if (self == NULL || msg == NULL) {
        return;
    }
    
    SessionPrivate* private = (SessionPrivate*)self->_private;
    if (private->isClosed) {
        return;
    }
    
    self->onMessage(self, msg);
}