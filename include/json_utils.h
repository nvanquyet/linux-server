#ifndef JSON_UTILS_H
#define JSON_UTILS_H

#include "group.h"

// Convert Group to JSON string
char* group_to_json(const Group* group);

// Convert JSON string to Group object (if needed)
Group* json_to_group(const char* json_str);

#endif // JSON_UTILS_H
