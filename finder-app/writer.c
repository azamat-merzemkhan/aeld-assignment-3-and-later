/* Assignment 2 implementation prepared with OpenAI Codex assistance. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

int main(int argc, char *argv[])
{
    openlog("writer", LOG_PID, LOG_USER);
    if (argc != 3 || argv[1][0] == '\0') {
        syslog(LOG_ERR, "Expected writefile and writestr arguments");
        fprintf(stderr, "Usage: %s writefile writestr\n", argv[0]);
        closelog();
        return EXIT_FAILURE;
    }

    FILE *output = fopen(argv[1], "w");
    if (output == NULL) {
        syslog(LOG_ERR, "Cannot open %s: %s", argv[1], strerror(errno));
        fprintf(stderr, "Cannot open %s: %s\n", argv[1], strerror(errno));
        closelog();
        return EXIT_FAILURE;
    }

    syslog(LOG_DEBUG, "Writing %s to %s", argv[2], argv[1]);
    int failed = 0;
    if (fprintf(output, "%s\n", argv[2]) < 0) {
        syslog(LOG_ERR, "Cannot write %s: %s", argv[1], strerror(errno));
        fprintf(stderr, "Cannot write %s: %s\n", argv[1], strerror(errno));
        failed = 1;
    }
    if (fclose(output) != 0) {
        syslog(LOG_ERR, "Cannot close %s: %s", argv[1], strerror(errno));
        fprintf(stderr, "Cannot close %s: %s\n", argv[1], strerror(errno));
        failed = 1;
    }
    closelog();
    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
