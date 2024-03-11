#include "aesdsocket.h"

int main(int argc, char **argv){

    struct addrinfo hints, *addr_res;
    int sockfd;
    int res;

    bool start_in_daemon = false;

    openlog(argv[0], LOG_PID, LOG_USER);

    if(argc == 2 && strcmp(argv[1], DAEMON_KEY) == 0){
        syslog(LOG_DEBUG, "daemon flag provided, server will start in daemon mode");
        start_in_daemon = true;
    }

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
        return -1;
    }

    res = bind(sockfd, addr_res->ai_addr, addr_res->ai_addrlen);
    if(res == -1){
        syslog(LOG_ERR, "bind error: %s", strerror(errno));
        return -1;
    }
}