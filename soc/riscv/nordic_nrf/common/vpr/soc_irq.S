/*
 * Copyright (C) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <offsets.h>
#include <zephyr/toolchain.h>

/* Exports */
GTEXT(__soc_handle_irq)
GTEXT(__soc_save_context)
GTEXT(__soc_restore_context)

/*
 * No need to clear anything, pending bit is cleared by HW.
 */
SECTION_FUNC(exception.other, __soc_handle_irq)
	ret

SECTION_FUNC(exception.other, __soc_save_context)
	csrr t0, 0x347
	sw t0, __soc_esf_t_minttresh_OFFSET(a0)

	ret

SECTION_FUNC(exception.other, __soc_restore_context)
	lw t0, __soc_esf_t_minttresh_OFFSET(a0)
	csrw 0x347, t0

	ret
