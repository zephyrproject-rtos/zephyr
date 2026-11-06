/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Platform firmware must reserve the watchdog for Zephyr from the last
 * peripheral reset and leave it stopped with its mode register unwritten.
 * Firmware configures and maintains the reset route; the driver checks it
 * but does not configure or clear it.
 */

#define DT_DRV_COMPAT renesas_rcar_gen5_wwdt

#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/renesas_rcar_generic.h>
#include <zephyr/drivers/syscon.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(wdt_renesas_rcar_wwdt, CONFIG_WDT_LOG_LEVEL);

/* WWDT register offsets. */
#define WWDT_TRIGGER_OFFSET 0x00U
#define WWDT_MODE_OFFSET    0x0cU

/* WWDT trigger value and mode/status bits. */
#define WWDT_TRIGGER    0xacU
#define WWDT_RUNNING    BIT(7)
#define WWDT_RESET_MODE BIT(2)

/* ECM reset-route register offsets. */
#define RCAR_ECM_CONTROL_OFFSET 0x000U
#define RCAR_ECM_STATUS_OFFSET  0x100U
#define RCAR_ECM_RESET_OFFSET   0x300U

struct rcar_wwdt_config {
	DEVICE_MMIO_ROM;
	const struct device *ecm;
	const struct device *bus_clock_dev;
	const struct device *counter_clock_dev;
	rcar_generic_clk_t bus_clock;
	rcar_generic_clk_t counter_clock;
	uint32_t bus_rate;
	uint32_t counter_rate;
	uint32_t ecm_group;
	uint32_t ecm_mask;
};

enum rcar_wwdt_state {
	RCAR_WWDT_STATE_INIT,
	RCAR_WWDT_STATE_INSTALLED,
	RCAR_WWDT_STATE_STARTING,
	RCAR_WWDT_STATE_MODE_WRITTEN,
	RCAR_WWDT_STATE_VERIFYING_RUN,
	RCAR_WWDT_STATE_POSSIBLY_RUNNING,
	RCAR_WWDT_STATE_RUNNING,
};

struct rcar_wwdt_data {
	DEVICE_MMIO_RAM;
	struct k_spinlock lock;
	enum rcar_wwdt_state state;
	uint8_t mode;
};

static int rcar_wwdt_window_mode(uint32_t min_ms, uint32_t max_ms, uint32_t rate, uint8_t *mode)
{
	if (rate == 0U || max_ms == 0U || min_ms > max_ms || mode == NULL) {
		return -EINVAL;
	}

	for (uint32_t shift = 9U; shift <= 16U; shift++) {
		uint32_t ticks = 1U << shift;

		if ((uint64_t)ticks * 1000U < (uint64_t)max_ms * rate) {
			continue;
		}

		/* Select the earliest supported opening at or after min_ms. */
		for (uint32_t closed = 0U; closed < 4U; closed++) {
			uint32_t closed_ticks = ticks * closed / 4U;

			if ((uint64_t)closed_ticks * 1000U >= (uint64_t)min_ms * rate) {
				*mode = (uint8_t)(((shift - 9U) << 4U) | (3U - closed));
				return 0;
			}
		}
	}

	return -EINVAL;
}

static int rcar_wwdt_check_ecm_route(const struct device *ecm, uint32_t group, uint32_t mask)
{
	uint32_t offset = group * sizeof(uint32_t);
	uint32_t control;
	uint32_t status;
	uint32_t reset;
	int ret;

	if (!device_is_ready(ecm)) {
		return -ENODEV;
	}

	ret = syscon_read_reg(ecm, RCAR_ECM_CONTROL_OFFSET + offset, &control);
	if (ret != 0) {
		return ret;
	}

	ret = syscon_read_reg(ecm, RCAR_ECM_STATUS_OFFSET + offset, &status);
	if (ret != 0) {
		return ret;
	}

	ret = syscon_read_reg(ecm, RCAR_ECM_RESET_OFFSET + offset, &reset);
	if (ret != 0) {
		return ret;
	}
	if ((control & mask) == 0U || (reset & mask) == 0U) {
		return -ENOTSUP;
	}
	if ((status & mask) != 0U) {
		return -EIO;
	}

	return 0;
}

