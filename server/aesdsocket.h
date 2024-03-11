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

#define DAEMON_KEY          "-d"
#define PORT                "9000"
#define DATA_FILE_NAME      "/var/tmp/aesdsocketdata"
#define RECV_BUFFER_LEN     512

#endif