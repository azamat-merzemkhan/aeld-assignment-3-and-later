#!/bin/bash
set -euo pipefail
cd -- "$(dirname -- "$0")"
if [ "$#" -ne 1 ] || [[ ! "$1" =~ ^[A-Za-z0-9]([A-Za-z0-9-]{0,37}[A-Za-z0-9])?$ ]] || [[ "$1" == *--* ]]; then
    printf 'Usage: %s actual-github-username\n' "$0" >&2
    exit 1
fi
printf '%s' "$1" > conf/username.txt
sed -i "s/return \".*\";/return \"$1\";/" examples/autotest-validate/autotest-validate.c
printf 'Configured GitHub username: %s\n' "$1"
