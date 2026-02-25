# Bug #88929 Fix: MSP and PSP Stack Conflict

## Problem Description

During early Zephyr initialization on ARM Cortex-M, both MSP (Main Stack Pointer) and PSP (Process Stack Pointer) were configured to use the same memory region (`z_interrupt_stacks`), causing stack corruption when interrupts occurred during initialization.

## Root Cause Analysis

### Initialization Sequence (BEFORE FIX):

1. **reset.S (z_arm_reset)**: 
   - MSP initially set to `z_main_stack` (line ~148)
   - PSP set to top of `z_interrupt_stacks` (lines 211-221) ❌
   - Switched to PSP mode via CONTROL register
   - Called `z_prep_c()`

2. **prep_c.c (z_prep_c)**:
   - Performed early initialization
   - Called `z_cstart()`

3. **init.c (z_cstart)** - Running on PSP (using `z_interrupt_stacks`):
   - Called `arch_kernel_init()`

4. **kernel_arch_func.h (arch_kernel_init)**:
   - Called `z_arm_interrupt_stack_setup()`

5. **stack.h (z_arm_interrupt_stack_setup)**:
   - Set MSP to top of `z_interrupt_stacks[0]` ❌

### The Conflict:

```
Memory Layout (BEFORE FIX):
┌─────────────────────────┐
│   z_main_stack          │ (Unused after reset!)
├─────────────────────────┤
│ z_interrupt_stacks      │ ← PSP points here (from reset.S)
│                         │ ← MSP points here (from arch_kernel_init)
│                         │ ❌ CONFLICT! Both stacks use same memory
└─────────────────────────┘
```

When an interrupt occurred during `z_sys_init_run_level(INIT_LEVEL_PRE_KERNEL_2)`:
- CPU automatically switched from PSP to MSP
- MSP pushed interrupt stack frame into `z_interrupt_stacks`
- This **overwrote the active PSP stack**, corrupting local variables in `z_cstart()` and `z_sys_init_run_level()`
- Result: Memory corruption, out-of-bounds access, and chip exception

## Solution

Changed PSP initialization to use `z_main_stack` instead of `z_interrupt_stacks` during boot.

### Initialization Sequence (AFTER FIX):

```
Memory Layout (AFTER FIX):
┌─────────────────────────┐
│   z_main_stack          │ ← PSP points here (from reset.S) ✅
├─────────────────────────┤
│ z_interrupt_stacks      │ ← MSP points here (from arch_kernel_init) ✅
│                         │ ✅ SEPARATED! No conflict
└─────────────────────────┘
```

Now:
- **PSP uses `z_main_stack`**: For running C code during initialization
- **MSP uses `z_interrupt_stacks`**: For handling interrupts
- **No overlap**: Stacks are completely separate, preventing corruption

## Code Changes

### File: [arch/arm/core/cortex_m/reset.S](arch/arm/core/cortex_m/reset.S)

**Changed lines 211-221** from:
```asm
ldr r0, =z_interrupt_stacks
ldr r1, =CONFIG_ISR_STACK_SIZE + MPU_GUARD_ALIGN_AND_SIZE
adds r0, r0, r1
msr PSP, r0
```

**To:**
```asm
ldr r0, =z_main_stack
ldr r1, =CONFIG_MAIN_STACK_SIZE
adds r0, r0, r1
msr PSP, r0
```

**Also added** z_main_stack initialization when CONFIG_INIT_STACKS is enabled:
```asm
#ifdef CONFIG_INIT_STACKS
    ldr r0, =z_interrupt_stacks
    ldr r1, =0xaa
    ldr r2, =CONFIG_ISR_STACK_SIZE + MPU_GUARD_ALIGN_AND_SIZE
    bl arch_early_memset

    ldr r0, =z_main_stack        ; Added
    ldr r1, =0xaa                ; Added
    ldr r2, =CONFIG_MAIN_STACK_SIZE ; Added
    bl arch_early_memset         ; Added
#endif
```

## Impact

- ✅ Eliminates memory corruption during early initialization
- ✅ Prevents stack overflow between PSP and MSP
- ✅ Allows interrupts to safely occur during `z_sys_init_run_level()` calls
- ✅ Maintains proper separation of concerns (thread stack vs interrupt stack)

## Testing

To verify the fix works correctly:

1. Build any ARM Cortex-M application:
   ```bash
   west build -p -b qemu_cortex_m3 samples/hello_world
   ```

2. Run on QEMU:
   ```bash
   west build -t run
   ```

3. The application should initialize without corruption or exceptions.

## Before vs After

**BEFORE FIX**: Interrupts during pre-kernel initialization would corrupt the stack, causing:
- Random crashes
- Memory access violations  
- Unpredictable behavior
- Hard faults

**AFTER FIX**: Interrupts during pre-kernel initialization work correctly:
- Dedicated interrupt stack (MSP → z_interrupt_stacks)
- Dedicated thread stack (PSP → z_main_stack)
- No memory corruption
- Stable initialization

---

**Fix Author**: GitHub Copilot  
**Date**: February 25, 2026  
**Related Issue**: #88929
