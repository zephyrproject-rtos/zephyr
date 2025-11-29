# Watchdog Bark Detection Test Application

## Overview

This application tests the fault tolerance framework's ability to detect and handle watchdog timer bark events. A watchdog bark is a pre-timeout warning that occurs before the watchdog timer expires (bite), allowing the system to take corrective action before a forced reset.

## Fault Description

Watchdog bark (FT_WATCHDOG_BARK) represents a warning condition that indicates:

- A critical thread has not fed the watchdog timer within the expected interval
- System responsiveness has degraded
- Potential deadlock or infinite loop detected
- Thread scheduling anomalies
- System overload condition

The bark event provides an opportunity for recovery before the watchdog forces a system reset.

## Detection Mechanism

The watchdog bark detection mechanism operates as follows:

1. Application configures watchdog with bark and bite timeouts
2. Critical threads periodically feed the watchdog timer
3. If timer is not fed before bark threshold, bark interrupt fires
4. Bark handler reports fault to fault tolerance framework
5. Recovery handler attempts to restore normal operation
6. If recovery fails, watchdog eventually expires causing system reset

## Recovery Strategy

Watchdog bark recovery strategies include:

1. Identify non-responsive threads causing the timeout
2. Log system state and thread activity
3. Attempt to reset or restart stuck threads
4. Reduce system load by suspending non-critical tasks
5. Feed the watchdog to prevent immediate reset
6. If recovery succeeds, resume normal operation
7. If recovery fails, allow watchdog to reset the system

## Test Implementation

The test application demonstrates watchdog bark handling by:

1. Registering a recovery handler for FT_WATCHDOG_BARK
2. Simulating a watchdog bark event with timeout information
3. Testing recovery handler invocation
4. Demonstrating watchdog feeding after recovery
5. Verifying statistics tracking

Note: This test simulates watchdog bark events. Integration with actual hardware watchdog timers would require additional platform-specific configuration.

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
west build -b qemu_cortex_m3 app/ft_watchdog_bark_test -p

# Incremental build
west build -b qemu_cortex_m3 app/ft_watchdog_bark_test

# Build and run
west build -b qemu_cortex_m3 app/ft_watchdog_bark_test && west build -t run
```

## Expected Output

Successful execution should produce output similar to:

```
*** Booting Zephyr OS ***
[00:00:00.000,000] <inf> ft_core: Fault tolerance subsystem initialized
[00:00:00.000,000] <inf> ft_watchdog_bark_test: Fault Tolerance Test: Watchdog Bark
[00:00:00.000,000] <inf> ft_core: Registered recovery handler for WATCHDOG_BARK
[00:00:02.010,000] <wrn> ft_watchdog_bark_test: SIMULATING WATCHDOG BARK
[00:00:02.010,000] <err> ft_core: FAULT DETECTED: Type=WATCHDOG_BARK, Severity=ERROR, Domain=SYSTEM
[00:00:02.010,000] <wrn> ft_watchdog_bark_test: === WATCHDOG BARK RECOVERY HANDLER ===
[00:00:02.010,000] <wrn> ft_watchdog_bark_test: Watchdog timeout imminent
[00:00:02.010,000] <inf> ft_watchdog_bark_test: Feeding watchdog to prevent reset
[00:00:02.020,000] <inf> ft_watchdog_bark_test: === TEST RESULT: PASS ===
```

## Configuration Options

The test uses the following Kconfig options:

- `CONFIG_FT_API_ENABLED=y` - Enable fault tolerance framework
- `CONFIG_FT_WATCHDOG_SUPPORT=y` - Enable watchdog monitoring
- `CONFIG_FT_EVENT_QUEUE_SIZE=16` - Event queue size
- `CONFIG_LOG=y` - Enable logging subsystem
- `CONFIG_LOG_MODE_DEFERRED=y` - Use deferred logging mode

## Success Criteria

The test is considered successful when:

1. Build completes without errors
2. Application boots and initializes fault tolerance framework
3. Watchdog bark recovery handler is registered successfully
4. Simulated watchdog bark is detected and reported
5. Recovery handler is invoked with timeout information
6. Recovery action is taken (watchdog feeding simulation)
7. Statistics show exactly 1 watchdog bark detected
8. Test completes showing successful recovery

## Limitations

This test has the following limitations:

1. Simulates watchdog bark without actual hardware timer
2. Does not test actual watchdog timeout and reset behavior
3. Recovery actions are simulated rather than real system operations
4. Does not test multi-watchdog scenarios
5. Platform-specific watchdog features not covered

## Future Enhancements

Potential improvements for this test:

1. Integration with Zephyr watchdog driver API
2. Test actual watchdog timeout and reset cycles
3. Multi-threaded watchdog feeding scenarios
4. Configurable bark/bite timeout thresholds
5. Persistent storage of watchdog events across resets
6. Watchdog channel multiplexing for multiple subsystems

## Fault Information

The watchdog bark event includes:

- **Fault Kind**: FT_WATCHDOG_BARK
- **Severity**: FT_SEV_ERROR (indicates correctable condition)
- **Domain**: FT_DOMAIN_SYSTEM (system-level monitoring)
- **Code**: Timeout value or watchdog channel identifier
- **Thread ID**: Thread responsible for feeding watchdog
- **Timestamp**: System uptime when bark occurred
- **Context**: Watchdog configuration and timeout information

## Related Documentation

- Zephyr Watchdog Driver API
- Watchdog Timer Hardware Reference
- Fault Tolerance Framework API Reference
- System Monitoring and Reliability Best Practices
