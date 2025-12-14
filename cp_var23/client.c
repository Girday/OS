#include <zmq.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#define MAX_MSG_LEN 4096
#define MAX_USERNAME_LEN 50

char username[MAX_USERNAME_LEN];
void* socket_global = NULL;
int running = 1;
int logged_in = 0;

void* receive_thread(void* arg) {
    void* socket = (void*)arg;
    char buffer[MAX_MSG_LEN];
    
    while (running) {
        int size = zmq_recv(socket, buffer, MAX_MSG_LEN - 1, ZMQ_DONTWAIT);
        
        if (size > 0) {
            buffer[size] = '\0';
            
            if (strncmp(buffer, "PRIVATE_MSG ", 12) == 0) {
                char sender[MAX_USERNAME_LEN];
                char msg[MAX_MSG_LEN];
                
                sscanf(buffer, "PRIVATE_MSG %s %[^\n]", sender, msg);
                printf("\n[PRIVATE from %s]: %s\n> ", sender, msg);
                fflush(stdout);
            }
            else if (strncmp(buffer, "GROUP_MSG ", 10) == 0) {
                char groupname[MAX_USERNAME_LEN];
                char sender[MAX_USERNAME_LEN];
                char msg[MAX_MSG_LEN];
                
                sscanf(buffer, "GROUP_MSG %s %s %[^\n]", groupname, sender, msg);
                printf("\n[GROUP %s - %s]: %s\n> ", groupname, sender, msg);
                fflush(stdout);
            }
            else if (strncmp(buffer, "LOGIN_OK", 8) == 0) {
                logged_in = 1;
                printf("\n✓ %s\n", buffer + 9);
                fflush(stdout);
            }
            else if (strncmp(buffer, "LOGIN_ERROR", 11) == 0) {
                printf("\n✗ Login failed: %s\n", buffer + 12);
                fflush(stdout);
            }
            else if (strncmp(buffer, "ERROR", 5) == 0) {
                printf("\n[ERROR]: %s\n> ", buffer + 6);
                fflush(stdout);
            }
            else if (strncmp(buffer, "MSG_SENT", 8) == 0) {
                printf("\n[✓]: %s\n> ", buffer + 9);
                fflush(stdout);
            }
            else if (strncmp(buffer, "GROUP_CREATED", 13) == 0) {
                printf("\n[✓]: %s\n> ", buffer + 14);
                fflush(stdout);
            }
            else if (strncmp(buffer, "GROUP_JOINED", 12) == 0) {
                printf("\n[✓]: %s\n> ", buffer + 13);
                fflush(stdout);
            }
            else if (strncmp(buffer, "USERS ", 6) == 0) {
                printf("\n=== Online Users ===\n%s\n> ", buffer + 6);
                fflush(stdout);
            }
            else if (strncmp(buffer, "GROUPS ", 7) == 0) {
                printf("\n=== Available Groups ===\n%s\n> ", buffer + 7);
                fflush(stdout);
            }
            else {
                printf("\n[SERVER]: %s\n> ", buffer);
                fflush(stdout);
            }
        }
        
        usleep(100000);
    }
    
    return NULL;
}

