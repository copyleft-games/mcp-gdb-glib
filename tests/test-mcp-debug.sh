#!/bin/bash
# Test script to debug the MCP server

cd "$(dirname "$0")/.."

# Create a named pipe for responses
FIFO=$(mktemp -u)
mkfifo "$FIFO"

# Start the server with debug output
G_MESSAGES_DEBUG=all ./build/gdb-mcp-server 2>&1 &
SERVER_PID=$!

# Give it time to start
sleep 0.5

# Send initialize
echo '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2024-11-05","capabilities":{},"clientInfo":{"name":"test","version":"1.0"}}}' >&0

sleep 0.5

# Send gdb_start
echo '{"jsonrpc":"2.0","id":2,"method":"tools/call","params":{"name":"gdb_start","arguments":{}}}' >&0

sleep 1

# Send gdb_load
echo '{"jsonrpc":"2.0","id":3,"method":"tools/call","params":{"name":"gdb_load","arguments":{"sessionId":"placeholder","program":"./build/gobject-demo"}}}' >&0

sleep 2

# Kill the server
kill $SERVER_PID 2>/dev/null
rm -f "$FIFO"
