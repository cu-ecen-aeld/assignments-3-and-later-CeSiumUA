#ifndef AESDSOCKET_H
#define AESDSOCKET_H

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <syslog.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <signal.h>
#include <thread_queue.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <time.h>
#include "../aesd-char-driver/aesd_ioctl.h"

typedef struct client_thread_data{
    int client_fd;
    char addr_str[INET6_ADDRSTRLEN];
    pthread_mutex_t *mutex;
    bool thread_completed;
} client_thread_data_t;

typedef struct timer_data{
    pthread_mutex_t *mutex;
} timer_data_t;

typedef struct thread_instance{
    pthread_t thread;
    client_thread_data_t *thread_data;
} thread_instance_t;

#define DAEMON_KEY              "-d"
#define PORT                    "9000"

#define USE_AESD_CHAR_DEVICE    (1)

#if USE_AESD_CHAR_DEVICE == 1
#define DATA_FILE_NAME          "/dev/aesdchar"
#else
#define DATA_FILE_NAME          "/var/tmp/aesdsocketdata"
#endif

#define AESD_SEEKTO_KEYWORD     "AESDCHAR_IOCSEEKTO"
#define AESD_SEEKTO_KEYWORD_LEN (sizeof(AESD_SEEKTO_KEYWORD) - 1)

#define RECV_BUFFER_LEN         512

#endif