/*
 * SPDX-FileCopyrightText: 2026 Aesc Silicon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/reset/aesc.h>
#include <zephyr/dt-bindings/reset/aesc-reset-controller.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/sys/util.h>

void sys_arch_reboot(int type)
{
	const uint32_t mask = BIT(HYDROGEN_RESET_SYSTEM) | BIT(HYDROGEN_RESET_FLASH);
	const struct device *rstctl = DEVICE_DT_GET(DT_NODELABEL(rstctl));

	ARG_UNUSED(type);

	if (reset_aesc_toggle_mask(rstctl, mask) != 0) {
		return;
	}

	/* The reset takes effect a few cycles after the request */
	for (;;) {
		arch_nop();
	}
}
