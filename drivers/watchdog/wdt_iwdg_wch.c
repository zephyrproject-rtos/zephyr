/*
 * Copyright (c) 2025 Michael Hope <michaelh@juju.nz>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT wch_iwdg

#include <zephyr/drivers/watchdog.h>
#include <zephyr/kernel.h>
#include <zephyr/sys_clock.h>
#include <errno.h>

#include <hal_ch32fun.h>

static int iwdg_wch_setup(const struct device *dev, uint8_t options)
{
	if (options != 0) {
		return -ENOTSUP;
	}

	IWDG->CTLR = CTLR_KEY_Enable;

	return 0;
}

static int iwdg_wch_disable(const struct device *dev)
{
	return -EPERM;
}

static int iwdg_wch_install_timeout(const struct device *dev,
					 const struct wdt_timeout_cfg *config)
{
	int prescaler = 0;
	/* The IWDT is driven by the 128 kHz LSI oscillator with at least a /4 prescaler. */
	uint32_t lsi_frequency = DT_PROP(DT_NODELABEL(clk_lsi), clock_frequency);
	uint32_t reload = config->window.max * (lsi_frequency / 1000 / 4);

	if (config->callback != NULL) {
		return -ENOTSUP;
	}
	if (config->window.min != 0) {
		return -ENOTSUP;
	}
	if ((config->flags & WDT_FLAG_RESET_MASK) != WDT_FLAG_RESET_SOC) {
		return -ENOTSUP;
	}

	for (; reload > IWDG_RL && prescaler < IWDG_PR;) {
		prescaler++;
		reload /= 2;
	}
	if (reload > IWDG_RL) {
		/* The reload is too high even with the maximum prescaler */
		return -EINVAL;
	}

	/* Wait for the watchdog to be idle, unlock it, update, and wait for idle. */
	while ((IWDG->STATR & (IWDG_RVU | IWDG_PVU)) != 0) {
	}

	IWDG->CTLR = IWDG_WriteAccess_Enable;
	IWDG->PSCR = prescaler;
	IWDG->RLDR = reload;

	while ((IWDG->STATR & (IWDG_RVU | IWDG_PVU)) != 0) {
	}

	return 0;
}

static int iwdg_wch_feed(const struct device *dev, int channel_id)
{
	IWDG->CTLR = CTLR_KEY_Reload;

	return 0;
}

static const struct wdt_driver_api iwdg_wch_api = {
	.setup = iwdg_wch_setup,
	.disable = iwdg_wch_disable,
	.install_timeout = iwdg_wch_install_timeout,
	.feed = iwdg_wch_feed,
};

static int iwdg_wch_init(const struct device *dev)
{
	return 0;
}

DEVICE_DT_INST_DEFINE(0, iwdg_wch_init, NULL, NULL, NULL, PRE_KERNEL_1,
		      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &iwdg_wch_api);
