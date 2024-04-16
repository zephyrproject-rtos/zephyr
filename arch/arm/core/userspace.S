/*
 * Userspace and service handler hooks
 *
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <offsets_short.h>
#include <zephyr/syscall.h>

#include <zephyr/arch/arm/exc.h>

#if defined(CONFIG_CPU_AARCH32_CORTEX_R)
#include <zephyr/arch/cpu.h>
#endif

_ASM_FILE_PROLOGUE

GTEXT(z_arm_userspace_enter)
GTEXT(z_arm_do_syscall)
GTEXT(arch_user_string_nlen)
GTEXT(z_arm_user_string_nlen_fault_start)
GTEXT(z_arm_user_string_nlen_fault_end)
GTEXT(z_arm_user_string_nlen_fixup)
GDATA(_kernel)

/* Imports */
GDATA(_k_syscall_table)

/**
 *
 * User space entry function
 *
 * This function is the entry point to user mode from privileged execution.
 * The conversion is one way, and threads which transition to user mode do
 * not transition back later, unless they are doing system calls.
 *
 * The function is invoked as:
 * z_arm_userspace_enter(user_entry, p1, p2, p3,
 *                        stack_info.start, stack_info.size);
 */
SECTION_FUNC(TEXT,z_arm_userspace_enter)
    /* move user_entry to lr */
    mov lr, r0

    /* prepare to set stack to privileged stack */
    ldr r0, =_kernel
    ldr r0, [r0, #_kernel_offset_to_current]
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
    /* move p1 to ip */
    mov ip, r1
    ldr r1, =_thread_offset_to_priv_stack_start
    ldr r0, [r0, r1]    /* priv stack ptr */
    ldr r1, =CONFIG_PRIVILEGED_STACK_SIZE
    add r0, r0, r1
    /* Restore p1 from ip */
    mov r1, ip
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
    ldr r0, [r0, #_thread_offset_to_priv_stack_start]    /* priv stack ptr */
    ldr ip, =CONFIG_PRIVILEGED_STACK_SIZE
    add r0, r0, ip
#elif defined(CONFIG_CPU_AARCH32_CORTEX_R)
    ldr r0, [r0, #_thread_offset_to_priv_stack_start]    /* priv stack ptr */
    ldr ip, =CONFIG_PRIVILEGED_STACK_SIZE
    add r0, r0, ip

    ldr ip, =_kernel
    ldr ip, [ip, #_kernel_offset_to_current]
    str r0, [ip, #_thread_offset_to_priv_stack_end] /* priv stack end */
#endif

    /* store current stack pointer to ip
     * the current stack pointer is needed to retrieve
     * stack_info.start and stack_info.size
     */
    mov ip, sp

#if defined(CONFIG_CPU_AARCH32_CORTEX_R)
    mov sp, r0
#else
    /* set stack to privileged stack
     *
     * Note [applies only when CONFIG_BUILTIN_STACK_GUARD is enabled]:
     * modifying PSP via MSR instruction is not subject to stack limit
     * checking, so we do not need to clear PSPLIM before setting PSP.
     * The operation is safe since, by design, the privileged stack is
     * located in memory higher than the default (user) thread stack.
     */
    msr PSP, r0
#endif

#if defined(CONFIG_BUILTIN_STACK_GUARD)
    /* At this point the privileged stack is not yet protected by PSPLIM.
     * Since we have just switched to the top of the privileged stack, we
     * are safe, as long as the stack can accommodate the maximum exception
     * stack frame.
     */

    /* set stack pointer limit to the start of the priv stack */
    ldr r0, =_kernel
    ldr r0, [r0, #_kernel_offset_to_current]
    ldr r0, [r0, #_thread_offset_to_priv_stack_start]    /* priv stack ptr */
    msr PSPLIM, r0
#endif

    /* push args to stack */
    push {r1,r2,r3,lr}
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
    mov r1, ip
    push {r0,r1}
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE) \
    || defined(CONFIG_CPU_AARCH32_CORTEX_R)
    push {r0,ip}
#endif

    /* Re-program dynamic memory map.
     *
     * Important note:
     * z_arm_configure_dynamic_mpu_regions() may re-program the MPU Stack Guard
     * to guard the privilege stack for overflows (if building with option
     * CONFIG_MPU_STACK_GUARD). There is a risk of actually overflowing the
     * stack while doing the re-programming. We minimize the risk by placing
     * this function immediately after we have switched to the privileged stack
     * so that the whole stack area is available for this critical operation.
     *
     * Note that the risk for overflow is higher if using the normal thread
     * stack, since we do not control how much stack is actually left, when
     * user invokes z_arm_userspace_enter().
     */
    ldr r0, =_kernel
    ldr r0, [r0, #_kernel_offset_to_current]
    bl z_arm_configure_dynamic_mpu_regions

#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
    pop {r0,r3}

    /* load up stack info from user stack */
    ldr r0, [r3]
    ldr r3, [r3, #4]
    mov ip, r3

    push {r0,r3}
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE) \
    || defined(CONFIG_CPU_AARCH32_CORTEX_R)
    pop {r0,ip}

    /* load up stack info from user stack */
    ldr r0, [ip]
    ldr ip, [ip, #4]

    push {r0,ip}
#endif

    /* clear the user stack area to clean out privileged data */
    /* from right past the guard right up to the end */
    mov r2, ip
#ifdef CONFIG_INIT_STACKS
    ldr r1,=0xaaaaaaaa
#else
    eors r1, r1
#endif
    bl memset

#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
    pop {r0, r1}
    mov ip, r1
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE) \
    || defined(CONFIG_CPU_AARCH32_CORTEX_R)
    pop {r0,ip}
#endif

    /* r0 contains user stack start, ip contains user stack size */
    add r0, r0, ip   /* calculate top of stack */

    /* pop remaining arguments from stack before switching stacks */
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
    /* Use r4 to pop lr, then restore r4 */
    mov ip, r4
    pop {r1,r2,r3,r4}
    mov lr, r4
    mov r4, ip
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE) \
    || defined(CONFIG_CPU_AARCH32_CORTEX_R)
    pop {r1,r2,r3,lr}
#endif

#if defined(CONFIG_CPU_AARCH32_CORTEX_R)
    /*
     * set stack to user stack.  We are in SYSTEM state, so r13 and r14 are
     * shared with USER state
     */
    mov sp, r0
#else
#if defined(CONFIG_BUILTIN_STACK_GUARD)
    /*
     * Guard the default (user) stack until thread drops privileges.
     *
     * Notes:
     * PSPLIM is configured *before* PSP switches to the default (user) stack.
     * This is safe, since the user stack is located, by design, in a lower
     * memory area compared to the privileged stack.
     *
     * However, we need to prevent a context-switch to occur, because that
     * would re-configure PSPLIM to guard the privileged stack; we enforce
     * a PendSV locking for this purporse.
     *
     * Between PSPLIM update and PSP switch, the privileged stack will be
     * left un-guarded; this is safe, as long as the privileged stack is
     * large enough to accommodate a maximum exception stack frame.
     */

    /* Temporarily store current IRQ locking status in ip */
    mrs ip, BASEPRI
    push {r0, ip}

    /* Lock PendSV while reprogramming PSP and PSPLIM */
    mov r0, #_EXC_PENDSV_PRIO_MASK
    msr BASEPRI_MAX, r0
    isb

    /* Set PSPLIM to guard the thread's user stack. */
    ldr r0, =_kernel
    ldr r0, [r0, #_kernel_offset_to_current]
    ldr r0, [r0, #_thread_offset_to_stack_info_start]
    msr PSPLIM, r0

    pop {r0, ip}
#endif

    /* set stack to user stack */
    msr PSP, r0
#endif

#if defined(CONFIG_BUILTIN_STACK_GUARD)
    /* Restore interrupt lock status */
    msr BASEPRI, ip
    isb
#endif

    /* restore r0 */
    mov r0, lr

#if defined(CONFIG_CPU_AARCH32_CORTEX_R)
    /* change processor mode to unprivileged, with all interrupts enabled. */
    msr CPSR_c, #MODE_USR
#else
    /* change processor mode to unprivileged */
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
    push {r0, r1, r2, r3}
    ldr r0, =_kernel
    ldr r0, [r0, #_kernel_offset_to_current]
    ldr r1, =_thread_offset_to_mode
    ldr r1, [r0, r1]
    movs r2, #1
    orrs r1, r1, r2
    mrs r3, CONTROL
    orrs r3, r3, r2
    mov ip, r3
    /* Store (unprivileged) mode in thread's mode state variable */
    ldr r2, =_thread_offset_to_mode
    str r1, [r0, r2]
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
    push {r0, r1}
    ldr r0, =_kernel
    ldr r0, [r0, #_kernel_offset_to_current]
    ldr r1, [r0, #_thread_offset_to_mode]
    orrs r1, r1, #1
    mrs ip, CONTROL
    orrs ip, ip, #1
    /* Store (unprivileged) mode in thread's mode state variable */
    str r1, [r0, #_thread_offset_to_mode]
#endif
    dsb
    msr CONTROL, ip
#endif

    /* ISB is not strictly necessary here (stack pointer is not being
     * touched), but it's recommended to avoid executing pre-fetched
     * instructions with the previous privilege.
     */
    isb
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
    pop {r0, r1, r2, r3}
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
    pop {r0, r1}
#endif

    /* jump to z_thread_entry entry */
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
    push {r0, r1}
    ldr r0, =z_thread_entry
    mov ip, r0
    pop {r0, r1}
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE) \
    || defined(CONFIG_CPU_AARCH32_CORTEX_R)
    ldr ip, =z_thread_entry
#endif
    bx ip

/**
 *
 * Userspace system call function
 *
 * This function is used to do system calls from unprivileged code.  This
 * function is responsible for the following:
 * 1) Fixing up bad syscalls
 * 2) Configuring privileged stack and loading up stack arguments
 * 3) Dispatching the system call
 * 4) Restoring stack and calling back to the caller of the SVC
 *
 */
SECTION_FUNC(TEXT, z_arm_do_syscall)

    /* Note [when using MPU-based stack guarding]:
     * The function is executing in privileged mode. This implies that we
     * shall not be allowed to use the thread's default unprivileged stack,
     * (i.e push to or pop from it), to avoid a possible stack corruption.
     *
     * Rationale: since we execute in PRIV mode and no MPU guard
     * is guarding the end of the default stack, we won't be able
     * to detect any stack overflows.
     *
     * Note [when using built-in stack limit checking on ARMv8-M]:
     * At this point PSPLIM is already configured to guard the default (user)
     * stack, so pushing to the default thread's stack is safe.
     */
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
    /* save current stack pointer (user stack) */
    mov ip, sp
    /* temporarily push to user stack */
    push {r0,r1}
    /* setup privileged stack */
    ldr r0, =_kernel
    ldr r0, [r0, #_kernel_offset_to_current]
    adds r0, r0, #_thread_offset_to_priv_stack_start
    ldr r0, [r0]    /* priv stack ptr */
    ldr r1, =CONFIG_PRIVILEGED_STACK_SIZE
    add r0, r1

    /* Store current SP and LR at the beginning of the priv stack */
    subs r0, #8
    mov r1, ip
    str r1, [r0, #0]
    mov r1, lr
    str r1, [r0, #4]
    mov ip, r0
    /* Restore user stack and original r0, r1 */
    pop {r0, r1}

#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
    /* setup privileged stack */
    ldr ip, =_kernel
    ldr ip, [ip, #_kernel_offset_to_current]
    ldr ip, [ip, #_thread_offset_to_priv_stack_start]    /* priv stack ptr */
    add ip, #CONFIG_PRIVILEGED_STACK_SIZE

    /* Store current SP and LR at the beginning of the priv stack */
    subs ip, #8
    str sp, [ip, #0]
    str lr, [ip, #4]
#elif defined(CONFIG_CPU_AARCH32_CORTEX_R)
    /*
     * The SVC handler has already switched to the privileged stack.
     * Store the user SP and LR at the beginning of the priv stack.
     */
    ldr ip, =_kernel
    ldr ip, [ip, #_kernel_offset_to_current]
    ldr ip, [ip, #_thread_offset_to_sp_usr]
    push {ip, lr}
#endif

#if !defined(CONFIG_CPU_AARCH32_CORTEX_R)
    /* switch to privileged stack */
    msr PSP, ip
#endif

    /* Note (applies when using stack limit checking):
     * We do not need to lock IRQs after switching PSP to the privileged stack;
     * PSPLIM is guarding the default (user) stack, which, by design, is
     * located at *lower* memory area. Since we switch to the top of the
     * privileged stack we are safe, as long as the stack can accommodate
     * the maximum exception stack frame.
     */

#if defined(CONFIG_BUILTIN_STACK_GUARD)
    /* Set stack pointer limit (needed in privileged mode) */
    ldr ip, =_kernel
    ldr ip, [ip, #_kernel_offset_to_current]
    ldr ip, [ip, #_thread_offset_to_priv_stack_start]    /* priv stack ptr */
    msr PSPLIM, ip
#endif

    /*
     * r0-r5 contain arguments
     * r6 contains call_id
     * r8 contains original LR
     */
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
    /* save  r0, r1 to ip, lr */
    mov ip, r0
    mov lr, r1
    ldr r0, =K_SYSCALL_BAD
    cmp r6, r0
    bne valid_syscall

    /* BAD SYSCALL path */
    /* fixup stack frame on the privileged stack, adding ssf */
    mov r1, sp
    /* ssf is present in r1 (sp) */
    push {r1,lr}
    push {r4,r5}
    /* restore r0, r1 */
    mov r0, ip
    mov r1, lr
    b dispatch_syscall
valid_syscall:
    /* push ssf to privileged stack */
    mov r1, sp
    push {r1}
    /* push args to complete stack frame */
    push {r4,r5}

dispatch_syscall:
    /* original r0 is saved in ip */
    ldr r0, =_k_syscall_table
    lsls r6, #2
    add r0, r6
    ldr r0, [r0]	/* load table address */
    /* swap ip and r0, restore r1 from lr */
    mov r1, ip
    mov ip, r0
    mov r0, r1
    mov r1, lr
    /* execute function from dispatch table */
    blx ip

    /* restore LR
     * r0 holds the return value and needs to be preserved
     */
    mov ip, r0
    mov r0, sp
    ldr r0, [r0,#16]
    mov lr, r0
    /* Restore r0 */
    mov r0, ip

#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE) \
    || defined(CONFIG_CPU_AARCH32_CORTEX_R)
    ldr ip, =K_SYSCALL_BAD
    cmp r6, ip
    bne valid_syscall

    /* BAD SYSCALL path */
    /* fixup stack frame on the privileged stack, adding ssf */
    mov ip, sp
    push {r4,r5,ip,lr}
    b dispatch_syscall

valid_syscall:
    /* push args to complete stack frame */
    mov ip, sp
    push {r4,r5,ip}

dispatch_syscall:
    ldr ip, =_k_syscall_table
    lsl r6, #2
    add ip, r6
    ldr ip, [ip]	/* load table address */

    /* execute function from dispatch table */
    blx ip

    /* restore LR */
    ldr lr, [sp,#16]
#endif


#if defined(CONFIG_BUILTIN_STACK_GUARD)
    /*
     * Guard the default (user) stack until thread drops privileges.
     *
     * Notes:
     * PSPLIM is configured *before* PSP switches to the default (user) stack.
     * This is safe, since the user stack is located, by design, in a lower
     * memory area compared to the privileged stack.
     *
     * However, we need to prevent a context-switch to occur, because that
     * would re-configure PSPLIM to guard the privileged stack; we enforce
     * a PendSV locking for this purporse.
     *
     * Between PSPLIM update and PSP switch, the privileged stack will be
     * left un-guarded; this is safe, as long as the privileged stack is
     * large enough to accommodate a maximum exception stack frame.
     */

    /* Temporarily store current IRQ locking status in r2 */
    mrs r2, BASEPRI

    /* Lock PendSV while reprogramming PSP and PSPLIM */
    mov r3, #_EXC_PENDSV_PRIO_MASK
    msr BASEPRI_MAX, r3
    isb

    /* Set PSPLIM to guard the thread's user stack. */
    ldr r3, =_kernel
    ldr r3, [r3, #_kernel_offset_to_current]
    ldr r3, [r3, #_thread_offset_to_stack_info_start]    /* stack_info.start */
    msr PSPLIM, r3
#endif

#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
    /* set stack back to unprivileged stack */
    mov ip, r0
    mov r0, sp
    ldr r0, [r0,#12]
    msr PSP, r0
    /* Restore r0 */
    mov r0, ip

#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
    /* set stack back to unprivileged stack */
    ldr ip, [sp,#12]
    msr PSP, ip
#endif

#if defined(CONFIG_BUILTIN_STACK_GUARD)
    /* Restore interrupt lock status */
    msr BASEPRI, r2
    isb
#endif

    push {r0, r1}
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
    push {r2, r3}
    ldr r0, =_kernel
    ldr r0, [r0, #_kernel_offset_to_current]
    ldr r2, =_thread_offset_to_mode
    ldr r1, [r0, r2]
    movs r3, #1
    orrs r1, r1, r3
    /* Store (unprivileged) mode in thread's mode state variable */
    str r1, [r0, r2]
    dsb
    /* drop privileges by setting bit 0 in CONTROL */
    mrs r2, CONTROL
    orrs r2, r2, r3
    msr CONTROL, r2
    pop {r2, r3}
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE) \
    || defined(CONFIG_CPU_AARCH32_CORTEX_R)
    ldr r0, =_kernel
    ldr r0, [r0, #_kernel_offset_to_current]
    ldr r1, [r0, #_thread_offset_to_mode]
    orrs r1, r1, #1
    /* Store (unprivileged) mode in thread's mode state variable */
    str r1, [r0, #_thread_offset_to_mode]
    dsb
#if defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
    /* drop privileges by setting bit 0 in CONTROL */
    mrs ip, CONTROL
    orrs ip, ip, #1
    msr CONTROL, ip
#endif
#endif

    /* ISB is not strictly necessary here (stack pointer is not being
     * touched), but it's recommended to avoid executing pre-fetched
     * instructions with the previous privilege.
     */
    isb
    pop {r0, r1}

#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
  /* Zero out volatile (caller-saved) registers so as to not leak state from
   * kernel mode. The C calling convention for the syscall handler will
   * restore the others to original values.
   */
   movs r2, #0
   movs r3, #0

   /*
    * return back to original function that called SVC, add 1 to force thumb
    * mode
    */

    /* Save return value temporarily to ip */
    mov ip, r0

    mov r0, r8
    movs r1, #1
    orrs r0, r0, r1

    /* swap ip, r0 */
    mov r1, ip
    mov ip, r0
    mov r0, r1
    movs r1, #0
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)

    /* Zero out volatile (caller-saved) registers so as to not leak state from
     * kernel mode. The C calling convention for the syscall handler will
     * restore the others to original values.
     */
    mov r1, #0
    mov r2, #0
    mov r3, #0

    /*
     * return back to original function that called SVC, add 1 to force thumb
     * mode
     */
    mov ip, r8
    orrs ip, ip, #1
#elif defined(CONFIG_CPU_AARCH32_CORTEX_R)
    /* Restore user stack pointer */
    ldr ip, [sp,#12]
    mov sp, ip

    /* Zero out volatile (caller-saved) registers so as to not leak state from
     * kernel mode. The C calling convention for the syscall handler will
     * restore the others to original values.
     */
    mov r1, #0
    mov r2, #0
    mov r3, #0

    /*
     * return back to original function that called SVC
     */
    mov ip, r8
    cps #MODE_USR
#endif

    bx ip


/*
 * size_t arch_user_string_nlen(const char *s, size_t maxsize, int *err_arg)
 */
SECTION_FUNC(TEXT, arch_user_string_nlen)
    push {r0, r1, r2, r4, r5, lr}

    /* sp+4 is error value, init to -1 */
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE) \
    || defined(CONFIG_CPU_AARCH32_CORTEX_R)
    ldr r3, =-1
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
    mov.w r3, #-1
#endif
    str	r3, [sp, #4]

    /* Perform string length calculation */
    movs r3, #0		/* r3 is the counter */

strlen_loop:
z_arm_user_string_nlen_fault_start:
    /* r0 contains the string. r5 = *(r0 + r3]). This could fault. */
    ldrb r5, [r0, r3]

z_arm_user_string_nlen_fault_end:
#if defined(CONFIG_CPU_AARCH32_CORTEX_R)
    cmp r5, #0
    beq strlen_done

    cmp r3, r1
    beq strlen_done

    adds r3, #1
    b strlen_loop
#else
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
    cmp r5, #0
    beq strlen_done
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
    cbz	r5, strlen_done
#endif
    cmp	r3, r1
    beq.n strlen_done

    adds r3, #1
    b.n	strlen_loop
#endif

strlen_done:
    /* Move length calculation from r3 to r0 (return value register) */
    mov	r0, r3

    /* Clear error value since we succeeded */
    movs r1, #0
    str	r1, [sp, #4]

z_arm_user_string_nlen_fixup:
    /* Write error value to err pointer parameter */
    ldr	r1, [sp, #4]
    str	r1, [r2, #0]

    add	sp, #12
    pop	{r4, r5, pc}
