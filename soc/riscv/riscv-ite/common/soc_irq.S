/*
 * Copyright (c) 2020 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * common interrupt management code for riscv SOCs supporting the riscv
 * privileged architecture specification
 */
#include <zephyr/kernel_structs.h>
#include <offsets.h>
#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <soc.h>

/* exports */
GTEXT(__soc_handle_irq)

/*
 * SOC-specific function to handle pending IRQ number generating the interrupt.
 * Exception number is given as parameter via register a0.
 * Jump to get_irq() function directly and return to caller by its
 * ret instruction.
 */
SECTION_FUNC(exception.other, __soc_handle_irq)
	j get_irq

/*
 * __soc_is_irq is defined as .weak to allow re-implementation by
 * SOCs that does not truely follow the riscv privilege specification.
 */
WTEXT(__soc_is_irq)

/*
 * SOC-specific function to determine if the exception is the result of a
 * an interrupt or an exception
 * return 1 (interrupt) or 0 (exception)
 *
 */
SECTION_FUNC(exception.other, __soc_is_irq)
	/* Read mcause and check if interrupt bit (bit 31) is set */
	csrr a0, mcause
	srli a0, a0, 31
	ret
