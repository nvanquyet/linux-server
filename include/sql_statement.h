#ifndef SQL_STATEMENT_H
#define SQL_STATEMENT_H

#define SQL_REGISTER "INSERT INTO users (username, password) VALUES (?, ?)"
#define SQL_LOGIN "SELECT * FROM users WHERE username=?"
#endif