#ifndef SQL_STATEMENT_H
#define SQL_STATEMENT_H

// 📌 User Queries
#define SQL_REGISTER "INSERT INTO users (username, password) VALUES (?, ?)"
#define SQL_UPDATE_USER_LOGIN "UPDATE users SET online=?, last_attendance_at=? WHERE id=? LIMIT 1"
#define SQL_UPDATE_USER_LOGOUT "UPDATE users SET online=? WHERE id=? LIMIT 1"
#define SQL_GET_USER_BY_ID "SELECT * FROM users WHERE id=?"
#define SQL_GET_ALL_USERS_EXCEPT "SELECT id, username, password, online, last_attendance_at FROM users WHERE id != ?"
#define SQL_GET_ALL_USERS "SELECT id, username, password, online, last_attendance_at FROM users"
#define SQL_LOGIN "SELECT id , password FROM users WHERE username = ?"

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
#define SQL_CHECK_MEMBER_EXISTENCE "SELECT COUNT(*) FROM group_members WHERE group_id = ? AND user_id = ?"

// 📌 Message Queries
#define SQL_GET_CHAT_HISTORIES_BY_USER \
"SELECT " \
"  CASE " \
"    WHEN group_id IS NOT NULL THEN -group_id " \
"    WHEN sender_id = ? THEN receiver_id " \
"    ELSE sender_id " \
"  END AS chat_id, " \
"  MAX(timestamp) AS last_time, " \
"  SUBSTRING_INDEX(message_content, '\n', 1) AS last_message, " \
"  sender_id, " \
"  u.username AS sender_name " /* Thêm JOIN để lấy tên người gửi */ \
"FROM messages m " \
"LEFT JOIN users u ON u.id = m.sender_id " /* JOIN với bảng users để lấy tên người gửi */ \
"WHERE sender_id = ? " \
"   OR receiver_id = ? " \
"   OR group_id IN (SELECT group_id FROM group_members WHERE user_id = ?) " \
"GROUP BY chat_id " \
"ORDER BY last_time DESC"

#define SQL_GET_MESSAGES_WITH_USER \
"SELECT m.sender_id, u.username AS sender_name, m.message_content, m.timestamp \
FROM messages m \
JOIN users u ON m.sender_id = u.id \
WHERE ((m.sender_id = ? AND m.receiver_id = ?) OR (m.sender_id = ? AND m.receiver_id = ?)) \
AND m.group_id = 0 \
ORDER BY m.timestamp ASC"

#define SQL_GET_MESSAGES_WITH_GROUP \
"SELECT m.sender_id, u.username AS sender_name, m.message_content, m.timestamp \
FROM messages m \
JOIN users u ON m.sender_id = u.id \
WHERE m.group_id = ? \
ORDER BY m.timestamp ASC"

// Gửi tin nhắn riêng
#define SQL_INSERT_PRIVATE_MESSAGE \
"INSERT INTO messages (sender_id, receiver_id, message_content, timestamp) " \
"VALUES (?, ?, ?, ?)"

// Gửi tin nhắn nhóm
#define SQL_INSERT_GROUP_MESSAGE \
"INSERT INTO messages (sender_id, group_id, message_content, timestamp) " \
"VALUES (?, ?, ?, ?)"

#endif // SQL_STATEMENT_H
