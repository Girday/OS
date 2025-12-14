#include <zmq.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define MAX_USERS 100
#define MAX_GROUPS 50
#define MAX_GROUP_MEMBERS 20
#define MAX_USERNAME_LEN 50
#define MAX_GROUPNAME_LEN 50
#define MAX_MSG_LEN 4096
#define MAX_IDENTITY_LEN 256

typedef struct {
    char username[MAX_USERNAME_LEN];
    char identity[MAX_IDENTITY_LEN];
    int identity_len;
    int active;
} User;

typedef struct {
    char groupname[MAX_GROUPNAME_LEN];
    char members[MAX_GROUP_MEMBERS][MAX_USERNAME_LEN];
    int member_count;
    int active;
} Group;

typedef struct {
    User users[MAX_USERS];
    int user_count;
    Group groups[MAX_GROUPS];
    int group_count;
} ServerState;

ServerState server_state = {0};

int find_user(const char* username) {
    for (int i = 0; i < server_state.user_count; i++)
        if (server_state.users[i].active && 
            strcmp(server_state.users[i].username, username) == 0)
            return i;
    return -1;
}

int find_group(const char* groupname) {
    for (int i = 0; i < server_state.group_count; i++)
        if (server_state.groups[i].active && 
            strcmp(server_state.groups[i].groupname, groupname) == 0)
            return i;
    return -1;
}

int add_user(const char* username, const char* identity, int identity_len) {
    if (server_state.user_count >= MAX_USERS)
        return -1;
    
    if (find_user(username) != -1)
        return -2;
    
    int idx = server_state.user_count;
    strncpy(server_state.users[idx].username, username, MAX_USERNAME_LEN - 1);
    memcpy(server_state.users[idx].identity, identity, identity_len);
    server_state.users[idx].identity_len = identity_len;
    server_state.users[idx].active = 1;
    server_state.user_count++;
    
    printf("[INFO] User '%s' connected (identity_len=%d)\n", username, identity_len);
    return idx;
}

int remove_user(const char* username) {
    int idx = find_user(username);
    if (idx == -1)
        return -1;
    
    server_state.users[idx].active = 0;
    printf("[INFO] User '%s' disconnected\n", username);
    return 0;
}

int create_group(const char* groupname, const char* creator) {
    if (server_state.group_count >= MAX_GROUPS)
        return -1;
    
    if (find_group(groupname) != -1)
        return -2;
    
    int idx = server_state.group_count;
    strncpy(server_state.groups[idx].groupname, groupname, MAX_GROUPNAME_LEN - 1);
    strncpy(server_state.groups[idx].members[0], creator, MAX_USERNAME_LEN - 1);
    server_state.groups[idx].member_count = 1;
    server_state.groups[idx].active = 1;
    server_state.group_count++;
    
    printf("[INFO] Group '%s' created by '%s'\n", groupname, creator);
    return idx;
}

int join_group(const char* groupname, const char* username) {
    int group_idx = find_group(groupname);
    if (group_idx == -1)
        return -1;
    
    Group* group = &server_state.groups[group_idx];
    
    for (int i = 0; i < group->member_count; i++)
        if (strcmp(group->members[i], username) == 0)
            return -2;

    
    if (group->member_count >= MAX_GROUP_MEMBERS)
        return -3;
    
    strncpy(group->members[group->member_count], username, MAX_USERNAME_LEN - 1);
    group->member_count++;
    
    printf("[INFO] User '%s' joined group '%s'\n", username, groupname);
    return 0;
}

int leave_group(const char* groupname, const char* username) {
    int group_idx = find_group(groupname);
    if (group_idx == -1)
        return -1;
    
    Group* group = &server_state.groups[group_idx];
    
    int found = -1;
    for (int i = 0; i < group->member_count; i++)
        if (strcmp(group->members[i], username) == 0) {
            found = i;
            break;
        }
    
    if (found == -1)
        return -2;
    
    for (int i = found; i < group->member_count - 1; i++)
        strcpy(group->members[i], group->members[i + 1]);
    group->member_count--;
    
    printf("[INFO] User '%s' left group '%s'\n", username, groupname);
    return 0;
}

void get_user_groups(const char* username, char* result, int max_len) {
    result[0] = '\0';
    int first = 1;
    
    for (int i = 0; i < server_state.group_count; i++) {
        if (!server_state.groups[i].active) continue;
        
        Group* group = &server_state.groups[i];
        for (int j = 0; j < group->member_count; j++)
            if (strcmp(group->members[j], username) == 0) {
                if (!first) strcat(result, " ");
                strcat(result, group->groupname);
                first = 0;
                break;
            }
    }
    
    if (result[0] == '\0')
        strcpy(result, "(none)");
}

