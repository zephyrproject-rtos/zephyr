/*
 * Copyright (c) 2023 Andes Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT andestech_atcwdt200

#include <zephyr/kernel.h>
#include <zephyr/drivers/watchdog.h>

#define LOG_LEVEL CONFIG_WDT_LOG_LEVEL
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/syscon.h>
LOG_MODULE_REGISTER(wdt_andes);

/* Watchdog register */
#define REG_IDR			0x00
#define REG_CTRL		0x10
#define REG_RESTAR		0x14
#define REG_WREN		0x18
#define REG_STATUS		0x1c

#define WDT_CTRL(addr)		(addr + REG_CTRL)
#define WDT_RESTAR(addr)	(addr + REG_RESTAR)
#define WDT_WREN(addr)		(addr + REG_WREN)
#define WDT_STATUS(addr)	(addr + REG_STATUS)

/* Atcwdt200 magic number */
/* 0x10 Control Register */

#define WDT_CTRL_RSTTIME_POW_2_7	0x000
#define WDT_CTRL_RSTTIME_POW_2_8	0x100
#define WDT_CTRL_RSTTIME_POW_2_9	0x200
#define WDT_CTRL_RSTTIME_POW_2_10	0x300
#define WDT_CTRL_RSTTIME_POW_2_11	0x400
#define WDT_CTRL_RSTTIME_POW_2_12	0x500
#define WDT_CTRL_RSTTIME_POW_2_13	0x600
#define WDT_CTRL_RSTTIME_POW_2_14	0x700

#define WDT_CTRL_INTTIME_POW_2_6	0x000
#define WDT_CTRL_INTTIME_POW_2_8	0x010
#define WDT_CTRL_INTTIME_POW_2_10	0x020
#define WDT_CTRL_INTTIME_POW_2_11	0x030
#define WDT_CTRL_INTTIME_POW_2_12	0x040
#define WDT_CTRL_INTTIME_POW_2_13	0x050
#define WDT_CTRL_INTTIME_POW_2_14	0x060
#define WDT_CTRL_INTTIME_POW_2_15	0x070
#define WDT_CTRL_INTTIME_POW_2_17	0x080
#define WDT_CTRL_INTTIME_POW_2_19	0x090
#define WDT_CTRL_INTTIME_POW_2_21	0x0A0
#define WDT_CTRL_INTTIME_POW_2_23	0x0B0
#define WDT_CTRL_INTTIME_POW_2_25	0x0C0
#define WDT_CTRL_INTTIME_POW_2_27	0x0D0
#define WDT_CTRL_INTTIME_POW_2_29	0x0E0
#define WDT_CTRL_INTTIME_POW_2_31	0x0F0

#define WDT_CTRL_RSTEN			0x8
#define WDT_CTRL_INTEN			0x4
#define WDT_CTRL_APBCLK			0x2
#define WDT_CTRL_EXTCLK			0x0
#define WDT_CTRL_EN			0x1

/* Magic Number for Restart Register */
#define WDT_RESTART_NUM                 0xcafe

/* Magic Number for Write Enable Register */
#define WDT_WREN_NUM                    0x5aa5

/* 0x1C Status Register */
#define WDT_ST_INTEXPIRED               0x1
#define WDT_ST_INTEXPIRED_CLR           0x1

#define WDOGCFG_PERIOD_MIN		BIT(7)
#define WDOGCFG_PERIOD_MAX		BIT(14)
#define EXT_CLOCK_FREQ			BIT(15)

static const struct device *const pit_counter_dev =
				DEVICE_DT_GET(DT_NODELABEL(pit0));

struct counter_alarm_cfg alarm_cfg;

struct wdt_atcwdt200_config {
	uintptr_t base;
};

struct wdt_atcwdt200_dev_data {
	bool timeout_valid;
	counter_alarm_callback_t counter_callback;
	struct k_spinlock lock;
};

static int wdt_atcwdt200_disable(const struct device *dev);

