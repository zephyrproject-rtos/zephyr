# Memory Corruption Detection Test Application

## Overview

This application tests the fault tolerance framework's ability to detect and handle memory corruption conditions including stack canary violations, heap corruption, and buffer overflows.

## Fault Description

Memory corruption (FT_MEM_CORRUPTION) identifies situations where:

- Stack canary values are overwritten
- Heap metadata is corrupted
- Buffer overflows damage adjacent memory
- Use-after-free conditions occur
- Memory region checksums fail validation

## Detection Mechanism

Memory corruption detection uses:

1. Stack canary checking before function returns
2. Heap metadata validation during allocations
3. Periodic memory region checksum verification
4. Boundary checking on critical buffers
5. Memory access pattern monitoring

## Recovery Strategy

Memory corruption recovery includes:

1. Identify corrupted memory regions
2. Determine the extent of corruption
3. Isolate affected subsystems
4. Restore from known-good state if possible
5. Terminate corrupted processes
6. Log corruption details for forensic analysis
7. Reboot if corruption is widespread

## Build Instructions

```bash
west build -b qemu_cortex_m3 app/ft_mem_corruption_test -p && west build -t run
```

## Expected Output

```
[00:00:02.010,000] <err> ft_core: FAULT DETECTED: Type=MEM_CORRUPTION, Severity=CRITICAL
[00:00:02.010,000] <err> ft_mem_corruption_test: Memory region corrupted: stack_canary
[00:00:02.010,000] <inf> ft_mem_corruption_test: === TEST RESULT: PASS ===
```

## Configuration

- `CONFIG_FT_API_ENABLED=y`
- `CONFIG_FT_MEMORY_CORRUPTION_DETECT=y`
- `CONFIG_STACK_SENTINEL=y` (for canary checking)

## Success Criteria

1. Memory corruption detected and reported
2. Recovery handler invoked with corruption details
3. Statistics show 1 memory corruption fault
4. Test completes successfully
