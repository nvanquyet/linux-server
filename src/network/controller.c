#include "controller.h"
#include "session.h"
#include "user.h"
#include "service.h"
#include "message.h"
#include "log.h"
#include "cmd.h"
#include <stdlib.h>

#include "db_message.h"
#include "group.h"
#include "group_member.h"
#include "json_utils.h"
#include "server_manager.h"


void controller_on_message(Controller* self, Message* message);
void controller_on_connection_fail(Controller* self);
void controller_on_disconnected(Controller* self);
void controller_on_connect_ok(Controller* self);
void controller_message_in_chat(Controller* self, Message* ms);
void controller_message_not_in_chat(Controller* self, Message* ms);
void controller_new_message(Controller* self, Message* ms);


void get_users(Session* session);

Controller* createController(Session* client){
    Controller* controller = (Controller*)malloc(sizeof(Controller));
    if (controller == NULL) {
        return NULL;
    }
    controller->client = client;
    controller->service = NULL;
    controller->user = NULL;
    controller->onMessage = controller_on_message;
    controller->onConnectionFail = controller_on_connection_fail;
    controller->onDisconnected = controller_on_disconnected;
    controller->onConnectOK = controller_on_connect_ok;
    controller->messageInChat = controller_message_in_chat;
    controller->messageNotInChat = controller_message_not_in_chat;
    controller->newMessage = controller_new_message;

    return controller;
}
void destroyController(Controller* controller){
    if(controller != NULL){
        free(controller);
    }
}
void controller_set_service(Controller* controller, Service* service){
    if(controller != NULL){
        controller->service = service;
    }
}
void controller_set_user(Controller* controller, User* user){
    if(controller != NULL){
        controller->user = user;
    }
}

void controller_on_message(Controller* self, Message* message){
    if(self == NULL || message == NULL){
        log_message(ERROR, "Client %d: message is NULL", self->client->id);
        return;
    }

    uint8_t command = message->command;
    switch (command)
    {
    case LOGIN:
        handle_login(self->client, message);
        break;
    case LOGOUT:
        //session_close_message(self->client);
        handle_logout(self->client, message);
        break;
    case REGISTER:
        handle_register(self->client, message);
        break;
    case GET_USERS:
        get_users(self->client);
        break;
    case GET_JOINED_GROUPS:
        get_joined_groups(self->client, message);
        break;
    case JOIN_GROUP:
        server_handle_join_group(self->client, message);
        break;
    case LEAVE_GROUP:
        server_handle_leave_group(self->client, message);
        break;
    case CREATE_GROUP:
        server_create_group(self->client, message);
        break;
    case DELETE_GROUP:
        server_delete_group(self->client, message);
        break;
    case USER_MESSAGE:
        server_receive_message(self->client, message);
        break;
    case GROUP_MESSAGE:
        server_receive_group_message(self->client, message);
        break;
    case GET_CHAT_HISTORY:
        get_chat_history(self->client, message);
        break;
    case GET_USERS_MESSAGE:
        get_user_message(self->client, message);
        break;
    case GET_GROUPS_MESSAGE:
        get_group_message(self->client, message);
        break;
    default:
        log_message(ERROR, "Client %d: unknown command %d", self->client->id, command);
        break;
    }
}

void controller_on_connection_fail(Controller* self){
    if(self == NULL){
        return;
    }
    log_message(ERROR, "Client %d: connection fail", self->client->id);
}

void controller_on_disconnected(Controller* self){
    if(self == NULL){
        return;
    }
    log_message(ERROR, "Client %d: disconnected", self->client->id);
}

void controller_on_connect_ok(Controller* self){
    if(self == NULL){
        return;
    }
    log_message(INFO, "Client %d: connected", self->client->id);
}

void controller_message_in_chat(Controller* self, Message* ms){
    if(self == NULL || ms == NULL){
        return;
    }
    log_message(INFO, "Client %d: message in chat", self->client->id);
}

void controller_message_not_in_chat(Controller* self, Message* ms){
    if(self == NULL || ms == NULL){
        return;
    }
    log_message(INFO, "Client %d: message not in chat", self->client->id);
}

void controller_new_message(Controller* self, Message* ms){
    if(self == NULL || ms == NULL){
        return;
    }
    log_message(INFO, "Client %d: new message", self->client->id);
}

