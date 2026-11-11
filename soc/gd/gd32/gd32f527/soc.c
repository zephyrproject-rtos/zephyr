/*
 * Copyright (c) 2026 GigaDevice Semiconductor Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <soc.h>

/*
 * Clear the whole SRAM so that the ECC check bits are initialized.
 * The loop bounds stay in registers because the stack lives in the
 * SRAM that is being cleared.
 */
void soc_reset_hook(void)
{
#if defined(__ICCARM__)
	register uint32_t r0 = DT_REG_ADDR(DT_CHOSEN(zephyr_sram));
	register uint32_t r1 =
		DT_REG_ADDR(DT_CHOSEN(zephyr_sram)) + DT_REG_SIZE(DT_CHOSEN(zephyr_sram));
#else
	register uint32_t r0 __asm__("r0") = DT_REG_ADDR(DT_CHOSEN(zephyr_sram));
	register uint32_t r1 __asm__("r1") =
		DT_REG_ADDR(DT_CHOSEN(zephyr_sram)) + DT_REG_SIZE(DT_CHOSEN(zephyr_sram));
#endif

	for (; r0 < r1; r0 += 4) {
		*(volatile uint32_t *)r0 = 0;
	}
}

void soc_early_init_hook(void)
{
	/* Without the PLL driver the HAL default clock setup is used */
	SystemInit();
}
