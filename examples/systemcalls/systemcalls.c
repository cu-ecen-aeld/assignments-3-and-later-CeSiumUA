#include "systemcalls.h"

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{
    openlog("assignment3::do_system", 0, LOG_USER);
/*
 * TODO  add your code here
 *  Call the system() function with the command set in the cmd
 *   and return a boolean true if the system() call completed with success
 *   or false() if it returned a failure
*/
    syslog(LOG_DEBUG | LOG_USER, "calling system with: %s\n", cmd);

    int wait_stat = system(cmd);

    syslog(LOG_DEBUG | LOG_USER, "system call returned: %d\n", wait_stat);

    if (wait_stat == -1)
    {
        return false;
    }
    else if (WIFEXITED(wait_stat))
    {
        if (WEXITSTATUS(wait_stat) == 0)
        {
            syslog(LOG_DEBUG | LOG_USER, "system call (%s) succeeded\n", cmd);
            closelog();
            return true;
        }
    }

    syslog(LOG_DEBUG | LOG_USER, "system call (%s) failed: %d\n", cmd, wait_stat);
    closelog();
    return false;
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    bool fn_result = false;
    openlog("assignment3::do_exec", 0, LOG_USER);
    
    int i;
    int res;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
        syslog(LOG_DEBUG | LOG_USER, "command[%d]: %s\n", i, command[i]);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    // command[count] = command[count];

/*
 * TODO:
 *   Execute a system command by calling fork, execv(),
 *   and wait instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *
*/
    int pid;
    syslog(LOG_DEBUG | LOG_USER, "calling fork\n");
    pid = fork();
    if (pid == -1){
        syslog(LOG_ERR | LOG_USER, "fork error\n");
        goto exit;
    }
    else if(pid == 0){
        res = execv(command[0], command);
        if (res == -1){
            syslog(LOG_ERR | LOG_USER, "execv error\n");
            exit(1);
        }
        syslog(LOG_ERR | LOG_USER, "after execv, which is unexpected\n");
    }
    else{
        int status;
        syslog(LOG_DEBUG | LOG_USER, "waiting for pid: %d\n", pid);
        res = waitpid(pid, &status, 0);
        if (res == -1){
            syslog(LOG_ERR | LOG_USER, "waitpid error\n");
            goto exit;
        }
        syslog(LOG_DEBUG | LOG_USER, "child process status: %d\n", status);
        if (WIFEXITED(status)){
            if (WEXITSTATUS(status) == 0){
                syslog(LOG_DEBUG | LOG_USER, "child process succeeded\n");
                fn_result = true;
            }
        }
    }

exit:
    syslog(LOG_DEBUG | LOG_USER, "exiting...\n");
    va_end(args);
    closelog();
    return fn_result;
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];


/*
 * TODO
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a reference,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *
*/

    va_end(args);

    return true;
}