static int rcar_wwdt_install_timeout(const struct device *dev,
				     const struct wdt_timeout_cfg *timeout_config)
{
	const struct rcar_wwdt_config *wdt_config = dev->config;
	struct rcar_wwdt_data *data = dev->data;
	uint8_t mode;
	int ret;

	if (timeout_config == NULL) {
		return -EINVAL;
	}
	if (timeout_config->flags != WDT_FLAG_RESET_SOC || timeout_config->callback != NULL) {
		return -ENOTSUP;
	}
#if defined(CONFIG_WDT_MULTISTAGE)
	if (timeout_config->next != NULL) {
		return -ENOTSUP;
	}
#endif

	ret = rcar_wwdt_window_mode(timeout_config->window.min, timeout_config->window.max,
				    wdt_config->counter_rate, &mode);
	if (ret != 0) {
		return ret;
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	switch (data->state) {
	case RCAR_WWDT_STATE_INIT:
		data->mode = mode | WWDT_RESET_MODE;
		data->state = RCAR_WWDT_STATE_INSTALLED;
		ret = 0;
		break;
	case RCAR_WWDT_STATE_INSTALLED:
		ret = -ENOMEM;
		break;
	default:
		ret = -EBUSY;
		break;
	}
	k_spin_unlock(&data->lock, key);

	return ret;
}

static int rcar_wwdt_setup(const struct device *dev, uint8_t options)
{
	const struct rcar_wwdt_config *config = dev->config;
	struct rcar_wwdt_data *data = dev->data;
	uintptr_t base = DEVICE_MMIO_GET(dev);
	k_spinlock_key_t key;
	int ret;

	/* No setup options, including sleep/debug pause, are supported. */
	if (options != 0U) {
		return -ENOTSUP;
	}

	key = k_spin_lock(&data->lock);
	if (data->state == RCAR_WWDT_STATE_INIT) {
		k_spin_unlock(&data->lock, key);
		return -EINVAL;
	}
	if (data->state != RCAR_WWDT_STATE_INSTALLED) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}
	data->state = RCAR_WWDT_STATE_STARTING;
	k_spin_unlock(&data->lock, key);

	ret = rcar_wwdt_check_ecm_route(config->ecm, config->ecm_group, config->ecm_mask);
	if (ret != 0) {
		LOG_ERR("WWDT reset route unavailable or unreadable: %d", ret);
		goto failed;
	}
	if (!device_is_ready(config->bus_clock_dev) ||
	    !device_is_ready(config->counter_clock_dev)) {
		LOG_ERR("WWDT clock device is not ready");
		ret = -ENODEV;
		goto failed;
	}

	ret = clock_control_on(config->bus_clock_dev, RCAR_CLOCK_SUBSYS(config->bus_clock));
	if (ret != 0) {
		LOG_ERR("Cannot enable WWDT bus clock: %d", ret);
		goto failed;
	}
	ret = clock_control_on(config->counter_clock_dev, RCAR_CLOCK_SUBSYS(config->counter_clock));
	if (ret != 0) {
		LOG_ERR("Cannot enable WWDT counter clock: %d", ret);
		goto failed;
	}

	/*
	 * The mode register's initial value needs three BUSCLK cycles after
	 * reset release to be reflected.
	 */
	k_busy_wait(DIV_ROUND_UP(3ULL * USEC_PER_SEC, config->bus_rate));
	key = k_spin_lock(&data->lock);
	if ((sys_read8(base + WWDT_TRIGGER_OFFSET) & WWDT_RUNNING) != 0U) {
		data->state = RCAR_WWDT_STATE_POSSIBLY_RUNNING;
		k_spin_unlock(&data->lock, key);
		LOG_ERR("WWDT is already running");
		ret = -EBUSY;
		goto failed;
	}
	k_spin_unlock(&data->lock, key);
	if (sys_read8(base + WWDT_MODE_OFFSET) != WWDT_RESET_MODE) {
		LOG_ERR("WWDT mode was already configured");
		ret = -EBUSY;
		goto failed;
	}

	key = k_spin_lock(&data->lock);
	data->state = RCAR_WWDT_STATE_MODE_WRITTEN;
	k_spin_unlock(&data->lock, key);
	sys_write8(data->mode, base + WWDT_MODE_OFFSET);
	/* The mode update and counter clear should not coincide. */
	k_busy_wait(MAX(DIV_ROUND_UP(3ULL * USEC_PER_SEC, config->bus_rate),
			DIV_ROUND_UP(3ULL * USEC_PER_SEC, config->counter_rate)));
	if (sys_read8(base + WWDT_MODE_OFFSET) != data->mode) {
		LOG_ERR("WWDT mode readback failed; peripheral reset may be needed");
		ret = -EIO;
		goto failed;
	}

	key = k_spin_lock(&data->lock);
	/* A failed readback cannot undo the trigger. */
	data->state = RCAR_WWDT_STATE_VERIFYING_RUN;
	sys_write8(WWDT_TRIGGER, base + WWDT_TRIGGER_OFFSET);
	k_spin_unlock(&data->lock, key);

	if ((sys_read8(base + WWDT_TRIGGER_OFFSET) & WWDT_RUNNING) == 0U) {
		key = k_spin_lock(&data->lock);
		data->state = RCAR_WWDT_STATE_POSSIBLY_RUNNING;
		k_spin_unlock(&data->lock, key);
		LOG_ERR("WWDT RUN readback failed; watchdog may still be armed");
		return -EIO;
	}

	key = k_spin_lock(&data->lock);
	data->state = RCAR_WWDT_STATE_RUNNING;
	k_spin_unlock(&data->lock, key);
	return 0;

failed:
	key = k_spin_lock(&data->lock);
	if (data->state == RCAR_WWDT_STATE_STARTING) {
		data->state = RCAR_WWDT_STATE_INSTALLED;
	}
	k_spin_unlock(&data->lock, key);
	return ret;
}

