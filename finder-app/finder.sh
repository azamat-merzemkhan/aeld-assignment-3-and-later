#!/bin/sh

if [ "$#" -ne 2 ]; then
    printf 'Usage: %s filesdir searchstr\n' "$0" >&2
    exit 1
fi
if [ ! -d "$1" ]; then
    printf 'Error: directory does not exist: %s\n' "$1" >&2
    exit 1
fi

files=$(find "$1" -type f -exec printf 'x\n' \; | wc -l)
matches=$(find "$1" -type f -exec grep -h -F -- "$2" {} + | wc -l)
printf 'The number of files are %s and the number of matching lines are %s\n' \
    "$(printf '%s' "$files" | tr -d '[:space:]')" \
    "$(printf '%s' "$matches" | tr -d '[:space:]')"
