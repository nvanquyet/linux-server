#ifndef CMD_H
#define CMD_H

#define SERVER_MESSAGE 0x00
#define GET_SESSION_ID 0x01
#define TRADE_DH_PARAMS 0x02
#define TRADE_KEY 0x03



#define LOGIN 0x04
#define LOGIN_SUCCESS 0x05
#define REGISTER 0x06
#define LOGOUT 0x07

#define GET_ONLINE_USERS 0x08


#define GET_JOINED_GROUPS 0x09

#define CREATE_GROUP 0x0A
#define JOIN_GROUP 0x0C

#define LEAVE_GROUP 0x0E

#define DELETE_GROUP 0x10

#define GET_CHAT_HISTORY 0x11
#define GROUP_MESSAGE 0x12
#define USER_MESSAGE 0x13

#endif
