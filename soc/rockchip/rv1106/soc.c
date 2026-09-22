/*
 * Copyright (c) 2026 Patryk Koscik
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/kernel.h>

void soc_reset_hook(void)
{
	/*
	 * U-Boot modifies VBAR to point to its own vectors - set it to the Zephyr's vector table.
	 */
	uint32_t vbar = 0;

	__set_VBAR(vbar);
}
