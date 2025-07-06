/*
 * Copyright (c) 2018 Henrik Brix Andersen <henrik@brixandersen.dk>
 * Copyright (c) 2017 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam0_watchdog

#include <soc.h>
#include <zephyr/drivers/watchdog.h>

#define LOG_LEVEL CONFIG_WDT_LOG_LEVEL
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(wdt_sam0);

#define WDT_REGS ((Wdt *)DT_INST_REG_ADDR(0))

#ifndef WDT_CONFIG_PER_8_Val
#define WDT_CONFIG_PER_8_Val WDT_CONFIG_PER_CYC8_Val
#endif
#ifndef WDT_CONFIG_PER_8K_Val
#define WDT_CONFIG_PER_8K_Val WDT_CONFIG_PER_CYC8192_Val
#endif
#ifndef WDT_CONFIG_PER_16K_Val
#define WDT_CONFIG_PER_16K_Val WDT_CONFIG_PER_CYC16384_Val
#endif

/* syncbusy check is different for SAM D/E */
#ifdef WDT_STATUS_SYNCBUSY
#define WDT_SYNCBUSY WDT_REGS->STATUS.bit.SYNCBUSY
#else
#define WDT_SYNCBUSY WDT_REGS->SYNCBUSY.reg
#endif

struct wdt_sam0_dev_data {
	wdt_callback_t cb;
	bool timeout_valid;
};

static struct wdt_sam0_dev_data wdt_sam0_data = { 0 };

static void wdt_sam0_wait_synchronization(void)
{
	while (WDT_SYNCBUSY) {
		/* wait for SYNCBUSY */
	}
}

static inline void wdt_sam0_set_enable(bool on)
{
#ifdef WDT_CTRLA_ENABLE
	WDT_REGS->CTRLA.bit.ENABLE = on;
#else
	WDT_REGS->CTRL.bit.ENABLE = on;
#endif
}

static inline bool wdt_sam0_is_enabled(void)
{
#ifdef WDT_CTRLA_ENABLE
	return WDT_REGS->CTRLA.bit.ENABLE;
#else
	return WDT_REGS->CTRL.bit.ENABLE;
#endif
}

static uint32_t wdt_sam0_timeout_to_wdt_period(uint32_t timeout_ms)
{
	uint32_t next_pow2;
	uint32_t cycles;

	/* Calculate number of clock cycles @ 1.024 kHz input clock */
	cycles = (timeout_ms * 1024U) / 1000;

	/* Minimum wdt period is 8 clock cycles (register value 0) */
	if (cycles <= 8U) {
		return 0;
	}

	/* Round up to next pow2 and calculate the register value */
	next_pow2 = (1ULL << 32) >> __builtin_clz(cycles - 1);
	return find_msb_set(next_pow2 >> 4);
}

static void wdt_sam0_isr(const struct device *dev)
{
	struct wdt_sam0_dev_data *data = dev->data;

	WDT_REGS->INTFLAG.reg = WDT_INTFLAG_EW;

	if (data->cb != NULL) {
		data->cb(dev, 0);
	}
}

static int wdt_sam0_setup(const struct device *dev, uint8_t options)
{
	struct wdt_sam0_dev_data *data = dev->data;

	if (wdt_sam0_is_enabled()) {
		LOG_ERR("Watchdog already setup");
		return -EBUSY;
	}

	if (!data->timeout_valid) {
		LOG_ERR("No valid timeout installed");
		return -EINVAL;
	}

	if (options & WDT_OPT_PAUSE_IN_SLEEP) {
		LOG_ERR("Pause in sleep not supported");
		return -ENOTSUP;
	}

	if (options & WDT_OPT_PAUSE_HALTED_BY_DBG) {
		LOG_ERR("Pause when halted by debugger not supported");
		return -ENOTSUP;
	}

	/* Enable watchdog */
	wdt_sam0_set_enable(1);
	wdt_sam0_wait_synchronization();

	return 0;
}

static int wdt_sam0_disable(const struct device *dev)
{
	if (!wdt_sam0_is_enabled()) {
		return -EFAULT;
	}

	wdt_sam0_set_enable(0);
	wdt_sam0_wait_synchronization();

	return 0;
}

