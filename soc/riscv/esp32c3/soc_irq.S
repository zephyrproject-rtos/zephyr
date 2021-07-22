/* Copyright 2021 Espressif Systems (Shanghai) PTE LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <offsets.h>
#include <toolchain.h>

/* Exports */
GTEXT(__soc_is_irq)
GTEXT(__soc_handle_irq)

SECTION_FUNC(exception.other, __soc_is_irq)
	csrr a0, mcause
	srli a0, a0, 31
	ret

SECTION_FUNC(exception.other, __soc_handle_irq)
	ret
