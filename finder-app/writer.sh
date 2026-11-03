#!/bin/sh

if [ "$#" -ne 2 ] || [ -z "$1" ]; then
    printf 'Usage: %s writefile writestr\n' "$0" >&2
    exit 1
fi
if ! mkdir -p -- "$(dirname -- "$1")"; then
    printf 'Error: cannot create parent directory for %s\n' "$1" >&2
    exit 1
fi
if ! printf '%s\n' "$2" > "$1"; then
    printf 'Error: cannot write file: %s\n' "$1" >&2
    exit 1
fi
