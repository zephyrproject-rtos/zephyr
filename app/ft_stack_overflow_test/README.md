# Fault Tolerance Test Application - Stack Overflow Detection

## Overview

This application demonstrates the fault tolerance framework's ability to detect and handle stack overflow faults. It intentionally triggers a stack overflow condition to validate the detection and recovery mechanisms.

## Fault Type: FT_STACK_OVERFLOW

### Description
Stack overflow occurs when a thread exceeds its allocated stack space, typically caused by:
- Deep recursion without proper base cases
- Large local variable allocations
- Infinite recursion
- Stack pointer corruption

### Detection Mechanism
- Zephyr's built-in stack canary values
- Hardware MPU (Memory Protection Unit) if available
- Custom fatal error handler integration

### Recovery Strategy
For stack overflow faults, recovery options are limited:
1. Log the fault event with thread information
2. Record fault in persistent storage
3. Attempt graceful degradation by terminating the faulty thread
4. System reboot as last resort if critical thread affected

## Application Structure

### Files
- `src/main.c` - Main application with stack overflow test
- `prj.conf` - Project configuration
- `README.md` - This documentation

### Test Scenario
1. Initialize fault tolerance subsystem
2. Register recovery handler for FT_STACK_OVERFLOW
3. Create a test thread with limited stack
4. Trigger recursive function to overflow stack
5. Verify fault detection and handler invocation
6. Log recovery attempt

## Building and Running

### Prerequisites
- Zephyr SDK installed
- West tool configured
- QEMU for ARM Cortex-M3 (or target hardware)

### Build Commands
```bash
cd /home/jack/ece553/zephyr/app/ft_stack_overflow_test
west build -b qemu_cortex_m3
```

### Run Commands
```bash
west build -t run
```

## Expected Output

The application should:
1. Initialize successfully
2. Start the overflow test thread
3. Detect stack overflow via fatal handler
4. Invoke custom recovery handler
5. Log fault details
6. Attempt recovery or controlled shutdown

## Configuration Options

Key configuration parameters in `prj.conf`:
- `CONFIG_FT_API_ENABLED=y` - Enable fault tolerance framework
- `CONFIG_FT_STACK_OVERFLOW_DETECT=y` - Enable stack overflow detection
- `CONFIG_THREAD_STACK_INFO=y` - Enable stack usage tracking
- `CONFIG_THREAD_MONITOR=y` - Enable thread monitoring

## Success Criteria

The test passes if:
1. Stack overflow is detected
2. Custom fatal handler is invoked
3. Recovery handler receives FT_STACK_OVERFLOW event
4. Fault statistics are updated correctly
5. System behavior is predictable and logged

## Limitations

Current implementation limitations:
- Recovery from stack overflow is generally not possible
- System reboot is often the only safe option
- Thread termination may not work if stack is severely corrupted

## Future Enhancements

Potential improvements:
- Stack usage monitoring and warnings before overflow
- Dynamic stack size adjustment
- Better thread isolation with MPU
- Pre-fault detection through stack watermark monitoring
