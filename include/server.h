#ifndef SERVER_H
#define SERVER_H

#include <stdbool.h>

typedef struct {
    int server_socket;
    bool is_running;
} Server;

/**
 * Initialize the server
 * @return true if initialization was successful, false otherwise
 */
bool server_init();

/**
 * Start the server and begin accepting connections
 * This function runs in a loop until server_stop is called
 */
void server_start();

/**
 * Stop the server and close all connections
 */
void server_stop();

/**
 * Get the server instance
 * @return pointer to the server instance
 */
Server* server_get_instance();

#endif