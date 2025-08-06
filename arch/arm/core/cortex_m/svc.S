/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 * Copyright (c) 2017-2019 Nordic Semiconductor ASA.
 * Copyright (c) 2020 Stephanos Ioannidis <root@stephanos.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SVC exception handling for ARM Cortex-M
 *
 * This module implements the z_arm_svc exception vector on ARM Cortex-M CPUs.
 */

#include <zephyr/toolchain.h>
#include <offsets_short.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/syscall.h>

_ASM_FILE_PROLOGUE

GTEXT(z_arm_svc)
GTEXT(z_do_kernel_oops)
#if defined(CONFIG_USERSPACE)
GTEXT(z_arm_do_syscall)
#endif

GDATA(_kernel)

/**
 *
 * @brief Service call handler
 *
 * The service call (svc) is used in the following occasions:
 * - IRQ offloading
 * - Kernel run-time exceptions
 * - System Calls (User mode)
 *
 */
SECTION_FUNC(TEXT, z_arm_svc)
  /* Use EXC_RETURN state to find out if stack frame is on the
   * MSP or PSP
   */
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
  movs r0, #_EXC_RETURN_SPSEL_Msk
  mov r1, lr
  tst r1, r0
  beq .L_stack_frame_msp
  mrs r0, PSP
  bne .L_stack_frame_endif
.L_stack_frame_msp:
  mrs r0, MSP
.L_stack_frame_endif:
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
    tst lr, #_EXC_RETURN_SPSEL_Msk /* did we come from thread mode ? */
    ite eq  /* if zero (equal), came from handler mode */
        mrseq r0, MSP   /* handler mode, stack frame is on MSP */
        mrsne r0, PSP   /* thread mode, stack frame is on PSP */
