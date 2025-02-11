/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Per-arch thread definition
 *
 * This file contains definitions for
 *
 *  struct _thread_arch
 *  struct _callee_saved
  *
 * necessary to instantiate instances of struct k_thread.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_THREAD_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_THREAD_H_

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>

struct _callee_saved {
	uint32_t v1;  /* r4 */
	uint32_t v2;  /* r5 */
	uint32_t v3;  /* r6 */
	uint32_t v4;  /* r7 */
	uint32_t v5;  /* r8 */
	uint32_t v6;  /* r9 */
	uint32_t v7;  /* r10 */
	uint32_t v8;  /* r11 */
	uint32_t psp; /* r13 */
#ifdef CONFIG_USE_SWITCH
	uint32_t lr;  /* lr */
#endif
};

typedef struct _callee_saved _callee_saved_t;

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
struct _preempt_float {
	float  s16;
	float  s17;
	float  s18;
	float  s19;
	float  s20;
	float  s21;
	float  s22;
	float  s23;
	float  s24;
	float  s25;
	float  s26;
	float  s27;
	float  s28;
	float  s29;
	float  s30;
	float  s31;
};
#endif

struct _thread_arch {

	/* interrupt locking key */
	uint32_t basepri;

	/* r0 in stack frame cannot be written to reliably */
	uint32_t swap_return_value;

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
	/*
	 * No cooperative floating point register set structure exists for
	 * the Cortex-M as it automatically saves the necessary registers
	 * in its exception stack frame.
	 */
	struct _preempt_float  preempt_float;
#endif

#if defined(CONFIG_CPU_AARCH32_CORTEX_A) || defined(CONFIG_CPU_AARCH32_CORTEX_R)
	int8_t exception_depth;
#endif

#if defined(CONFIG_ARM_STORE_EXC_RETURN) || defined(CONFIG_USERSPACE)
	/*
	 * Status variable holding several thread status flags
	 * as follows:
	 *
	 * byte 0
	 * +-bits 4-7-----bit-3----------bit-2--------bit-1---+----bit-0------+
	 * :          |             |              |          |               |
	 * : reserved |<Guard FLOAT>|   reserved   | reserved |  <priv mode>  |
	 * :   bits   |             |              |          | CONTROL.nPRIV |
	 * +------------------------------------------------------------------+
	 *
	 * byte 1
	 * +----------------------------bits 8-15-----------------------------+
	 * :              Least significant byte of EXC_RETURN                |
	 * : bit 15| bit 14| bit 13 | bit 12| bit 11 | bit 10 | bit 9 | bit 8 |
	 * :  Res  |   S   |  DCRS  | FType |  Mode  | SPSel  |  Res  |  ES   |
	 * +------------------------------------------------------------------+
	 *
	 * Bit 0: thread's current privileged mode (Supervisor or User mode)
	 *        Mirrors CONTROL.nPRIV flag.
	 * Bit 2: Deprecated in favor of FType. Note: FType = !CONTROL.FPCA.
	 *        indicating whether the thread has an active FP context.
	 *        Mirrors CONTROL.FPCA flag.
	 * Bit 3: indicating whether the thread is applying the long (FLOAT)
	 *        or the default MPU stack guard size.
	 *
	 * Bits 8-15: Least significant octet of the EXC_RETURN value when a
	 *            thread is switched-out. The value is copied from LR when
	 *            entering the PendSV handler. When the thread is
	 *            switched in again, the value is restored to LR before
	 *            exiting the PendSV handler.
	 */
	union {
		uint32_t mode;

#if defined(CONFIG_ARM_STORE_EXC_RETURN)
		struct {
			uint8_t mode_bits;
			uint8_t mode_exc_return;
			uint16_t mode_reserved2;
		};
#endif
	};

#if defined(CONFIG_USERSPACE)
	uint32_t priv_stack_start;
#if defined(CONFIG_CPU_AARCH32_CORTEX_R)
	uint32_t priv_stack_end;
	uint32_t sp_usr;
#endif
#endif
#endif
};

#if defined(CONFIG_FPU_SHARING) && defined(CONFIG_MPU_STACK_GUARD)
#define Z_ARM_MODE_MPU_GUARD_FLOAT_Msk (1 << 3)
#endif
typedef struct _thread_arch _thread_arch_t;

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_THREAD_H_ */
