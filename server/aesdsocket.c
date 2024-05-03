#include "aesdsocket.h"

bool is_active = true;
int sockfd = -1;

static void remove_data_file(void);
static void *get_in_addr(struct sockaddr *sa);
static void signal_handler(int sig);
#if USE_AESD_CHAR_DEVICE == 0
static void timer_expired_handler(union sigval sv);
#endif

void* connection_handler(void *current_thread_data){
    int res;
    char recv_buffer[RECV_BUFFER_LEN] = {0};
    client_thread_data_t *thread_data = NULL;

    thread_data = (client_thread_data_t *) current_thread_data;

    if(thread_data == NULL){
        syslog(LOG_ERR, "Thread data is NULL");
        return current_thread_data;
    }

    res = pthread_mutex_lock(thread_data->mutex);
    if(res != 0){
        syslog(LOG_ERR, "pthread_mutex_lock error: %d", res);
        goto close_client;
    }

    syslog(LOG_DEBUG, "Opening data file");

    FILE *data_file = fopen(DATA_FILE_NAME, "a+");

    if(data_file == NULL){
        syslog(LOG_ERR, "Error opening data file: %s", strerror(errno));
        goto close_client;
    }

    syslog(LOG_DEBUG, "Data file opened");

    while((res = recv(thread_data->client_fd, recv_buffer, sizeof(recv_buffer), 0)) > 0){
        syslog(LOG_DEBUG, "Received %d bytes", res);
#if USE_AESD_CHAR_DEVICE == 1
        if(res >= (int)AESD_SEEKTO_KEYWORD_LEN && strncmp(recv_buffer, AESD_SEEKTO_KEYWORD, AESD_SEEKTO_KEYWORD_LEN) == 0){
#else
        if(false){
#endif
            syslog(LOG_INFO, "Received seekto keyword");
            char *save_ptr;
            char delimiter[] = ":,";

            char *token = strtok_r(recv_buffer, delimiter, &save_ptr);
            syslog(LOG_DEBUG, "First token: %s", token);

            token = strtok_r(NULL, delimiter, &save_ptr);
            syslog(LOG_DEBUG, "Second token: %s", token);

            int cmd_offset = atoi(token);

            token = strtok_r(NULL, delimiter, &save_ptr);
            syslog(LOG_DEBUG, "Third token: %s", token);

            int cmd_char_offset = atoi(token);

            struct aesd_seekto seekto = {
                .write_cmd = cmd_offset,
                .write_cmd_offset = cmd_char_offset
            };

            int fd = fileno(data_file);

            unsigned long request = AESDCHAR_IOCSEEKTO;

            syslog(LOG_DEBUG, "Sending ioctl request: %lu", request);

            res = ioctl(fd, request, &seekto);
            if(res < 0){
                syslog(LOG_ERR, "ioctl error: %s", strerror(errno));
            }

            break;
        }
        else{
            fwrite(recv_buffer, sizeof(*recv_buffer), res, data_file);
        }

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

#if USE_AESD_CHAR_DEVICE == 0
    if(fseek(data_file, 0, SEEK_SET) == -1){
        syslog(LOG_ERR, "Error seeking data file: %s", strerror(errno));
        goto close_client_file;
    }
#endif

    while((res = fread(recv_buffer, sizeof(*recv_buffer), sizeof(recv_buffer), data_file)) > 0){
        syslog(LOG_DEBUG, "Sending %d bytes", res);
        syslog(LOG_DEBUG, "Data: %.*s", res, recv_buffer);
        res = send(thread_data->client_fd, recv_buffer, res, 0);
        if(res == -1){
            syslog(LOG_ERR, "send error: %s", strerror(errno));
            goto close_client_file;
        }
        memset(recv_buffer, 0, sizeof(recv_buffer));
    }

    syslog(LOG_INFO, "Data sent to client");

close_client_file:
    fclose(data_file);
close_client:
    pthread_mutex_unlock(thread_data->mutex);
    close(thread_data->client_fd);
    thread_data->client_fd = -1;

    syslog(LOG_INFO, "Closed connection from %s", thread_data->addr_str);

    thread_data->thread_completed = true;

    return current_thread_data;
}

int main(int argc, char **argv){
    struct addrinfo hints, *addr_res;
#if USE_AESD_CHAR_DEVICE == 0
    struct itimerspec timer;
    struct sigevent sev;
    timer_data_t timer_data;
    timer_t timer_id;
#endif
    pthread_mutex_t file_mutex;
    struct sigaction sa = {0};
    int client_fd = -1;
    int res;
    int return_val = 0;
    bool start_in_daemon = false;

    openlog(argv[0], LOG_PID, LOG_USER);

#if USE_AESD_CHAR_DEVICE == 1
    syslog(LOG_INFO, "compiled to work with char device");
#else
    syslog(LOG_INFO, "compiled to work with temp file");
#endif

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

#if USE_AESD_CHAR_DEVICE == 0
    memset(&timer, 0, sizeof(timer));
    timer.it_value.tv_sec = 10;
    timer.it_interval.tv_sec = 10;

    timer_data.mutex = &file_mutex;

    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = &timer_expired_handler;
    sev.sigev_value.sival_ptr = &timer_data;
    sev.sigev_notify_attributes = NULL;

    res = timer_create(CLOCK_MONOTONIC, &sev, &timer_id);
    if(res != 0){
        syslog(LOG_ERR, "timer_create error: %s", strerror(errno));
        goto exit;
    }

    syslog(LOG_DEBUG, "Timer created");

    res = timer_settime(timer_id, 0, &timer, NULL);
    if(res != 0){
        syslog(LOG_ERR, "timer_settime error: %s", strerror(errno));
        goto exit;
    }
#endif

    while(is_active){
        struct sockaddr_storage client_addr;
        client_thread_data_t *thread_data = NULL;
        thread_instance_t *thread_instance = NULL;

        client_fd = accept(sockfd, (struct sockaddr *)&client_addr, &(socklen_t){sizeof(client_addr)});
        if (client_fd == -1){
            syslog(LOG_ERR, "accept error: %s", strerror(errno));
            continue;
        }

        thread_data = (client_thread_data_t *) malloc(sizeof(*thread_data));

        if(thread_data == NULL){
            syslog(LOG_ERR, "Error allocating memory for thread data: %s", strerror(errno));
            goto listener_close_client;
            continue;
        }

        thread_data->client_fd = client_fd;
        thread_data->mutex = &file_mutex;
        thread_data->thread_completed = false;

        inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr *)&client_addr), thread_data->addr_str, sizeof(thread_data->addr_str));

        syslog(LOG_INFO, "Accepted connection from %s", thread_data->addr_str);

        syslog(LOG_DEBUG, "Spawning new thread to handle connection from %s", thread_data->addr_str);

        thread_instance = (thread_instance_t *) malloc(sizeof(*thread_instance));
        memset(thread_instance, 0, sizeof(*thread_instance));

        if(thread_instance == NULL){
            syslog(LOG_ERR, "Error allocating memory for thread instance: %s", strerror(errno));
            goto listener_free_thread_data;
            continue;
        }

        thread_instance->thread_data = thread_data;

        res = pthread_create(&(thread_instance->thread), NULL, connection_handler, (void *)thread_data);
        if(res != 0){
            syslog(LOG_ERR, "pthread_create error: %d", res);
            goto listener_free_thread_instance;
        }

        syslog(LOG_INFO, "Thread spawned successfully");

        res = queue_enqueue(thread_instance);
        if(!res){
            syslog(LOG_ERR, "Error adding thread instance to queue");
            goto listener_kill_thread;
        }

        syslog(LOG_DEBUG, "Thread instance added to queue");

        node_t *node = queue_get_head();
        do{
            syslog(LOG_DEBUG, "Checking thread");
            if(node == NULL){
                syslog(LOG_ERR, "Node is NULL");
                break;
            }

            if(node->data == NULL){
                syslog(LOG_ERR, "Node data is NULL");
                break;
            }

            thread_instance_t *thread_instance = (thread_instance_t *)node->data;
            node = node->next;
            
            if(thread_instance == NULL){
                syslog(LOG_ERR, "Thread instance is NULL");
                continue;
            }
            if(thread_instance->thread_data == NULL){
                syslog(LOG_ERR, "Thread data is NULL");
                continue;
            }

            syslog(LOG_DEBUG, "Checking thread completion");

            if(thread_instance->thread_data->thread_completed){
                syslog(LOG_DEBUG, "Thread completed. Joining and freeing resources");
                if(thread_instance->thread_data->client_fd != -1){
                    close(thread_instance->thread_data->client_fd);
                }
                free(thread_instance->thread_data);
                pthread_join(thread_instance->thread, NULL);
                queue_remove(thread_instance);
                free(thread_instance);
                syslog(LOG_DEBUG, "Thread resources freed");
            }
        }while(node != NULL);

        syslog(LOG_DEBUG, "Thread queue checked");

        continue;

    listener_kill_thread:
        pthread_cancel(thread_instance->thread);
    listener_free_thread_instance:
        free(thread_instance);
    listener_free_thread_data:
        free(thread_data);
    listener_close_client:
        close(client_fd);
    }

    syslog(LOG_DEBUG, "Cleaning allocated resources and threads");

    thread_instance_t *thread_instance;
    while ((thread_instance = (thread_instance_t *) queue_dequeue()) != NULL)
    {
        if(thread_instance != NULL){
            if(thread_instance->thread_data != NULL){
                if(thread_instance->thread_data->client_fd != -1){
                    close(thread_instance->thread_data->client_fd);
                }
                free(thread_instance->thread_data);
            }
            pthread_join(thread_instance->thread, NULL);
            free(thread_instance);
        }
    }

    syslog(LOG_DEBUG, "All threads cleaned");

