#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
typedef struct {
    bool show_log;
    int port;
    int ip_address_limit;
    char *db_host;
    int db_port;
    char *db_user;
    char *db_password;
    char *db_name;
} Config;


Config* config_get_instance();
bool config_load();


bool config_get_show_log();
int config_get_port();
int config_get_ip_address_limit();
const char* config_get_db_host();
int config_get_db_port();
const char* config_get_db_user();
const char* config_get_db_password();
const char* config_get_db_name();

void config_cleanup();

#endif