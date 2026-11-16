/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Per-arch thread definition
 */

#ifndef ZEPHYR_INCLUDE_ARCH_HEXAGON_THREAD_H_
#define ZEPHYR_INCLUDE_ARCH_HEXAGON_THREAD_H_

/* Hexagon requires 8-byte stack alignment. */
#define ARCH_STACK_PTR_ALIGN 8

#ifdef CONFIG_HW_STACK_PROTECTION
/*
 * FRAMELIMIT sits at the start of a thread's stack buffer. The event
 * frame pushed for the stack-limit exception itself lands in this area
 * below it, so the thread keeps all of the stack size it asked for.
 */
#define ARCH_KERNEL_STACK_RESERVED 128
#define ARCH_THREAD_STACK_RESERVED 128
#endif

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>
#include <zephyr/arch/arch_interface.h>
#include <hexagon_vm.h>

#ifdef CONFIG_USERSPACE
/*
 * Per-partition entry budget for the greedy largest-aligned-size-class
 * decomposition hexagon_mem_domain_rebuild() uses to turn an arbitrary
 * [start, start+size) range into VM_TRANS_TYPE_LINEAR entries. A
 * well-aligned region (the common case) needs exactly one entry. The
 * budget also covers oddly-aligned or sized regions. Running out
 * mid-region is logged and stops early rather than treating it as
 * fatal.
 */
#define HEXAGON_MEM_DOMAIN_PARTITION_ENTRIES 4
#endif

/**
 * @brief Callee-saved register context for cooperative context switching.
 */
struct _callee_saved {
	/** General-purpose register R16 (callee-saved). */
	uint32_t r16;
	/** General-purpose register R17 (callee-saved). */
	uint32_t r17;
	/** General-purpose register R18 (callee-saved). */
	uint32_t r18;
	/** General-purpose register R19 (callee-saved). */
	uint32_t r19;
	/** General-purpose register R20 (callee-saved). */
	uint32_t r20;
	/** General-purpose register R21 (callee-saved). */
	uint32_t r21;
	/** General-purpose register R22 (callee-saved). */
	uint32_t r22;
	/** General-purpose register R23 (callee-saved). */
	uint32_t r23;
	/** General-purpose register R24 (callee-saved). */
	uint32_t r24;
	/** General-purpose register R25 (callee-saved). */
	uint32_t r25;
	/** General-purpose register R26 (callee-saved). */
	uint32_t r26;
	/** General-purpose register R27 (callee-saved). */
	uint32_t r27;

	/** Stack pointer (R29). */
	uint32_t r29_sp;
	/** Frame pointer (R30). */
	uint32_t r30_fp;

	/** Link register (R31). */
	uint32_t r31_lr;
};

typedef struct _callee_saved _callee_saved_t;

/* Thread flags */
#define HEXAGON_THREAD_FLAG_STACK_PROT 0x04

/**
 * @brief Architecture-specific thread data.
 */
struct _thread_arch {
	/** Return value from arch_switch. */
	uint32_t swap_return_value;

	/* Flags */
	uint8_t flags;

#ifdef CONFIG_HW_STACK_PROTECTION
	/*
	 * FRAMELIMIT while this thread runs in guest (kernel) mode: set
	 * just above its stack's lowest address, or priv_stack's once it
	 * has entered user mode.
	 */
	uint32_t framelimit;
#ifdef CONFIG_USERSPACE
	/* FRAMELIMIT while this thread runs in user mode */
	uint32_t user_framelimit;
#endif
#endif

#ifdef CONFIG_USERSPACE
	/*
	 * 1 while this thread runs in Hexagon user mode. Mirrored into the
	 * global _hexagon_user_mode_active flag by z_hexagon_user_mode_sync()
	 * on every event exit.
	 */
	uint8_t priv_level;

	/*
	 * 1 while a trap0 handler for this thread has re-enabled guest
	 * interrupts and not yet returned to user mode. Lets
	 * z_hexagon_event_exit_user_sync() tell a genuine return to user
	 * mode apart from resuming an interrupted trap0 handler.
	 */
	uint8_t trap0_active;

	/* Entry point and arguments for a K_USER thread, saved by
	 * arch_new_thread() and consumed by hexagon_user_thread_entry().
	 */
	k_thread_entry_t user_entry;
	void *user_p1;
	void *user_p2;
	void *user_p3;

	/*
	 * Linear map installed while this thread runs in user mode: its own
	 * stack, then a chain entry to its memory domain's list. map_stale
	 * is set whenever either changes, so the next install invalidates
	 * this map's H2 ASID.
	 */
	uint8_t map_stale;
	struct hexagon_linear_entry map[CONFIG_HEXAGON_USER_STACK_ENTRIES + 1] __aligned(8);

	/*
	 * Dedicated kernel-mode stack used while executing in kernel mode
	 * (trap0/exception handling); see arch_user_mode_enter()'s comment
	 * in userspace.c for why it can't be carved out of anything else.
	 */
	uint8_t priv_stack[CONFIG_PRIVILEGED_STACK_SIZE] __aligned(ARCH_STACK_PTR_ALIGN);
#endif
};

typedef struct _thread_arch _thread_arch_t;

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_HEXAGON_THREAD_H_ */
