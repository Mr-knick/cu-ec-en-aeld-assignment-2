#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <syslog.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>

#define PORT 9000
#define DATA_FILE "/var/tmp/aesdsocketdata"

// Global state
static int running = 1;
pthread_mutex_t data_mutex;
FILE *data_file;
int server_fd = -1;

// Manual Linked List for thread management
struct thread_node {
    pthread_t thread_id;
    int client_fd;
    int completed;
    struct thread_node *next;
};

struct thread_node *head = NULL;

// Signal handler
void handle_signal(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        syslog(LOG_INFO, "Caught signal, exiting");
        running = 0;
        // Shutdown server socket to break the accept() block
        if (server_fd != -1) {
            shutdown(server_fd, SHUT_RDWR);
        }
    }
}

void *client_thread(void *arg) {
    struct thread_node *node = (struct thread_node *)arg;
    int client_fd = node->client_fd;

    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    getpeername(client_fd, (struct sockaddr *)&address, &addrlen);
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(address.sin_addr), client_ip, INET_ADDRSTRLEN);

    char buffer[1024];
    ssize_t bytes_received;

    while ((bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0)) > 0) {
        pthread_mutex_lock(&data_mutex);
        
        fwrite(buffer, 1, bytes_received, data_file);
        fflush(data_file);

        // Check if packet is complete '\n'
        if (memchr(buffer, '\n', bytes_received) != NULL) {
            fseek(data_file, 0, SEEK_SET);
            while (fgets(buffer, sizeof(buffer), data_file) != NULL) {
                send(client_fd, buffer, strlen(buffer), 0);
            }
            pthread_mutex_unlock(&data_mutex);
            break; // Exit receive loop after sending file back
        }
        pthread_mutex_unlock(&data_mutex);
    }

    close(client_fd);
    node->completed = 1; // Mark for the main loop to join/free
    return NULL;
}

void *append_timestamp(void *arg) {
    while (running) {
        sleep(10);
        if (!running) break;

        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char timestamp[128];
        strftime(timestamp, sizeof(timestamp), "timestamp:%a, %d %b %Y %H:%M:%S %z\n", tm_info);

        pthread_mutex_lock(&data_mutex);
        if (data_file) {
            fputs(timestamp, data_file);
            fflush(data_file);
        }
        pthread_mutex_unlock(&data_mutex);
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    int daemonize = 0;
    if (argc > 1 && strcmp(argv[1], "-d") == 0) {
        daemonize = 1;
    }

    openlog("aesdsocket", LOG_PID, LOG_USER);

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) return -1;

    int reuse = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(server_fd);
        return -1;
    }

    if (daemonize) {
        pid_t pid = fork();
        if (pid < 0) return -1;
        if (pid > 0) exit(0);
        setsid();
        chdir("/");
        int fd = open("/dev/null", O_RDWR);
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        close(fd);
    }

    if (listen(server_fd, 10) < 0) return -1;

    data_file = fopen(DATA_FILE, "w+");
    pthread_mutex_init(&data_mutex, NULL);

    pthread_t time_thread;
    pthread_create(&time_thread, NULL, append_timestamp, NULL);

    while (running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);

        if (client_fd == -1) {
            if (running) perror("accept");
            continue;
        }

        struct thread_node *new_node = malloc(sizeof(struct thread_node));
        new_node->client_fd = client_fd;
        new_node->completed = 0;
        new_node->next = head;
        head = new_node;

        pthread_create(&new_node->thread_id, NULL, client_thread, new_node);

        // Garbage collection: Join and free completed threads
        struct thread_node *curr = head;
        struct thread_node *prev = NULL;
        while (curr != NULL) {
            if (curr->completed) {
                pthread_join(curr->thread_id, NULL);
                if (prev == NULL) head = curr->next;
                else prev->next = curr->next;
                
                struct thread_node *temp = curr;
                curr = curr->next;
                free(temp);
            } else {
                prev = curr;
                curr = curr->next;
            }
        }
    }

    // Cleanup before exit
    pthread_join(time_thread, NULL);
    while (head != NULL) {
        struct thread_node *temp = head;
        pthread_join(temp->thread_id, NULL);
        head = head->next;
        free(temp);
    }

    pthread_mutex_destroy(&data_mutex);
    if (data_file) fclose(data_file);
    unlink(DATA_FILE);
    close(server_fd);
    closelog();

    return 0;
}