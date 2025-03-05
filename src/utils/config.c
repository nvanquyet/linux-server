#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "config.h"
#include "log.h"

static Config *instance = NULL;

static char *trim_string(char *str)
{
    char *end;

    while (*str == ' ')
        str++;

    if (*str == 0)
        return str;

    end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\n' || *end == '\r'))
        end--;

    end[1] = '\0';

    return str;
}

static char *duplicate_string(const char *str)
{
    if (str == NULL)
        return NULL;
    char *result = malloc(strlen(str) + 1);
    if (result != NULL)
    {
        strcpy(result, str);
    }
    return result;
}

Config *config_get_instance()
{
    if (instance == NULL)
    {
        instance = calloc(1, sizeof(Config));
        if (instance == NULL)
        {
            log_message(LogLevel.ERROR, "Failed to allocate memory for Config");
            return NULL;
        }
    }
    return instance;
}

bool config_load()
{
    FILE *file = fopen("config.properties", "r");
    if (file == NULL)
    {
        log_message(LogLevel.ERROR, "Failed to open config file");
        return false;
    }

    Config *cfg = config_get_instance();
    if (cfg == NULL)
    {
        fclose(file);
        return false;
    }

    char line[256];
    char key[128];
    char value[128];

    while (fgets(line, sizeof(line), file))
    {

        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
            continue;

        char *eq = strchr(line, '=');
        if (eq == NULL)
            continue;

        *eq = '\0';
        strncpy(key, trim_string(line), sizeof(key) - 1);
        key[sizeof(key) - 1] = '\0';

        strncpy(value, trim_string(eq + 1), sizeof(value) - 1);
        value[sizeof(value) - 1] = '\0';

        char log_buffer[256];
        snprintf(log_buffer, sizeof(log_buffer), "Config - %s: %s", key, value);
        log_info(log_buffer);

        if (strcmp(key, "server.port") == 0)
        {
            cfg->port = atoi(value);
        }
        else if (strcmp(key, "login.limit") == 0)
        {
            cfg->ip_address_limit = atoi(value);
        }
        else if (strcmp(key, "db.host") == 0)
        {
            cfg->db_host = duplicate_string(value);
        }
        else if (strcmp(key, "db.port") == 0)
        {
            cfg->db_port = atoi(value);
        }
        else if (strcmp(key, "db.user") == 0)
        {
            cfg->db_user = duplicate_string(value);
        }
        else if (strcmp(key, "db.password") == 0)
        {
            cfg->db_password = duplicate_string(value);
        }
        else if (strcmp(key, "db.dbname") == 0)
        {
            cfg->db_name = duplicate_string(value);
        }
    }

    fclose(file);
    return true;
}

bool config_get_show_log()
{
    return instance->show_log;
}

int config_get_port()
{
    return instance->port;
}

int config_get_ip_address_limit()
{
    return instance->ip_address_limit;
}

const char *config_get_db_host()
{
    return instance->db_host;
}

int config_get_db_port()
{
    return instance->db_port;
}

const char *config_get_db_user()
{
    return instance->db_user;
}

const char *config_get_db_password()
{
    return instance->db_password;
}

const char *config_get_db_name()
{
    return instance->db_name;
}

void config_cleanup()
{
    if (instance != NULL)
    {
        free(instance->db_host);
        free(instance->db_user);
        free(instance->db_password);
        free(instance->db_name);
        free(instance);
        instance = NULL;
    }
}