exit:
#if USE_AESD_CHAR_DEVICE == 0
    if(timer_delete(timer_id) != 0){
        syslog(LOG_ERR, "timer_delete error: %s", strerror(errno));
    }
    if(pthread_mutex_destroy(&file_mutex) != 0){
        syslog(LOG_ERR, "pthread_mutex_destroy error: %s", strerror(errno));
    }
#endif
    freeaddrinfo(addr_res);
    close(sockfd);
    sockfd = -1;
    remove_data_file();
    closelog();
    return return_val;
}

static void remove_data_file(void){
#if USE_AESD_CHAR_DEVICE == 0
    if(remove(DATA_FILE_NAME) == 0){
        syslog(LOG_INFO, "Removed data file");
    } else {
        syslog(LOG_ERR, "Error removing data file: %s", strerror(errno));
    }
#else
    syslog(LOG_INFO, "using  aesd char device, not removing data file");
#endif
}

static void *get_in_addr(struct sockaddr *sa){
    if(sa->sa_family == AF_INET){
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

static void signal_handler(int sig){
    int old_errno = errno;
    if(sig == SIGINT || sig == SIGTERM){
        syslog(LOG_INFO, "Received SIGINT/SIGTERM (%d). Shutting down...", sig);
        is_active = false;
        if(sockfd != -1){
            shutdown(sockfd, SHUT_RDWR);
        }
    }
    else if(sig == SIGALRM){
        syslog(LOG_INFO, "Received SIGALRM (%d)", sig);
    }

    errno = old_errno;
}

#if USE_AESD_CHAR_DEVICE == 0
static void timer_expired_handler(union sigval sv){
    syslog(LOG_INFO, "Timer event invoked");

    time_t curr_time;
    struct tm *time_info;
    struct tm *time_info_saved;
    timer_data_t *timer_data = NULL;
    int res;
    int str_size;
    char time_str[1024] = {0};

    curr_time = time(NULL);
    if(curr_time == -1){
        syslog(LOG_ERR, "time error: %s", strerror(errno));
        return;
    }

    time_info = (struct tm *) malloc(sizeof(*time_info));
    if(time_info == NULL){
        syslog(LOG_ERR, "Error allocating memory for time info");
        return;
    }

    time_info_saved = time_info;
    time_info = localtime_r(&curr_time, time_info);

    if(time_info == NULL){
        syslog(LOG_ERR, "localtime_r error: %s", strerror(errno));
        time_info = time_info_saved;
        goto timer_expired_exit;
    }

    str_size = strftime(time_str, sizeof(time_str), "timestamp: %Y, %m, %d, %H, %M, %S\n", time_info);
    if(str_size == 0){
        syslog(LOG_ERR, "strftime error: %s", strerror(errno));
        goto timer_expired_exit;
    }

    syslog(LOG_DEBUG, "Current time: %s", time_str);

    timer_data = (timer_data_t *) sv.sival_ptr;
    if(timer_data == NULL){
        syslog(LOG_ERR, "Timer data is NULL");
        goto timer_expired_exit;
    }

    res = pthread_mutex_lock(timer_data->mutex);
    if(res != 0){
        syslog(LOG_ERR, "pthread_mutex_lock error: %d", res);
        goto timer_expired_exit;
    }

    FILE *data_file = fopen(DATA_FILE_NAME, "a+");

    if(data_file == NULL){
        syslog(LOG_ERR, "Error opening data file: %s", strerror(errno));
        goto timer_expired_exit;
    }

    res = fwrite(time_str, sizeof(*time_str), str_size, data_file);
    if(res != str_size){
        syslog(LOG_ERR, "fwrite error: %s", strerror(errno));
        goto timer_expired_close_file;
    }

    syslog(LOG_INFO, "Wrote timestamp to data file");

timer_expired_close_file:
    fclose(data_file);
    pthread_mutex_unlock(timer_data->mutex);
timer_expired_exit:
    free(time_info);
}
#endif