<!--
SPDX-License-Identifier: Apache-2.0
SPDX-FileCopyrightText: Copyright (c) 2025 Arm Limited (or its affiliates)
-->

# ARM64 Neon / SVE2 SIMD Context Switch Stress Test

## Overview

This test validates the correctness and resilience of SIMD register save/restore
during frequent thread preemptions in Zephyr RTOS on ARM AArch64 platforms using
Neon or SVE2 instructions.

The test performs mixed workloads (F32 matrix x vector multiplication and
complex number multiplication) across multiple threads, while a high-priority
preemptor thread intentionally clobbers SIMD registers to simulate
context-switch interference.

## Features

- Supports both ARM Neon and SVE2 instruction sets
- Multi-threaded SIMD workloads using Zephyr's kernel threading API
- Preemption stress through a high-priority thread that:
  - Forces frequent context switches
  - Clobbers SIMD/SVE registers to simulate register save/restore errors
- Data integrity validation:
  - Compares SIMD results against scalar reference implementations
  - Detects register corruption and computational divergence
- Configurable test size, tolerance, and timing

## Test Architecture

### Thread Types

- **SIMD Worker threads**: Execute either matrix x vector or complex multiplication
  tests using SIMD instructions
- **Preemptor Thread**: Runs at higher priority, forces preemptions and clobbers
  SIMD registers

### Workload Distribution

- 8 total worker threads (`SIMD_THREAD_CNT = 8`)
  - 4 matrix x vector tests (`MATRIX_TEST_CTX = 4`)
  - 4 complex multiplication tests (`CMPLX_MUL_TEST_CTX = 4`)

### Test Flow

1. Worker threads initialize their test data with unique per-thread seeds
2. Each worker computes a reference result using scalar operations
3. Workers continuously execute SIMD computations in a loop
4. The high-priority preemptor thread:
   - Clobbers all SIMD registers every 1ms
   - Forces context switches via timeslicing
   - Periodically prints statistics
5. Workers validate SIMD results against reference after each iteration
6. Test tracks context switches that occur during SIMD operations
7. Test runs for `TEST_DURATION_MSEC` (10 seconds by default)
8. Final assertions verify zero failures and successful context switching

## Build and Run

### Prerequisites

- Zephyr RTOS environment set up with an AArch64 toolchain
- Target board with Neon or SVE2 SIMD support (e.g., ARMv8-A, ARMv9 cores)
- FVP model or physical hardware

### Build with west

```bash
west build -b fvp_base_revc_2xaem/v8a tests/arch/arm64/arm64_sve_stress
```

For ARMv9 with SVE:
```bash
west build -b fvp_base_revc_2xaem/v9a tests/arch/arm64/arm64_sve_stress
```

### Run with west

```bash
west build -t run
```

### Run with Twister

```bash
west twister -T tests/arch/arm64/arm64_sve_stress/
```

### Expected Output

```
=== ARM Neon/SVE2 SIMD context switch stress test ===
task 0 succ 5743 fail 0 switch_during_simd 23936
task 1 succ 5744 fail 0 switch_during_simd 24000
task 2 succ 5744 fail 0 switch_during_simd 24084
task 3 succ 5744 fail 0 switch_during_simd 24114
task 4 succ 3559 fail 0 switch_during_simd 5162
task 5 succ 3559 fail 0 switch_during_simd 5166
task 6 succ 3559 fail 0 switch_during_simd 5168
task 7 succ 3556 fail 0 switch_during_simd 5162
...
=== Final Test Results ===
task 0: success=11493 failures=0 switches_during_simd=47968
task 1: success=11494 failures=0 switches_during_simd=47944
task 2: success=11494 failures=0 switches_during_simd=47820
task 3: success=11494 failures=0 switches_during_simd=48164
task 4: success=7122 failures=0 switches_during_simd=10346
task 5: success=7122 failures=0 switches_during_simd=10350
task 6: success=7122 failures=0 switches_during_simd=10352
task 7: success=7118 failures=0 switches_during_simd=10282
Total: success=75059 failures=0 switches=235826
```

If any test fails, the program halts with error diagnostics showing the
mismatched SIMD vs scalar results.

## Test Success Criteria

The test passes if:
1. All SIMD operations complete with zero failures (`total_failures == 0`)
2. At least one successful SIMD operation was completed (`total_success > 0`)
3. At least one context switch occurred during SIMD operations (`total_switches > 0`)

This ensures that:
- SIMD context switching is functioning correctly
- Register clobbering by the preemptor thread is properly handled
- No computational errors occur despite aggressive context switching
