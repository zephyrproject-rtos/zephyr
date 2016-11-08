/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file
 * @brief Interrupt helper functions (ARC)
 *
 * This file contains private nanokernel structures definitions and various
 * other definitions for the ARCv2 processor architecture.
 */

#ifndef _ARCV2_IRQ__H_
#define _ARCV2_IRQ__H_

#ifdef __cplusplus
extern "C" {
#endif

#define _ARC_V2_AUX_IRQ_CTRL_BLINK (1 << 9)
#define _ARC_V2_AUX_IRQ_CTRL_LOOP_REGS (1 << 10)
#define _ARC_V2_AUX_IRQ_CTRL_LP (1 << 13)
#define _ARC_V2_AUX_IRQ_CTRL_14_REGS 7
#define _ARC_V2_AUX_IRQ_CTRL_16_REGS 8
#define _ARC_V2_AUX_IRQ_CTRL_32_REGS 16

#define _ARC_V2_DEF_IRQ_LEVEL (CONFIG_NUM_IRQ_PRIO_LEVELS-1)
#define _ARC_V2_WAKE_IRQ_LEVEL _ARC_V2_DEF_IRQ_LEVEL

#ifndef _ASMLANGUAGE

extern void _firq_stack_setup(void);
extern char _interrupt_stack[];

/*
 * _irq_setup
 *
 * Configures interrupt handling parameters
 */
static ALWAYS_INLINE void _irq_setup(void)
{
	uint32_t aux_irq_ctrl_value = (
		_ARC_V2_AUX_IRQ_CTRL_LOOP_REGS | /* save lp_xxx registers */
		_ARC_V2_AUX_IRQ_CTRL_BLINK     | /* save blink */
		_ARC_V2_AUX_IRQ_CTRL_14_REGS     /* save r0 -> r13 (caller-saved) */
	);

	nano_cpu_sleep_mode = _ARC_V2_WAKE_IRQ_LEVEL;
	_arc_v2_aux_reg_write(_ARC_V2_AUX_IRQ_CTRL, aux_irq_ctrl_value);

	_kernel.irq_stack = _interrupt_stack + CONFIG_ISR_STACK_SIZE;
	_firq_stack_setup();
}

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* _ARCV2_IRQ__H_ */
