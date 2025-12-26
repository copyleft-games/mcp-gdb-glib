# Test GDB MCP Server Tools

Comprehensive test of all 21 GDB MCP server tools using the gobject-demo example application.

## Instructions

You will systematically test every tool in the GDB MCP server. For each tool, call it with the specified parameters and verify the response.

### Phase 0: Build the Example

First, ensure the example application is built:

```bash
make examples
```

Verify `./build/gobject-demo` exists.

### Phase 1: Session Management

1. **gdb_list_sessions** (empty state)
   - Call with no arguments
   - Verify: Shows 0 active sessions

2. **gdb_start**
   - Call with no arguments
   - Verify: Returns a sessionId
   - **Save this sessionId for all subsequent tests**

3. **gdb_list_sessions** (with session)
   - Call with no arguments
   - Verify: Shows 1 active session with the ID from step 2

### Phase 2: Program Loading

4. **gdb_load**
   - sessionId: (from step 2)
   - program: "./build/gobject-demo"
   - Verify: Program loaded successfully

### Phase 3: Breakpoints

5. **gdb_set_breakpoint** (function)
   - sessionId: (from step 2)
   - location: "main"
   - Verify: Breakpoint set

6. **gdb_set_breakpoint** (function with condition)
   - sessionId: (from step 2)
   - location: "demo_object_increment"
   - condition: "self->counter >= 0"
   - Verify: Conditional breakpoint set

### Phase 4: Execution Control

7. **gdb_continue** (start program, hit main breakpoint)
   - sessionId: (from step 2)
   - Verify: Program RUNS and stops at main breakpoint

8. **gdb_next** (step over)
   - sessionId: (from step 2)
   - Verify: Advances to next source line

9. **gdb_step** (step into)
   - sessionId: (from step 2)
   - Verify: Steps into the next function call

10. **gdb_finish** (finish current function)
    - sessionId: (from step 2)
    - Verify: Returns from current function

11. **gdb_continue** (to demo_object_increment breakpoint)
    - sessionId: (from step 2)
    - Verify: Stops at demo_object_increment breakpoint

### Phase 5: Inspection

12. **gdb_backtrace**
    - sessionId: (from step 2)
    - Verify: Shows call stack

13. **gdb_backtrace** (full with locals)
    - sessionId: (from step 2)
    - full: true
    - Verify: Shows call stack with local variables

14. **gdb_print** (variable)
    - sessionId: (from step 2)
    - expression: "self"
    - Verify: Shows pointer value

15. **gdb_print** (struct member)
    - sessionId: (from step 2)
    - expression: "self->counter"
    - Verify: Shows integer value

16. **gdb_examine** (memory)
    - sessionId: (from step 2)
    - expression: "self"
    - format: "x"
    - count: 4
    - Verify: Shows hex memory dump

17. **gdb_info_registers**
    - sessionId: (from step 2)
    - Verify: Shows register values

18. **gdb_command** (arbitrary)
    - sessionId: (from step 2)
    - command: "info locals"
    - Verify: Shows local variables

### Phase 6: GLib/GObject Tools

19. **gdb_glib_print_gobject**
    - sessionId: (from step 2)
    - expression: "self"
    - Verify: Shows GObject info (type, ref_count)

20. **gdb_glib_print_glist**
    - sessionId: (from step 2)
    - expression: "self->items"
    - Verify: Shows list contents

21. **gdb_glib_print_ghash**
    - sessionId: (from step 2)
    - expression: "self->properties"
    - Verify: Shows hash table entries

22. **gdb_glib_type_hierarchy**
    - sessionId: (from step 2)
    - expression: "self"
    - Verify: Shows type inheritance

23. **gdb_glib_signal_info**
    - sessionId: (from step 2)
    - expression: "self"
    - Verify: Shows signal information

### Phase 7: Cleanup

24. **gdb_terminate**
    - sessionId: (from step 2)
    - Verify: Session terminated

25. **gdb_list_sessions** (final)
    - Call with no arguments
    - Verify: Shows 0 active sessions

## Report Format

After testing, report results in this format:

| # | Tool | Status | Notes |
|---|------|--------|-------|
| 1 | gdb_list_sessions | PASS/FAIL | |
| 2 | gdb_start | PASS/FAIL | sessionId: xxx |
| ... | ... | ... | ... |

## Summary

- Total tools: 21
- Passed: X
- Failed: X
- Skipped: X

If any tests fail, note the error message and any troubleshooting steps attempted.
