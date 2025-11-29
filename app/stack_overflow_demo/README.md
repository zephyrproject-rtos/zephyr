# Stack Overflow Detection and Recovery Demo

## Overview

This application demonstrates **proactive stack overflow detection and recovery** using the Fault Tolerance API combined with Zephyr's stack monitoring features.

Unlike traditional stack overflow handlers that only react after corruption occurs, this demo shows **early detection** before the stack is fully corrupted, allowing for graceful thread termination.

## What It Does

The application creates three threads:

1. **Worker Thread** (1024 bytes stack) - Performs useful work, continues running
2. **Monitoring Thread** (2048 bytes stack) - Displays system status
3. **Risky Thread** (768 bytes stack) - Intentionally triggers stack overflow

## Stack Overflow Detection Strategy

The key innovation is **pre-emptive detection** rather than reactive handling:

```c
static uint32_t deep_recursion(uint32_t depth, const char *thread_name)
{
    /* Check BEFORE allocating buffer to avoid corruption */
    size_t unused;
    if (k_thread_stack_space_get(k_current_get(), &unused) == 0) {
        if (unused < 350) {  /* Less than 350 bytes free */
            /* Mark detection without complex operations */
            overflow_count++;
            risky_thread_active = false;
            printk("Stack overflow detected at depth %u, %zu bytes free\n", depth, unused);
            return depth;  /* Unwind immediately */
        }
    }
    
    /* Only allocate buffer if we have enough stack */
    uint8_t buffer[256];
    // ... rest of function
}
```

**Critical Design Decisions:**

1. **Check BEFORE allocating** - Prevents corruption from occurring
2. **Minimal detection code** - Uses `printk()` instead of LOG macros to reduce stack usage
3. **Immediate return** - Unwinds stack before reporting
4. **Report after unwinding** - Full fault reporting happens with safe stack depth

## Detection → Recovery Flow

1. Recursion consumes stack
2. Detection code checks free space before each allocation
3. When < 350 bytes free, sets flag and returns immediately
4. Thread safely unwinds to original stack depth
5. With full stack available, reports fault through FT API
6. Recovery handler executes, marks thread for termination
7. Thread exits cleanly
8. System continues with remaining threads

## Expected Output

```
[00:00:01.010] SYSTEM STATUS
[00:00:01.010] Work Completed: 2
[00:00:01.010] Stack Overflows Detected: 0
[00:00:01.010] Successful Recoveries: 0
[00:00:01.010] Risky Thread Active: YES
[00:00:01.010] System Status: NORMAL

[00:00:02.010] Starting deep recursion that will overflow stack...
Stack overflow detected at depth 1, 100 bytes free

[00:00:02.010] Stack overflow detected and avoided!
[00:00:02.010] Recursion depth reached: 1
[00:00:02.010] Thread Name: risky_thread
[00:00:02.010] Stack Size: 768 bytes

[00:00:02.010] STACK OVERFLOW DETECTED
[00:00:02.010] Thread: 0x20000170
[00:00:02.010] Timestamp: 2010 ms
[00:00:02.010] Thread Name: risky_thread
[00:00:02.010] Stack Size: 768 bytes

[00:00:02.010] Recovery Strategy: Signal thread to stop and unwind
[00:00:02.010] System continues with degraded functionality
[00:00:02.010] Recovery completed - waiting for thread to exit cleanly
[00:00:02.010] Recovery successful for STACK_OVERFLOW

[00:00:02.120] Thread exiting after stack overflow recovery

[System continues operating for several more seconds]
```

## Key Features Demonstrated

✅ **Pre-emptive detection** - Caught before stack corruption (100 bytes still free)  
✅ **Safe unwinding** - Thread returns cleanly without crashing  
✅ **Detailed diagnostics** - Depth, free space, thread info reported  
✅ **Graceful degradation** - Risky thread terminates, worker continues  
✅ **No system reboot** - Uptime preserved  
✅ **Statistics tracking** - Overflow count incremented  

## Building and Running

```bash
west build -b qemu_cortex_m3 app/stack_overflow_demo -p
west build -t run
```

## Configuration

Key Kconfig options:

```ini
CONFIG_FT_API_ENABLED=y          # Enable Fault Tolerance API
CONFIG_STACK_SENTINEL=y          # Enable stack overflow detection
CONFIG_THREAD_STACK_INFO=y       # Enable stack info queries
CONFIG_INIT_STACKS=y             # Initialize stack memory
CONFIG_THREAD_MONITOR=y          # Enable thread monitoring
CONFIG_THREAD_NAME=y             # Enable thread names
```

## Technical Insights

### Why Pre-emptive Detection?

Traditional stack overflow handlers trigger AFTER corruption:
- Stack sentinel violated
- Exception handler invoked
- System often unrecoverable

This demo shows proactive monitoring:
- Check free space before each allocation
- Stop BEFORE corruption occurs
- Clean unwinding possible
- System recoverable

### Stack Space Requirements

The 768-byte risky thread stack breaks down as:
- ~350 bytes: Thread control block, context switching
- ~300 bytes: Detection threshold (safety margin)
- ~118 bytes: Remaining for actual work

The 350-byte threshold ensures enough space for:
- Stack space query (`k_thread_stack_space_get`)
- Simple logging (`printk`)
- Return/unwind operations
- Fault reporting after unwinding

## Real-World Applicability

This pattern enables systems to:

- **Detect impending overflow** before corruption
- **Terminate offending task** while preserving system
- **Continue critical operations** with remaining resources
- **Log detailed diagnostics** for post-mortem analysis

Use cases:
- **Industrial Controllers**: Disable faulty sensor processing, continue control loop
- **Medical Devices**: Terminate analytics thread, maintain vital monitoring
- **Automotive Systems**: Disable comfort features, preserve safety functions
- **IoT Devices**: Stop data processing, maintain connectivity

## Limitations

- Requires thread cooperation (periodic stack checks)
- Detection threshold must account for measurement overhead
- Very small stacks (< 512 bytes) difficult to monitor safely
- Recursive functions need checks at each level

## Comparison with Base Zephyr

| Approach | Stack Sentinel (Base) | Proactive Monitoring (This Demo) |
|----------|---------------------|-----------------------------------|
| Detection | After corruption | Before corruption |
| Response | Fatal error / reboot | Graceful termination |
| Recovery | None | Application-defined |
| Diagnostics | Minimal | Rich context |
| System State | Halted | Continues operating |

## Summary

This demo proves that **proactive stack monitoring** combined with the Fault Tolerance API enables graceful recovery from stack overflow conditions. By detecting the problem before corruption occurs and safely unwinding the stack, systems can terminate faulty threads while continuing to operate with degraded functionality instead of requiring a complete halt or reboot.
