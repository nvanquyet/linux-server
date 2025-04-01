#ifndef GROUP_H
#define GROUP_H
#include "user.h"
#include <stdbool.h>

#define MAX_MEMBERS 100
#define MAX_GROUP 100

typedef struct Group {
    int id;
    char name[50];
    long created_at;
    User* created_by;
    int member_count;
} Group;


bool create_group(Group *self, const char *group_name, User *creator);
bool delete_group(Group *self, User *user);
Group *get_group(int group_id);

#endif // GROUP_H
