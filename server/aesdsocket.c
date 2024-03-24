#include "aesdsocket.h"

bool is_active = true;
int sockfd = -1;
pthread_mutex_t file_mutex;

static void remove_data_file(void);
static void *get_in_addr(struct sockaddr *sa);
static void signal_handler(int sig);

void connection_handler(void *current_thread_data){
    int res;
    char recv_buffer[RECV_BUFFER_LEN] = {0};
    client_thread_data_t *thread_data = NULL;

    thread_data = (client_thread_data_t *) current_thread_data;

    if(thread_data == NULL){
        syslog(LOG_ERR, "Thread data is NULL");
        return;
    }

    res = pthread_mutex_lock(&file_mutex);
    if(res != 0){
        syslog(LOG_ERR, "pthread_mutex_lock error: %d", res);
        goto close_client;
    }

    FILE *data_file = fopen(DATA_FILE_NAME, "a+");

    if(data_file == NULL){
        syslog(LOG_ERR, "Error opening data file: %s", strerror(errno));
        goto close_client;
    }

    while((res = recv(thread_data->client_fd, recv_buffer, sizeof(recv_buffer), 0)) > 0){
        syslog(LOG_DEBUG, "Received %d bytes", res);

        fwrite(recv_buffer, sizeof(*recv_buffer), res, data_file);

        if(memchr(recv_buffer, '\n', res) != NULL){
            syslog(LOG_DEBUG, "Newline detected. Packet fully received");
            break;
        }

        memset(recv_buffer, 0, sizeof(recv_buffer));
    }

    if(res == 0){
        syslog(LOG_INFO, "Connection closed by client");
        goto close_client_file;
    } else if(res == -1){
        syslog(LOG_ERR, "recv error: %s", strerror(errno));
        goto close_client_file;
    }

    if(fseek(data_file, 0, SEEK_SET) == -1){
        syslog(LOG_ERR, "Error seeking data file: %s", strerror(errno));
        goto close_client_file;
    }

    while((res = fread(recv_buffer, sizeof(*recv_buffer), sizeof(recv_buffer), data_file)) > 0){
        syslog(LOG_DEBUG, "Sending %d bytes", res);
        res = send(thread_data->client_fd, recv_buffer, res, 0);
        if(res == -1){
            syslog(LOG_ERR, "send error: %s", strerror(errno));
            goto close_client_file;
        }
        memset(recv_buffer, 0, sizeof(recv_buffer));
    }

close_client_file:
    fclose(data_file);
close_client:
    pthread_mutex_unlock(&file_mutex);
    close(thread_data->client_fd);
    thread_data->client_fd = -1;

    syslog(LOG_INFO, "Closed connection from %s", thread_data->addr_str);
}

int main(int argc, char **argv){
    struct addrinfo hints, *addr_res;
    struct sigaction sa = {0};
    int client_fd = -1;
    int res;
    int return_val = 0;

    bool start_in_daemon = false;

    openlog(argv[0], LOG_PID, LOG_USER);

    if(argc == 2 && strcmp(argv[1], DAEMON_KEY) == 0){
        syslog(LOG_DEBUG, "daemon flag provided, server will start in daemon mode");
        start_in_daemon = true;
    }

    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    
    res = getaddrinfo(NULL, PORT, &hints, &addr_res);
    if(res != 0){
        syslog(LOG_ERR, "getaddrinfo error: %s", gai_strerror(res));
        return -1;
    }

    sockfd = socket(addr_res->ai_family, addr_res->ai_socktype, addr_res->ai_protocol);
    if(sockfd == -1){
        syslog(LOG_ERR, "socket error: %s", strerror(errno));
        return_val = -1;
        goto exit;
    }

    res = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
    if(res == -1){
        syslog(LOG_ERR, "setsockopt error: %s", strerror(errno));
        return_val = -1;
        goto exit;
    }

    res = bind(sockfd, addr_res->ai_addr, addr_res->ai_addrlen);
    if(res == -1){
        syslog(LOG_ERR, "bind error: %s", strerror(errno));
        return_val = -1;
        goto exit;
    }

    if (start_in_daemon){
        res = daemon(0, 0);
        if(res == -1){
            syslog(LOG_ERR, "daemon creation error: %s", strerror(errno));
            return_val = -1;
            goto exit;
        }
    }

    res = listen(sockfd, 20);
    if(res == -1){
        syslog(LOG_ERR, "listen error: %s", strerror(errno));
        return_val = -1;
        goto exit;
    }

    res = pthread_mutex_init(&file_mutex, NULL);
    if(res != 0){
        syslog(LOG_ERR, "pthread_mutex_init error: %d", res);
        return_val = -1;
        goto exit;
    }

    while(is_active){
        struct sockaddr_storage client_addr;
        client_thread_data_t *thread_data = NULL;

        client_fd = accept(sockfd, (struct sockaddr *)&client_addr, &(socklen_t){sizeof(client_addr)});
        if (client_fd == -1){
            syslog(LOG_ERR, "accept error: %s", strerror(errno));
            continue;
        }

        thread_data = (client_thread_data_t *) malloc(sizeof(*thread_data));

        if(thread_data == NULL){
            syslog(LOG_ERR, "Error allocating memory for thread data: %s", strerror(errno));
            close(client_fd);
            continue;
        }

        thread_data->client_fd = client_fd;

        inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr *)&client_addr), thread_data->addr_str, sizeof(thread_data->addr_str));

        syslog(LOG_INFO, "Accepted connection from %s", thread_data->addr_str);

        syslog(LOG_INFO, "Spawning new thread to handle connection from %s", thread_data->addr_str);
    }

exit:
    pthread_mutex_destroy(&file_mutex);
    freeaddrinfo(addr_res);
    close(sockfd);
    sockfd = -1;
    remove_data_file();
    closelog();
    return return_val;
}

static void remove_data_file(void){
    if(remove(DATA_FILE_NAME) == 0){
        syslog(LOG_INFO, "Removed data file");
    } else {
        syslog(LOG_ERR, "Error removing data file: %s", strerror(errno));
    }
}

static void *get_in_addr(struct sockaddr *sa){
    if(sa->sa_family == AF_INET){
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

static void signal_handler(int sig){
    if(sig == SIGINT || sig == SIGTERM){
        syslog(LOG_INFO, "Received SIGINT/SIGTERM (%d). Shutting down...", sig);
        is_active = false;
        if(sockfd != -1){
            shutdown(sockfd, SHUT_RDWR);
        }
        if(client_fd != -1){
            shutdown(client_fd, SHUT_RDWR);
        }
    }
}