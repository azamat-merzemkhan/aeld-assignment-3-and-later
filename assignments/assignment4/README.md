# Assignment 4 Part 1: Threading

`start_thread_obtaining_mutex` allocates one argument structure per thread,
starts the thread without joining it, and frees the allocation if thread
creation fails. The worker waits in milliseconds, locks the supplied mutex,
waits again, and unlocks it. Interrupted sleeps resume using the remaining
interval. The caller joins the thread, checks `thread_complete_success`, and
frees the returned structure, as required by the course interface.

Run `./unit-test.sh` on Linux. The official tests cover initially locked and
unlocked mutexes and concurrent threads with shared or separate mutexes.
The existing system-call tests remain part of the suite.

The finder test also supports the Assignment 4 Part 2 image: executables are
resolved from PATH, target configuration comes from `/etc/finder-app/conf`,
and the finder result is saved to `/tmp/assignment4-result.txt`. Local/manual
Assignment 3 layouts remain supported.

Implementation prepared with OpenAI Codex AI assistance using official
course starter code and tests. No other student's solution was used.
