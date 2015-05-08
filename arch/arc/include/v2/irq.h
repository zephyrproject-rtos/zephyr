/* irq.h - interrupt helper functions (ARC) */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * DESCRIPTION
 * This file contains private nanokernel structures definitions and various
 * other definitions for the ARCv2 processor architecture.
 */

#ifndef _ARCV2_IRQ__H_
#define _ARCV2_IRQ__H_

#define _ARC_V2_AUX_IRQ_CTRL_BLINK (1 << 9)
#define _ARC_V2_AUX_IRQ_CTRL_LOOP_REGS (1 << 10)
#define _ARC_V2_AUX_IRQ_CTRL_14_REGS 7
#define _ARC_V2_AUX_IRQ_CTRL_16_REGS 8
#define _ARC_V2_AUX_IRQ_CTRL_32_REGS 16

#define _ARC_V2_DEF_IRQ_LEVEL 15
#define _ARC_V2_WAKE_IRQ_LEVEL 15

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

	_nanokernel.rirq_sp = _interrupt_stack + CONFIG_ISR_STACK_SIZE;
	_firq_stack_setup();
}

#endif /* _ASMLANGUAGE */
#endif /* _ARCV2_IRQ__H_ */
