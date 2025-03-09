#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "user.h"
#include "server_manager.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include "user.h"
#include "server_manager.h"
#include "log.h"

static ServerManager server_manager = {
    .initialized = false,
    .user_count = 0,
    .ip_count = 0
};

static pthread_mutex_t init_mutex = PTHREAD_MUTEX_INITIALIZER;

ServerManager* server_manager_get_instance() {
    if (!server_manager.initialized) {
        pthread_mutex_lock(&init_mutex);
        if (!server_manager.initialized) {
            init_server_manager();
        }
        pthread_mutex_unlock(&init_mutex);
    }
    return &server_manager;
}

void init_server_manager() {
    if (server_manager.initialized) {
        return;
    }
    
    log_message(INFO, "Initializing server manager");
    
    for (int i = 0; i < MAX_USERS; i++) {
        server_manager.users[i] = NULL;
        server_manager.ips[i] = NULL;
    }
    
    server_manager.user_count = 0;
    server_manager.ip_count = 0;
    
    pthread_rwlock_init(&server_manager.lock_user, NULL);
    pthread_rwlock_init(&server_manager.lock_session, NULL);
    
    server_manager.initialized = true;
    
    log_message(INFO, "Server manager initialized");
}

void server_manager_get_users(User *buffer[], int *count) {
    ServerManager *manager = server_manager_get_instance();
    pthread_rwlock_rdlock(&manager->lock_user);
    *count = manager->user_count;
    memcpy(buffer, manager->users, sizeof(User*) * manager->user_count);
    pthread_rwlock_unlock(&manager->lock_user);
}

int server_manager_get_number_online() {
    ServerManager *manager = server_manager_get_instance();
    pthread_rwlock_rdlock(&manager->lock_user);
    int count = manager->user_count;
    pthread_rwlock_unlock(&manager->lock_user);
    return count;
}

int server_manager_frequency(const char *ip) {
    ServerManager *manager = server_manager_get_instance();
    int count = 0;
    pthread_rwlock_rdlock(&manager->lock_session);
    for (int i = 0; i < manager->ip_count; i++) {
        if (strcmp(manager->ips[i], ip) == 0) {
            count++;
        }
    }
    pthread_rwlock_unlock(&manager->lock_session);
    return count;
}

User *server_manager_find_user_by_id(int id) {
    ServerManager *manager = server_manager_get_instance();
    pthread_rwlock_rdlock(&manager->lock_user);
    for (int i = 0; i < manager->user_count; i++) {
        if (manager->users[i]->id == id) {
            pthread_rwlock_unlock(&manager->lock_user);
            return manager->users[i];
        }
    }
    pthread_rwlock_unlock(&manager->lock_user);
    return NULL;
}

User *server_manager_find_user_by_username(const char *username) {
    ServerManager *manager = server_manager_get_instance();
    pthread_rwlock_rdlock(&manager->lock_user);
    for (int i = 0; i < manager->user_count; i++) {
        if (strcmp(manager->users[i]->username, username) == 0) {
            pthread_rwlock_unlock(&manager->lock_user);
            return manager->users[i];
        }
    }
    pthread_rwlock_unlock(&manager->lock_user);
    return NULL;
}

void server_manager_add_user(User *user) {
    ServerManager *manager = server_manager_get_instance();
    pthread_rwlock_wrlock(&manager->lock_user);
    if (manager->user_count < MAX_USERS) {
        manager->users[manager->user_count++] = user;
    }
    pthread_rwlock_unlock(&manager->lock_user);
}

void server_manager_remove_user(User *user) {
    if (user == NULL) {
        return;
    }
    
    ServerManager *manager = server_manager_get_instance();
    pthread_rwlock_wrlock(&manager->lock_user);
    for (int i = 0; i < manager->user_count; i++) {
        if (manager->users[i] != NULL && manager->users[i]->id == user->id) {
            manager->users[i] = manager->users[--manager->user_count];  
            break;
        }
    }
    pthread_rwlock_unlock(&manager->lock_user);
}

void server_manager_add_ip(const char *ip) {
    ServerManager *manager = server_manager_get_instance();
    pthread_rwlock_wrlock(&manager->lock_session);
    if (manager->ip_count < MAX_USERS) {
        manager->ips[manager->ip_count++] = strdup(ip);
    }
    pthread_rwlock_unlock(&manager->lock_session);
}

void server_manager_remove_ip(const char *ip) {
    if (ip == NULL) {
        return;
    }
    
    ServerManager *manager = server_manager_get_instance();
    pthread_rwlock_wrlock(&manager->lock_session);
    for (int i = 0; i < manager->ip_count; i++) {
        if (manager->ips[i] != NULL && strcmp(manager->ips[i], ip) == 0) {
            free(manager->ips[i]);
            manager->ips[i] = manager->ips[--manager->ip_count];
            break;
        }
    }
    pthread_rwlock_unlock(&manager->lock_session);
}

void destroy_server_manager() {
    if (!server_manager.initialized) {
        return;
    }
    
    for (int i = 0; i < MAX_USERS; i++) {
        if (server_manager.users[i] != NULL) {
            server_manager.users[i] = NULL;
        }
        if (server_manager.ips[i] != NULL) {
            free(server_manager.ips[i]);
            server_manager.ips[i] = NULL;
        }
    }
    
    pthread_rwlock_destroy(&server_manager.lock_user);
    pthread_rwlock_destroy(&server_manager.lock_session);
    server_manager.initialized = false;
}