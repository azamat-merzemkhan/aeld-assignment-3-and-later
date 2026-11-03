#!/bin/bash
set -euo pipefail
cd -- "$(dirname -- "$0")"
testdir=$(mktemp -d)
trap 'rm -rf -- "$testdir"' EXIT
expect_error() {
    local status=0
    "$@" >/dev/null 2>&1 || status=$?
    test "$status" -eq 1
}
expect_error ./finder-app/finder.sh
expect_error ./finder-app/finder.sh "$testdir"
expect_error ./finder-app/finder.sh "$testdir/missing" word
expect_error ./finder-app/writer.sh
expect_error ./finder-app/writer.sh "$testdir/output"
./finder-app/writer.sh "$testdir/nested folder/a.txt" 'literal.[pattern]'
./finder-app/writer.sh "$testdir/b.txt" $'literal.[pattern]\nliteral.[pattern]'
./finder-app/writer.sh "$testdir/c.txt" 'unrelated'
test "$(./finder-app/finder.sh "$testdir" 'literal.[pattern]')" = \
    'The number of files are 3 and the number of matching lines are 3'
./finder-app/writer.sh "$testdir/c.txt" 'replacement'
test "$(cat "$testdir/c.txt")" = replacement
test "$(./finder-app/finder.sh "$testdir" 'absent')" = \
    'The number of files are 3 and the number of matching lines are 0'
mkdir "$testdir/empty"
test "$(./finder-app/finder.sh "$testdir/empty" word)" = \
    'The number of files are 0 and the number of matching lines are 0'
expect_error ./finder-app/writer.sh "$testdir/c.txt/invalid" x
printf 'PASS: argument validation, nested paths, literal search, matching lines, overwrite, empty directories, write failure\n'
