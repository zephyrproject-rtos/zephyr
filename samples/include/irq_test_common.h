/* irq-test-common.h - IRQ utilities for tests */

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
DESCRIPTION

Interrupt stuff, abstracted across CPU architectures.
*/

#ifndef _IRQ_TEST_COMMON__H_
#define _IRQ_TEST_COMMON__H_

/* defines */

#if defined(VXMICRO_ARCH_x86)
  #define IRQ_PRIORITY 3
#elif defined(VXMICRO_ARCH_arm)
  #if defined(CONFIG_CPU_CORTEXM)
    #define IRQ_PRIORITY _EXC_PRIO(3)
  #endif /* CONFIG_CPU_CORTEXM */
#endif

/*
 * NUM_SW_IRQS must be defined before this file is included, and it
 * currently only supports 1 or 2 as valid values.
 */
#if !defined(NUM_SW_IRQS)
  #error NUM_SW_IRQS must be defined before including irq-test-common.h
#elif NUM_SW_IRQS < 1 || NUM_SW_IRQS > 2
  #error NUM_SW_IRQS only supports 1 or 2 IRQs
#endif

#if defined(VXMICRO_ARCH_x86)
  static NANO_CPU_INT_STUB_DECL(nanoIntStub1);
#if NUM_SW_IRQS >= 2
  static NANO_CPU_INT_STUB_DECL(nanoIntStub2);
#endif /* NUM_SW_IRQS >= 2 */
#endif

typedef void (*vvfn)(void);	/* void-void function pointer */
typedef void (*vvpfn)(void *);	/* void-void_pointer function pointer */

#if defined(VXMICRO_ARCH_x86)
/*
 * Opcode for generating a software interrupt.  The ISR associated with each
 * of these software interrupts will call either nano_isr_lifo_put() or
 * nano_isr_lifo_get().  The imm8 data in the opcode sequence will need to be
 * filled in after calling irq_connect().
 */

static char sw_isr_trigger_0[] = {
	0xcd,     /* OPCODE: INT imm8 */
	0x00,     /* imm8 data (vector to trigger) filled in at runtime */
	0xc3      /* OPCODE: RET (near) */
	};

#if NUM_SW_IRQS >= 2
static char sw_isr_trigger_1[] = {
	/* same as above */
	0xcd,
	0x00,
	0xc3
	};
#endif /* NUM_SW_IRQS >= 2 */

#elif defined(VXMICRO_ARCH_arm)
#if defined(CONFIG_CPU_CORTEXM)
#include <nanokernel.h>
static inline void sw_isr_trigger_0(void)
	{
	_NvicSwInterruptTrigger(0);
	}

#if NUM_SW_IRQS >= 2
static inline void sw_isr_trigger_1(void)
	{
	_NvicSwInterruptTrigger(1);
	}
#endif /* NUM_SW_IRQS >= 2 */
#endif  /* CONFIG_CPU_CORTEXM */
#endif

struct isrInitInfo {
	vvpfn isr[2];
	void *arg[2];
	};

/*******************************************************************************
*
* initIRQ - init interrupts
*
*/

static int initIRQ
	(
	struct isrInitInfo *i
	)
	{
#if defined(VXMICRO_ARCH_x86)
	int  vector;     /* vector to which interrupt is connected */

	if (i->isr[0]) {
	vector = irq_connect (NANO_SOFT_IRQ, IRQ_PRIORITY, i->isr[0],
				    i->arg[0], nanoIntStub1);
	if (-1 == vector) {
	    return -1;
	    }
	sw_isr_trigger_0[1] = vector;
	}

#if NUM_SW_IRQS >= 2
	if (i->isr[1]) {
	vector = irq_connect (NANO_SOFT_IRQ, IRQ_PRIORITY, i->isr[1],
				    i->arg[1], nanoIntStub2);
	if (-1 == vector) {
	    return -1;
	    }
	sw_isr_trigger_1[1] = vector;
	}
#endif /* NUM_SW_IRQS >= 2 */
#elif defined(VXMICRO_ARCH_arm)
#if defined(CONFIG_CPU_CORTEXM)
	if (i->isr[0]) {
	(void) irq_connect (0, IRQ_PRIORITY, i->isr[0], i->arg[0]);
	irq_enable (0);
	}
	if (i->isr[1]) {
	(void) irq_connect (1, IRQ_PRIORITY, i->isr[1], i->arg[1]);
	irq_enable (1);
	}
#endif /* CONFIG_CPU_CORTEXM */
#endif /* VXMICRO_ARCH_x86 */

	return 0;
	}

#endif /* _IRQ_TEST_COMMON__H_ */
