# Assignment 3 Part 1: System Calls

Based on the Assignment 2 solution and the course's `assignment3-part-1`
starter branch. The official `assignment-autotest` submodule is pinned to
`a421651f00a585e488572e9db94d896d364cfc5f`.

## Implementation

- `do_system` invokes the shell and returns true only for a normal zero exit.
- `do_exec` uses `fork`, `execv`, and `waitpid` with the supplied absolute path.
- `do_exec_redirect` redirects the child's stdout using `open` and `dup2`,
  truncates an existing output file, and reports command or redirection errors.
- Interrupted waits are retried; failed child setup or exec exits using `_exit`.
- `finder-app/finder-test.sh` no longer builds the writer. Build it separately
  before running that script, as required for Assignment 3 Part 2.

## Validation

Run `./unit-test.sh` on Linux with CMake, a C compiler, make, and Ruby installed.
This runs the official course tests plus four additional tests for invalid
commands, nonzero/signal exits, redirection errors, and output truncation.

The GitHub Actions `unit-test` job runs this suite in the course's Linux image.
On macOS the official test's `/usr/bin/test` executable is absent (`test` is
located at `/bin/test`), so that Linux-specific assertion cannot pass locally.
The other official tests and the four additional cases were checked locally.

As noted in the course instructions, `full-test.sh` requires the Assignment 3
Part 2 work and is not the acceptance check for Part 1.

## Assistance disclosure

The implementation and additional tests were prepared with OpenAI Codex AI
assistance. No other student's solution was used as a reference. The official
course starter code and tests were used to determine the required behavior.
