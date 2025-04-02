//
// Created by vawnwuyest on 4/1/25.
//
#ifndef GROUPMEMBER_H
#define GROUPMEMBER_H

#include "group.h"
#include <stdbool.h>
#define ERROR_MESSAGE_SIZE 256

bool add_group_member(int group_id, int user_id, char *error_message);
bool remove_group_member(int group_id, int user_id, char *error_message);
bool check_member_exists(int group_id, int user_id);
Group **find_groups_by_user(int user_id, int *out_count);
#endif // GROUPMEMBER_H
