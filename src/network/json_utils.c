#include "../../include/json_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Convert Group struct to JSON string
char* group_to_json(const Group* group) {
    if (group == NULL) return NULL;

    char* json_str = (char*)malloc(256);
    if (!json_str) return NULL;

    snprintf(json_str, 256,
             "{ \"id\": %d, \"name\": \"%s\", \"created_at\": %ld }",
             group->id, group->name, group->created_at);

    return json_str; // Remember to free() after use
}

// Convert JSON string to Group object (Optional)
Group* json_to_group(const char* json_str) {
    if (json_str == NULL) return NULL;

    Group* group = (Group*)malloc(sizeof(Group));
    if (!group) return NULL;

    sscanf(json_str, "{ \"id\": %d, \"name\": \"%[^\"]\", \"created_at\": %ld }",
           &group->id, group->name, &group->created_at);

    return group;
}
