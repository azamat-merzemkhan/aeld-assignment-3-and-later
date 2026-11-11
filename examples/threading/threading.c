#define _POSIX_C_SOURCE 200809L
#include "threading.h"
#include <errno.h>
#include <stdlib.h>
#include <time.h>

static bool sleep_ms(int milliseconds)
{
    struct timespec remaining = {
        .tv_sec = milliseconds / 1000,
        .tv_nsec = (milliseconds % 1000) * 1000000L
    };
    while (nanosleep(&remaining, &remaining) != 0) {
        if (errno != EINTR) {
            return false;
        }
    }
    return true;
}

void *threadfunc(void *thread_param)
{
    struct thread_data *data = thread_param;
    if (data == NULL) {
        return NULL;
    }
    data->thread_complete_success = false;
    if (!sleep_ms(data->wait_to_obtain_ms)) {
        return data;
    }
    if (pthread_mutex_lock(data->mutex) != 0) {
        return data;
    }
    bool waited = sleep_ms(data->wait_to_release_ms);
    int unlocked = pthread_mutex_unlock(data->mutex);
    data->thread_complete_success = waited && unlocked == 0;
    return data;
}

bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,
                                 int wait_to_obtain_ms, int wait_to_release_ms)
{
    if (thread == NULL || mutex == NULL || wait_to_obtain_ms < 0 || wait_to_release_ms < 0) {
        return false;
    }
    struct thread_data *data = malloc(sizeof(*data));
    if (data == NULL) {
        return false;
    }
    *data = (struct thread_data) {
        .mutex = mutex,
        .wait_to_obtain_ms = wait_to_obtain_ms,
        .wait_to_release_ms = wait_to_release_ms,
        .thread_complete_success = false
    };
    if (pthread_create(thread, NULL, threadfunc, data) != 0) {
        free(data);
        return false;
    }
    return true;
}
