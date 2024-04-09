/*
 * Driver for NEORV32 watchdog timer.
 *
 * Copyright © 2024 Calian Ltd.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT neorv32_wdt

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/syscon.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/drivers/hwinfo.h>
#include <soc.h>

enum neorv32_wdt_axi_register {
	REG_CTRL = 0x00,
	REG_RESET = 0x04,
};

enum neorv32_wdt_ctrl_bits {
	CTRL_TIMEOUT_MASK = BIT_MASK(24),
	CTRL_TIMEOUT_SHIFT = 8,
	CTRL_RCAUSE_MASK = BIT_MASK(2),
	CTRL_RCAUSE_SHIFT = 5,
	CTRL_STRICT = BIT(4),		/* strict mode */
	CTRL_SEN = BIT(3),		/* WDT continues running in sleep */
	CTRL_DBEN = BIT(2),		/* WDT continues running in debug mode */
	CTRL_LOCK = BIT(1),		/* lock configuration */
	CTRL_EN = BIT(0),		/* enable WDT */
};

enum neorv32_wdt_reset_cause {
	NEORV32_RESET_PIN = 0x0,
	NEORV32_RESET_DEBUG = 0x1,
	NEORV32_RESET_WATCHDOG = 0x2,
};

enum {
	RESET_PASSWORD = 0x709D1AB3,
	WDT_CLOCK_DIVIDER = 4096,
};

LOG_MODULE_REGISTER(wdt_neorv32, CONFIG_WDT_LOG_LEVEL);

struct neorv32_wdt_config {
	mem_addr_t base;
};

struct neorv32_wdt_data {
	struct k_spinlock lock;
	bool timeout_active;
	bool wdt_started;
};

static int wdt_neorv32_setup(const struct device *dev, uint8_t options)
{
	const struct neorv32_wdt_config *config = dev->config;
	struct neorv32_wdt_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	uint32_t ctrl = sys_read32(config->base + REG_CTRL);
	int ret;

	if (!data->timeout_active) {
		ret = -EINVAL;
		goto out;
	}

	if (data->wdt_started) {
		ret = -EBUSY;
		goto out;
	}

	if (options & WDT_OPT_PAUSE_IN_SLEEP) {
		ctrl &= ~CTRL_SEN;
	} else {
		ctrl |= CTRL_SEN;
	}
	if (options & WDT_OPT_PAUSE_HALTED_BY_DBG) {
		ctrl &= ~CTRL_DBEN;
	} else {
		ctrl |= CTRL_DBEN;
	}
	ctrl |= CTRL_EN;
	sys_write32(ctrl, config->base + REG_CTRL);
	data->wdt_started = true;
	ret = 0;

out:
	k_spin_unlock(&data->lock, key);
	return ret;
}

static int wdt_neorv32_disable(const struct device *dev)
{
	const struct neorv32_wdt_config *config = dev->config;
	struct neorv32_wdt_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	uint32_t ctrl = sys_read32(config->base + REG_CTRL);
	int ret;

	if (!data->wdt_started) {
		ret = -EFAULT;
		goto out;
	}

	ctrl &= ~CTRL_EN;
	sys_write32(ctrl, config->base + REG_CTRL);
	data->wdt_started = false;
	ret = 0;

out:
	k_spin_unlock(&data->lock, key);
	return ret;
}

static int wdt_neorv32_install_timeout(const struct device *dev,
					  const struct wdt_timeout_cfg *cfg)
{
	const struct neorv32_wdt_config *config = dev->config;
	struct neorv32_wdt_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	uint32_t timeout;
	int ret;

	if (data->timeout_active) {
		ret = -ENOMEM;
		goto out;
	}

	if (!(cfg->flags & WDT_FLAG_RESET_CPU_CORE)) {
		ret = -ENOTSUP;
		goto out;
	}

	if (cfg->window.min != 0 ||
		cfg->window.max > ((uint64_t)CTRL_TIMEOUT_MASK * WDT_CLOCK_DIVIDER * 1000) /
				  sys_clock_hw_cycles_per_sec()) {
		ret = -EINVAL;
		goto out;
	}

	timeout = (cfg->window.max * (sys_clock_hw_cycles_per_sec() / WDT_CLOCK_DIVIDER)) / 1000;
	/* clear all other register bits */
	sys_write32(timeout << CTRL_TIMEOUT_SHIFT, config->base + REG_CTRL);
	data->timeout_active = true;
	ret = 0;

out:
	k_spin_unlock(&data->lock, key);
	return ret;
}

static int wdt_neorv32_feed(const struct device *dev, int channel_id)
{
	const struct neorv32_wdt_config *config = dev->config;
	struct neorv32_wdt_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	int ret;

	if (channel_id != 0 || !data->timeout_active) {
		ret = -EINVAL;
		goto out;
	}

	sys_write32(RESET_PASSWORD, config->base + REG_RESET);
	ret = 0;

out:
	k_spin_unlock(&data->lock, key);
	return ret;
}

static int wdt_neorv32_init(const struct device *dev)
{
	const struct device *syscon = DEVICE_DT_GET(DT_INST_PHANDLE(0, syscon));
	uint32_t features;
	int err;

	if (!device_is_ready(syscon)) {
		LOG_ERR("syscon device not ready");
		return -EINVAL;
	}

	err = syscon_read_reg(syscon, NEORV32_SYSINFO_FEATURES, &features);
	if (err < 0) {
		LOG_ERR("failed to determine implemented features (err %d)", err);
		return -EIO;
	}

	if ((features & NEORV32_SYSINFO_FEATURES_IO_WDT) == 0) {
		LOG_ERR("neorv32 WDT not supported");
		return -ENODEV;
	}

	return 0;
}

static const struct wdt_driver_api wdt_neorv32_api = {
	.setup = wdt_neorv32_setup,
	.disable = wdt_neorv32_disable,
	.install_timeout = wdt_neorv32_install_timeout,
	.feed = wdt_neorv32_feed,
};

static struct neorv32_wdt_data wdt_neorv32_data;

static const struct neorv32_wdt_config wdt_neorv32_config = {
	.base = DT_INST_REG_ADDR(0),
};

int z_impl_hwinfo_get_reset_cause(uint32_t *cause)
{
	uint32_t reset_cause =
		(sys_read32(wdt_neorv32_config.base + REG_CTRL) >> CTRL_RCAUSE_SHIFT) &
		CTRL_RCAUSE_MASK;
	switch (reset_cause) {
	case NEORV32_RESET_PIN:
		*cause = RESET_PIN;
		break;
	case NEORV32_RESET_DEBUG:
		*cause = RESET_DEBUG;
		break;
	case NEORV32_RESET_WATCHDOG:
		*cause = RESET_WATCHDOG;
		break;
	default:
		LOG_WRN("Unknown reset cause: %u", reset_cause);
		*cause = 0;
		break;
	}

	return 0;
}

int z_impl_hwinfo_get_supported_reset_cause(uint32_t *supported)
{
	*supported = RESET_PIN | RESET_DEBUG | RESET_WATCHDOG;
	return 0;
}

DEVICE_DT_INST_DEFINE(0, wdt_neorv32_init, NULL,
		&wdt_neorv32_data, &wdt_neorv32_config, PRE_KERNEL_1,
		CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &wdt_neorv32_api);
