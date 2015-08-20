/* nvic.c - ARM CORTEX-M Series Nested Vector Interrupt Controller */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
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
Provide an interface to the Nested Vectored Interrupt Controller found on
ARM Cortex-M processors.

The API does not account for all possible usages of the NVIC, only the
functionalities needed by the kernel.

The same effect can be achieved by directly writing in the registers of the
NVIC, with the layout available from scs.h, using the __scs.nvic data
structure (or hardcoded values), but these APIs are less error-prone,
especially for registers with multiple instances to account for potentially
240 interrupt lines. If access to a missing functionality is needed, this is
the way to implement it.

Supports up to 240 IRQs and 256 priority levels.
 */

#ifndef _NVIC_H_
#define _NVIC_H_

#include <arch/arm/cortex_m/scs.h>

/* for assembler, only works with constants */
#define _EXC_PRIO(pri) (((pri) << (8 - CONFIG_NUM_IRQ_PRIO_BITS)) & 0xff)
#if defined(CONFIG_ZERO_LATENCY_IRQS)
#define _EXC_IRQ_DEFAULT_PRIO _EXC_PRIO(0x03)
#else
#define _EXC_IRQ_DEFAULT_PRIO _EXC_PRIO(0x02)
#endif

/* no exc #0 */
#define _EXC_RESET 1
#define _EXC_NMI 2
#define _EXC_HARD_FAULT 3
#define _EXC_MPU_FAULT 4
#define _EXC_BUS_FAULT 5
#define _EXC_USAGE_FAULT 6
/* 7-10 reserved */
#define _EXC_SVC 11
#define _EXC_DEBUG 12
/* 13 reserved */
#define _EXC_PENDSV 14
#define _EXC_SYSTICK 15
/* 16+ IRQs */

#define NUM_IRQS_PER_REG 32
#define REG_FROM_IRQ(irq) (irq / NUM_IRQS_PER_REG)
#define BIT_FROM_IRQ(irq) (irq % NUM_IRQS_PER_REG)

#if !defined(_ASMLANGUAGE)

#include <stdint.h>
#include <misc/__assert.h>

/**
 *
 * @brief Enable an IRQ
 *
 * Enable IRQ #<irq>, which is equivalent to exception #<irq>+16
 *
 * @return N/A
 */

static inline void _NvicIrqEnable(unsigned int irq /* IRQ number */
				  )
{
	__scs.nvic.iser[REG_FROM_IRQ(irq)] = 1 << BIT_FROM_IRQ(irq);
}

/**
 *
 * @brief Find out if an IRQ is enabled
 *
 * Find out if IRQ #<irq> is enabled.
 *
 * @return 1 if IRQ is enabled, 0 otherwise
 */

static inline int _NvicIsIrqEnabled(unsigned int irq /* IRQ number */
				    )
{
	return __scs.nvic.iser[REG_FROM_IRQ(irq)] & (1 << BIT_FROM_IRQ(irq));
}

/**
 *
 * @brief Disable an IRQ
 *
 * Disable IRQ #<irq>, which is equivalent to exception #<irq>+16
 *
 * @return N/A
 */

static inline void _NvicIrqDisable(unsigned int irq /* IRQ number */
				   )
{
	__scs.nvic.icer[REG_FROM_IRQ(irq)] = 1 << BIT_FROM_IRQ(irq);
}

/**
 *
 * @brief Pend an IRQ
 *
 * Pend IRQ #<irq>, which is equivalent to exception #<irq>+16. CPU will handle
 * the IRQ when interrupts are enabled and/or returning from a higher priority
 * interrupt.
 *
 * @return N/A
 */

static inline void _NvicIrqPend(unsigned int irq /* IRQ number */
				)
{
	__scs.nvic.ispr[REG_FROM_IRQ(irq)] = 1 << BIT_FROM_IRQ(irq);
}

/**
 *
 * @brief Find out if an IRQ is pending
 *
 * Find out if IRQ #<irq> is pending
 *
 * @return 1 if IRQ is pending, 0 otherwise
 */

static inline int _NvicIsIrqPending(unsigned int irq /* IRQ number */
				    )
{
	return __scs.nvic.ispr[REG_FROM_IRQ(irq)] & (1 << BIT_FROM_IRQ(irq));
}

/**
 *
 * @brief Unpend an IRQ
 *
 * Unpend IRQ #<irq>, which is equivalent to exception #<irq>+16. The previously
 * pending interrupt will be ignored when either unlocking interrupts or
 * returning from a higher priority exception.
 *
 * @return N/A
 */

static inline void _NvicIrqUnpend(unsigned int irq /* IRQ number */
				  )
{
	__scs.nvic.icpr[REG_FROM_IRQ(irq)] = 1 << BIT_FROM_IRQ(irq);
}

/**
 *
 * @brief Set priority of an IRQ
 *
 * Set priority of IRQ #<irq> to <prio>. There are 256 priority levels.
 *
 * @return N/A
 */

static inline void _NvicIrqPrioSet(unsigned int irq, /* IRQ number */
				   unsigned int prio /* priority */
				   )
{
	__ASSERT(prio < 256, "invalid priority\n");
	__scs.nvic.ipr[irq] = prio;
}

/**
 *
 * @brief Get priority of an IRQ
 *
 * Get priority of IRQ #<irq>.
 *
 * @return the priority level of the IRQ
 */

static inline uint32_t _NvicIrqPrioGet(unsigned int irq /* IRQ number */
				       )
{
	return __scs.nvic.ipr[irq];
}

/**
 *
 * @brief Trigger an interrupt via software
 *
 * Trigger interrupt #<irq>. The CPU will handle the IRQ when interrupts are
 * enabled and/or returning from a higher priority interrupt.
 *
 * @return N/A
 */

static inline void _NvicSwInterruptTrigger(unsigned int irq /* IRQ number */
					   )
{
#if defined(CONFIG_PLATFORM_TI_LM3S6965_QEMU)
	/* the QEMU does not simulate the STIR register: this is a workaround */
	_NvicIrqPend(irq);
#else
	__scs.stir = irq;
#endif
}

#endif /* !_ASMLANGUAGE */

#endif /* _NVIC_H_ */