void handle_login(Session* session, Message* message) {
    char errorMsg[256];

    char username[256];
    char password[256];
    Message *msg = message_create(LOGIN);
    if (msg == NULL) {
        log_message(ERROR, "Failed to create message");
        return;
    }
    message->position = 0;

    if (!message_read_string(message, username, sizeof(username))
        || !message_read_string(message, password, sizeof(username))) {
        log_message(WARN, "Invalid username");
        message_write_bool(msg, false);
        message_write_string(msg, "Cant read value");
        session_send_message(session, msg);
        return;
    }
    int loginOk = session->login(session, message, errorMsg, sizeof(errorMsg));
    message_write_bool(msg, loginOk > 0);
    free(message);
    if (!loginOk) {
        message_write_string(msg, errorMsg);
    }else {
        message_write_int(msg, loginOk);
        message_write_string(msg, username);
        message_write_string(msg, password);
    }
    session_send_message(session, msg);
}
void handle_register(Session* session, Message* message) {
    char errorMsg[256];
    bool registerOk = session->clientRegister(session, message, errorMsg, sizeof(errorMsg));

    Message *msg = message_create(REGISTER);
    if (msg == NULL) {
        log_message(ERROR, "Failed to create register response message");
        return;
    }

    message_write_bool(msg, registerOk);
    if (!registerOk) {
        message_write_string(msg, errorMsg);
    }

    session_send_message(session, msg);
    free(message);
}

void handle_logout(Session* session, Message* message) {
    session->user->logout(session->user);
    session->isLogin = false;
    free(message);
    Message *msg = message_create(LOGOUT);
    message_write_bool(msg, true);
    session_send_message(session, msg);
}
void get_users(Session* session){
    ServerManager *manager = server_manager_get_instance();
    if(manager == NULL){
        return;
    }
    if(session == NULL){
        return;
    }
    int all_user_count = 0;
    User* all_users = get_all_users(&all_user_count);
    User *users[MAX_USERS];
    int count = 0;
    server_manager_get_users(users, &count);
    for (int i = 0; i < all_user_count; i++) {
        all_users[i].isOnline = false;
        for (int j = 0; j < count; j++) {
            if (all_users[i].id == users[j]->id) {
                all_users[i].isOnline = true;
                break;
            }
        }
    }
    Message *msg = message_create(GET_USERS);
    if(msg == NULL){
        log_message(ERROR, "Failed to create message");
        return;
    }
    message_write_int(msg, all_user_count);
    for (int i = 0; i < all_user_count; i++) {
        message_write_int(msg, all_users[i].id);
        message_write_string(msg, all_users[i].username);
        message_write_bool(msg, all_users[i].isOnline);
    }
    session_send_message(session, msg);
}

void get_joined_groups(Session* session, Message* msg){
    ServerManager *manager = server_manager_get_instance();
    if(manager == NULL){
        return;
    }
    if(session == NULL){
        return;
    }

    Message *message = message_create(GET_JOINED_GROUPS);
    if(message == NULL){
        log_message(ERROR, "Failed to create message");
        return;
    }
    msg->position = 0;

    int user_id = (int)message_read_int(msg);
    int group_count = 0;
    Group **groups = find_groups_by_user(user_id, &group_count);
    if (!groups) {
        message_write_bool(message, false);
        log_message(INFO, "No groups for user %d\n", user_id);
    }else {
        message_write_bool(message, true);
        message_write_int(message, group_count);

        for (int i = 0; i < group_count; i++) {
            message_write_int(message, groups[i]->id);
            message_write_string(message, groups[i]->name);
            message_write_long(message, groups[i]->created_at);
            if (groups[i]->created_by) {
                message_write_int(message, groups[i]->created_by->id);
                message_write_string(message, groups[i]->created_by->username);
            } else {
                message_write_int(message, -1);  // Không có chủ nhóm
                message_write_string(message, "Unknown");  // Tên mặc định
            }
            log_message(INFO, "Nhóm %d: %s, Created by %s (%d) at %ld",
                groups[i]->id, groups[i]->name,
                groups[i]->created_by ? groups[i]->created_by->username : "Unknown",
                groups[i]->created_by ? groups[i]->created_by->id : -1, groups[i]->created_at);
            free(groups[i]);
        }
        free(groups);
    }

    session_send_message(session, message);
}

