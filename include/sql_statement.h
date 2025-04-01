#ifndef SQL_STATEMENT_H
#define SQL_STATEMENT_H

// 📌 User Queries
#define SQL_REGISTER "INSERT INTO users (username, password) VALUES (?, ?)"
#define SQL_LOGIN "SELECT * FROM users WHERE username=?"
#define SQL_UPDATE_USER_LOGIN "UPDATE users SET online=?, last_attendance_at=? WHERE id=? LIMIT 1"
#define SQL_UPDATE_USER_LOGOUT "UPDATE users SET online=? WHERE id=? LIMIT 1"
#define SQL_GET_USER_BY_ID "SELECT id, username, password, online, last_attendance_at FROM users WHERE id=?"


// 📌 Group Queries
#define SQL_CREATE_GROUP "INSERT INTO groups (group_name, created_by, created_at) VALUES (?, ?, ?)"
#define SQL_DELETE_GROUP "DELETE FROM groups WHERE group_id=?"
#define SQL_GET_GROUP "SELECT * FROM groups WHERE group_id=?"
#define SQL_GET_ALL_GROUPS "SELECT * FROM groups ORDER BY create_at DESC"

// 📌 GroupMember Queries
#define SQL_ADD_GROUP_MEMBER "INSERT INTO group_members (group_id, user_id, joined_at, role) VALUES (?, ?, ?, ?)"
#define SQL_REMOVE_GROUP_MEMBER "DELETE FROM group_members WHERE group_id=? AND user_id=?"
#define SQL_GET_GROUP_MEMBERS "SELECT * FROM group_members WHERE group_id=?"
#define SQL_GET_GROUPS_BY_USER "SELECT g.group_id, g.group_name, g.created_at, g.created_by FROM groups g JOIN group_members gm ON g.group_id = gm.group_id WHERE gm.user_id=? ORDER BY g.created_at DESC"
#define SQL_UPDATE_MEMBER_ROLE "UPDATE group_members SET role=? WHERE group_id=? AND user_id=?"

// 📌 Message Queries
#define SQL_ADD_MESSAGE "INSERT INTO messages (message_id, sender_id, receiver_id, content, created_at, group_id) VALUES (?, ?, ?, ?, ?, ?)"
#define SQL_GET_MESSAGE "SELECT * FROM messages WHERE message_id=?"
#define SQL_GET_GROUP_MESSAGES "SELECT * FROM messages WHERE group_id=? ORDER BY created_at ASC"
#define SQL_GET_USER_MESSAGES "SELECT * FROM messages WHERE sender_id=? ORDER BY created_at DESC"
#define SQL_DELETE_MESSAGE "DELETE FROM messages WHERE message_id=?"

#endif // SQL_STATEMENT_H
