/*
 * Copyright (c) 2026 David Paul-Beier
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Cadence watchdog timer (r1p2), e.g. the Zynq-7000 System Watchdog Timer.
 *
 * The 24-bit down-counter is loaded with {CRV, 12'b0} and decremented at the
 * input clock divided by the CLKSEL prescaler. On expiry the block can drive
 * a system reset. MODE, CONTROL and RESTART writes need a key.
 */

#define DT_DRV_COMPAT cdns_wdt_r1p2

#include <zephyr/device.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(wdt_cdns, CONFIG_WDT_LOG_LEVEL);

#define CDNS_WDT_MODE    0x00U
#define CDNS_WDT_CONTROL 0x04U
#define CDNS_WDT_RESTART 0x08U

#define CDNS_WDT_MODE_WDEN      BIT(0)
#define CDNS_WDT_MODE_RSTEN     BIT(1)
/*
 * IRQLN [8:7] and RSTLN [6:4]: keep the reset values. With RSTLN = 0 the
 * reset pulse is too short and no reset happens.
 */
#define CDNS_WDT_MODE_PULSE_LEN 0x1C0U
#define CDNS_WDT_MODE_ZKEY      (0xABCU << 12)

#define CDNS_WDT_CONTROL_CKEY      (0x248U << 14)
#define CDNS_WDT_CONTROL_CRV_SHIFT 2U
#define CDNS_WDT_CONTROL_CRV_MAX   0xFFFU

#define CDNS_WDT_RESTART_KEY 0x1999U

/* One CRV step is 4096 prescaled clock cycles */
#define CDNS_WDT_CRV_UNIT 4096U

static const uint32_t cdns_wdt_prescaler[] = {8U, 64U, 512U, 4096U};

struct wdt_cdns_config {
	DEVICE_MMIO_ROM;
	uint32_t clk_freq;
};

struct wdt_cdns_data {
	DEVICE_MMIO_RAM;
	uint32_t control;
	bool reset_soc;
	bool installed;
	bool enabled;
};

/* Finest prescaler whose CRV range reaches the timeout */
static int wdt_cdns_calc_control(const struct device *dev, uint32_t timeout_ms, uint32_t *control)
{
	const struct wdt_cdns_config *config = dev->config;
	uint64_t ticks = (uint64_t)config->clk_freq * timeout_ms / 1000U;

	for (size_t sel = 0; sel < ARRAY_SIZE(cdns_wdt_prescaler); sel++) {
		uint64_t crv = ticks / ((uint64_t)cdns_wdt_prescaler[sel] * CDNS_WDT_CRV_UNIT);

		if ((crv >= 1U) && (crv <= CDNS_WDT_CONTROL_CRV_MAX)) {
			*control = CDNS_WDT_CONTROL_CKEY |
				   ((uint32_t)crv << CDNS_WDT_CONTROL_CRV_SHIFT) | (uint32_t)sel;
			return 0;
		}
	}

	return -EINVAL;
}

static int wdt_cdns_install_timeout(const struct device *dev, const struct wdt_timeout_cfg *cfg)
{
	struct wdt_cdns_data *data = dev->data;
	uint32_t flags = cfg->flags & WDT_FLAG_RESET_MASK;
	uint32_t control;
	int ret;

	if (data->enabled) {
		return -EBUSY;
	}

	if (data->installed) {
		return -ENOMEM;
	}

	if (cfg->callback != NULL) {
		return -ENOTSUP;
	}

	if ((flags != WDT_FLAG_RESET_SOC) && (flags != WDT_FLAG_RESET_NONE)) {
		return -ENOTSUP;
	}

	if (cfg->window.min != 0U) {
		return -EINVAL;
	}

	ret = wdt_cdns_calc_control(dev, cfg->window.max, &control);
	if (ret < 0) {
		LOG_ERR("Timeout %u ms out of range", cfg->window.max);
		return ret;
	}

	data->control = control;
	data->reset_soc = (flags == WDT_FLAG_RESET_SOC);
	data->installed = true;

	return 0;
}

static int wdt_cdns_setup(const struct device *dev, uint8_t options)
{
	struct wdt_cdns_data *data = dev->data;
	mm_reg_t base = DEVICE_MMIO_GET(dev);
	uint32_t mode = CDNS_WDT_MODE_ZKEY | CDNS_WDT_MODE_PULSE_LEN | CDNS_WDT_MODE_WDEN;

	if (options != 0U) {
		return -ENOTSUP;
	}

	if (!data->installed) {
		return -EINVAL;
	}

	if (data->enabled) {
		return -EBUSY;
	}

	if (data->reset_soc) {
		mode |= CDNS_WDT_MODE_RSTEN;
	}

	sys_write32(data->control, base + CDNS_WDT_CONTROL);
	sys_write32(mode, base + CDNS_WDT_MODE);
	sys_write32(CDNS_WDT_RESTART_KEY, base + CDNS_WDT_RESTART);
	data->enabled = true;

	return 0;
}

static int wdt_cdns_disable(const struct device *dev)
{
	struct wdt_cdns_data *data = dev->data;
	mm_reg_t base = DEVICE_MMIO_GET(dev);

	if (!data->enabled) {
		return -EFAULT;
	}

	sys_write32(CDNS_WDT_MODE_ZKEY | CDNS_WDT_MODE_PULSE_LEN, base + CDNS_WDT_MODE);
	data->enabled = false;
	data->installed = false;

	return 0;
}

static int wdt_cdns_feed(const struct device *dev, int channel_id)
{
	mm_reg_t base = DEVICE_MMIO_GET(dev);

	if (channel_id != 0) {
		return -EINVAL;
	}

	sys_write32(CDNS_WDT_RESTART_KEY, base + CDNS_WDT_RESTART);

	return 0;
}

static DEVICE_API(wdt, wdt_cdns_api) = {
	.setup = wdt_cdns_setup,
	.disable = wdt_cdns_disable,
	.install_timeout = wdt_cdns_install_timeout,
	.feed = wdt_cdns_feed,
};

static int wdt_cdns_init(const struct device *dev)
{
	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	if (IS_ENABLED(CONFIG_WDT_DISABLE_AT_BOOT)) {
		sys_write32(CDNS_WDT_MODE_ZKEY | CDNS_WDT_MODE_PULSE_LEN,
			    DEVICE_MMIO_GET(dev) + CDNS_WDT_MODE);
	}

	return 0;
}

#define WDT_CDNS_INIT(n)                                                                           \
	static struct wdt_cdns_data wdt_cdns_data_##n;                                             \
	static const struct wdt_cdns_config wdt_cdns_config_##n = {                                \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                                              \
		.clk_freq = DT_INST_PROP(n, clock_frequency),                                      \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, wdt_cdns_init, NULL, &wdt_cdns_data_##n, &wdt_cdns_config_##n,    \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &wdt_cdns_api);

DT_INST_FOREACH_STATUS_OKAY(WDT_CDNS_INIT)