static void wdt_counter_cb(const struct device *counter_dev, uint8_t chan_id,
			uint32_t counter,
			void *user_data)
{
	const struct device *dev = DEVICE_DT_INST_GET(0);
	struct wdt_atcwdt200_dev_data *wdt_data = dev->data;
	uint32_t wdt_addr = ((const struct wdt_atcwdt200_config *)(dev->config))->base;
	k_spinlock_key_t key;

	key = k_spin_lock(&wdt_data->lock);

	sys_write32(WDT_WREN_NUM, WDT_WREN(wdt_addr));
	sys_write32(WDT_RESTART_NUM, WDT_RESTAR(wdt_addr));

	counter_set_channel_alarm(counter_dev, 2, &alarm_cfg);

	k_spin_unlock(&wdt_data->lock, key);
}

/**
 * @brief Set maximum length of timeout to watchdog
 *
 * @param dev Watchdog device struct
 */
static void wdt_atcwdt200_set_max_timeout(const struct device *dev)
{
	struct wdt_atcwdt200_dev_data *data = dev->data;
	k_spinlock_key_t key;
	uint32_t wdt_addr = ((const struct wdt_atcwdt200_config *)(dev->config))->base;
	uint32_t reg, counter_freq;

	key = k_spin_lock(&data->lock);

	counter_freq = counter_get_frequency(pit_counter_dev);

	alarm_cfg.flags = 0;
	alarm_cfg.callback = wdt_counter_cb;
	alarm_cfg.user_data = &alarm_cfg;
	alarm_cfg.ticks = ((WDOGCFG_PERIOD_MAX * counter_freq) / EXT_CLOCK_FREQ) >> 1;

	reg = WDT_CTRL_RSTTIME_POW_2_14;

	sys_write32(WDT_WREN_NUM, WDT_WREN(wdt_addr));
	sys_write32(reg, WDT_CTRL(wdt_addr));

	data->timeout_valid = true;

	k_spin_unlock(&data->lock, key);
}

static int wdt_atcwdt200_disable(const struct device *dev)
{
	struct wdt_atcwdt200_dev_data *data = dev->data;
	uint32_t wdt_addr = ((const struct wdt_atcwdt200_config *)(dev->config))->base;
	k_spinlock_key_t key;
	uint32_t reg;

	key = k_spin_lock(&data->lock);

	reg = sys_read32(WDT_CTRL(wdt_addr));
	reg &= ~(WDT_CTRL_RSTEN | WDT_CTRL_EN);

	sys_write32(WDT_WREN_NUM, WDT_WREN(wdt_addr));
	sys_write32(reg, WDT_CTRL(wdt_addr));

	k_spin_unlock(&data->lock, key);

	wdt_atcwdt200_set_max_timeout(dev);
	counter_cancel_channel_alarm(pit_counter_dev, 2);

	return 0;
}

