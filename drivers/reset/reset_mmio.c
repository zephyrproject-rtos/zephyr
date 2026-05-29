/* Copyright (c) 2025 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/kernel.h>

struct reset_mmio_dev_config {
	uint32_t base;
	uint8_t num_resets;
	bool active_low;
};

struct reset_mmio_dev_data {
	struct k_spinlock lock;
};

static inline int reset_mmio_status(const struct device *dev, uint32_t id, uint8_t *status)
{
	const struct reset_mmio_dev_config *config = dev->config;
	uint32_t value;

	if (id >= config->num_resets) {
		return -EINVAL;
	}

	value = sys_read32(config->base);
	*status = FIELD_GET(BIT(id), value);

	/* If active low, invert the logic */
	if (config->active_low) {
		*status = !(*status);
	}

	return 0;
}

static inline int reset_mmio_update(const struct device *dev, uint32_t id, uint8_t assert)
{
	const struct reset_mmio_dev_config *config = dev->config;
	struct reset_mmio_dev_data *data = dev->data;
	uint32_t value;
	bool set;

	if (id >= config->num_resets) {
		return -EINVAL;
	}

	/* If active low, invert the logic */
	set = config->active_low ? !assert : assert;

	K_SPINLOCK(&data->lock) {
		value = sys_read32(config->base);

		if (set) {
			value |= BIT(id);
		} else {
			value &= ~BIT(id);
		}

		sys_write32(value, config->base);
	}

	return 0;
}

static int reset_mmio_line_assert(const struct device *dev, uint32_t id)
{
	return reset_mmio_update(dev, id, 1);
}

static int reset_mmio_line_deassert(const struct device *dev, uint32_t id)
{
	return reset_mmio_update(dev, id, 0);
}

static int reset_mmio_line_toggle(const struct device *dev, uint32_t id)
{
	uint8_t reset_status = 0;
	int status;

	status = reset_mmio_status(dev, id, &reset_status);
	if (status) {
		return status;
	}

	return reset_mmio_update(dev, id, !reset_status);
}

static DEVICE_API(reset, reset_mmio_driver_api) = {
	.status = reset_mmio_status,
	.line_assert = reset_mmio_line_assert,
	.line_deassert = reset_mmio_line_deassert,
	.line_toggle = reset_mmio_line_toggle,
};

#define DT_DRV_COMPAT reset_mmio
#define RESET_MMIO_INIT(n)                                                                         \
	BUILD_ASSERT(DT_INST_PROP(n, num_resets) > 0 && DT_INST_PROP(n, num_resets) < 32,          \
		     "num-resets needs to be in [1, 31].");                                        \
	static const struct reset_mmio_dev_config reset_mmio_dev_config_##n = {                    \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.num_resets = DT_INST_PROP(n, num_resets),                                         \
		.active_low = DT_INST_PROP(n, active_low)};                                        \
	static struct reset_mmio_dev_data reset_mmio_dev_data_##n;                                 \
	DEVICE_DT_INST_DEFINE(n, NULL, NULL, &reset_mmio_dev_data_##n, &reset_mmio_dev_config_##n, \
			      POST_KERNEL, CONFIG_RESET_INIT_PRIORITY, &reset_mmio_driver_api);
DT_INST_FOREACH_STATUS_OKAY(RESET_MMIO_INIT)
