#!/usr/bin/env python3
"""Test script to debug GDB MCP server session issues."""

import json
import os
import subprocess
import sys
import time
import threading
import select

def send_request(proc, method, params=None, req_id=None):
    """Send a JSON-RPC request to the server."""
    if req_id is None:
        req_id = int(time.time() * 1000)

    request = {
        "jsonrpc": "2.0",
        "id": req_id,
        "method": method,
    }
    if params:
        request["params"] = params

    msg = json.dumps(request)
    print(f">>> {msg}", file=sys.stderr)
    proc.stdin.write(msg + "\n")
    proc.stdin.flush()

def read_response(proc, timeout=5):
    """Read a response from the server."""
    ready, _, _ = select.select([proc.stdout], [], [], timeout)
    if ready:
        line = proc.stdout.readline()
        if line:
            line = line.strip()
            if line.startswith("{"):
                print(f"<<< {line}", file=sys.stderr)
                return json.loads(line)
            else:
                print(f"SKIP: {line}", file=sys.stderr)
                return read_response(proc, timeout)
    return None

def main():
    # Start the server with debug output enabled
    env = dict(os.environ)
    env["G_MESSAGES_DEBUG"] = "all"

    proc = subprocess.Popen(
        ["./build/gdb-mcp-server"],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        bufsize=1,
        env=env
    )

    # Start a thread to read stderr
    def read_stderr():
        for line in proc.stderr:
            print(f"ERR: {line.rstrip()}", file=sys.stderr)

    stderr_thread = threading.Thread(target=read_stderr, daemon=True)
    stderr_thread.start()

    try:
        time.sleep(0.5)  # Wait for server to start

        # Initialize
        send_request(proc, "initialize", {
            "protocolVersion": "2024-11-05",
            "capabilities": {},
            "clientInfo": {"name": "test", "version": "1.0"}
        }, 1)
        resp = read_response(proc)
        print(f"Initialize response received", file=sys.stderr)

        # Send initialized notification
        proc.stdin.write('{"jsonrpc":"2.0","method":"notifications/initialized"}\n')
        proc.stdin.flush()
        time.sleep(0.5)

        # Start a GDB session
        send_request(proc, "tools/call", {
            "name": "gdb_start",
            "arguments": {}
        }, 2)
        resp = read_response(proc, timeout=15)
        print(f"gdb_start response: {json.dumps(resp, indent=2) if resp else 'None'}", file=sys.stderr)

        if resp and "result" in resp:
            # Extract session ID from the response text
            content = resp["result"].get("content", [])
            if content:
                text = content[0].get("text", "")
                print(f"gdb_start text:\n{text}", file=sys.stderr)

                # Parse session ID
                session_id = None
                for line in text.split("\n"):
                    if "Session ID:" in line:
                        session_id = line.split("Session ID:")[1].strip()
                        break

                if session_id:
                    print(f"\n*** Session ID: {session_id} ***", file=sys.stderr)
                    print(f"*** Waiting 2 seconds before gdb_load ***\n", file=sys.stderr)
                    time.sleep(2)

                    # Now try to load
                    send_request(proc, "tools/call", {
                        "name": "gdb_load",
                        "arguments": {
                            "sessionId": session_id,
                            "program": "./build/gobject-demo"
                        }
                    }, 3)
                    resp = read_response(proc, timeout=15)
                    print(f"gdb_load response: {json.dumps(resp, indent=2) if resp else 'None'}", file=sys.stderr)

        time.sleep(1)

    finally:
        proc.terminate()
        try:
            proc.wait(timeout=5)
        except:
            proc.kill()

if __name__ == "__main__":
    main()
