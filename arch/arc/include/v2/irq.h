/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Interrupt helper functions (ARC)
 *
 * This file contains private kernel structures definitions and various
 * other definitions for the ARCv2 processor architecture.
 */

#ifndef ZEPHYR_ARCH_ARC_INCLUDE_V2_IRQ_H_
#define ZEPHYR_ARCH_ARC_INCLUDE_V2_IRQ_H_

#include <arch/cpu.h>

#ifdef __cplusplus
extern "C" {
#endif

#define _ARC_V2_AUX_IRQ_CTRL_BLINK (1 << 9)
#define _ARC_V2_AUX_IRQ_CTRL_LOOP_REGS (1 << 10)
#define _ARC_V2_AUX_IRQ_CTRL_U (1 << 11)
#define _ARC_V2_AUX_IRQ_CTRL_LP (1 << 13)
#define _ARC_V2_AUX_IRQ_CTRL_14_REGS 7
#define _ARC_V2_AUX_IRQ_CTRL_16_REGS 8
#define _ARC_V2_AUX_IRQ_CTRL_32_REGS 16


#ifdef CONFIG_ARC_SECURE_FIRMWARE
#define _ARC_V2_DEF_IRQ_LEVEL (ARC_N_IRQ_START_LEVEL - 1)
#else
#define _ARC_V2_DEF_IRQ_LEVEL (CONFIG_NUM_IRQ_PRIO_LEVELS - 1)
#endif

#define _ARC_V2_WAKE_IRQ_LEVEL _ARC_V2_DEF_IRQ_LEVEL

/*
 * INIT_IRQ_LOCK_KEY is init interrupt level setting of a thread.
 * It's configured by seti instruction when a thread starts to run
 *, i.e., z_thread_entry_wrapper and z_user_thread_entry_wrapper
 */
#define _ARC_V2_INIT_IRQ_LOCK_KEY (0x10 | _ARC_V2_DEF_IRQ_LEVEL)

#ifndef _ASMLANGUAGE

extern K_THREAD_STACK_DEFINE(_interrupt_stack, CONFIG_ISR_STACK_SIZE);

/*
 * z_irq_setup
 *
 * Configures interrupt handling parameters
 */
static ALWAYS_INLINE void z_irq_setup(void)
{
	u32_t aux_irq_ctrl_value = (
		_ARC_V2_AUX_IRQ_CTRL_LOOP_REGS | /* save lp_xxx registers */
#ifdef CONFIG_CODE_DENSITY
		_ARC_V2_AUX_IRQ_CTRL_LP | /* save code density registers */
#endif
		_ARC_V2_AUX_IRQ_CTRL_BLINK     | /* save blink */
		_ARC_V2_AUX_IRQ_CTRL_14_REGS     /* save r0 -> r13 (caller-saved) */
	);

	z_arc_cpu_sleep_mode = _ARC_V2_WAKE_IRQ_LEVEL;

#ifdef CONFIG_ARC_NORMAL_FIRMWARE
	/* normal mode cannot write irq_ctrl, ignore it */
	aux_irq_ctrl_value = aux_irq_ctrl_value;
#else
	z_arc_v2_aux_reg_write(_ARC_V2_AUX_IRQ_CTRL, aux_irq_ctrl_value);
#endif
}

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_ARC_INCLUDE_V2_IRQ_H_ */
