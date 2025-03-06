#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stddef.h>
#include <arpa/inet.h>

#include "server.h"
#include "session.h"
#include "config.h"
#include "log.h"
#include "server_manager.h"

static Server server = {
    .server_socket = -1,
    .is_running = false};

Server *server_get_instance()
{
    return &server;
}

bool server_init()
{
    server.is_running = false;
    return true;
}

void server_start()
{
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_socket;
    char client_ip[INET_ADDRSTRLEN];
    int id = 0;

    server.server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server.server_socket < 0)
    {
        log_message(ERROR, "Failed to create server socket");
        return;
    }

    int opt = 1;
    if (setsockopt(server.server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        log_message(ERROR, "Failed to set socket options");
        close(server.server_socket);
        return;
    }

    Config *config = config_get_instance();
    if (config == NULL)
    {
        log_message(ERROR, "Failed to get config instance");
        close(server.server_socket);
        return;
    }

    int port = config_get_port(config);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server.server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        log_message(ERROR, "Failed to bind socket to port %d", port);
        close(server.server_socket);
        return;
    }

    if (listen(server.server_socket, 10) < 0)
    {
        log_message(ERROR, "Failed to listen on socket");
        close(server.server_socket);
        return;
    }

    log_message(INFO, "Start socket port=%d", port);
    server.is_running = true;
    log_message(INFO, "Start server Success!");

    while (server.is_running)
    {
        client_socket = accept(server.server_socket, (struct sockaddr *)&client_addr, &client_len);

        if (client_socket < 0)
        {
            if (server.is_running)
            {
                log_message(ERROR, "Failed to accept client connection");
            }
            continue;
        }

        inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);

        int connection_count = server_manager_frequency(client_ip);
        log_message(INFO, "IP: %s connection number: %d connected", client_ip, connection_count);

        if (connection_count >= config_get_ip_address_limit())
        {
            close(client_socket);
            log_message(ERROR, "IP: %s connection number: %d is over limit, connection refused", client_ip, connection_count);
            continue;
        }

        Session *session = createSession(client_socket, ++id);
        if (session == NULL)
        {
            log_message(ERROR, "Failed to create session for client");
            close(client_socket);
            continue;
        }
        server_manager_add_ip(client_ip);
    }
}
/*
void server_stop() {
    if (!server.is_running) {
        return;
    }

    server.is_running = false;


    ServerManager* manager = server_manager_get_instance();
    User** users = server_manager_get_users(manager);
    size_t user_count = server_manager_get_user_count(manager);

    for (size_t i = 0; i < user_count; i++) {
        if (!users[i]->is_cleaned) {
            session_close(users[i]->session);
        }
    }


    if (server.server_socket != -1) {
        close(server.server_socket);
        server.server_socket = -1;
    }

    log_message(INFO, "End socket");
} */