void print_help() {
    printf("\n=== Available Commands ===\n");
    printf("/msg <username> <message>  - Send private message\n");
    printf("/create <groupname>        - Create a group chat\n");
    printf("/join <groupname>          - Join a group chat\n");
    printf("/group <groupname> <msg>   - Send message to group\n");
    printf("/users                     - List online users\n");
    printf("/groups                    - List available groups\n");
    printf("/help                      - Show this help\n");
    printf("/quit                      - Exit the program\n");
    printf("==========================\n\n");
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <server_address>\n", argv[0]);
        printf("Example: %s tcp://localhost:5555\n", argv[0]);
        return 1;
    }
    
    printf("=== ZeroMQ Message Client ===\n");
    printf("Connecting to %s\n\n", argv[1]);
    
    void* context = zmq_ctx_new();
    void* dealer = zmq_socket(context, ZMQ_DEALER);
    
    int timeout = 5000;
    zmq_setsockopt(dealer, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
    zmq_setsockopt(dealer, ZMQ_SNDTIMEO, &timeout, sizeof(timeout));
    
    int rc = zmq_connect(dealer, argv[1]);
    if (rc != 0) {
        printf("Error connecting to server: %s\n", zmq_strerror(errno));
        return 1;
    }
    
    socket_global = dealer;
    
    printf("Establishing connection...\n");
    sleep(1);
    
    pthread_t recv_thread;
    pthread_create(&recv_thread, NULL, receive_thread, dealer);
    
    printf("Enter your username: ");
    fgets(username, MAX_USERNAME_LEN, stdin);
    username[strcspn(username, "\n")] = 0;
    
    char login_msg[MAX_MSG_LEN];
    snprintf(login_msg, MAX_MSG_LEN, "LOGIN %s", username);
    
    printf("Sending login request...\n");
    int sent = zmq_send(dealer, login_msg, strlen(login_msg), 0);
    if (sent == -1) {
        printf("Failed to send login: %s\n", zmq_strerror(errno));
        running = 0;
        pthread_join(recv_thread, NULL);
        zmq_close(dealer);
        zmq_ctx_destroy(context);
        return 1;
    }
    printf("Login request sent (%d bytes)\n", sent);
    
    int wait_count = 0;
    while (!logged_in && wait_count < 30 && running) {
        sleep(1);
        wait_count++;
    }
    
    if (!logged_in) {
        printf("Login timeout. Server not responding.\n");
        running = 0;
        pthread_join(recv_thread, NULL);
        zmq_close(dealer);
        zmq_ctx_destroy(context);
        return 1;
    }
    
    print_help();
    
    char input[MAX_MSG_LEN];
    printf("> ");
    fflush(stdout);
    
    while (running && fgets(input, MAX_MSG_LEN, stdin) != NULL) {
        input[strcspn(input, "\n")] = 0;
        
        if (strlen(input) == 0) {
            printf("> ");
            fflush(stdout);
            continue;
        }
        
        if (strcmp(input, "/quit") == 0) {
            char logout_msg[MAX_MSG_LEN];
            snprintf(logout_msg, MAX_MSG_LEN, "LOGOUT %s", username);
            
            printf("Logging out...\n");
            int sent = zmq_send(dealer, logout_msg, strlen(logout_msg), ZMQ_DONTWAIT);
            
            if (sent == -1)
                printf("Server is unreachable. Closing connection.\n");
            else
                usleep(200000);
            
            printf("Goodbye!\n");
            running = 0;
            break;
        }
        else if (strcmp(input, "/help") == 0)
            print_help();
        else if (strcmp(input, "/users") == 0)
            zmq_send(dealer, "LIST_USERS", 10, 0);
        else if (strcmp(input, "/groups") == 0)
            zmq_send(dealer, "LIST_GROUPS", 11, 0);
        else if (strncmp(input, "/msg ", 5) == 0) {
            char recipient[MAX_USERNAME_LEN];
            char message[MAX_MSG_LEN];
            
            if (sscanf(input, "/msg %s %[^\n]", recipient, message) == 2) {
                char cmd[MAX_MSG_LEN];
                snprintf(cmd, MAX_MSG_LEN, "PRIVATE_MSG %s %s %s", username, recipient, message);
                zmq_send(dealer, cmd, strlen(cmd), 0);
            } else
                printf("Usage: /msg <username> <message>\n");
        }
        else if (strncmp(input, "/create ", 8) == 0) {
            char groupname[MAX_USERNAME_LEN];
            
            if (sscanf(input, "/create %s", groupname) == 1) {
                char cmd[MAX_MSG_LEN];
                snprintf(cmd, MAX_MSG_LEN, "CREATE_GROUP %s %s", username, groupname);
                zmq_send(dealer, cmd, strlen(cmd), 0);
            } else
                printf("Usage: /create <groupname>\n");
        }
        else if (strncmp(input, "/join ", 6) == 0) {
            char groupname[MAX_USERNAME_LEN];
            
            if (sscanf(input, "/join %s", groupname) == 1) {
                char cmd[MAX_MSG_LEN];
                snprintf(cmd, MAX_MSG_LEN, "JOIN_GROUP %s %s", username, groupname);
                zmq_send(dealer, cmd, strlen(cmd), 0);
            } else
                printf("Usage: /join <groupname>\n");
        }
        else if (strncmp(input, "/group ", 7) == 0) {
            char groupname[MAX_USERNAME_LEN];
            char message[MAX_MSG_LEN];
            
            if (sscanf(input, "/group %s %[^\n]", groupname, message) == 2) {
                char cmd[MAX_MSG_LEN];
                snprintf(cmd, MAX_MSG_LEN, "GROUP_MSG %s %s %s", username, groupname, message);
                zmq_send(dealer, cmd, strlen(cmd), 0);
            } else
                printf("Usage: /group <groupname> <message>\n");
        }
        else
            printf("Unknown command. Type /help for available commands.\n");
        
        printf("> ");
        fflush(stdout);
    }
    
    running = 0;
    pthread_join(recv_thread, NULL);
    
    zmq_close(dealer);
    zmq_ctx_destroy(context);
    
    return 0;
}
