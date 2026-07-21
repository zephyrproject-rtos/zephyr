/*
 * Copyright (C) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>

/* Exports */
GTEXT(__soc_handle_irq)

/*
 * No need to clear anything, pending bit is cleared by HW.
 */
SECTION_FUNC(exception.other, __soc_handle_irq)
	ret
