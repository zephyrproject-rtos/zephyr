/*
 * Copyright (C) 2017 Intel Deutschland GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam_watchdog

/**
 * @brief Watchdog (WDT) Driver for Atmel SAM MCUs
 *
 * Note:
 * - Once the watchdog disable bit is set, it cannot be cleared till next
 *   power reset, i.e, the watchdog cannot be started once stopped.
 * - Since the MCU boots with WDT enabled, the CONFIG_WDT_DISABLE_AT_BOOT
 *   is set default at boot and watchdog module is disabled in the MCU for
 *   systems that don't need watchdog functionality.
 * - If the application needs to use the watchdog in the system, then
 *   CONFIG_WDT_DISABLE_AT_BOOT must be unset in the app's config file
 */

#include <drivers/watchdog.h>
#include <soc.h>

#define LOG_LEVEL CONFIG_WDT_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(wdt_sam);

#define SAM_PRESCALAR   128
#define WDT_MAX_VALUE   4095

/* Device constant configuration parameters */
struct wdt_sam_dev_cfg {
	Wdt *regs;
};

struct wdt_sam_dev_data {
	wdt_callback_t cb;
	uint32_t mode;
	bool timeout_valid;
	bool mode_set;
};

static struct wdt_sam_dev_data wdt_sam_data = { 0 };

#define DEV_CFG(dev) \
	((const struct wdt_sam_dev_cfg *const)(dev)->config)

static void wdt_sam_isr(const struct device *dev)
{
	uint32_t wdt_sr;
	Wdt *const wdt = DEV_CFG(dev)->regs;
	struct wdt_sam_dev_data *data = dev->data;

	/* Clear status bit to acknowledge interrupt by dummy read. */
	wdt_sr = wdt->WDT_SR;

	data->cb(dev, 0);
}

/**
 * @brief Calculates the watchdog counter value (WDV)
 *        to be installed in the watchdog timer
 *
 * @param timeout Timeout value in milliseconds.
 * @param slow clock on board in Hz.
 */
int wdt_sam_convert_timeout(uint32_t timeout, uint32_t sclk)
{
	uint32_t max, min;

	timeout = timeout * 1000U;
	min =  (SAM_PRESCALAR * 1000000) / sclk;
	max = min * WDT_MAX_VALUE;
	if ((timeout < min) || (timeout > max)) {
		LOG_ERR("Invalid timeout value allowed range:"
			"%d ms to %d ms", min / 1000U, max / 1000U);
		return -EINVAL;
	}

	return WDT_MR_WDV(timeout / min);
}

static int wdt_sam_disable(const struct device *dev)
{
	Wdt *const wdt = DEV_CFG(dev)->regs;
	struct wdt_sam_dev_data *data = dev->data;

	/* since Watchdog mode register is 'write-once', we can't disable if
	 * someone has already set the mode register
	 */
	if (data->mode_set) {
		return -EPERM;
	}

	/* do we handle -EFAULT here */

	/* Watchdog Mode register is 'write-once' only register.
	 * Once disabled, it cannot be enabled until the device is reset
	 */
	wdt->WDT_MR |= WDT_MR_WDDIS;
	data->mode_set = true;

	return 0;
}

static int wdt_sam_setup(const struct device *dev, uint8_t options)
{

	Wdt *const wdt = DEV_CFG(dev)->regs;
	struct wdt_sam_dev_data *data = dev->data;

	if (!data->timeout_valid) {
		LOG_ERR("No valid timeouts installed");
		return -EINVAL;
	}

	/* since Watchdog mode register is 'write-once', we can't set if
	 * someone has already set the mode register
	 */
	if (data->mode_set) {
		return -EPERM;
	}

	if ((options & WDT_OPT_PAUSE_IN_SLEEP) == WDT_OPT_PAUSE_IN_SLEEP) {
		data->mode |= WDT_MR_WDIDLEHLT;
	}

	if ((options & WDT_OPT_PAUSE_HALTED_BY_DBG) ==
	    WDT_OPT_PAUSE_HALTED_BY_DBG) {
		data->mode |= WDT_MR_WDDBGHLT;
	}

	wdt->WDT_MR = data->mode;
	data->mode_set = true;

	return 0;
}