void server_create_group(Session* session, Message* msg){
    ServerManager *manager = server_manager_get_instance();
    if(manager == NULL){
        return;
    }
    if(session == NULL){
        return;
    }

    msg->position = 0;
    char group_name[256] = {0};
    Message *message = message_create(CREATE_GROUP);
    if(message == NULL){
        log_message(ERROR, "Failed to create group");
        return;
    }
    if (!message_read_string(msg, group_name, sizeof(group_name))) {
        log_message(ERROR, "Failed to read data");
        message_write_bool(message, false);
    }else {
        int user_id = (int) message_read_int(msg);
        User* user = findUserById(user_id);
        Group *newGroup = (Group *)malloc(sizeof(Group));
        if (create_group(newGroup, group_name, user)) {
            message_write_bool(message, true);
            //Todo: return value for client
            message_write_int(message, newGroup->id);
        }else {
            message_write_bool(message, false);
        }
    }
    session_send_message(session, message);
}

void server_handle_join_group(Session* session, Message* msg) {
    ServerManager *manager = server_manager_get_instance();
    if (!manager || !session || !msg) {
        return;
    }

    Message *message = message_create(JOIN_GROUP);
    if (!message) {
        log_message(ERROR, "Failed to create message");
        return;
    }
    msg->position = 0;
    int group_id = (int) message_read_int(msg);
    int user_id = (int) message_read_int(msg);
    char *error_message = (char *)malloc(256);
    bool joined = add_group_member(group_id, user_id, error_message);
    message_write_bool(message, joined);
    if (!joined) {
        message_write_string(message, error_message);
        log_message(INFO, "User %d was member of group %d", user_id, group_id);
    }
    session_send_message(session, message);
}

void server_handle_leave_group(Session* session, Message* msg) {
    ServerManager *manager = server_manager_get_instance();
    if (!manager || !session || !msg) {
        return;
    }
    Message *message = message_create(LEAVE_GROUP);
    if (!message) {
        log_message(ERROR, "Failed to create message");
        return;
    }

    msg->position = 0;
    int group_id = (int) message_read_int(msg);
    int user_id = (int) message_read_int(msg);
    char *error_message = (char *)malloc(256);

    Group *group = get_group(group_id);
    if (!group) {
        log_message(ERROR, "Failed to get group");
        message_write_bool(message, false);
    }else {
        if (group->created_by->id == user_id) {
            message_write_bool(message, false);
            message_write_string(message, "U are creator cant leave group");
            log_message(INFO, "User %d cant leave group %d", user_id, group_id);
        }else {
            bool result = remove_group_member(group_id, user_id, error_message);
            message_write_bool(message, result);
            if (!result) {
                message_write_string(message, error_message);
                log_message(INFO, "User %d isnt member of group %d", user_id, group_id);
            }
        }
    }
    session_send_message(session, message);
}

void server_receive_message(Session* session, Message* msg) {
    ServerManager *manager = server_manager_get_instance();
    if (!manager || !session || !msg) {
        return;
    }
    msg->position = 0;
    int sender_id = (int) message_read_int(msg);
    int receiver_id = (int) message_read_int(msg);
    char content[1024];
    if (!message_read_string(msg, content, sizeof(content))) {
        log_message(ERROR, "Failed to read data");
        return;
    }
    save_private_message(sender_id, receiver_id, content);
    //send to other client
    //session_send_message(session, msg);
    session->service->direct_message(receiver_id, msg);
}

void server_delete_group(Session* session, Message* msg) {
    ServerManager *manager = server_manager_get_instance();
    if (!manager || !session || !msg) {
        return;
    }
    Message *message = message_create(DELETE_GROUP);
    if (!message) {
        log_message(ERROR, "Failed to create message");
        return;
    }

    msg->position = 0;
    int group_id = (int) message_read_int(msg);
    int user_id = (int) message_read_int(msg);

    Group *group = get_group(group_id);
    if (!group) {
        log_message(ERROR, "Failed to get group");
        message_write_bool(message, false);
    }else {
        if (group->created_by->id == user_id) {
            User *user = findUserById(user_id);
            if (!user) {
                log_message(ERROR, "Failed to find user");
                message_write_bool(message, false);
            }else {
                bool deleted = delete_group(group, user);
                message_write_bool(message, deleted);
            }
        }else {
            message_write_bool(message, false);
            message_write_string(message, "You arent creator cant delete group");
        }

    }
    session_send_message(session, message);
}

