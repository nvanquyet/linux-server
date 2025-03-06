#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include "config.h"
#include "database_connector.h"
#include "config.h"
#include "server.h"
#include "log.h"
#include "utils.h"


static bool is_stop = false;
void* server_start_thread(void* arg) {
    server_start();
    return NULL;
}

int main(int argc, char* argv[]) {
    if (config_load()) {
        
        if (!db_manager_start()) {
            return EXIT_FAILURE;
        }
        
        
        if (is_port_available(config_get_port())) {
            pthread_t server_thread;
            if (pthread_create(&server_thread, NULL, server_start_thread, NULL) != 0) {
                log_message(ERROR, "Failed to create server thread");
                return EXIT_FAILURE;
            }
            
            pthread_detach(server_thread);
        
        } else {
            char error_msg[100];
            snprintf(error_msg, sizeof(error_msg), "Port %d da duoc su dung!", config_get_port());
            log_message(ERROR, error_msg);
            return EXIT_FAILURE;
        }
    } else {
        log_message(ERROR, "Failed to load config");
        return EXIT_FAILURE;
    }
    
    
    
    while (!is_stop) {
        
        sleep(1);
    }
    
    return EXIT_SUCCESS;
}

