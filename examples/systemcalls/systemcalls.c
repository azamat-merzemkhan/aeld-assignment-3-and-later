#include "systemcalls.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

static bool wait_for_success(pid_t child)
{
    int status;
    pid_t result;

    do {
        result = waitpid(child, &status, 0);
    } while (result == -1 && errno == EINTR);

    return result == child && WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

static bool run_command(char *const command[], const char *outputfile)
{
    if (command[0] == NULL || command[0][0] != '/') {
        return false;
    }

    fflush(stdout);
    pid_t child = fork();
    if (child == -1) {
        perror("fork");
        return false;
    }

    if (child == 0) {
        if (outputfile != NULL) {
            int output = open(outputfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (output == -1) {
                perror("open output file");
                _exit(EXIT_FAILURE);
            }
            if (dup2(output, STDOUT_FILENO) == -1) {
                perror("dup2");
                close(output);
                _exit(EXIT_FAILURE);
            }
            if (output != STDOUT_FILENO) {
                close(output);
            }
        }

        execv(command[0], command);
        perror("execv");
        _exit(EXIT_FAILURE);
    }

    return wait_for_success(child);
}

bool do_system(const char *cmd)
{
    if (cmd == NULL) {
        return false;
    }

    int status = system(cmd);
    return status != -1 && WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

bool do_exec(int count, ...)
{
    if (count < 1) {
        return false;
    }

    char *command[count + 1];
    va_list args;
    va_start(args, count);
    for (int i = 0; i < count; i++) {
        command[i] = va_arg(args, char *);
        if (command[i] == NULL) {
            va_end(args);
            return false;
        }
    }
    va_end(args);
    command[count] = NULL;

    return run_command(command, NULL);
}

bool do_exec_redirect(const char *outputfile, int count, ...)
{
    if (outputfile == NULL || count < 1) {
        return false;
    }

    char *command[count + 1];
    va_list args;
    va_start(args, count);
    for (int i = 0; i < count; i++) {
        command[i] = va_arg(args, char *);
        if (command[i] == NULL) {
            va_end(args);
            return false;
        }
    }
    va_end(args);
    command[count] = NULL;

    return run_command(command, outputfile);
}