int get_group_members(const char* groupname, char* result, int max_len) {
    int group_idx = find_group(groupname);
    if (group_idx == -1)
        return -1;
    
    Group* group = &server_state.groups[group_idx];
    result[0] = '\0';
    
    for (int i = 0; i < group->member_count; i++) {
        if (i > 0) strcat(result, " ");
        strcat(result, group->members[i]);
    }
    
    return 0;
}

void handle_login(void* router, char* msg, char* identity, int id_len) {
    char username[MAX_USERNAME_LEN];
    char response[MAX_MSG_LEN];
    
    sscanf(msg, "LOGIN %s", username);
    
    int result = add_user(username, identity, id_len);
    
    if (result >= 0)
        snprintf(response, MAX_MSG_LEN, "LOGIN_OK Welcome %s!", username);
    else if (result == -2)
        snprintf(response, MAX_MSG_LEN, "LOGIN_ERROR Username already taken");
    else
        snprintf(response, MAX_MSG_LEN, "LOGIN_ERROR Server full");
    
    zmq_send(router, identity, id_len, ZMQ_SNDMORE);
    zmq_send(router, response, strlen(response), 0);
}

void handle_private_msg(void* router, char* msg, char* sender_identity, int sender_id_len) {
    char sender[MAX_USERNAME_LEN];
    char recipient[MAX_USERNAME_LEN];
    char message[MAX_MSG_LEN];
    char response[MAX_MSG_LEN];
    
    char* token = strtok(msg, " ");
    if (token == NULL) return;
    
    token = strtok(NULL, " ");
    if (token == NULL) return;
    strncpy(sender, token, MAX_USERNAME_LEN - 1);
    
    token = strtok(NULL, " ");
    if (token == NULL) return;
    strncpy(recipient, token, MAX_USERNAME_LEN - 1);
    
    token = strtok(NULL, "");
    if (token == NULL) return;
    strncpy(message, token, MAX_MSG_LEN - 1);
    
    int recipient_idx = find_user(recipient);
    
    if (recipient_idx == -1) {
        snprintf(response, MAX_MSG_LEN, "ERROR User '%s' not found", recipient);
        zmq_send(router, sender_identity, sender_id_len, ZMQ_SNDMORE);
        zmq_send(router, response, strlen(response), 0);
        return;
    }
    
    snprintf(response, MAX_MSG_LEN, "PRIVATE_MSG %s %s", sender, message);
    zmq_send(router, server_state.users[recipient_idx].identity, 
             server_state.users[recipient_idx].identity_len, ZMQ_SNDMORE);
    zmq_send(router, response, strlen(response), 0);
    
    printf("[INFO] Sent private message to %s (identity_len=%d)\n", 
           recipient, server_state.users[recipient_idx].identity_len);
    
    snprintf(response, MAX_MSG_LEN, "MSG_SENT Message sent to %s", recipient);
    zmq_send(router, sender_identity, sender_id_len, ZMQ_SNDMORE);
    zmq_send(router, response, strlen(response), 0);
    
    printf("[INFO] Private message from %s to %s\n", sender, recipient);
}

void handle_create_group(void* router, char* msg, char* identity, int id_len) {
    char username[MAX_USERNAME_LEN];
    char groupname[MAX_GROUPNAME_LEN];
    char response[MAX_MSG_LEN];
    
    sscanf(msg, "CREATE_GROUP %s %s", username, groupname);
    
    int result = create_group(groupname, username);
    
    if (result >= 0)
        snprintf(response, MAX_MSG_LEN, "GROUP_CREATED Group '%s' created successfully", groupname);
    else if (result == -2)
        snprintf(response, MAX_MSG_LEN, "ERROR Group '%s' already exists", groupname);
    else
        snprintf(response, MAX_MSG_LEN, "ERROR Cannot create group");
    
    zmq_send(router, identity, id_len, ZMQ_SNDMORE);
    zmq_send(router, response, strlen(response), 0);
}

