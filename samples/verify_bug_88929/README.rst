# Bug #88929 Fix Verification Sample

## Overview

This sample application verifies that Bug #88929 (MSP/PSP Stack Conflict) has been properly fixed.

## The Bug

During early Zephyr initialization on ARM Cortex-M:
- PSP (Process Stack Pointer) was set to `z_interrupt_stacks`
- MSP (Main Stack Pointer) was also set to `z_interrupt_stacks`
- When interrupts occurred during `z_sys_init_run_level(INIT_LEVEL_PRE_KERNEL_2)`, the MSP stack overwrote the PSP stack
- This caused memory corruption and system crashes

## The Fix

Changed `arch/arm/core/cortex_m/reset.S` to:
- Set PSP to `z_main_stack` instead of `z_interrupt_stacks`
- Keep MSP pointing to `z_interrupt_stacks`
- This separates the stacks and prevents corruption

## What This Sample Tests

1. Registers an initialization function at `PRE_KERNEL_2` level (where the bug occurred)
2. Uses stack canary values to detect corruption
3. Allows interrupts to occur during initialization
4. Verifies that stack canaries remain intact (no corruption)
5. Reports results

## Building

```bash
west build -b qemu_cortex_m3 samples/verify_bug_88929
west build -t run
```

Or for real hardware:
```bash
west build -b nucleo_f103rb samples/verify_bug_88929
west flash
```

## Expected Output

```
========================================
Bug #88929 Fix Verification Test
MSP/PSP Stack Conflict Resolution
========================================

PRE_KERNEL_2 init: Testing interrupt during initialization
  Stack canary address: 0x20000xyz
  Stack canary value: 0xDEADBEEF
  Stack canary after interrupts: 0xDEADBEEF ✅
PRE_KERNEL_2 init: Complete (No corruption detected)

Test Results:
----------------------------------------
✅ Global stack canary intact: 0xDEADBEEF
✅ System initialization completed
✅ No memory corruption detected
✅ MSP and PSP stacks properly separated

========================================
Fix Explanation:
========================================
BEFORE FIX:
  PSP = z_interrupt_stacks (WRONG)
  MSP = z_interrupt_stacks (WRONG)
  Result: Stack corruption ❌

AFTER FIX:
  PSP = z_main_stack (CORRECT)
  MSP = z_interrupt_stacks (CORRECT)
  Result: No corruption ✅

========================================
TEST PASSED ✅
========================================
```

## Supported Boards

This sample works on all ARM Cortex-M boards:
- qemu_cortex_m3
- nucleo_f103rb
- stm32f4_disco
- nrf52840dk
- And any other ARM Cortex-M board
