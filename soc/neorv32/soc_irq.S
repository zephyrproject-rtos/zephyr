/*
 * Copyright (c) 2021 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>

GTEXT(__soc_handle_irq)

/*
 * SOC-specific function to handle pending IRQ number generating the interrupt.
 * Exception number is given as parameter via register a0.
 */
SECTION_FUNC(exception.other, __soc_handle_irq)

	/**
	 * The RISC-V standard interrupts need no explicit clearing -
	 * for example, machine timer clears on its own once TIMECMP is advanced.
	 * NeoRV32 FIRQs also clear automatically when the interrupt is cleared at the source.
	 */
	ret
