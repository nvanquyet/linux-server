#include "controller.h"
#include "session.h"
#include "user.h"
#include "service.h"
#include "message.h"
#include "log.h"
#include "cmd.h"
#include <stdlib.h>

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


void get_online_users(Session* session);

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
        self->client->login(self->client, message);
        break;
    case REGISTER:
        self->client->clientRegister(self->client, message);
        break;
    case LOGOUT:
        log_message(INFO, "Client %d: logout", self->client->id);
        break;
    case GET_ONLINE_USERS:
        get_online_users(self->client);
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
    case SEND_MESSAGE:
        server_receive_message(self->client, message);
        break;
    case SEND_GROUP_MESSAGE:
        server_receive_group_message(self->client, message);
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

void get_online_users(Session* session){
    ServerManager *manager = server_manager_get_instance();
    if(manager == NULL){
        return;
    }
    if(session == NULL){
        return;
    }

    User *users[MAX_USERS];
    int count = 0;
    server_manager_get_users(users, &count);

    Message *msg = message_create(GET_ONLINE_USERS);
    if(msg == NULL){
        log_message(ERROR, "Failed to create message");
        return;
    }
    message_write_int(msg, count);
    for(int i = 0; i < count; i++){
        message_write_string(msg, users[i]->username);
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

    bool joined = add_group_member(group_id, user_id);

    message_write_bool(message, joined);
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
    bool result = remove_group_member(group_id, user_id);
    message_write_bool(message, result);
    session_send_message(session, message);
}

void server_receive_message(Session* session, Message* msg) {
    // ServerManager *manager = server_manager_get_instance();
    // if (!manager || !session || !msg) {
    //     return;
    // }
    //
    // msg->position = 0;
    // int sender_id = (int) message_read_int(msg);
    // int group_id = (int) message_read_int(msg);
    // char content[1024];
    // if (!message_read_string(msg, content, sizeof(content))) {
    //     log_message(ERROR, "Failed to read data");
    //     return;
    // }
    //
    // //bool sent = store_group_message(group_id, sender_id, content);
    //
    // Message *message = message_create(RECEIVE_GROUP_MESSAGE);
    // if (!message) {
    //     log_message(ERROR, "Failed to create message");
    //     return;
    // }
    //
    // //message_write_bool(message, sent);
    // // if (sent) {
    // //     message_write_int(message, group_id);
    // // }
    // session_send_message(session, message);
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
        User *user = findUserById(user_id);
        if (!user) {
            log_message(ERROR, "Failed to find user");
            message_write_bool(message, false);
        }else {
            bool deleted = delete_group(group, user);
            message_write_bool(message, deleted);
        }
    }
    session_send_message(session, message);
}

void server_receive_group_message(Session* session, Message* msg) {
    // ServerManager *manager = server_manager_get_instance();
    // if (!manager || !session || !msg) {
    //     return;
    // }
    //
    // msg->position = 0;
    // int group_id = (int) message_read_int(msg);
    // int message_count = 0;
    // GroupMessage **messages = get_group_messages(group_id, &message_count);
    //
    // Message *response = message_create(GROUP_MESSAGE_HISTORY);
    // if (!response) {
    //     log_message(ERROR, "Failed to create message");
    //     return;
    // }
    //
    // message_write_int(response, message_count);
    // for (int i = 0; i < message_count; i++) {
    //     message_write_int(response, messages[i]->sender_id);
    //     message_write_string(response, messages[i]->content);
    //     message_write_long(response, messages[i]->timestamp);
    //     free(messages[i]);
    // }
    // free(messages);
    //
    // session_send_message(session, response);
}
