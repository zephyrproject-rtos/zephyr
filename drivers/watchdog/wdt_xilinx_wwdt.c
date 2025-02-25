/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT xlnx_versal_wwdt

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(xilinx_wwdt, CONFIG_WDT_LOG_LEVEL);

/* Register offsets for the WWDT device */
#define XWWDT_MWR_OFFSET	0x00
#define XWWDT_ESR_OFFSET	0x04
#define XWWDT_FCR_OFFSET	0x08
#define XWWDT_FWR_OFFSET	0x0c
#define XWWDT_SWR_OFFSET	0x10

/* Master Write Control Register Masks */
#define XWWDT_MWR_MASK	BIT(0)

/* Enable and Status Register Masks */
#define XWWDT_ESR_WINT_MASK	BIT(16)
#define XWWDT_ESR_WSW_MASK	BIT(8)
#define XWWDT_ESR_WEN_MASK	BIT(0)

/* Watchdog Second Window Shift */
#define XWWDT_ESR_WSW_SHIFT	8U

/* Maximum count value of each 32 bit window */
#define XWWDT_MAX_COUNT_WINDOW	GENMASK(31, 0)

/* Maximum count value of closed and open window combined */
#define XWWDT_MAX_COUNT_WINDOW_COMBINED	GENMASK64(32, 1)

struct xilinx_wwdt_config {
	uint32_t wdt_clock_freq;
	mem_addr_t base;
};

struct xilinx_wwdt_data {
	struct k_spinlock lock;
	bool timeout_active;
	bool wdt_started;
};

static int wdt_xilinx_wwdt_setup(const struct device *dev, uint8_t options)
{
	const struct xilinx_wwdt_config *config = dev->config;
	struct xilinx_wwdt_data *data = dev->data;
	uint32_t reg_value;
	int ret = 0;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if (!data->timeout_active) {
		ret = -EINVAL;
		goto out;
	}

	if (data->wdt_started) {
		ret = -EBUSY;
		goto out;
	}

	/*
	 * There is no control at driver level whether the WDT pauses in CPU sleep
	 * or when halted by debugger. Hence there is no check for the options.
	 */

	/* Read enable status register and update WEN bit */
	reg_value = sys_read32(config->base + XWWDT_ESR_OFFSET) | XWWDT_ESR_WEN_MASK;

	/* Write enable status register with updated WEN value */
	sys_write32(reg_value, config->base + XWWDT_ESR_OFFSET);
	data->wdt_started = true;
out:
	k_spin_unlock(&data->lock, key);
	return ret;
}

static int wdt_xilinx_wwdt_install_timeout(const struct device *dev,
					   const struct wdt_timeout_cfg *cfg)
{
	const struct xilinx_wwdt_config *config = dev->config;
	struct xilinx_wwdt_data *data = dev->data;
	uint64_t closed_window_ms_count;
	uint64_t open_window_ms_count;
	uint64_t max_hw_timeout_ms;
	uint64_t timeout_ms_count;
	uint32_t timeout_ms;
	uint64_t ms_count;
	int ret = 0;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if (data->wdt_started) {
		ret = -EBUSY;
		goto out;
	}

	if (cfg->flags != WDT_FLAG_RESET_SOC) {
		ret = -ENOTSUP;
		goto out;
	}

	timeout_ms = cfg->window.max;
	max_hw_timeout_ms = (XWWDT_MAX_COUNT_WINDOW_COMBINED * 1000) / config->wdt_clock_freq;

	/* Timeout greater than the maximum hardware timeout is invalid. */
	if (timeout_ms > max_hw_timeout_ms) {
		ret = -EINVAL;
		goto out;
	}

	/* Calculate ticks for 1 milli-second */
	ms_count = (config->wdt_clock_freq) / 1000;
	timeout_ms_count = timeout_ms * ms_count;

	closed_window_ms_count = cfg->window.min * ms_count;
	if (closed_window_ms_count > XWWDT_MAX_COUNT_WINDOW) {
		LOG_ERR("The closed window timeout is invalid.");
		ret = -EINVAL;
		goto out;
	}

	open_window_ms_count = timeout_ms_count - closed_window_ms_count;
	if (open_window_ms_count > XWWDT_MAX_COUNT_WINDOW) {
		LOG_ERR("The open window timeout is invalid.");
		ret = -EINVAL;
		goto out;
	}

	sys_write32(XWWDT_MWR_MASK, config->base + XWWDT_MWR_OFFSET);
	sys_write32(~(uint32_t)XWWDT_ESR_WEN_MASK, config->base + XWWDT_ESR_OFFSET);
	sys_write32(closed_window_ms_count, config->base + XWWDT_FWR_OFFSET);
	sys_write32(open_window_ms_count, config->base + XWWDT_SWR_OFFSET);

	data->timeout_active = true;
out:
	k_spin_unlock(&data->lock, key);
	return ret;
}