static int wdt_sam_install_timeout(const struct device *dev,
				   const struct wdt_timeout_cfg *cfg)
{
	uint32_t wdt_mode = 0U;
	int timeout_value;

	struct wdt_sam_dev_data *data = dev->data;

	if (data->timeout_valid) {
		LOG_ERR("No more timeouts can be installed");
		return -ENOMEM;
	}

	if (cfg->window.min != 0U) {
		return -EINVAL;
	}

	/*
	 * Convert time to cycles. SAM3X SoC doesn't supports window
	 * timeout config. So the api expects the timeout to be filled
	 * in the max field of the timeout config.
	 */
	timeout_value = wdt_sam_convert_timeout(cfg->window.max,
						(uint32_t) CHIP_FREQ_XTAL_32K);

	if (timeout_value < 0) {
		return -EINVAL;
	}

	switch (cfg->flags) {
	case WDT_FLAG_RESET_SOC:
		/*A Watchdog fault (underflow or error) activates all resets */
		wdt_mode = WDT_MR_WDRSTEN;  /* WDT reset enable */
		break;

	case WDT_FLAG_RESET_NONE:
		/* A Watchdog fault (underflow or error) asserts interrupt. */
		if (cfg->callback) {
			wdt_mode = WDT_MR_WDFIEN;   /* WDT fault interrupt. */
			data->cb = cfg->callback;
		} else {
			LOG_ERR("Invalid(NULL) ISR callback passed\n");
			return -EINVAL;
		}
		break;

		/* Processor only reset mode not available in same70 series */
#ifdef WDT_MR_WDRPROC
	case WDT_FLAG_RESET_CPU_CORE:
		/*A Watchdog fault activates the processor reset*/
		LOG_DBG("Configuring reset CPU only mode\n");
		wdt_mode = WDT_MR_WDRSTEN   |   /* WDT reset enable */
			   WDT_MR_WDRPROC;      /* WDT reset processor only*/
		break;
#endif
	default:
		LOG_ERR("Unsupported watchdog config Flag\n");
		return -ENOTSUP;
	}

	data->mode = wdt_mode |
		     WDT_MR_WDV(timeout_value) |
		     WDT_MR_WDD(timeout_value);

	data->timeout_valid = true;

	return 0;
}

static int wdt_sam_feed(const struct device *dev, int channel_id)
{
	/*
	 * On watchdog restart the Watchdog counter is immediately
	 * reloaded/feeded with the 12-bit watchdog counter
	 * value from WDT_MR and restarted
	 */
	Wdt *const wdt = DEV_CFG(dev)->regs;

	wdt->WDT_CR |= WDT_CR_KEY_PASSWD | WDT_CR_WDRSTT;

	return 0;
}

static const struct wdt_driver_api wdt_sam_api = {
	.setup = wdt_sam_setup,
	.disable = wdt_sam_disable,
	.install_timeout = wdt_sam_install_timeout,
	.feed = wdt_sam_feed,
};

static const struct wdt_sam_dev_cfg wdt_sam_cfg = {
	.regs = (Wdt *)DT_INST_REG_ADDR(0),
};

static void wdt_sam_irq_config(void)
{
	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority), wdt_sam_isr,
		    DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));
}

static int wdt_sam_init(const struct device *dev)
{
#ifdef CONFIG_WDT_DISABLE_AT_BOOT
	wdt_sam_disable(dev);
#endif

	wdt_sam_irq_config();
	return 0;
}

DEVICE_DT_INST_DEFINE(0, wdt_sam_init, device_pm_control_nop,
		    &wdt_sam_data, &wdt_sam_cfg, PRE_KERNEL_1,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &wdt_sam_api);