void server_receive_group_message(Session* session, Message* msg) {
    ServerManager *manager = server_manager_get_instance();
    if (!manager || !session || !msg) {
        return;
    }
    msg->position = 0;
    int sender_id = (int) message_read_int(msg);
    int group_id = (int) message_read_int(msg);
    char content[1024];
    if (!message_read_string(msg, content, sizeof(content))) {
        log_message(ERROR, "Failed to read data");
        return;
    }
    bool is_member = check_member_exists(group_id, sender_id);
    log_message(INFO, "User %d is member of group %d", group_id, sender_id);
    if (is_member) {
        save_group_message(sender_id, group_id, content);
        //session_send_message(session, msg);
        //broadcast to all members of gtoups
        int count = 0;
        int* member_ids = get_group_members(group_id, &count);

        if (!member_ids || count == 0) {
            log_message(WARN, "Không tìm thấy thành viên nào trong group_id = %d", group_id);
            return;
        }
        broadcast_message(member_ids, count, msg);
        free(member_ids);
    } else {
        Message* mess = message_create(GROUP_MESSAGE);
        mess->position = 0;
        message_write_bool(mess, false);
        message_write_string(mess, "You arent not member of group");
        session_send_message(session, mess);
    }
}

void get_chat_history(Session* session, Message* msg) {
    ServerManager *manager = server_manager_get_instance();
    if (manager == NULL || session == NULL || msg == NULL) return;

    msg->position = 0;
    int user_id = (int) message_read_int(msg);
    int count = 0;

    Message* message = message_create(GET_CHAT_HISTORY);
    if (message == NULL) {
        log_message(ERROR, "Failed to create message");
        return;
    }

    ChatHistory* histories = get_chat_histories_by_user(user_id, &count);
    if (!histories || count == 0) {
        message_write_bool(message, false);
    } else {
        message_write_bool(message, true);
        message_write_int(message, count);
        for (int i = 0; i < count; i++) {
            log_message(INFO, "INFO id: %d, chat with: %s, time: %ld, content: %s, sender_id: %d, sender_name: %s",
                histories[i].id,
                histories[i].chat_with,
                histories[i].last_time,
                histories[i].last_message,
                histories[i].id,                // sender_id
                histories[i].sender_name);      // sender_name

            message_write_int(message, histories[i].id);            // chat_id
            message_write_string(message, histories[i].chat_with);  // chat_with
            message_write_long(message, histories[i].last_time);    // last_time
            message_write_string(message, histories[i].last_message); // last_message
            message_write_int(message, histories[i].id);            // sender_id
            message_write_string(message, histories[i].sender_name); // sender_name
        }
        free(histories);
    }

    session_send_message(session, message);
}
void get_user_message(Session* session, Message* msg) {
    ServerManager *manager = server_manager_get_instance();
    if (manager == NULL || session == NULL || msg == NULL) return;

    msg->position = 0;
    int user_id = (int) message_read_int(msg);
    int target_id = (int) message_read_int(msg);

    Message* response_msg = message_create(GET_USERS_MESSAGE);
    if (response_msg == NULL) {
        log_message(ERROR, "Failed to create message");
        return;
    }

    int count = 0;
    MessageData* messages = get_chat_messages(user_id, target_id, -1, &count);

    if (messages == NULL || count == 0) {
        message_write_bool(response_msg, false);
    } else {
        message_write_bool(response_msg, true);
        message_write_int(response_msg, count);

        // Gửi từng tin nhắn
        for (int i = 0; i < count; i++) {
            message_write_int(response_msg, messages[i].sender_id);
            message_write_string(response_msg, messages[i].sender_name);
            message_write_string(response_msg, messages[i].content);
            message_write_long(response_msg, messages[i].timestamp);
        }

        free(messages);
    }
    session_send_message(session, response_msg);
}
void get_group_message(Session* session, Message* msg) {
    ServerManager *manager = server_manager_get_instance();
    if (manager == NULL || session == NULL || msg == NULL) return;

    msg->position = 0;
    int user_id = (int) message_read_int(msg);
    int group_id = (int) message_read_int(msg);

    Message* response_msg = message_create(GET_GROUPS_MESSAGE);
    if (response_msg == NULL) {
        log_message(ERROR, "Failed to create message");
        return;
    }

    int count = 0;
    MessageData* messages = get_chat_messages(user_id, -1, group_id, &count);

    if (messages == NULL || count == 0) {
        message_write_bool(response_msg, false);
    } else {
        message_write_bool(response_msg, true);
        message_write_int(response_msg, count);

        // Gửi từng tin nhắn
        for (int i = 0; i < count; i++) {
            message_write_int(response_msg, messages[i].sender_id);
            message_write_string(response_msg, messages[i].sender_name);
            message_write_string(response_msg, messages[i].content);
            message_write_long(response_msg, messages[i].timestamp);
        }

        free(messages);
    }
    session_send_message(session, response_msg);
}