void handle_join_group(void* router, char* msg, char* identity, int id_len) {
    char username[MAX_USERNAME_LEN];
    char groupname[MAX_GROUPNAME_LEN];
    char response[MAX_MSG_LEN];
    
    sscanf(msg, "JOIN_GROUP %s %s", username, groupname);
    
    int result = join_group(groupname, username);
    
    if (result == 0)
        snprintf(response, MAX_MSG_LEN, "GROUP_JOINED You joined group '%s'", groupname);
    else if (result == -1)
        snprintf(response, MAX_MSG_LEN, "ERROR Group '%s' not found", groupname);
    else if (result == -2)
        snprintf(response, MAX_MSG_LEN, "ERROR Already in group '%s'", groupname);
    else
        snprintf(response, MAX_MSG_LEN, "ERROR Group is full");
    
    zmq_send(router, identity, id_len, ZMQ_SNDMORE);
    zmq_send(router, response, strlen(response), 0);
}

void handle_group_msg(void* router, char* msg, char* sender_identity, int sender_id_len) {
    char sender[MAX_USERNAME_LEN];
    char groupname[MAX_GROUPNAME_LEN];
    char message[MAX_MSG_LEN];
    char response[MAX_MSG_LEN];
    
    char* token = strtok(msg, " ");
    if (token == NULL) return;
    
    token = strtok(NULL, " ");
    if (token == NULL) return;
    strncpy(sender, token, MAX_USERNAME_LEN - 1);
    
    token = strtok(NULL, " ");
    if (token == NULL) return;
    strncpy(groupname, token, MAX_GROUPNAME_LEN - 1);
    
    token = strtok(NULL, "");
    if (token == NULL) return;
    strncpy(message, token, MAX_MSG_LEN - 1);
    
    int group_idx = find_group(groupname);
    
    if (group_idx == -1) {
        snprintf(response, MAX_MSG_LEN, "ERROR Group '%s' not found", groupname);
        zmq_send(router, sender_identity, sender_id_len, ZMQ_SNDMORE);
        zmq_send(router, response, strlen(response), 0);
        return;
    }
    
    Group* group = &server_state.groups[group_idx];
    
    int is_member = 0;
    for (int i = 0; i < group->member_count; i++)
        if (strcmp(group->members[i], sender) == 0) {
            is_member = 1;
            break;
        }
    
    if (!is_member) {
        snprintf(response, MAX_MSG_LEN, "ERROR You are not a member of group '%s'", groupname);
        zmq_send(router, sender_identity, sender_id_len, ZMQ_SNDMORE);
        zmq_send(router, response, strlen(response), 0);
        return;
    }
    
    int sent_count = 0;
    for (int i = 0; i < group->member_count; i++) {
        if (strcmp(group->members[i], sender) == 0) 
            continue;
        
        int user_idx = find_user(group->members[i]);
        if (user_idx != -1) {
            snprintf(response, MAX_MSG_LEN, "GROUP_MSG %s %s %s", groupname, sender, message);
            zmq_send(router, server_state.users[user_idx].identity, 
                     server_state.users[user_idx].identity_len, ZMQ_SNDMORE);
            zmq_send(router, response, strlen(response), 0);
            sent_count++;
            printf("[INFO] Sent group message to %s (identity_len=%d)\n", 
                   group->members[i], server_state.users[user_idx].identity_len);
        }
    }
    
    snprintf(response, MAX_MSG_LEN, "MSG_SENT Message sent to %d members of %s", sent_count, groupname);
    zmq_send(router, sender_identity, sender_id_len, ZMQ_SNDMORE);
    zmq_send(router, response, strlen(response), 0);
    
    printf("[INFO] Group message from %s to %s\n", sender, groupname);
}

void handle_list_users(void* router, char* identity, int id_len) {
    char response[MAX_MSG_LEN] = "USERS";
    
    for (int i = 0; i < server_state.user_count; i++)
        if (server_state.users[i].active) {
            strcat(response, " ");
            strcat(response, server_state.users[i].username);
        }
    
    zmq_send(router, identity, id_len, ZMQ_SNDMORE);
    zmq_send(router, response, strlen(response), 0);
}

void handle_list_groups(void* router, char* identity, int id_len) {
    char response[MAX_MSG_LEN] = "GROUPS";
    
    for (int i = 0; i < server_state.group_count; i++)
        if (server_state.groups[i].active) {
            strcat(response, " ");
            strcat(response, server_state.groups[i].groupname);
        }
    
    zmq_send(router, identity, id_len, ZMQ_SNDMORE);
    zmq_send(router, response, strlen(response), 0);
}

