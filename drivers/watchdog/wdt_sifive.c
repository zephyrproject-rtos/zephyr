/*
 * Copyright (C) 2020 Katsuhiro Suzuki
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Watchdog (WDT) Driver for SiFive Freedom
 */

#define DT_DRV_COMPAT sifive_wdt

#include <zephyr/kernel.h>
#include <soc.h>
#include <zephyr/drivers/watchdog.h>

#define LOG_LEVEL CONFIG_WDT_LOG_LEVEL
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(wdt_sifive);

#define WDOGCFG_SCALE_MAX     0xf
#define WDOGCFG_SCALE_SHIFT   0
#define WDOGCFG_SCALE_MASK    (WDOGCFG_SCALE_MAX << WDOGCFG_SCALE_SHIFT)
#define WDOGCFG_RSTEN         BIT(8)
#define WDOGCFG_ZEROCMP       BIT(9)
#define WDOGCFG_ENALWAYS      BIT(12)
#define WDOGCFG_COREAWAKE     BIT(13)
#define WDOGCFG_IP0           BIT(28)

#define WDOGCMP_MAX        0xffff

#define WDOG_KEY           0x51f15e
#define WDOG_FEED          0xd09f00d

#define WDOG_CLK           32768

struct wdt_sifive_reg {
	/* offset: 0x000 */
	uint32_t wdogcfg;
	uint32_t dummy0;
	uint32_t wdogcount;
	uint32_t dummy1;
	/* offset: 0x010 */
	uint32_t wdogs;
	uint32_t dummy2;
	uint32_t wdogfeed;
	uint32_t wdogkey;
	/* offset: 0x020 */
	uint32_t wdogcmp0;
};

struct wdt_sifive_device_config {
	uintptr_t regs;
};

struct wdt_sifive_dev_data {
	wdt_callback_t cb;
	bool enable_cb;
	bool timeout_valid;
};

#define DEV_REG(dev) \
	((struct wdt_sifive_reg *) \
	 ((const struct wdt_sifive_device_config *const)(dev)->config)->regs)

/**
 * @brief Set maximum length of timeout to watchdog
 *
 * @param dev Watchdog device struct
 */
static void wdt_sifive_set_max_timeout(const struct device *dev)
{
	volatile struct wdt_sifive_reg *wdt = DEV_REG(dev);
	uint32_t t;

	t = wdt->wdogcfg;
	t |= WDOGCFG_SCALE_MASK;

	wdt->wdogkey = WDOG_KEY;
	wdt->wdogcfg = t;
	wdt->wdogkey = WDOG_KEY;
	wdt->wdogcmp0 = WDOGCMP_MAX;
}

static void wdt_sifive_isr(const struct device *dev)
{
	volatile struct wdt_sifive_reg *wdt = DEV_REG(dev);
	struct wdt_sifive_dev_data *data = dev->data;
	uint32_t t;

	wdt_sifive_set_max_timeout(dev);

	t = wdt->wdogcfg;
	t &= ~WDOGCFG_IP0;

	wdt->wdogkey = WDOG_KEY;
	wdt->wdogcfg = t;

	if (data->enable_cb && data->cb) {
		data->enable_cb = false;
		data->cb(dev, 0);
	}
}

static int wdt_sifive_disable(const struct device *dev)
{
	struct wdt_sifive_dev_data *data = dev->data;

	wdt_sifive_set_max_timeout(dev);

	data->enable_cb = false;

	return 0;
}

static int wdt_sifive_setup(const struct device *dev, uint8_t options)
{
	volatile struct wdt_sifive_reg *wdt = DEV_REG(dev);
	struct wdt_sifive_dev_data *data = dev->data;
	uint32_t t, mode;

	if (!data->timeout_valid) {
		LOG_ERR("No valid timeouts installed");
		return -EINVAL;
	}

	mode = WDOGCFG_ENALWAYS;
	if ((options & WDT_OPT_PAUSE_IN_SLEEP) ==
	    WDT_OPT_PAUSE_IN_SLEEP) {
		mode = WDOGCFG_COREAWAKE;
	}
	if ((options & WDT_OPT_PAUSE_HALTED_BY_DBG) ==
	    WDT_OPT_PAUSE_HALTED_BY_DBG) {
		mode = WDOGCFG_COREAWAKE;
	}

	t = wdt->wdogcfg;
	t &= ~(WDOGCFG_ENALWAYS | WDOGCFG_COREAWAKE);
	t |= mode;

	wdt->wdogkey = WDOG_KEY;
	wdt->wdogcfg = t;

	return 0;
}

