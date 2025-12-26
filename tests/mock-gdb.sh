#!/bin/bash
#
# mock-gdb.sh - Mock GDB script for unit testing
#
# Copyright (C) 2025 Zach Podbielniak
# SPDX-License-Identifier: AGPL-3.0-or-later
#
# This script mimics GDB's MI interface for predictable unit testing.
# It responds to common MI commands with pre-defined responses.

# Disable stdout buffering for subprocess communication
if [[ -z "$_UNBUFFERED" ]] && command -v stdbuf >/dev/null 2>&1; then
    _UNBUFFERED=1 exec stdbuf -oL "$0" "$@"
fi

# State variables
loaded_program=""
breakpoint_num=1
is_running=0

# Output the initial prompt
echo "(gdb)"

# Read and process commands
while IFS= read -r line
do
    # Skip empty lines
    [[ -z "$line" ]] && continue

    # Extract token if present (e.g., "123-exec-run" -> token="123", cmd="-exec-run")
    if [[ "$line" =~ ^([0-9]+)(-.*)$ ]]
    then
        token="${BASH_REMATCH[1]}"
        cmd="${BASH_REMATCH[2]}"
    else
        token=""
        cmd="$line"
    fi

    # Process commands
    case "$cmd" in
        -gdb-exit|quit)
            echo "${token}^exit"
            exit 0
            ;;

        -file-exec-and-symbols\ *)
            # Extract program path
            program="${cmd#-file-exec-and-symbols }"
            loaded_program="$program"
            echo "${token}^done"
            echo "(gdb)"
            ;;

        -file-exec-file\ *)
            program="${cmd#-file-exec-file }"
            loaded_program="$program"
            echo "${token}^done"
            echo "(gdb)"
            ;;

        -exec-run)
            is_running=1
            echo "${token}^running"
            # Simulate immediate stop at breakpoint
            echo "*stopped,reason=\"breakpoint-hit\",disp=\"keep\",bkptno=\"1\",frame={addr=\"0x0000555555555149\",func=\"main\",args=[],file=\"test.c\",fullname=\"/tmp/test.c\",line=\"5\"}"
            echo "(gdb)"
            ;;

        -exec-continue)
            echo "${token}^running"
            # Simulate program exit
            echo "*stopped,reason=\"exited-normally\""
            echo "(gdb)"
            ;;

        -exec-step|-exec-stepi)
            echo "${token}^running"
            echo "*stopped,reason=\"end-stepping-range\",frame={addr=\"0x0000555555555150\",func=\"main\",args=[],file=\"test.c\",fullname=\"/tmp/test.c\",line=\"6\"}"
            echo "(gdb)"
            ;;

        -exec-next|-exec-nexti)
            echo "${token}^running"
            echo "*stopped,reason=\"end-stepping-range\",frame={addr=\"0x0000555555555160\",func=\"main\",args=[],file=\"test.c\",fullname=\"/tmp/test.c\",line=\"7\"}"
            echo "(gdb)"
            ;;

        -exec-finish)
            echo "${token}^running"
            echo "*stopped,reason=\"function-finished\",frame={addr=\"0x0000555555555170\",func=\"main\",args=[]}"
            echo "(gdb)"
            ;;

        -break-insert\ *)
            location="${cmd#-break-insert }"
            echo "${token}^done,bkpt={number=\"${breakpoint_num}\",type=\"breakpoint\",disp=\"keep\",enabled=\"y\",addr=\"0x0000555555555149\",func=\"${location}\",file=\"test.c\",fullname=\"/tmp/test.c\",line=\"5\",thread-groups=[\"i1\"],times=\"0\"}"
            ((breakpoint_num++))
            echo "(gdb)"
            ;;

        -stack-list-frames)
            echo "${token}^done,stack=[frame={level=\"0\",addr=\"0x0000555555555149\",func=\"main\",file=\"test.c\",fullname=\"/tmp/test.c\",line=\"5\"},frame={level=\"1\",addr=\"0x00007ffff7c29d90\",func=\"__libc_start_call_main\"}]"
            echo "(gdb)"
            ;;

        -data-evaluate-expression\ *)
            expr="${cmd#-data-evaluate-expression }"
            echo "${token}^done,value=\"42\""
            echo "(gdb)"
            ;;

        -data-read-memory\ *)
            echo "${token}^done,addr=\"0x12345678\",nr-bytes=\"16\",total-bytes=\"16\",next-row=\"0x12345688\",prev-row=\"0x12345668\",next-page=\"0x12345688\",prev-page=\"0x12345668\",memory=[{addr=\"0x12345678\",data=[\"0x00\",\"0x01\",\"0x02\",\"0x03\"]}]"
            echo "(gdb)"
            ;;

        -data-list-register-values\ *)
            echo "${token}^done,register-values=[{number=\"0\",value=\"0x0\"},{number=\"1\",value=\"0x0\"},{number=\"2\",value=\"0x7fffffffe000\"}]"
            echo "(gdb)"
            ;;

        -target-attach\ *)
            pid="${cmd#-target-attach }"
            echo "${token}^done"
            echo "*stopped,reason=\"signal-received\",signal-name=\"SIGSTOP\""
            echo "(gdb)"
            ;;

        -interpreter-exec\ *)
            # Handle arbitrary GDB commands
            echo "~\"Output from command\\n\""
            echo "${token}^done"
            echo "(gdb)"
            ;;

        help)
            echo "~\"List of classes of commands:\\n\""
            echo "~\"Type \\\"help\\\" followed by a class name\\n\""
            echo "${token}^done"
            echo "(gdb)"
            ;;

        *)
            # Unknown command - return done
            echo "${token}^done"
            echo "(gdb)"
            ;;
    esac
done

# EOF on stdin - exit cleanly
exit 0
