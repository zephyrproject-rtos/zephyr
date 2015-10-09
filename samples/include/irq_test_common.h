/* irq-test-common.h - IRQ utilities for tests */

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

/*
DESCRIPTION

Interrupt stuff, abstracted across CPU architectures.
 */

#ifndef _IRQ_TEST_COMMON__H_
#define _IRQ_TEST_COMMON__H_

#if defined(CONFIG_X86)
  #define IRQ_PRIORITY 3
#elif defined(CONFIG_ARM)
  #if defined(CONFIG_CPU_CORTEX_M)
    #define IRQ_PRIORITY _EXC_PRIO(3)
  #endif /* CONFIG_CPU_CORTEX_M */
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

typedef void (*vvfn)(void);	/* void-void function pointer */
typedef void (*vvpfn)(void *);	/* void-void_pointer function pointer */

#if defined(CONFIG_X86)
/*
 * Opcode for generating a software interrupt.  The ISR associated with each
 * of these software interrupts will call either nano_isr_lifo_put() or
 * nano_isr_lifo_get().  The imm8 data in the opcode sequence will need to be
 * filled in after calling irq_connect().
 */
#include <drivers/loapic.h>
static uint8_t sw_irq_vectors[NUM_SW_IRQS];

static inline void sw_isr_trigger_0(void)
{
	loapic_int_vec_trigger(sw_irq_vectors[0]);
}

#if NUM_SW_IRQS >= 2
static inline void sw_isr_trigger_1(void)
{
	loapic_int_vec_trigger(sw_irq_vectors[1]);
}
#endif /* NUM_SW_IRQS >= 2 */

#elif defined(CONFIG_ARM)
#if defined(CONFIG_CPU_CORTEX_M)
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
#endif  /* CONFIG_CPU_CORTEX_M */
#endif

struct isrInitInfo {
	vvpfn isr[2];
	void *arg[2];
};

/**
 *
 * @brief Init interrupts
 *
 */

static int initIRQ(struct isrInitInfo *i)
{
#if defined(CONFIG_X86)
	int  vector;     /* vector to which interrupt is connected */

	if (i->isr[0]) {
		vector = irq_connect(NANO_SOFT_IRQ, IRQ_PRIORITY, i->isr[0],
				     i->arg[0], 0);
		if (-1 == vector) {
			return -1;
		}
		sw_irq_vectors[0] = vector;
	}

#if NUM_SW_IRQS >= 2
	if (i->isr[1]) {
		vector = irq_connect(NANO_SOFT_IRQ, IRQ_PRIORITY, i->isr[1],
				     i->arg[1], 0);
		if (-1 == vector) {
			return -1;
		}
		sw_irq_vectors[1] = vector;
	}
#endif /* NUM_SW_IRQS >= 2 */
#elif defined(CONFIG_ARM)
#if defined(CONFIG_CPU_CORTEX_M)
	if (i->isr[0]) {
		(void) irq_connect(0, IRQ_PRIORITY, i->isr[0], i->arg[0], 0);
		irq_enable(0);
	}
	if (i->isr[1]) {
		(void) irq_connect(1, IRQ_PRIORITY, i->isr[1], i->arg[1], 0);
		irq_enable(1);
	}
#endif /* CONFIG_CPU_CORTEX_M */
#endif /* CONFIG_X86 */

	return 0;
}

#endif /* _IRQ_TEST_COMMON__H_ */
