#!/bin/bash
# Debug version of mock GDB

# Disable stdout buffering by re-executing with stdbuf if available
if [[ -z "$_UNBUFFERED" ]] && command -v stdbuf >/dev/null 2>&1; then
    _UNBUFFERED=1 exec stdbuf -oL "$0" "$@"
fi

exec 2>/tmp/mock-gdb-debug.log

echo "DEBUG: mock-gdb.sh started with args: $@" >&2
echo "DEBUG: PID $$" >&2
echo "DEBUG: _UNBUFFERED=$_UNBUFFERED" >&2

# Output the initial prompt
echo "(gdb)"
echo "DEBUG: Sent (gdb) prompt" >&2

# Read and process commands
while IFS= read -r line
do
    echo "DEBUG: Received line: $line" >&2
    
    # Skip empty lines
    [[ -z "$line" ]] && continue

    case "$line" in
        *-gdb-exit)
            echo "^exit"
            echo "DEBUG: Exiting" >&2
            exit 0
            ;;
        *)
            echo "^done"
            echo "(gdb)"
            ;;
    esac
done

echo "DEBUG: EOF on stdin" >&2
