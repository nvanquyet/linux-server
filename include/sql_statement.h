#ifndef SQL_STATEMENT_H
#define SQL_STATEMENT_H

#define SQL_REGISTER "INSERT INTO users (username, password) VALUES (?, ?)"
#define SQL_LOGIN "SELECT * FROM users WHERE username=?"
#define SQL_UPDATE_USER_LOGIN "UPDATE `users` SET `online`=?, `last_attendance_at`=? WHERE `id`=? LIMIT 1"
#define SQL_UPDATE_USER_LOGOUT "UPDATE `users` SET `online`=? WHERE `id`=? LIMIT 1"
#endif