static int wdt_xilinx_wwdt_feed(const struct device *dev, int channel_id)
{
	const struct xilinx_wwdt_config *config = dev->config;
	struct xilinx_wwdt_data *data = dev->data;
	uint32_t control_status_reg;
	uint32_t is_sec_window;
	int ret = 0;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if (channel_id != 0 || !data->timeout_active) {
		ret = -EINVAL;
		goto out;
	}

	/* Enable write access control bit for the WWDT. */
	sys_write32(XWWDT_MWR_MASK, config->base + XWWDT_MWR_OFFSET);

	/* Trigger restart kick to WWDT. */
	control_status_reg = sys_read32(config->base + XWWDT_ESR_OFFSET);

	/* Check if WWDT is in Second window. */
	is_sec_window = (control_status_reg & (uint32_t)XWWDT_ESR_WSW_MASK) >> XWWDT_ESR_WSW_SHIFT;

	if (is_sec_window != 1) {
		LOG_ERR("Feed in Closed window is not supported.");
		ret = -ENOTSUP;
		goto out;
	}

	control_status_reg |= (uint32_t)XWWDT_ESR_WSW_MASK;
	sys_write32(control_status_reg, config->base + XWWDT_ESR_OFFSET);
out:
	k_spin_unlock(&data->lock, key);
	return ret;
}

static int wdt_xilinx_wwdt_disable(const struct device *dev)
{
	const struct xilinx_wwdt_config *config = dev->config;
	struct xilinx_wwdt_data *data = dev->data;
	uint32_t is_wwdt_enable;
	uint32_t is_sec_window;
	uint32_t reg_value;
	int ret = 0;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	is_wwdt_enable = sys_read32(config->base + XWWDT_ESR_OFFSET) & XWWDT_ESR_WEN_MASK;

	if (is_wwdt_enable == 0) {
		ret = -EFAULT;
		goto out;
	}

	/* Read enable status register and check if WWDT is in open window. */
	is_sec_window = (sys_read32(config->base + XWWDT_ESR_OFFSET) & XWWDT_ESR_WSW_MASK) >>
				   XWWDT_ESR_WSW_SHIFT;

	if (is_sec_window != 1)	{
		LOG_ERR("Disabling WWDT in closed window is not allowed.");
		ret = -EPERM;
		goto out;
	}

	/* Read enable status register and update WEN bit. */
	reg_value = sys_read32(config->base + XWWDT_ESR_OFFSET) & (~XWWDT_ESR_WEN_MASK);

	/* Write enable status register with updated WEN and WSW value. */
	sys_write32(reg_value, config->base + XWWDT_ESR_OFFSET);

	data->wdt_started = false;
out:
	k_spin_unlock(&data->lock, key);
	return ret;
}

static int wdt_xilinx_wwdt_init(const struct device *dev)
{
	const struct xilinx_wwdt_config *config = dev->config;
	int ret = 0;

	if (config->wdt_clock_freq == 0) {
		return -EINVAL;
	}

	return ret;
}

static DEVICE_API(wdt, wdt_xilinx_wwdt_api) = {
	.setup = wdt_xilinx_wwdt_setup,
	.install_timeout = wdt_xilinx_wwdt_install_timeout,
	.feed = wdt_xilinx_wwdt_feed,
	.disable = wdt_xilinx_wwdt_disable,
};

#define WDT_XILINX_WWDT_INIT(inst)								\
	static struct xilinx_wwdt_data wdt_xilinx_wwdt_##inst##_dev_data;			\
												\
	static const struct xilinx_wwdt_config wdt_xilinx_wwdt_##inst##_cfg = {			\
		.base = DT_INST_REG_ADDR(inst),							\
		.wdt_clock_freq = DT_INST_PROP_BY_PHANDLE(inst, clocks, clock_frequency),	\
	};											\
												\
	DEVICE_DT_INST_DEFINE(inst, &wdt_xilinx_wwdt_init, NULL,				\
				&wdt_xilinx_wwdt_##inst##_dev_data,				\
				&wdt_xilinx_wwdt_##inst##_cfg, PRE_KERNEL_1,			\
				CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &wdt_xilinx_wwdt_api);

DT_INST_FOREACH_STATUS_OKAY(WDT_XILINX_WWDT_INIT)
