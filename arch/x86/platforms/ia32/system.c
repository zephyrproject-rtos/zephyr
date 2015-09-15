/* system.c - system/hardware module for the ia32 platform */

/*
 * Copyright (c) 2011-2015, Wind River Systems, Inc.
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
This module provides routines to initialize and support board-level hardware
for the ia32 platform.
 */

#include <nanokernel.h>
#include "board.h"
#include <drivers/uart.h>
#include <drivers/pic.h>
#include <device.h>
#include <init.h>


#ifdef CONFIG_LOAPIC
#include <drivers/loapic.h>
static inline void loapic_init(void)
{
	_loapic_init();
}
#else
#define loapic_init()      \
	do {/* nothing */ \
	} while ((0))
#endif /* CONFIG_LOAPIC */

#ifdef CONFIG_IOAPIC
#include <drivers/ioapic.h>
static inline void ioapic_init(void)
{
	_ioapic_init();
}
#else
#define ioapic_init(mask)  \
	do {/* nothing */ \
	} while ((0))
#endif /* CONFIG_IOAPIC */

#ifdef CONFIG_HPET_TIMER
#include <drivers/hpet.h>
static inline void hpet_irq_set(void)
{
	_ioapic_irq_set(CONFIG_HPET_TIMER_IRQ,
			CONFIG_HPET_TIMER_IRQ + INT_VEC_IRQ0,
			HPET_IOAPIC_FLAGS);
}
#else
#define hpet_irq_set()   \
	do { /* nothing */   \
	} while ((0))
#endif

#ifdef CONFIG_CONSOLE_HANDLER
static inline void console_irq_set(void)
{
	_ioapic_irq_set(CONFIG_UART_CONSOLE_IRQ,
			CONFIG_UART_CONSOLE_IRQ + INT_VEC_IRQ0,
			UART_IOAPIC_FLAGS);
}
#else
#define console_irq_set()	\
	do { /* nothing */	\
	} while ((0))
#endif

/**
 *
 * @brief Perform basic hardware initialization
 *
 * Initialize the interrupt controller and UARTs present in the
 * platform.
 *
 * @return 0
 */
static int ia32_init(struct device *arg)
{
	ARG_UNUSED(arg);

	loapic_init();    /* NOP if not needed */

	ioapic_init();    /* NOP if not needed */
	hpet_irq_set();   /* NOP if not needed */
	console_irq_set();   /* NOP if not needed */
	return 0;
}

#if defined(CONFIG_PIC_DISABLE)

DECLARE_DEVICE_INIT_CONFIG(pic_0, "", _i8259_init, NULL);
pure_early_init(pic_0, NULL);

#endif /* CONFIG_PIC_DISABLE */

DECLARE_DEVICE_INIT_CONFIG(ia32_0, "", ia32_init, NULL);
pure_early_init(ia32_0, NULL);
