/*
 * Copyright (c) 2025 Realtek Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT realtek_rts5817_watchdog

#include <zephyr/kernel.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/irq.h>
#include "wdt_rts5817.h"

#define LOG_LEVEL CONFIG_WDT_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wdt_rts5817);

#define WDOG_TIME_1S 0x0
#define WDOG_TIME_2S 0x1
#define WDOG_TIME_4S 0x2
#define WDOG_TIME_8S 0x3

struct rts_fp_wdt_config {
	mem_addr_t base;
};

struct rts_fp_wdt_data {
	wdt_callback_t callback;
};

static inline uint32_t rts_fp_read_reg(const struct device *dev)
{
	const struct rts_fp_wdt_config *config = dev->config;

	return sys_read32(config->base);
}

static inline void rts_fp_write_reg(const struct device *dev, uint32_t value)
{
	const struct rts_fp_wdt_config *config = dev->config;

	sys_write32(value, config->base);
}

static void rts_fp_wdt_isr(const struct device *dev)
{
	struct rts_fp_wdt_data *data = dev->data;
	uint32_t value;

	value = rts_fp_read_reg(dev);

	/* Only handle the interrupt when it is actually pending */
	if (!(value & WDOG_INT_MASK)) {
		return;
	}

	/* Clear interrupt flag and count */
	value |= WDOG_INT_CLR_MASK | WDOG_CLR_MASK;
	rts_fp_write_reg(dev, value);

	if (data->callback != NULL) {
		data->callback(dev, 0);
	}
}

static int rts_fp_wdt_setup(const struct device *dev, uint8_t options)
{
	uint32_t value;

	if ((options & WDT_OPT_PAUSE_IN_SLEEP) || (options & WDT_OPT_PAUSE_HALTED_BY_DBG)) {
		LOG_ERR("Pause in sleep or halted by dbg is not supported");
		return -ENOTSUP;
	}

	value = rts_fp_read_reg(dev);
	value |= WDOG_EN_MASK;
	rts_fp_write_reg(dev, value);

	return 0;
}

static int rts_fp_wdt_disable(const struct device *dev)
{
	uint32_t value;

	value = rts_fp_read_reg(dev);
	value &= ~WDOG_EN_MASK;
	rts_fp_write_reg(dev, value);

	return 0;
}

static int rts_fp_wdt_install_timeout(const struct device *dev, const struct wdt_timeout_cfg *cfg)
{
	struct rts_fp_wdt_data *data = dev->data;
	uint32_t value;

	if (cfg->window.min > 0 || cfg->window.max > 8000) {
		LOG_ERR("watchdog window.min should be equal to 0 and window.max should not exceed "
			"8000");
		return -ENOTSUP;
	}

	value = rts_fp_read_reg(dev);

	/* Set mode of watchdog and callback */
	switch (cfg->flags) {
	case WDT_FLAG_RESET_CPU_CORE:
		return -ENOTSUP;
	case WDT_FLAG_RESET_NONE:
		value &= ~(WDOG_REG_RST_EN_MASK | WDOG_BUS_RST_EN_MASK);
		value |= WDOG_INT_EN_MASK;
		data->callback = cfg->callback;
		break;
	case WDT_FLAG_RESET_SOC:
		value |= WDOG_REG_RST_EN_MASK | WDOG_BUS_RST_EN_MASK;
		value &= ~WDOG_INT_EN_MASK;
		if (cfg->callback != NULL) {
			LOG_WRN("watchdog callback is not supported if WDT_FLAG_RESET_SOC");
		}
		break;
	default:
		LOG_ERR("unknown watchdog flags");
		return -EINVAL;
	}

	if (cfg->window.max <= 1000) {
		value |= WDOG_TIME_1S << WDOG_TIME_OFFSET;
	} else if (cfg->window.max > 1000 && cfg->window.max <= 2000) {
		value |= WDOG_TIME_2S << WDOG_TIME_OFFSET;
	} else if (cfg->window.max > 2000 && cfg->window.max <= 4000) {
		value |= WDOG_TIME_4S << WDOG_TIME_OFFSET;
	} else if (cfg->window.max > 4000 && cfg->window.max <= 8000) {
		value |= WDOG_TIME_8S << WDOG_TIME_OFFSET;
	} else {
		return -ENOTSUP;
	}

	rts_fp_write_reg(dev, value);

	return 0;
}

static int rts_fp_wdt_feed(const struct device *dev, int channel_id)
{
	uint32_t value;

	value = rts_fp_read_reg(dev);
	value |= WDOG_CLR_MASK;
	rts_fp_write_reg(dev, value);

	return 0;
}

static int rts_fp_wdt_init(const struct device *dev)
{
	/* Note: Watchdog is enabled by default for rts5817.
	 * so CONFIG_WDT_DISABLE_AT_BOOT needs to be selected by default
	 */

	if (IS_ENABLED(CONFIG_WDT_DISABLE_AT_BOOT)) {
		rts_fp_wdt_disable(dev);
	}

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), rts_fp_wdt_isr,
		    DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));

	return 0;
}

static DEVICE_API(wdt, rts_fp_wdt_driver_api) = {
	.setup = rts_fp_wdt_setup,
	.disable = rts_fp_wdt_disable,
	.install_timeout = rts_fp_wdt_install_timeout,
	.feed = rts_fp_wdt_feed,
};

const struct rts_fp_wdt_config rts_fp_config = {
	.base = DT_INST_REG_ADDR(0),
};

static struct rts_fp_wdt_data rts_fp_data;

DEVICE_DT_INST_DEFINE(0, &rts_fp_wdt_init, NULL, &rts_fp_data, &rts_fp_config, PRE_KERNEL_1,
		      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &rts_fp_wdt_driver_api);

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) <= 1,
	     "Only one watchdog instance can be supported");
