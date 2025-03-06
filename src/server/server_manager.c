#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "user.h"
#include "server_manager.h"


static ServerManager server_manager;

void init_server_manager() {
    
    for (int i = 0; i < MAX_USERS; i++) {
        server_manager.users[i] = NULL;
        server_manager.ips[i] = NULL;
    }
    
    
    server_manager.user_count = 0;
    server_manager.ip_count = 0;
    
    
    pthread_rwlock_init(&server_manager.lock_user, NULL);
    pthread_rwlock_init(&server_manager.lock_session, NULL);
}

void destroy_server_manager() {
    for (int i = 0; i < MAX_USERS; i++) {
        if (server_manager.users[i] != NULL) {
            free(server_manager.users[i]);
        }
        if (server_manager.ips[i] != NULL) {
            free(server_manager.ips[i]);
        }
    }
    
    pthread_rwlock_destroy(&server_manager.lock_user);
    pthread_rwlock_destroy(&server_manager.lock_session);
}

void server_manager_get_users(User *buffer[], int *count) {
    pthread_rwlock_rdlock(&server_manager.lock_user);
    *count = server_manager.user_count;
    memcpy(buffer, server_manager.users, sizeof(User*) * server_manager.user_count);
    pthread_rwlock_unlock(&server_manager.lock_user);
}

int server_manager_get_number_online() {
    pthread_rwlock_rdlock(&server_manager.lock_user);
    int count = server_manager.user_count;
    pthread_rwlock_unlock(&server_manager.lock_user);
    return count;
}

int server_manager_frequency(const char *ip) {
    int count = 0;
    pthread_rwlock_rdlock(&server_manager.lock_session);
    for (int i = 0; i < server_manager.ip_count; i++) {
        if (strcmp(server_manager.ips[i], ip) == 0) {
            count++;
        }
    }
    pthread_rwlock_unlock(&server_manager.lock_session);
    return count;
}

User *server_manager_find_user_by_id(int id) {
    pthread_rwlock_rdlock(&server_manager.lock_user);
    for (int i = 0; i < server_manager.user_count; i++) {
        if (server_manager.users[i]->id == id) {
            pthread_rwlock_unlock(&server_manager.lock_user);
            return server_manager.users[i];
        }
    }
    pthread_rwlock_unlock(&server_manager.lock_user);
    return NULL;
}

User *server_manager_find_user_by_username(const char *username) {
    pthread_rwlock_rdlock(&server_manager.lock_user);
    for (int i = 0; i < server_manager.user_count; i++) {
        if (strcmp(server_manager.users[i]->username, username) == 0) {
            pthread_rwlock_unlock(&server_manager.lock_user);
            return server_manager.users[i];
        }
    }
    pthread_rwlock_unlock(&server_manager.lock_user);
    return NULL;
}

void server_manager_add_user(User *user) {
    pthread_rwlock_wrlock(&server_manager.lock_user);
    if (server_manager.user_count < MAX_USERS) {
        server_manager.users[server_manager.user_count++] = user;
    }
    pthread_rwlock_unlock(&server_manager.lock_user);
}

void server_manager_remove_user(User *user) {
    pthread_rwlock_wrlock(&server_manager.lock_user);
    for (int i = 0; i < server_manager.user_count; i++) {
        if (server_manager.users[i]->id == user->id) {
            server_manager.users[i] = server_manager.users[--server_manager.user_count];  
            break;
        }
    }
    pthread_rwlock_unlock(&server_manager.lock_user);
}

void server_manager_add_ip(const char *ip) {
    pthread_rwlock_wrlock(&server_manager.lock_session);
    if (server_manager.ip_count < MAX_USERS) {
        server_manager.ips[server_manager.ip_count++] = strdup(ip);
    }

    pthread_rwlock_unlock(&server_manager.lock_session);
}

void server_manager_remove_ip(const char *ip) {
    pthread_rwlock_wrlock(&server_manager.lock_session);
    for (int i = 0; i < server_manager.ip_count; i++) {
        if (strcmp(server_manager.ips[i], ip) == 0) {
            free(server_manager.ips[i]);
            server_manager.ips[i] = server_manager.ips[--server_manager.ip_count];
            break;
        }
    }
    pthread_rwlock_unlock(&server_manager.lock_session);
}