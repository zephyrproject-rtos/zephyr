/*
 * Copyright (c) 2020 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/devicetree.h>
#include <zephyr/platform/hooks.h>

/*
 * Magic value that causes the bootloader to stay in bootloader mode instead of
 * starting the application.
 */
#if CONFIG_BOOTLOADER_BOSSA_ADAFRUIT_UF2
#define DOUBLE_TAP_MAGIC 0xf01669ef
#elif CONFIG_BOOTLOADER_BOSSA_ARDUINO
#define DOUBLE_TAP_MAGIC 0x07738135
#else
#error Unsupported BOSSA bootloader variant
#endif

void soc_reboot_to_bootloader_hook(void)
{
	uint32_t *top = (uint32_t *)(DT_REG_ADDR(DT_NODELABEL(sram0)) +
				     DT_REG_SIZE(DT_NODELABEL(sram0)));

	/* The bootloader looks for the magic value in the last word of SRAM. */
	top[-1] = DOUBLE_TAP_MAGIC;

	NVIC_SystemReset();
}