/**
 * @brief Calculates the watchdog counter value (wdogcmp0) and
 *        scaler (wdogscale) to be installed in the watchdog timer
 *
 * @param timeout Timeout value in milliseconds.
 * @param clk     Clock of watchdog in Hz.
 * @param scaler  Pointer to return scaler power of 2
 *
 * @return Watchdog counter value
 */
static int wdt_sifive_convtime(uint32_t timeout, int clk, int *scaler)
{
	uint64_t cnt;
	int i;

	cnt = (uint64_t)timeout * clk / 1000;
	for (i = 0; i < 16; i++) {
		if (cnt <= WDOGCMP_MAX) {
			break;
		}

		cnt >>= 1;
	}

	if (i == 16) {
		/* Maximum counter and scaler */
		LOG_ERR("Invalid timeout value allowed range");

		*scaler = WDOGCFG_SCALE_MAX;
		return WDOGCMP_MAX;
	}

	*scaler = i;

	return cnt;
}

static int wdt_sifive_install_timeout(const struct device *dev,
				      const struct wdt_timeout_cfg *cfg)
{
	volatile struct wdt_sifive_reg *wdt = DEV_REG(dev);
	struct wdt_sifive_dev_data *data = dev->data;
	uint32_t mode = 0, t;
	int cmp, scaler;

	if (data->timeout_valid) {
		LOG_ERR("No more timeouts can be installed");
		return -ENOMEM;
	}
	if (cfg->window.min != 0U || cfg->window.max == 0U) {
		return -EINVAL;
	}

	/*
	 * Freedom watchdog does not support window timeout config.
	 * So use max field of window.
	 */
	cmp = wdt_sifive_convtime(cfg->window.max, WDOG_CLK, &scaler);
	if (cmp < 0 || WDOGCMP_MAX < cmp) {
		LOG_ERR("Unsupported watchdog timeout\n");
		return -EINVAL;
	}

	switch (cfg->flags) {
	case WDT_FLAG_RESET_SOC:
		/* WDT supports global SoC reset but cannot callback. */
		mode = WDOGCFG_RSTEN | WDOGCFG_ZEROCMP;
		break;
	case WDT_FLAG_RESET_NONE:
		/* No reset */
		mode = WDOGCFG_ZEROCMP;
		break;
	case WDT_FLAG_RESET_CPU_CORE:
	default:
		LOG_ERR("Unsupported watchdog config flags\n");

		wdt_sifive_disable(dev);
		return -ENOTSUP;
	}

	t = wdt->wdogcfg;
	t &= ~(WDOGCFG_RSTEN | WDOGCFG_ZEROCMP | WDOGCFG_SCALE_MASK);
	t |= mode | scaler;

	wdt->wdogkey = WDOG_KEY;
	wdt->wdogcfg = t;
	wdt->wdogkey = WDOG_KEY;
	wdt->wdogcmp0 = cmp;

	data->cb = cfg->callback;
	data->enable_cb = true;
	data->timeout_valid = true;

	return 0;
}

static int wdt_sifive_feed(const struct device *dev, int channel_id)
{
	volatile struct wdt_sifive_reg *wdt = DEV_REG(dev);

	wdt->wdogkey = WDOG_KEY;
	wdt->wdogfeed = WDOG_FEED;

	return 0;
}

static const struct wdt_driver_api wdt_sifive_api = {
	.setup = wdt_sifive_setup,
	.disable = wdt_sifive_disable,
	.install_timeout = wdt_sifive_install_timeout,
	.feed = wdt_sifive_feed,
};

static void wdt_sifive_irq_config(void)
{
	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority), wdt_sifive_isr,
		    DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));
}

static int wdt_sifive_init(const struct device *dev)
{
#ifdef CONFIG_WDT_DISABLE_AT_BOOT
	wdt_sifive_disable(dev);
#endif
	wdt_sifive_irq_config();

	return 0;
}

static struct wdt_sifive_dev_data wdt_sifive_data;

static const struct wdt_sifive_device_config wdt_sifive_cfg = {
	.regs = DT_INST_REG_ADDR(0),
};

DEVICE_DT_INST_DEFINE(0, wdt_sifive_init, NULL,
		      &wdt_sifive_data, &wdt_sifive_cfg, PRE_KERNEL_1,
		      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &wdt_sifive_api);
