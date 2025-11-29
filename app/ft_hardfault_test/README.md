# Hard Fault Detection Test Application

## Overview

This application tests the fault tolerance framework's ability to detect and handle hard fault conditions. Hard faults are critical processor exceptions that occur due to memory access violations, illegal instructions, bus faults, or other severe runtime errors.

## Fault Description

Hard faults (FT_HARDFAULT) represent critical processor-level exceptions that indicate:

- Memory access violations (invalid memory addresses)
- Unaligned memory access on architectures that require alignment
- Execution of illegal or undefined instructions
- Bus faults during memory transactions
- Usage faults (division by zero, unaligned access, etc.)
- Stack pointer corruption

## Detection Mechanism

The hard fault detection mechanism relies on ARM Cortex-M exception handling:

1. Processor detects a fault condition during instruction execution
2. Hardware automatically saves context to the stack
3. Exception handler is invoked (HardFault_Handler)
4. Fault status registers are examined to determine the cause
5. Fault event is reported to the fault tolerance framework
6. Recovery handler is invoked to attempt remediation

## Recovery Strategy

Hard faults are typically unrecoverable at the thread level. The recovery strategy includes:

1. Capture fault information from processor registers
2. Log fault details (PC, fault status, fault address)
3. Identify the faulting thread
4. Terminate the faulting thread if possible
5. Report system reboot requirement if thread termination is insufficient
6. Optionally save diagnostic information before reboot

## Test Implementation

The test application demonstrates hard fault handling by:

1. Registering a recovery handler for FT_HARDFAULT
2. Creating a test thread that deliberately triggers a hard fault
3. Simulating hard fault scenarios:
   - Null pointer dereference
   - Invalid memory address access
   - Execution of illegal instruction (optional)
4. Verifying the recovery handler is invoked
5. Checking statistics to confirm fault detection

Note: Due to the critical nature of hard faults, this test simulates the fault reporting rather than triggering an actual processor exception, which would halt the system.

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
west build -b qemu_cortex_m3 app/ft_hardfault_test -p

# Incremental build
west build -b qemu_cortex_m3 app/ft_hardfault_test

# Build and run
west build -b qemu_cortex_m3 app/ft_hardfault_test && west build -t run
```

## Expected Output

Successful execution should produce output similar to:

```
*** Booting Zephyr OS ***
[00:00:00.000,000] <inf> ft_core: Fault tolerance subsystem initialized
[00:00:00.000,000] <inf> ft_hardfault_test: Fault Tolerance Test: Hard Fault
[00:00:00.000,000] <inf> ft_hardfault_test: Registering hard fault recovery handler...
[00:00:00.000,000] <inf> ft_core: Registered recovery handler for HARDFAULT
[00:00:02.010,000] <wrn> ft_hardfault_test: SIMULATING HARD FAULT
[00:00:02.010,000] <err> ft_core: FAULT DETECTED: Type=HARDFAULT, Severity=FATAL, Domain=HARDWARE
[00:00:02.010,000] <err> ft_hardfault_test: === HARD FAULT RECOVERY HANDLER ===
[00:00:02.010,000] <err> ft_hardfault_test: Fault cause: Memory access violation
[00:00:02.020,000] <inf> ft_hardfault_test: === TEST COMPLETED ===
[00:00:02.020,000] <inf> ft_hardfault_test: Total faults: 1
[00:00:02.020,000] <inf> ft_hardfault_test: Hard fault count: 1
```

## Configuration Options

The test uses the following Kconfig options:

- `CONFIG_FT_API_ENABLED=y` - Enable fault tolerance framework
- `CONFIG_FT_EVENT_QUEUE_SIZE=16` - Event queue size
- `CONFIG_FT_WORKER_STACK_SIZE=2048` - Worker thread stack size
- `CONFIG_LOG=y` - Enable logging subsystem
- `CONFIG_LOG_MODE_DEFERRED=y` - Use deferred logging mode
- `CONFIG_THREAD_STACK_INFO=y` - Enable thread stack information
- `CONFIG_DEBUG_INFO=y` - Include debug symbols

## Success Criteria

The test is considered successful when:

1. Build completes without errors
2. Application boots and initializes fault tolerance framework
3. Hard fault recovery handler is registered successfully
4. Simulated hard fault is detected and reported
5. Recovery handler is invoked with correct fault information
6. Statistics show exactly 1 hard fault detected
7. Test completes without hanging or crashing unexpectedly

## Limitations

This test has the following limitations:

1. Simulates hard fault reporting rather than triggering actual processor exceptions
2. Actual hard fault would halt the system before recovery can occur
3. Does not test all possible hard fault causes (division by zero, etc.)
4. Recovery options are limited - most hard faults require system reboot
5. Does not test hard fault priority and nesting behavior

## Future Enhancements

Potential improvements for this test:

1. Add fault injection for specific hard fault types
2. Test hard fault information extraction from processor registers
3. Implement thread-level recovery (isolate and restart faulting thread)
4. Add diagnostic data collection and persistent storage
5. Test integration with external watchdog for hard fault recovery
6. Implement safe mode boot after hard fault detection

## Fault Information

The hard fault event includes:

- **Fault Kind**: FT_HARDFAULT
- **Severity**: FT_SEV_FATAL (indicates system cannot continue normally)
- **Domain**: FT_DOMAIN_HARDWARE (processor-level fault)
- **Code**: Fault status register value or fault type identifier
- **Thread ID**: ID of the thread that caused the fault
- **Timestamp**: System uptime when fault occurred
- **Context**: Additional fault information (PC, LR, fault address)

## Related Documentation

- Zephyr RTOS Exception Handling
- ARM Cortex-M Fault Status Registers
- Fault Tolerance Framework API Reference
- FT_STACK_OVERFLOW Test Application