int main(void) {
    printf("=== ZeroMQ Message Server ===\n");
    printf("Starting server on tcp://*:5555\n\n");
    
    void* context = zmq_ctx_new();
    void* router = zmq_socket(context, ZMQ_ROUTER);
    
    int rc = zmq_bind(router, "tcp://*:5555");
    if (rc != 0) {
        printf("Error binding socket: %s\n", zmq_strerror(errno));
        return 1;
    }
    
    printf("Server is running. Waiting for clients...\n\n");
    
    while (1) {
        char identity[MAX_IDENTITY_LEN];
        char message[MAX_MSG_LEN];
        
        int id_len = zmq_recv(router, identity, MAX_IDENTITY_LEN, 0);
        if (id_len == -1)
            continue;
        
        int msg_len = zmq_recv(router, message, MAX_MSG_LEN, 0);
        if (msg_len == -1)
            continue;
        
        message[msg_len] = '\0';
        
        if (strncmp(message, "LOGIN ", 6) == 0)
            handle_login(router, message, identity, id_len);
        else if (strncmp(message, "PRIVATE_MSG ", 12) == 0)
            handle_private_msg(router, message, identity, id_len);
        else if (strncmp(message, "CREATE_GROUP ", 13) == 0)
            handle_create_group(router, message, identity, id_len);
        else if (strncmp(message, "JOIN_GROUP ", 11) == 0)
            handle_join_group(router, message, identity, id_len);
        else if (strncmp(message, "GROUP_MSG ", 10) == 0)
            handle_group_msg(router, message, identity, id_len);
        else if (strcmp(message, "LIST_USERS") == 0)
            handle_list_users(router, identity, id_len);
        else if (strcmp(message, "LIST_GROUPS") == 0)
            handle_list_groups(router, identity, id_len);
        else if (strncmp(message, "LEAVE_GROUP ", 12) == 0) {
            char username[MAX_USERNAME_LEN];
            char groupname[MAX_GROUPNAME_LEN];
            char response[MAX_MSG_LEN];
            
            sscanf(message, "LEAVE_GROUP %s %s", username, groupname);
            
            int result = leave_group(groupname, username);
            
            if (result == 0)
                snprintf(response, MAX_MSG_LEN, "GROUP_LEFT You left group '%s'", groupname);
            else if (result == -1)
                snprintf(response, MAX_MSG_LEN, "ERROR Group '%s' not found", groupname);
            else
                snprintf(response, MAX_MSG_LEN, "ERROR You are not in group '%s'", groupname);
            
            zmq_send(router, identity, id_len, ZMQ_SNDMORE);
            zmq_send(router, response, strlen(response), 0);
        }
        else if (strncmp(message, "MY_GROUPS ", 10) == 0) {
            char username[MAX_USERNAME_LEN];
            char groups[MAX_MSG_LEN];
            char response[MAX_MSG_LEN];
            
            sscanf(message, "MY_GROUPS %s", username);
            get_user_groups(username, groups, MAX_MSG_LEN);
            
            snprintf(response, MAX_MSG_LEN, "MY_GROUPS %s", groups);
            zmq_send(router, identity, id_len, ZMQ_SNDMORE);
            zmq_send(router, response, strlen(response), 0);
        }
        else if (strncmp(message, "GROUP_MEMBERS ", 14) == 0) {
            char groupname[MAX_GROUPNAME_LEN];
            char members[MAX_MSG_LEN];
            char response[MAX_MSG_LEN];
            
            sscanf(message, "GROUP_MEMBERS %s", groupname);
            
            int result = get_group_members(groupname, members, MAX_MSG_LEN);
            
            if (result == 0)
                snprintf(response, MAX_MSG_LEN, "MEMBERS %s: %s", groupname, members);
            else
                snprintf(response, MAX_MSG_LEN, "ERROR Group '%s' not found", groupname);
            
            zmq_send(router, identity, id_len, ZMQ_SNDMORE);
            zmq_send(router, response, strlen(response), 0);
        }
        else if (strncmp(message, "LOGOUT ", 7) == 0) {
            char username[MAX_USERNAME_LEN];
            sscanf(message, "LOGOUT %s", username);
            
            remove_user(username);
            
            char response[MAX_MSG_LEN];
            snprintf(response, MAX_MSG_LEN, "LOGOUT_OK Goodbye %s!", username);
            zmq_send(router, identity, id_len, ZMQ_SNDMORE);
            zmq_send(router, response, strlen(response), 0);
        }
        else {
            char response[MAX_MSG_LEN];
            snprintf(response, MAX_MSG_LEN, "ERROR Unknown command");
            zmq_send(router, identity, id_len, ZMQ_SNDMORE);
            zmq_send(router, response, strlen(response), 0);
        }
    }
    
    zmq_close(router);
    zmq_ctx_destroy(context);
    
    return 0;
}