static int rcar_wwdt_feed(const struct device *dev, int channel_id)
{
	struct rcar_wwdt_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	int ret;

	if (channel_id != 0) {
		ret = -EINVAL;
	} else {
		switch (data->state) {
		case RCAR_WWDT_STATE_RUNNING:
			sys_write8(WWDT_TRIGGER, DEVICE_MMIO_GET(dev) + WWDT_TRIGGER_OFFSET);
			ret = 0;
			break;
		case RCAR_WWDT_STATE_STARTING:
		case RCAR_WWDT_STATE_VERIFYING_RUN:
			ret = -EAGAIN;
			break;
		case RCAR_WWDT_STATE_POSSIBLY_RUNNING:
			ret = -EIO;
			break;
		default:
			ret = -EINVAL;
			break;
		}
	}
	k_spin_unlock(&data->lock, key);

	return ret;
}

static int rcar_wwdt_disable(const struct device *dev)
{
	struct rcar_wwdt_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	int ret;

	switch (data->state) {
	case RCAR_WWDT_STATE_RUNNING:
	case RCAR_WWDT_STATE_VERIFYING_RUN:
	case RCAR_WWDT_STATE_POSSIBLY_RUNNING:
		ret = -EPERM;
		break;
	default:
		ret = -EFAULT;
		break;
	}

	k_spin_unlock(&data->lock, key);
	return ret;
}

static int rcar_wwdt_init(const struct device *dev)
{
	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);
	return 0;
}

static DEVICE_API(wdt, rcar_wwdt_api) = {
	.setup = rcar_wwdt_setup,
	.disable = rcar_wwdt_disable,
	.install_timeout = rcar_wwdt_install_timeout,
	.feed = rcar_wwdt_feed,
};

#define RCAR_WWDT_DEFINE(inst)                                                                     \
	BUILD_ASSERT(DT_INST_PROP_BY_IDX(inst, renesas_ecm_error, 0) < 64);                        \
	BUILD_ASSERT(DT_INST_PROP_BY_IDX(inst, renesas_ecm_error, 1) < 32);                        \
	static const struct rcar_wwdt_config rcar_wwdt_config_##inst = {                           \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(inst)),                                           \
		.ecm = DEVICE_DT_GET(DT_INST_PHANDLE(inst, renesas_ecm)),                          \
		.bus_clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_IDX(inst, 0)),               \
		.counter_clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_IDX(inst, 1)),           \
		.bus_clock = RCAR_DT_INST_CLOCKS_CELL_BY_IDX(inst, 0),                             \
		.counter_clock = RCAR_DT_INST_CLOCKS_CELL_BY_IDX(inst, 1),                         \
		.bus_rate = DT_INST_PROP(inst, bus_clock_frequency),                               \
		.counter_rate = DT_INST_PROP(inst, counter_clock_frequency),                       \
		.ecm_group = DT_INST_PROP_BY_IDX(inst, renesas_ecm_error, 0),                      \
		.ecm_mask = BIT(DT_INST_PROP_BY_IDX(inst, renesas_ecm_error, 1)),                  \
	};                                                                                         \
	static struct rcar_wwdt_data rcar_wwdt_data_##inst;                                        \
	DEVICE_DT_INST_DEFINE(inst, rcar_wwdt_init, NULL, &rcar_wwdt_data_##inst,                  \
			      &rcar_wwdt_config_##inst, POST_KERNEL,                               \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &rcar_wwdt_api);

DT_INST_FOREACH_STATUS_OKAY(RCAR_WWDT_DEFINE)
