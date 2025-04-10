#ifndef GROUP_H
#define GROUP_H
#include "user.h"
#include <stdbool.h>

#define MAX_MEMBERS 100
#define MAX_GROUP 100

typedef struct Group {
    int id;
    char name[50];
    char password[256];
    long created_at;
    User* created_by;
    int member_count;
} Group;


Group *create_group(const char *group_name, const char *password, User *creator, char *error_message);
bool delete_group(Group *self, User *user, char *error_message);
Group *get_group_by_id(int group_id);
Group *get_group(Group *self, char *errorMsg, size_t errorSize);
Group *create_new_group(char* group_name, char *password);
void broad_cast_to_group(int group_id, Message *msg);
#endif // GROUP_H
