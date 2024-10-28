/* Copyright 2024 Espressif Systems (Shanghai) PTE LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/offsets.h>
#include <zephyr/toolchain.h>

/* Exports */
GTEXT(__soc_handle_irq)
GTEXT(soc_intr_get_next_source)

SECTION_FUNC(exception.other, __soc_handle_irq)
	addi sp, sp,-4
	sw  ra, 0x00(sp)
	la t1, soc_intr_get_next_source
	jalr ra, t1, 0
	lw  ra, 0x00(sp)
	addi sp, sp, 4
	ret