static int wdt_atcwdt200_setup(const struct device *dev, uint8_t options)
{
	struct wdt_atcwdt200_dev_data *data = dev->data;
	uint32_t wdt_addr = ((const struct wdt_atcwdt200_config *)(dev->config))->base;
	k_spinlock_key_t key;
	uint32_t reg;
	uint32_t ret = 0;

	if (!data->timeout_valid) {
		LOG_ERR("No valid timeouts installed");
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	reg = sys_read32(WDT_CTRL(wdt_addr));
	reg |= (WDT_CTRL_RSTEN | WDT_CTRL_EN);

	if ((options & WDT_OPT_PAUSE_HALTED_BY_DBG) ==
			WDT_OPT_PAUSE_HALTED_BY_DBG) {
		counter_cancel_channel_alarm(pit_counter_dev, 2);
		sys_write32(WDT_WREN_NUM, WDT_WREN(wdt_addr));
		sys_write32(reg, WDT_CTRL(wdt_addr));
		goto out;
	} else {
		ret = counter_set_channel_alarm(pit_counter_dev, 2, &alarm_cfg);
		if (ret != 0) {
			ret = -EINVAL;
			goto out;
		}

		sys_write32(WDT_WREN_NUM, WDT_WREN(wdt_addr));
		sys_write32(reg, WDT_CTRL(wdt_addr));
	}

out:
	k_spin_unlock(&data->lock, key);
	return ret;
}

/**
 * @brief Calculates the watchdog counter value (wdogcmp0) and
 *        scaler (wdogscale) to be installed in the watchdog timer
 *
 * @param timeout Timeout value in milliseconds.
 * @param scaler  Pointer to return scaler power of 2
 *
 * @return Watchdog counter value
 */
static uint32_t wdt_atcwdt200_convtime(uint32_t timeout, uint32_t *scaler)
{
	int i;
	uint32_t rst_period, cnt;

	cnt = (uint32_t)((timeout * EXT_CLOCK_FREQ) / 1000);
	rst_period = cnt;

	for (i = 0; i < 14 && cnt > 0; i++) {
		cnt >>= 1;
	}

	*scaler = i;

	return rst_period;
}

static int wdt_atcwdt200_install_timeout(const struct device *dev,
				      const struct wdt_timeout_cfg *cfg)
{
	struct wdt_atcwdt200_dev_data *data = dev->data;
	uint32_t wdt_addr = ((const struct wdt_atcwdt200_config *)(dev->config))->base;
	k_spinlock_key_t key;
	uint32_t rst_period, reg, counter_freq, scaler;

	if (cfg->window.min != 0U || cfg->window.max == 0U) {
		return -EINVAL;
	}

	counter_freq = counter_get_frequency(pit_counter_dev);
	rst_period = wdt_atcwdt200_convtime(cfg->window.max, &scaler);

	if (rst_period < 0 || WDOGCFG_PERIOD_MAX < rst_period) {
		LOG_ERR("Unsupported watchdog timeout\n");
		return -EINVAL;
	}

	wdt_atcwdt200_disable(dev);

	key = k_spin_lock(&data->lock);

	switch (cfg->flags) {
	case WDT_FLAG_RESET_SOC:
		if (scaler < 7) {
			reg = WDT_CTRL_RSTTIME_POW_2_7;
		} else {
			scaler = scaler - 7;
			reg = scaler << 8;
		}

		alarm_cfg.flags = 0;
		alarm_cfg.callback = wdt_counter_cb;
		alarm_cfg.user_data = &alarm_cfg;
		alarm_cfg.ticks = (((cfg->window.max * counter_freq) / 1000) >> 1);

		break;
	case WDT_FLAG_RESET_NONE:
	case WDT_FLAG_RESET_CPU_CORE:
	default:
		LOG_ERR("Unsupported watchdog config flags\n");
		k_spin_unlock(&data->lock, key);
		return -ENOTSUP;
	}

	sys_write32(WDT_WREN_NUM, WDT_WREN(wdt_addr));
	sys_write32(reg, WDT_CTRL(wdt_addr));

	k_spin_unlock(&data->lock, key);
	return 0;
}

static int wdt_atcwdt200_feed(const struct device *dev, int channel_id)
{
	uint32_t wdt_addr = ((const struct wdt_atcwdt200_config *)(dev->config))->base;

	ARG_UNUSED(channel_id);

	sys_write32(WDT_WREN_NUM, WDT_WREN(wdt_addr));
	sys_write32(WDT_RESTART_NUM, WDT_RESTAR(wdt_addr));

	return 0;
}

static const struct wdt_driver_api wdt_atcwdt200_api = {
	.setup = wdt_atcwdt200_setup,
	.disable = wdt_atcwdt200_disable,
	.install_timeout = wdt_atcwdt200_install_timeout,
	.feed = wdt_atcwdt200_feed,
};

static int wdt_atcwdt200_init(const struct device *dev)
{
	struct wdt_atcwdt200_dev_data *data = dev->data;

	data->timeout_valid = false;
	data->counter_callback = wdt_counter_cb;

	counter_start(pit_counter_dev);

#ifdef CONFIG_WDT_DISABLE_AT_BOOT
	wdt_atcwdt200_disable(dev);
#else
	data->timeout_valid = true;
	wdt_atcwdt200_set_max_timeout(dev);
	wdt_atcwdt200_setup(dev, 0x0);
#endif
	return 0;
}

static struct wdt_atcwdt200_dev_data wdt_atcwdt200_data;

static const struct wdt_atcwdt200_config wdt_atcwdt200_cfg = {
	.base = DT_INST_REG_ADDR(0),
};

DEVICE_DT_INST_DEFINE(0, wdt_atcwdt200_init, NULL,
		      &wdt_atcwdt200_data, &wdt_atcwdt200_cfg, PRE_KERNEL_2,
		      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &wdt_atcwdt200_api);
