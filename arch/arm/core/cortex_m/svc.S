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
#include <zephyr/linker/sections.h>

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
    /* Build _callee_saved_t. To match the struct
     * definition we push the psp & then r11-r4
     */
    mrs r1, PSP
    push {r1, r2} /* r2 for padding */
#if defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
    push {r4-r11}
#elif defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
    mov  r1, r10
    mov  r2, r11
    push {r1, r2}
    mov  r1, r8
    mov  r2, r9
    push {r1, r2}
    push {r4-r7}
#else
#error Unknown ARM architecture
#endif
    mov  r1, sp /* pointer to _callee_saved_t */
#endif /* CONFIG_EXTRA_EXCEPTION_INFO */
    mov r2, lr /* EXC_RETURN */
    bl z_do_kernel_oops
    /* return from SVC exception is done here */
#if defined(CONFIG_EXTRA_EXCEPTION_INFO)
    /* We do not need to restore any register state here
     * because we did not use any callee-saved registers
     * in this routine. Therefore, we can just reset
     * the MSP to its value prior to entering the function
     */
    add sp, #40
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
/*
 * Build a privilege stack frame from the user stack frame, then switch PSP
 * to it. This ensures return from SVC does not rely on the user stack.
 *
 * Layout of privilege stack created from user stack:
 *
 *  +------+-------------------------+------+-------------------------+--------------------------+
 *  |  User stack                    |  Privilege stack               | Notes                    |
 *  +------+-------------------------+------+-------------------------+--------------------------+
 *  |Offset| contents                |Offset| contents                |                          |
 *  +------+-------------------------+------+-------------------------+--------------------------+
 *  |  0   | R0                   -> | 0    | R0                      | PSP switches from 0th    |
 *  |      |                         |      |                         | offset of user stack to  |
 *  |      |                         |      |                         | 0th offset of priv stack |
 *  |  4   | R1                   -> | 4    | R1                      |                          |
 *  |  8   | R2                   -> | 8    | R2                      |                          |
 *  | 12   | R3                   -> |12    | R3                      |                          |
 *  | 16   | R12                  -> |16    | R12                     |                          |
 *  | 20   | LR                   -> |20    | LR                      |                          |
 *  | 24   | Return Address      -x> |24    | z_arm_do_syscall        |return address from user  |
 *  |      |                         |      |                         |sf is not copied. Instead,|
 *  |      |                         |      |                         |it is replaced so that    |
 *  |      |                         |      |                         |z_arm_svc returns to      |
 *  |      |                         |      |                         |z_arm_do_syscall.         |
 *  |      |                         |      |                         |                          |
 *  | 28   | xPSR (w/ or w/o pad) -> |28    | xPSR (pad bit cleared)  |This completes the basic  |
 *  |      |                         |      |                         |exception sf w/ or w/o pad|
 *  |      |                         |      |                         |                          |
 *  | --   | FP regs + FPSCR      -> |--    | FP regs + FPSCR         |For arch supporting fp    |
 *  |      | (w/ or w/o pad)         |      |                         |context an additional     |
 *  |      |                         |      |                         |extended sf is copied.    |
 *  |________________________________|______|_________________________|__________________________|
 *  |      |                         |      |                         |On returning to           |
 *  |      |                         |      |                         |z_arm_do_syscall, the     |
 *  |      |                         |      |                         |above sf has already been |
 *  |      |                         |      |                         |unstacked and 8B from the |
 *  |      |                         |      |                         |then sf are used to pass  |
 *  |      |                         |      |                         |original pre-svc sp & the |
 *  |      |                         |      |                         |return address.           |
 *  |      |                         |      |                         |Note: at the moment       |
 *  |      |                         |      |                         |z_arm_do_syscall also     |
 *  |      |                         |      |                         |expects the return address|
 *  |      |                         |      |                         |to be set in r8.          |
 *  |      |                         |      |                         |                          |
 *  |      |                         | 0    | address that            |z_arm_do_syscall expects  |
 *  |      |                         |      | z_arm_do_syscall should |the original pre-svc sp at|
 *  |      |                         |      | set as PSP before       |0th offset i.e. new sp[0] |
 *  |      |                         |      | returning from svc.     |and,                      |
 *  |      |                         |      |                         |                          |
 *  |      |                         | 4    | Address that            |the return address at     |
 *  |      |                         |      | z_arm_do_syscall should |sp[4]. Note that this is  |
 *  |      |                         |      | return to after handling|the return address copied |
 *  |      |                         |      | svc                     |from user exception sf[24]|
 *  |      |                         |      |                         |which was not copied in   |
 *  |      |                         |      |                         |the previous sf.          |
 *  +------+-------------------------+------+-------------------------+--------------------------+
 * "sf" in this function is used as abbreviation for "stack frame".
 * Note that the "FP regs + FPSCR" are only present if CONFIG_FPU_SHARING=y, and the optional pad
 * is only present if PSP was not 8-byte aligned when SVC was executed.
 * Also note that FPU cannot be present in ARMv6-M or ARMv8-M Baseline implementations
 * (i.e., it may only be present when CONFIG_ARMV7_M_ARMV8_M_MAINLINE is enabled).
 */
 /* Start by fetching the top of privileged stack */
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
    ldr r1, =_kernel
    ldr r1, [r1, #_kernel_offset_to_current]
    adds r1, r1, #_thread_offset_to_priv_stack_start
    ldr r1, [r1] /* bottom of priv stack */
    ldr r3, =CONFIG_PRIVILEGED_STACK_SIZE
    subs r3, #(_EXC_HW_SAVED_BASIC_SF_SIZE+8) /* 8 for original sp and pc */
    add r1, r3
    mov ip, r1
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
    ldr ip, =_kernel
    ldr ip, [ip, #_kernel_offset_to_current]
    ldr ip, [ip, #_thread_offset_to_priv_stack_start] /* bottom of priv stack */
    add ip, #CONFIG_PRIVILEGED_STACK_SIZE
#ifdef CONFIG_FPU_SHARING
    /* Assess whether svc calling thread had been using the FP registers. */
    tst lr, #_EXC_RETURN_FTYPE_Msk
    ite eq
    moveq r8, #_EXC_HW_SAVED_EXTENDED_SF_SIZE
    movne r8, #_EXC_HW_SAVED_BASIC_SF_SIZE
#else
    mov r8, #_EXC_HW_SAVED_BASIC_SF_SIZE
#endif
    sub ip, #8 /* z_arm_do_syscall will use this to get original sp and pc */
    sub ip, r8 /* 32 for basic sf + 72 for the optional esf */
#endif

    /*
     * At this point:
     * r0 has PSP i.e. top of user stack
     * ip has top of privilege stack
     * r8 has hardware-saved stack frame size (only in case of mainline)
     */
    push {r4-r7}
    push {r2}
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
    mov  r2, r0 /* safe to use r2 since it is saved on MSP */

    /* Check for padding in the sf */
    ldr  r1, [r0, #_EXC_HW_SAVED_BASIC_SF_XPSR_OFFSET] /* grab xPSR from sf which has the pad bit */
    movs r3, #1
    /* Check if pad bit 9 is set */
    lsls r3, r3, #9
    tst  r1, r3
    beq  .L_no_padding
    /* special handling for padded sf */
    bics r1, r3 /* clear the pad bit (priv stack is aligned and doesn't need it) */
    adds r2, #4
.L_no_padding:
    /* Calculate original pre-svc user sp which is psp + sf size (+4B if pad bit was set) */
    adds r2, #_EXC_HW_SAVED_BASIC_SF_SIZE
    mov  r3, ip
    str r2,[r3, #0]

    /* Store the pre-SVC user SP at the offset expected by z_arm_do_syscall,
     * as detailed in the table above.
     */
    str r2,[r3, #_EXC_HW_SAVED_BASIC_SF_SIZE]
    /* sf of priv stack has the same xPSR as user stack but with 9th bit reset */
    str r1,[r3, #_EXC_HW_SAVED_BASIC_SF_XPSR_OFFSET]

    /* r0-r3, r12, LR from user stack sf are copied to sf of priv stack */
    mov r1, r0
    mov r2, r3
    ldmia r1!, {r4-r7}
    stmia r2!, {r4-r7}
    ldmia r1!, {r4-r5}
    stmia r2!, {r4-r5}

    /* Store the svc return address at the offset expected by z_arm_do_syscall,
     * as detailed in the table above.
     */
    str r5, [r3, #(_EXC_HW_SAVED_BASIC_SF_SIZE+4)]

    ldr r1, =z_arm_do_syscall
    str r1, [r3, #_EXC_HW_SAVED_BASIC_SF_RETADDR_OFFSET] /* Execution return to z_arm_do_syscall */
    ldr r1, [r0, #_EXC_HW_SAVED_BASIC_SF_RETADDR_OFFSET] /* grab address of PC from stack frame */
    /* Store the svc return address (i.e. next instr to svc) in r8 as expected by z_arm_do_syscall.
     */
    mov r8, r1

#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
    mov  r2, r0 /* safe to use r2 since it is saved on MSP */

    /* Calculate original pre-svc user sp without pad which is psp + sf size */
    add r2, r8

    /* Also, check for padding in the sf */
    ldr  r1, [r0, #_EXC_HW_SAVED_BASIC_SF_XPSR_OFFSET] /* grab xPSR from sf which has the pad bit */
    tst r1, #(1<<9) /* Check if pad bit 9 is set */
    beq .L_no_padding
    bics r1, #(1<<9) /* clear the pad bit (priv stack is aligned and doesn't need it) */
    /* Calculate original pre-svc user sp with pad */
    add  r2, #4
.L_no_padding:
    str r2,[ip, #0]
    /* Store the pre-SVC user SP at the offset expected by z_arm_do_syscall,
     * as detailed in the table above.
     */
    str r2,[ip, r8]
    str r1,[ip, #_EXC_HW_SAVED_BASIC_SF_XPSR_OFFSET] /* priv sf get user sf xPSR with bit9 reset */

    /* r0-r3, r12, LR from user stack sf are copied to sf of priv stack */
    mov r1, r0
    mov r2, ip
    ldmia r1!, {r4-r7}
    stmia r2!, {r4-r7}
    ldmia r1!, {r4-r5}
    stmia r2!, {r4-r5}

    /* Store the svc return address at the offset expected by z_arm_do_syscall,
     * as detailed in the table above.
     */
    add r8, #4
    str r5, [ip, r8]

    ldr r1, =z_arm_do_syscall
    str r1, [ip, #_EXC_HW_SAVED_BASIC_SF_RETADDR_OFFSET] /* Execution return to z_arm_do_syscall */
    ldr r1, [r0, #_EXC_HW_SAVED_BASIC_SF_RETADDR_OFFSET] /* grab address of PC from stack frame */
    /* Store the svc return address (i.e. next instr to svc) in r8 as expected by z_arm_do_syscall.
     */
    mov r8, r1

    /* basic stack frame is copied at this point to privilege stack,
     * now time to copy the fp context
     */
#ifdef CONFIG_FPU_SHARING
    tst lr, #_EXC_RETURN_FTYPE_Msk
    bne .L_skip_fp_copy
    add r1, r0, #32
    add r2, ip, #32

    vldmia r1!, {s0-s15}
    vstmia r2!, {s0-s15}

    /* copy FPSCR + reserved (8 bytes) */
    ldmia   r1!, {r4, r5}
    stmia   r2!, {r4, r5}
.L_skip_fp_copy:
#endif

#endif
    pop {r2} /* restore CONTROL value */
    pop {r4-r7}

    /* Point PSP to privilege stack,
     * note that r0 still has the old PSP
     */
    msr PSP, ip

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
    /* Set stack pointer limit (needed in privileged mode) */
    ldr ip, =_kernel
    ldr ip, [ip, #_kernel_offset_to_current]
    ldr ip, [ip, #_thread_offset_to_priv_stack_start]    /* priv stack ptr */
    msr PSPLIM, ip
#endif

    /* Take lock before returning from svc */
#if defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
    mov r0, #_EXC_IRQ_DEFAULT_PRIO
    msr BASEPRI, r0
#else
    cpsid i
#endif

    /* return from SVC to the modified LR - z_arm_do_syscall */
    bx lr
#endif /* CONFIG_USERSPACE */
