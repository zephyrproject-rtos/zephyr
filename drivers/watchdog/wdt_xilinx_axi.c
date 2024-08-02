/*
 * Driver for Xilinx AXI Timebase WDT core, as described in
 * Xilinx document PG128.
 *
 * Note that the window mode of operation of the core is
 * currently not supported. Also, only a full SoC reset is
 * supported as a watchdog expiry action.
 *
 * Copyright © 2023 Calian Ltd.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT xlnx_xps_timebase_wdt_1_00_a

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/drivers/hwinfo.h>

enum xilinx_wdt_axi_register {
	REG_TWCSR0 = 0x00, /* Control/Status Register 0 */
	REG_TWCSR1 = 0x04, /* Control/Status Register 1 */
	REG_TBR = 0x08,	   /* Timebase Register */
	REG_MWR = 0x0C,	   /* Master Write Control Register */
};

enum xilinx_wdt_csr0_bits {
	CSR0_WRS = BIT(3),
	CSR0_WDS = BIT(2),
	CSR0_EWDT1 = BIT(1),
	CSR0_EWDT2 = BIT(0),
};

enum xilinx_wdt_csr1_bits {
	CSR1_EWDT2 = BIT(0),
};

enum {
	TIMER_WIDTH_MIN = 8,
};

LOG_MODULE_REGISTER(wdt_xilinx_axi, CONFIG_WDT_LOG_LEVEL);

struct xilinx_wdt_axi_config {
	mem_addr_t base;
	uint32_t clock_rate;
	uint8_t timer_width_max;
	bool enable_once;
};

struct xilinx_wdt_axi_data {
	struct k_spinlock lock;
	bool timeout_active;
	bool wdt_started;
};

static const struct device *first_wdt_dev;

static int wdt_xilinx_axi_setup(const struct device *dev, uint8_t options)
{
	const struct xilinx_wdt_axi_config *config = dev->config;
	struct xilinx_wdt_axi_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	int ret;

	if (!data->timeout_active) {
		ret = -EINVAL;
		goto out;
	}

	if (data->wdt_started) {
		ret = -EBUSY;
		goto out;
	}

	/* We don't actually know or control at the driver level whether
	 * the WDT pauses in CPU sleep or when halted by the debugger,
	 * so we don't check anything with the options.
	 */
	sys_write32(CSR0_EWDT1 | CSR0_WDS, config->base + REG_TWCSR0);
	sys_write32(CSR1_EWDT2, config->base + REG_TWCSR1);
	data->wdt_started = true;
	ret = 0;

out:
	k_spin_unlock(&data->lock, key);
	return ret;
}

static int wdt_xilinx_axi_disable(const struct device *dev)
{
	const struct xilinx_wdt_axi_config *config = dev->config;
	struct xilinx_wdt_axi_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	int ret;

	if (config->enable_once) {
		ret = -EPERM;
		goto out;
	}

	if (!data->wdt_started) {
		ret = -EFAULT;
		goto out;
	}

	sys_write32(CSR0_WDS, config->base + REG_TWCSR0);
	sys_write32(0, config->base + REG_TWCSR1);
	data->wdt_started = false;
	ret = 0;

out:
	k_spin_unlock(&data->lock, key);
	return ret;
}

static int wdt_xilinx_axi_install_timeout(const struct device *dev,
					  const struct wdt_timeout_cfg *cfg)
{
	const struct xilinx_wdt_axi_config *config = dev->config;
	struct xilinx_wdt_axi_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	uint32_t timer_width;
	bool good_timer_width = false;
	int ret;

	if (data->timeout_active) {
		ret = -ENOMEM;
		goto out;
	}

	if (!(cfg->flags & WDT_FLAG_RESET_SOC)) {
		ret = -ENOTSUP;
		goto out;
	}

	if (cfg->window.min != 0) {
		ret = -EINVAL;
		goto out;
	}

	for (timer_width = TIMER_WIDTH_MIN; timer_width <= config->timer_width_max; timer_width++) {
		/* Note: WDT expiry happens after 2 wraps of the timer (first raises an interrupt
		 * which is not used, second triggers a reset) so add 1 to timer_width.
		 */
		const uint64_t expiry_cycles = ((uint64_t)1) << (timer_width + 1);
		const uint64_t expiry_msec = expiry_cycles * 1000 / config->clock_rate;

		if (expiry_msec >= cfg->window.max) {
			LOG_INF("Set timer width to %u, actual expiry %u msec", timer_width,
				(unsigned int)expiry_msec);
			good_timer_width = true;
			break;
		}
	}

	if (!good_timer_width) {
		LOG_ERR("Cannot support timeout value of %u msec", cfg->window.max);
		ret = -EINVAL;
		goto out;
	}

	sys_write32(timer_width, config->base + REG_MWR);
	data->timeout_active = true;
	ret = 0;

out:
	k_spin_unlock(&data->lock, key);
	return ret;
}

