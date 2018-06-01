/*
 * Copyright (c) 2017 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <watchdog.h>

#define WDT_REGS ((Wdt *)CONFIG_WDT_SAM0_BASE_ADDRESS)

struct wdt_sam0_dev_data {
	void (*cb)(struct device *dev);
};

static struct device DEVICE_NAME_GET(wdt_sam0);

static void wdt_sam0_wait_synchronization(void)
{
	while (WDT_REGS->STATUS.bit.SYNCBUSY) {
	}
}

static void wdt_sam0_isr(struct device *dev)
{
	struct wdt_sam0_dev_data *data = dev->driver_data;

	WDT_REGS->INTFLAG.reg = WDT_INTFLAG_EW;
	if (data->cb != NULL) {
		data->cb(dev);
	}
}

static void wdt_sam0_enable(struct device *dev)
{
	WDT_REGS->CTRL.reg = WDT_CTRL_ENABLE;
	wdt_sam0_wait_synchronization();
}

static int wdt_sam0_disable(struct device *dev)
{
	WDT_REGS->CTRL.reg = 0;
	wdt_sam0_wait_synchronization();

	return 0;
}

static int wdt_sam0_set_config(struct device *dev, struct wdt_config *config)
{
	struct wdt_sam0_dev_data *data = dev->driver_data;
	WDT_CTRL_Type ctrl = WDT_REGS->CTRL;
	int divisor;

	/* As per wdt_esp32.c, the Zephyr watchdog API is modeled
	 * after the Quark MCU where:
	 *
	 *  timeout_ms = 2**(config->timeout + 11) / 1000
	 *
	 * The SAM0 is also power-of-two based with a 1 kHz clock, so
	 *  2**14 / 1kHz ~= 2**29 / 32 MHz.
	 */
	divisor = config->timeout + WDT_CONFIG_PER_16K_Val - WDT_2_29_CYCLES;

	/* Limit to 16x so that 8x is available for early warning. */
	if (divisor < WDT_CONFIG_PER_16_Val) {
		return -EINVAL;
	} else if (divisor > WDT_CONFIG_PER_16K_Val) {
		return -EINVAL;
	}

	/* Disable the WDT to change the config. */
	wdt_sam0_disable(dev);

	switch (config->mode) {
	case WDT_MODE_RESET:
		WDT_REGS->INTENCLR.reg = WDT_INTENCLR_EW;
		wdt_sam0_wait_synchronization();
		break;

	case WDT_MODE_INTERRUPT_RESET:
		/* Fire the early warning earlier. */
		WDT_REGS->EWCTRL.bit.EWOFFSET = divisor - 1;
		wdt_sam0_wait_synchronization();

		/* Clear the pending interrupt, if any. */
		WDT_REGS->INTFLAG.reg = WDT_INTFLAG_EW;
		wdt_sam0_wait_synchronization();

		WDT_REGS->INTENSET.reg = WDT_INTENSET_EW;
		wdt_sam0_wait_synchronization();
		break;

	default:
		return -EINVAL;
	}

	WDT_REGS->CONFIG.bit.PER = divisor;
	wdt_sam0_wait_synchronization();

	data->cb = config->interrupt_fn;

	WDT_REGS->CTRL = ctrl;
	wdt_sam0_wait_synchronization();

	return 0;
}

static void wdt_sam0_get_config(struct device *dev, struct wdt_config *config)
{
	struct wdt_sam0_dev_data *data = dev->driver_data;

	if (WDT_REGS->INTENSET.bit.EW) {
		config->mode = WDT_MODE_INTERRUPT_RESET;
	} else {
		config->mode = WDT_MODE_RESET;
	}

	config->timeout = WDT_REGS->CONFIG.bit.PER
		+ WDT_2_29_CYCLES - WDT_CONFIG_PER_16K_Val;
	config->interrupt_fn = data->cb;
}

static void wdt_sam0_reload(struct device *dev)
{
	WDT_REGS->CLEAR.bit.CLEAR = WDT_CLEAR_CLEAR_KEY_Val;
}

static const struct wdt_driver_api wdt_sam0_api = {
	.enable = wdt_sam0_enable,
	.disable = wdt_sam0_disable,
	.get_config = wdt_sam0_get_config,
	.set_config = wdt_sam0_set_config,
	.reload = wdt_sam0_reload,
};

static int wdt_sam0_init(struct device *dev)
{
	/* Enable APB clock */
	PM->APBAMASK.bit.WDT_ = 1;

	/* Connect to GCLK2 (~1 kHz) */
	GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID_WDT
		| GCLK_CLKCTRL_GEN_GCLK2
		| GCLK_CLKCTRL_CLKEN;

	IRQ_CONNECT(CONFIG_WDT_SAM0_IRQ,
		    CONFIG_WDT_SAM0_IRQ_PRIORITY, wdt_sam0_isr,
		    DEVICE_GET(wdt_sam0), 0);
	irq_enable(CONFIG_WDT_SAM0_IRQ);

	return 0;
}

static struct wdt_sam0_dev_data wdt_sam0_data;

DEVICE_AND_API_INIT(wdt_sam0, CONFIG_WDT_0_NAME, wdt_sam0_init,
		    &wdt_sam0_data, NULL, PRE_KERNEL_1,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &wdt_sam0_api);
