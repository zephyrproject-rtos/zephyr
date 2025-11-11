/*
 * Copyright (c) 2024 Michael Hope
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <offsets.h>
#include <zephyr/toolchain.h>

/* Exports */
GTEXT(__soc_handle_irq)

SECTION_FUNC(exception.other, __soc_handle_irq)
	ret
