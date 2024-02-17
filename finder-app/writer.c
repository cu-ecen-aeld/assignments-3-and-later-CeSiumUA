#include <syslog.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

int main(int argc, char **argv){

    openlog(argv[0], LOG_PID, LOG_USER);

    if(argc != 3){
        syslog(LOG_ERR, "Expected 2 arguments to be provided, got: %d", argc-1);
        goto err_exit;
    }

    char *file_name = argv[1];
    char *txt = argv[2];
    syslog(LOG_DEBUG, "Writing %s to %s", txt, file_name);

    FILE *fp = NULL;
    int res;

    fp = fopen(file_name, "w");

    if (fp == NULL){
        syslog(LOG_ERR, "Error (%d) opening file %s for writing", errno, file_name);
        goto err_exit;
    }

    res = fputs(txt, fp);
    if (res == EOF){
        syslog(LOG_ERR, "Error (%d) writing %s to file: %s", errno, txt, file_name);
        goto err_exit;
    }

    syslog(LOG_DEBUG, "Successfully written %s to file: %s", txt, file_name);

    fclose(fp);
    closelog();
    return 0;

err_exit:
    // Despite it's not necessary to call closelog, I think it's a more "graceful" way while exiting program :)
    closelog();
    exit(1);
}