#define _POSIX_C_SOURCE 200809L
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <syslog.h>
#include <unistd.h>

#define DATAFILE "/var/tmp/aesdsocketdata"
static volatile sig_atomic_t stopping;
static void on_signal(int sig) { (void)sig; stopping = 1; }

static int ready(int fd, short events)
{
    struct pollfd p = { .fd = fd, .events = events };
    while (!stopping) {
        int n = poll(&p, 1, 200);
        if (n > 0) return (p.revents & (events | POLLHUP)) ? 0 : -1;
        if (n < 0 && errno != EINTR) return -1;
    }
    return -1;
}

static int write_all(int fd, const char *buf, size_t len, int socket_fd)
{
    while (len && !stopping) {
        if (socket_fd && ready(fd, POLLOUT)) return -1;
        ssize_t n = socket_fd ? send(fd, buf, len, 0) : write(fd, buf, len);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return -1;
        buf += n;
        len -= (size_t)n;
    }
    return len ? -1 : 0;
}

static int reply_file(int client, int data)
{
    char buf[4096];
    if (lseek(data, 0, SEEK_SET) < 0) return -1;
    while (!stopping) {
        ssize_t n = read(data, buf, sizeof(buf));
        if (n < 0 && errno == EINTR) continue;
        if (n < 0) return -1;
        if (!n) return 0;
        if (write_all(client, buf, (size_t)n, 1)) return -1;
    }
    return -1;
}

static void serve(int client, int data)
{
    char input[4096], *packet = NULL;
    size_t used = 0, capacity = 0;
    while (!stopping && !ready(client, POLLIN)) {
        ssize_t n = recv(client, input, sizeof(input), 0);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) break;
        size_t offset = 0;
        while (offset < (size_t)n && !stopping) {
            char *newline = memchr(input + offset, '\n', (size_t)n - offset);
            size_t count = newline ? (size_t)(newline - input) - offset + 1 : (size_t)n - offset;
            if (count > SIZE_MAX - used) goto done;
            size_t needed = used + count;
            if (needed > capacity) {
                size_t grown = capacity ? capacity : 4096;
                while (grown < needed && grown <= SIZE_MAX / 2) grown *= 2;
                if (grown < needed) grown = needed;
                char *next = realloc(packet, grown);
                if (!next) { syslog(LOG_ERR, "Packet allocation failed"); goto done; }
                packet = next;
                capacity = grown;
            }
            memcpy(packet + used, input + offset, count);
            used += count;
            offset += count;
            if (newline) {
                if (write_all(data, packet, used, 0) || reply_file(client, data)) goto done;
                used = 0;
            }
        }
    }
done:
    free(packet);
}

int main(int argc, char **argv)
{
    int daemon_mode = argc == 2 && !strcmp(argv[1], "-d");
    if (argc > 2 || (argc == 2 && !daemon_mode)) return -1;
    openlog("aesdsocket", LOG_PID, LOG_USER);
    struct sigaction action = { .sa_handler = on_signal };
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGINT, &action, NULL) || sigaction(SIGTERM, &action, NULL)) return -1;
    signal(SIGPIPE, SIG_IGN);
    struct addrinfo hints = { .ai_family = AF_INET, .ai_socktype = SOCK_STREAM, .ai_flags = AI_PASSIVE };
    struct addrinfo *addresses = NULL;
    int listener = -1;
    if (getaddrinfo(NULL, "9000", &hints, &addresses)) return -1;
    for (struct addrinfo *a = addresses; a; a = a->ai_next) {
        listener = socket(a->ai_family, a->ai_socktype, a->ai_protocol);
        if (listener < 0) continue;
        int yes = 1;
        if (!setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) &&
            !bind(listener, a->ai_addr, a->ai_addrlen) && !listen(listener, 10)) break;
        close(listener);
        listener = -1;
    }
    freeaddrinfo(addresses);
    if (listener < 0) { syslog(LOG_ERR, "Socket setup failed: %s", strerror(errno)); return -1; }
    if (daemon_mode) {
        pid_t pid = fork();
        if (pid < 0) { close(listener); return -1; }
        if (pid > 0) { close(listener); return 0; }
        if (setsid() < 0 || chdir("/")) { close(listener); return -1; }
        int nullfd = open("/dev/null", O_RDWR);
        if (nullfd < 0) { close(listener); return -1; }
        for (int i = 0; i <= 2; i++) {
            if (dup2(nullfd, i) < 0) { close(nullfd); close(listener); return -1; }
        }
        if (nullfd > 2) close(nullfd);
    }
    int data = open(DATAFILE, O_CREAT | O_RDWR | O_APPEND, 0644);
    if (data < 0) { close(listener); return -1; }
    while (!stopping && !ready(listener, POLLIN)) {
        struct sockaddr_in peer;
        socklen_t length = sizeof(peer);
        int client = accept(listener, (struct sockaddr *)&peer, &length);
        if (client < 0) { if (errno == EINTR) continue; break; }
        char ip[INET_ADDRSTRLEN];
        if (!inet_ntop(AF_INET, &peer.sin_addr, ip, sizeof(ip))) strcpy(ip, "unknown");
        syslog(LOG_INFO, "Accepted connection from %s", ip);
        serve(client, data);
        close(client);
        syslog(LOG_INFO, "Closed connection from %s", ip);
    }
    if (stopping) syslog(LOG_INFO, "Caught signal, exiting");
    close(data);
    close(listener);
    unlink(DATAFILE);
    closelog();
    return 0;
}