static int wdt_sam0_install_timeout(const struct device *dev,
				    const struct wdt_timeout_cfg *cfg)
{
	struct wdt_sam0_dev_data *data = dev->data;
	uint32_t window, per;

	/* CONFIG is enable protected, error out if already enabled */
	if (wdt_sam0_is_enabled()) {
		LOG_ERR("Watchdog already setup");
		return -EBUSY;
	}

	if (cfg->flags != WDT_FLAG_RESET_SOC) {
		LOG_ERR("Only SoC reset supported");
		return -ENOTSUP;
	}

	if (cfg->window.max == 0) {
		LOG_ERR("Upper limit timeout out of range");
		return -EINVAL;
	}

	per = wdt_sam0_timeout_to_wdt_period(cfg->window.max);
	if (per > WDT_CONFIG_PER_16K_Val) {
		LOG_ERR("Upper limit timeout out of range");
		goto timeout_invalid;
	}

	if (cfg->window.min) {
		/* Window mode */
		window = wdt_sam0_timeout_to_wdt_period(cfg->window.min);
		if (window > WDT_CONFIG_PER_8K_Val) {
			LOG_ERR("Lower limit timeout out of range");
			goto timeout_invalid;
		}
		if (per <= window) {
			/* Ensure we have a window */
			per = window + 1;
		}
#ifdef WDT_CTRLA_WEN
		WDT_REGS->CTRLA.bit.WEN = 1;
#else
		WDT_REGS->CTRL.bit.WEN = 1;
#endif
		wdt_sam0_wait_synchronization();
	} else {
		/* Normal mode */
		if (cfg->callback) {
			if (per == WDT_CONFIG_PER_8_Val) {
				/* Ensure we have time for the early warning */
				per += 1U;
			}
			WDT_REGS->EWCTRL.bit.EWOFFSET = per - 1U;
		}
		window = WDT_CONFIG_PER_8_Val;
#ifdef WDT_CTRLA_WEN
		WDT_REGS->CTRLA.bit.WEN = 0;
#else
		WDT_REGS->CTRL.bit.WEN = 0;
#endif
		wdt_sam0_wait_synchronization();
	}

	WDT_REGS->CONFIG.reg = WDT_CONFIG_WINDOW(window) | WDT_CONFIG_PER(per);
	wdt_sam0_wait_synchronization();

	/* Only enable IRQ if a callback was provided */
	data->cb = cfg->callback;
	if (data->cb) {
		WDT_REGS->INTENSET.reg = WDT_INTENSET_EW;
	} else {
		WDT_REGS->INTENCLR.reg = WDT_INTENCLR_EW;
		WDT_REGS->INTFLAG.reg = WDT_INTFLAG_EW;
	}

	data->timeout_valid = true;

	return 0;

timeout_invalid:
	data->timeout_valid = false;
	data->cb = NULL;

	return -EINVAL;
}

static int wdt_sam0_feed(const struct device *dev, int channel_id)
{
	struct wdt_sam0_dev_data *data = dev->data;

	if (!data->timeout_valid) {
		LOG_ERR("No valid timeout installed");
		return -EINVAL;
	}

	if (WDT_SYNCBUSY) {
		return -EAGAIN;
	}

	WDT_REGS->CLEAR.reg = WDT_CLEAR_CLEAR_KEY_Val;

	return 0;
}

static DEVICE_API(wdt, wdt_sam0_api) = {
	.setup = wdt_sam0_setup,
	.disable = wdt_sam0_disable,
	.install_timeout = wdt_sam0_install_timeout,
	.feed = wdt_sam0_feed,
};

static int wdt_sam0_init(const struct device *dev)
{
#ifdef CONFIG_WDT_DISABLE_AT_BOOT
	/* Ignore any errors */
	wdt_sam0_disable(dev);
#endif
	/* Enable APB clock */
#ifdef MCLK
	MCLK->APBAMASK.bit.WDT_ = 1;

	/* watchdog clock is fed by OSCULP32K */
#else
	PM->APBAMASK.bit.WDT_ = 1;

	/* Connect to GCLK2 (~1.024 kHz) */
	GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID_WDT
		| GCLK_CLKCTRL_GEN_GCLK2
		| GCLK_CLKCTRL_CLKEN;
#endif

	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority), wdt_sam0_isr,
		    DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));

	return 0;
}

static struct wdt_sam0_dev_data wdt_sam0_data;

DEVICE_DT_INST_DEFINE(0, wdt_sam0_init, NULL,
		    &wdt_sam0_data, NULL, PRE_KERNEL_1,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &wdt_sam0_api);
