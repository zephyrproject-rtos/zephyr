# Deadlock Detection Test Application

## Overview

This application tests the fault tolerance framework's ability to detect and handle deadlock conditions between threads. Deadlocks occur when two or more threads are blocked indefinitely, each waiting for resources held by the other, creating a circular dependency.

## Fault Description

Deadlock detection (FT_DEADLOCK_DETECTED) identifies situations where:

- Multiple threads are waiting for mutually-held resources
- Circular lock acquisition patterns exist
- Thread priority inversion causes indefinite blocking
- Resource starvation prevents forward progress
- Livelock conditions where threads remain active but make no progress

## Detection Mechanism

The deadlock detection mechanism operates through:

1. Resource allocation graph analysis
2. Lock acquisition order tracking
3. Thread wait state monitoring
4. Timeout-based detection for blocked threads
5. Cycle detection in resource dependencies
6. Reporting when circular wait conditions are identified

## Recovery Strategy

Deadlock recovery strategies include:

1. Identify threads involved in the deadlock cycle
2. Select a victim thread to break the deadlock
3. Terminate or restart the victim thread
4. Release resources held by the victim
5. Allow remaining threads to proceed
6. Log deadlock information for analysis
7. Implement lock ordering policies to prevent recurrence

## Test Implementation

The test application demonstrates deadlock handling by:

1. Registering a recovery handler for FT_DEADLOCK_DETECTED
2. Creating multiple threads with mutex resources
3. Simulating a deadlock scenario with circular lock dependencies
4. Detecting the deadlock condition
5. Invoking recovery to break the deadlock
6. Verifying successful resolution or controlled termination

Note: This test simulates deadlock detection and reporting. Actual deadlock scenarios would require thread termination or resource release to resolve.

## Build Instructions

### Prerequisites

Ensure the Zephyr development environment is configured:

```bash
source /home/jack/ece553/.venv/bin/activate
cd /home/jack/ece553/zephyr
```

### Build Commands

```bash
# Clean build
west build -b qemu_cortex_m3 app/ft_deadlock_detected_test -p

# Incremental build
west build -b qemu_cortex_m3 app/ft_deadlock_detected_test

# Build and run
west build -b qemu_cortex_m3 app/ft_deadlock_detected_test && west build -t run
```

## Expected Output

Successful execution should produce output similar to:

```
*** Booting Zephyr OS ***
[00:00:00.000,000] <inf> ft_core: Fault tolerance subsystem initialized
[00:00:00.000,000] <inf> ft_deadlock_test: Fault Tolerance Test: Deadlock Detection
[00:00:00.000,000] <inf> ft_core: Registered recovery handler for DEADLOCK_DETECTED
[00:00:02.010,000] <wrn> ft_deadlock_test: SIMULATING DEADLOCK CONDITION
[00:00:02.010,000] <err> ft_core: FAULT DETECTED: Type=DEADLOCK_DETECTED, Severity=CRITICAL, Domain=SYSTEM
[00:00:02.010,000] <err> ft_deadlock_test: === DEADLOCK RECOVERY HANDLER ===
[00:00:02.010,000] <err> ft_deadlock_test: Deadlock cycle detected: Thread A -> Thread B -> Thread A
[00:00:02.010,000] <wrn> ft_deadlock_test: Breaking deadlock by terminating victim thread
[00:00:02.020,000] <inf> ft_deadlock_test: === TEST RESULT: PASS ===
```

## Configuration Options

The test uses the following Kconfig options:

- `CONFIG_FT_API_ENABLED=y` - Enable fault tolerance framework
- `CONFIG_FT_DEADLOCK_DETECT=y` - Enable deadlock detection
- `CONFIG_FT_EVENT_QUEUE_SIZE=16` - Event queue size
- `CONFIG_LOG=y` - Enable logging subsystem
- `CONFIG_LOG_MODE_DEFERRED=y` - Use deferred logging mode
- `CONFIG_THREAD_STACK_INFO=y` - Thread stack information

## Success Criteria

The test is considered successful when:

1. Build completes without errors
2. Application boots and initializes fault tolerance framework
3. Deadlock recovery handler is registered successfully
4. Simulated deadlock is detected and reported
5. Recovery handler is invoked with thread dependency information
6. Recovery action is taken (victim thread selection)
7. Statistics show exactly 1 deadlock detected
8. Test completes showing recovery attempted

## Limitations

This test has the following limitations:

1. Simulates deadlock detection rather than creating actual deadlocks
2. Does not test all deadlock scenarios (lock ordering, priority inversion)
3. Recovery is simulated rather than actual thread termination
4. Does not verify complete system recovery after deadlock resolution
5. Limited to simple circular dependency patterns

## Future Enhancements

Potential improvements for this test:

1. Actual deadlock creation with mutex resources
2. Automatic deadlock detection using resource graphs
3. Multiple deadlock resolution strategies
4. Integration with kernel thread monitoring
5. Lock ordering enforcement policies
6. Deadlock prevention through resource allocation strategies

## Fault Information

The deadlock event includes:

- **Fault Kind**: FT_DEADLOCK_DETECTED
- **Severity**: FT_SEV_CRITICAL (indicates serious system condition)
- **Domain**: FT_DOMAIN_SYSTEM (system-level resource management)
- **Code**: Deadlock type or resource identifier
- **Thread ID**: First thread involved in deadlock cycle
- **Timestamp**: System uptime when deadlock detected
- **Context**: Thread dependency graph and lock information

## Related Documentation

- Zephyr RTOS Threading and Synchronization
- Deadlock Prevention and Detection Algorithms
- Resource Allocation Graph Theory
- Fault Tolerance Framework API Reference
- System Concurrency Best Practices