static int wdt_xilinx_axi_feed(const struct device *dev, int channel_id)
{
	const struct xilinx_wdt_axi_config *config = dev->config;
	struct xilinx_wdt_axi_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	uint32_t twcsr0 = sys_read32(config->base + REG_TWCSR0);
	int ret;

	if (channel_id != 0 || !data->timeout_active) {
		ret = -EINVAL;
		goto out;
	}

	twcsr0 |= CSR0_WDS;
	if (data->wdt_started) {
		twcsr0 |= CSR0_EWDT1;
	}

	sys_write32(twcsr0, config->base + REG_TWCSR0);
	ret = 0;

out:
	k_spin_unlock(&data->lock, key);
	return ret;
}

static int wdt_xilinx_axi_init(const struct device *dev)
{
	if (IS_ENABLED(CONFIG_WDT_XILINX_AXI_HWINFO_API)) {
		if (first_wdt_dev) {
			LOG_WRN("Multiple WDT instances, only first will implement HWINFO");
		} else {
			first_wdt_dev = dev;
		}
	}

	return 0;
}

#ifdef CONFIG_WDT_XILINX_AXI_HWINFO_API

int z_impl_hwinfo_get_reset_cause(uint32_t *cause)
{
	if (!first_wdt_dev) {
		return -ENOSYS;
	}

	const struct xilinx_wdt_axi_config *config = first_wdt_dev->config;

	if ((sys_read32(config->base + REG_TWCSR0) & CSR0_WRS) != 0) {
		*cause = RESET_WATCHDOG;
	} else {
		*cause = 0;
	}

	return 0;
}

int z_impl_hwinfo_clear_reset_cause(void)
{
	if (!first_wdt_dev) {
		return -ENOSYS;
	}

	const struct xilinx_wdt_axi_config *config = first_wdt_dev->config;
	struct xilinx_wdt_axi_data *data = first_wdt_dev->data;

	k_spinlock_key_t key = k_spin_lock(&data->lock);
	uint32_t twcsr0 = sys_read32(config->base + REG_TWCSR0);

	if ((twcsr0 & CSR0_WRS) != 0) {
		twcsr0 |= CSR0_WRS;
		sys_write32(twcsr0, config->base + REG_TWCSR0);
	}

	k_spin_unlock(&data->lock, key);

	return 0;
}

int z_impl_hwinfo_get_supported_reset_cause(uint32_t *supported)
{
	if (!first_wdt_dev) {
		return -ENOSYS;
	}

	*supported = RESET_WATCHDOG;
	return 0;
}

#endif

static const struct wdt_driver_api wdt_xilinx_api = {
	.setup = wdt_xilinx_axi_setup,
	.disable = wdt_xilinx_axi_disable,
	.install_timeout = wdt_xilinx_axi_install_timeout,
	.feed = wdt_xilinx_axi_feed,
};

#define WDT_XILINX_AXI_INIT(inst)                                                                  \
	static struct xilinx_wdt_axi_data wdt_xilinx_axi_##inst##_dev_data;                        \
                                                                                                   \
	static const struct xilinx_wdt_axi_config wdt_xilinx_axi_##inst##_cfg = {                  \
		.base = DT_INST_REG_ADDR(inst),                                                    \
		.clock_rate = DT_INST_PROP_BY_PHANDLE(inst, clocks, clock_frequency),              \
		.timer_width_max = DT_INST_PROP(inst, xlnx_wdt_interval),                          \
		.enable_once = DT_INST_PROP(inst, xlnx_wdt_enable_once),                           \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &wdt_xilinx_axi_init, NULL, &wdt_xilinx_axi_##inst##_dev_data, \
			      &wdt_xilinx_axi_##inst##_cfg, PRE_KERNEL_1,                          \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &wdt_xilinx_api);

DT_INST_FOREACH_STATUS_OKAY(WDT_XILINX_AXI_INIT)