#endif


    /* Figure out what SVC call number was invoked */

    ldr r1, [r0, #24]   /* grab address of PC from stack frame */
    /* SVC is a two-byte instruction, point to it and read the
     * SVC number (lower byte of SCV instruction)
     */
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
    subs r1, r1, #2
    ldrb r1, [r1]
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
    ldrb r1, [r1, #-2]
#endif

   /*
    * grab service call number:
    * 0: Unused
    * 1: irq_offload (if configured)
    * 2: kernel panic or oops (software generated fatal exception)
    * 3: System call (if user mode supported)
    */
#if defined(CONFIG_USERSPACE)
    mrs r2, CONTROL

    cmp r1, #3
    beq .L_do_syscall

    /*
     * check that we are privileged before invoking other SVCs
     * oops if we are unprivileged
     */
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
    movs r3, #0x1
    tst r2, r3
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
    tst r2, #0x1
#endif
    bne .L_oops

#endif /* CONFIG_USERSPACE */

    cmp r1, #2
    beq .L_oops

#if defined(CONFIG_IRQ_OFFLOAD)
    push {r0, lr}
    bl z_irq_do_offload  /* call C routine which executes the offload */
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
    pop {r0, r3}
    mov lr, r3
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
    pop {r0, lr}
#endif

#ifdef CONFIG_USE_SWITCH
    /* FIXME: this uses the pre-hook LR, but we want to refetch the
     * one in the stack frame, otherwise irq_offload() won't context
     * switch correctly (which... isn't tested?)
     */
    push {r0, lr}
    bl arm_m_legacy_exit
    pop {r0, lr}
    bx lr
#else
    ldr r0, =z_arm_int_exit
    bx r0
#endif

#endif

.L_oops:
    push {r0, lr}
#if defined(CONFIG_EXTRA_EXCEPTION_INFO)
#if defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
    /* Build _callee_saved_t. To match the struct
     * definition we push the psp & then r11-r4
     */
    mrs r1, PSP
    push {r1, r2}
    push {r4-r11}
    mov  r1, sp /* pointer to _callee_saved_t */
#endif /* CONFIG_ARMV7_M_ARMV8_M_MAINLINE */
#endif /* CONFIG_EXTRA_EXCEPTION_INFO */
    mov r2, lr /* EXC_RETURN */
    bl z_do_kernel_oops
    /* return from SVC exception is done here */
#if defined(CONFIG_EXTRA_EXCEPTION_INFO)
#if defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
    /* We do not need to restore any register state here
     * because we did not use any callee-saved registers
     * in this routine. Therefore, we can just reset
     * the MSP to its value prior to entering the function
     */
    add sp, #40
#endif /* CONFIG_ARMV7_M_ARMV8_M_MAINLINE */
#endif /* CONFIG_EXTRA_EXCEPTION_INFO */

#ifdef CONFIG_USE_SWITCH
    bl arm_m_legacy_exit
#endif

    pop {r0, pc}

#if defined(CONFIG_USERSPACE)
    /*
     * System call will setup a jump to the z_arm_do_syscall() function
     * when the SVC returns via the bx lr.
     *
     * There is some trickery involved here because we have to preserve
     * the original PC value so that we can return back to the caller of
     * the SVC.
     *
     * On SVC exeption, the stack looks like the following:
     * r0 - r1 - r2 - r3 - r12 - LR - PC - PSR
     *
     * Registers look like:
     * r0 - arg1
     * r1 - arg2
     * r2 - arg3
     * r3 - arg4
     * r4 - arg5
     * r5 - arg6
     * r6 - call_id
     * r8 - saved link register
     */
.L_do_syscall:
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
    movs r3, #24
    ldr r1, [r0, r3]   /* grab address of PC from stack frame */
    mov r8, r1
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
    ldr r8, [r0, #24]   /* grab address of PC from stack frame */
#endif
    ldr r1, =z_arm_do_syscall
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
    str r1, [r0, r3]   /* overwrite the PC to point to z_arm_do_syscall */
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
    str r1, [r0, #24]   /* overwrite the PC to point to z_arm_do_syscall */
#endif

#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
    ldr r3, =K_SYSCALL_LIMIT
    cmp r6, r3
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
    /* validate syscall limit */
    ldr ip, =K_SYSCALL_LIMIT
    cmp r6, ip
#endif
    /* The supplied syscall_id must be lower than the limit
     * (Requires unsigned integer comparison)
     */
    blo .L_valid_syscall_id

    /* bad syscall id.  Set arg1 to bad id and set call_id to SYSCALL_BAD */
    str r6, [r0]
    ldr r6, =K_SYSCALL_BAD

    /* Bad syscalls treated as valid syscalls with ID K_SYSCALL_BAD. */

.L_valid_syscall_id:
    ldr r0, =_kernel
    ldr r0, [r0, #_kernel_offset_to_current]
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
    mov ip, r2
    ldr r1, =_thread_offset_to_mode
    ldr r3, [r0, r1]
    movs r2, #1
    bics r3, r2
    /* Store (privileged) mode in thread's mode state variable */
    str r3, [r0, r1]
    mov r2, ip
    dsb
    /* set mode to privileged, r2 still contains value from CONTROL */
    movs r3, #1
    bics r2, r3
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
    ldr r1, [r0, #_thread_offset_to_mode]
    bic r1, #1
    /* Store (privileged) mode in thread's mode state variable */
    str r1, [r0, #_thread_offset_to_mode]
    dsb
    /* set mode to privileged, r2 still contains value from CONTROL */
    bic r2, #1
#endif
    msr CONTROL, r2

    /* ISB is not strictly necessary here (stack pointer is not being
     * touched), but it's recommended to avoid executing pre-fetched
     * instructions with the previous privilege.
     */
    isb

#if defined(CONFIG_BUILTIN_STACK_GUARD)
    /* Thread is now in privileged mode; after returning from SCVall it
     * will use the default (user) stack before switching to the privileged
     * stack to execute the system call. We need to protect the user stack
     * against stack overflows until this stack transition.
     */
    ldr r1, [r0, #_thread_offset_to_stack_info_start]    /* stack_info.start */
    msr PSPLIM, r1
#endif /* CONFIG_BUILTIN_STACK_GUARD */

    /* return from SVC to the modified LR - z_arm_do_syscall */
    bx lr
#endif /* CONFIG_USERSPACE */
