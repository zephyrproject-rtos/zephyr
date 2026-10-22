/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/toolchain.h>
#include <zephyr/drivers/gpio.h>
#include <fsl_power.h>

#if CONFIG_GPIO && DT_NODE_EXISTS(DT_NODELABEL(btn_wk))

#if (DT_GPIO_FLAGS(DT_NODELABEL(btn_wk), gpios)) & GPIO_ACTIVE_LOW
#define POWEROFF_WAKEUP (kWAKEUP_PIN_ENABLE | \
			 kWAKEUP_PIN_PUP_EN | \
			 kWAKEUP_PIN_WAKEUP_LOW_LVL)
#else /* !GPIO_ACTIVE_LOW */
#define POWEROFF_WAKEUP (kWAKEUP_PIN_ENABLE | \
			 kWAKEUP_PIN_PDN_EN | \
			 kWAKEUP_PIN_WAKEUP_HIGH_LVL)
#endif /* GPIO_ACTIVE_LOW */

#else
#define POWEROFF_WAKEUP kWAKEUP_PIN_DISABLE
#endif /* CONFIG_GPIO && DT_NODE_EXISTS(DT_NODELABEL(btn_wk)) */

void z_sys_poweroff(void)
{
	POWER_EnterPowerOff(0, POWEROFF_WAKEUP);

	CODE_UNREACHABLE;
}
