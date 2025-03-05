#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include "server.h"
#include "config.h"
#include "log.h"

static int server_socket = -1;
static bool server_running = false;

bool server_init() {
    if (server_running) {
        log_message(WARN, "Server already initialized");
        return true;
    }
    
    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        log_message(ERROR, "Failed to create server socket");
        return false;
    }
    
    // Enable port reuse
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        log_message(WARN, "Failed to set socket options");
    }
    
    // Configure server address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(config_get_port());
    
    // Bind to port
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        log_message(ERROR, "Failed to bind to port %d", config_get_port());
        close(server_socket);
        return false;
    }
    
    // Start listening
    if (listen(server_socket, 10) < 0) {
        log_message(ERROR, "Failed to listen on server socket");
        close(server_socket);
        return false;
    }
    
    server_running = true;
    return true;
}

void server_start() {
    if (!server_running || server_socket < 0) {
        if (!server_init()) {
            log_message(ERROR, "Failed to initialize server");
            return;
        }
    }
    
    log_message(INFO, "Server started on port %d", config_get_port());
    
    // Main server loop
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_socket;
    
    while (server_running) {
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            log_message(ERROR, "Failed to accept connection");
            continue;
        }
        
        // Handle client connection (simplified, you would create a thread per client)
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        log_message(INFO, "New connection from %s", client_ip);
        
        // TODO: Handle client communication
        
        close(client_socket);
    }
}

void server_stop() {
    if (server_running) {
        server_running = false;
        if (server_socket >= 0) {
            close(server_socket);
            server_socket = -1;
        }
        log_message(INFO, "Server stopped");
    }
}