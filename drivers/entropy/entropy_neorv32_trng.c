/*
 * Copyright (c) 2021 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT neorv32_trng

#include <zephyr/device.h>
#include <zephyr/drivers/syscon.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/sys_io.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(neorv32_trng, CONFIG_ENTROPY_LOG_LEVEL);

/* TRNG CTRL register bits */
#define NEORV32_TRNG_CTRL_DATA_MASK BIT_MASK(8)
#define NEORV32_TRNG_CTRL_EN        BIT(30)
#define NEORV32_TRNG_CTRL_VALID     BIT(31)

struct neorv32_trng_config {
	const struct device *syscon;
	mm_reg_t base;
};

static inline uint32_t neorv32_trng_read_ctrl(const struct device *dev)
{
	const struct neorv32_trng_config *config = dev->config;

	return sys_read32(config->base);
}

static inline void neorv32_trng_write_ctrl(const struct device *dev, uint32_t ctrl)
{
	const struct neorv32_trng_config *config = dev->config;

	sys_write32(ctrl, config->base);
}

static int neorv32_trng_get_entropy(const struct device *dev, uint8_t *buffer, uint16_t len)
{
	uint32_t ctrl;

	while (len > 0) {
		ctrl = neorv32_trng_read_ctrl(dev);

		if ((ctrl & NEORV32_TRNG_CTRL_VALID) != 0) {
			*buffer++ = ctrl & NEORV32_TRNG_CTRL_DATA_MASK;
			len--;
		}
	}

	return 0;
}

static int neorv32_trng_get_entropy_isr(const struct device *dev, uint8_t *buffer,
					uint16_t len, uint32_t flags)
{
	uint32_t ctrl;
	int err;

	if ((flags & ENTROPY_BUSYWAIT) == 0) {
		ctrl = neorv32_trng_read_ctrl(dev);
		if ((ctrl & NEORV32_TRNG_CTRL_VALID) != 0) {
			*buffer = ctrl & NEORV32_TRNG_CTRL_DATA_MASK;
			return 1;
		}

		/* No entropy available */
		return -ENODATA;
	}

	err = neorv32_trng_get_entropy(dev, buffer, len);
	if (err < 0) {
		return err;
	}

	return len;
}

static int neorv32_trng_init(const struct device *dev)
{
	const struct neorv32_trng_config *config = dev->config;
	uint32_t features;
	int err;

	if (!device_is_ready(config->syscon)) {
		LOG_ERR("syscon device not ready");
		return -EINVAL;
	}

	err = syscon_read_reg(config->syscon, NEORV32_SYSINFO_FEATURES, &features);
	if (err < 0) {
		LOG_ERR("failed to determine implemented features (err %d)", err);
		return err;
	}

	if ((features & NEORV32_SYSINFO_FEATURES_IO_TRNG) == 0) {
		LOG_ERR("neorv32 trng not supported");
		return -ENODEV;
	}

	neorv32_trng_write_ctrl(dev, NEORV32_TRNG_CTRL_EN);

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int neorv32_trng_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		neorv32_trng_write_ctrl(dev, 0);
		break;
	case PM_DEVICE_ACTION_RESUME:
		neorv32_trng_write_ctrl(dev, NEORV32_TRNG_CTRL_EN);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif /* CONFIG_PM_DEVICE */

static const struct entropy_driver_api neorv32_trng_driver_api = {
	.get_entropy = neorv32_trng_get_entropy,
	.get_entropy_isr = neorv32_trng_get_entropy_isr,
};

#define NEORV32_TRNG_INIT(n)						\
	static const struct neorv32_trng_config neorv32_trng_##n##_config = { \
		.syscon = DEVICE_DT_GET(DT_INST_PHANDLE(n, syscon)),	\
		.base = DT_INST_REG_ADDR(n),				\
	};								\
									\
	PM_DEVICE_DT_INST_DEFINE(n, neorv32_trng_pm_action);		\
									\
	DEVICE_DT_INST_DEFINE(n, &neorv32_trng_init,			\
			 PM_DEVICE_DT_INST_GET(n),			\
			 NULL,						\
			 &neorv32_trng_##n##_config,			\
			 PRE_KERNEL_1,					\
			 CONFIG_ENTROPY_INIT_PRIORITY,			\
			 &neorv32_trng_driver_api);

DT_INST_FOREACH_STATUS_OKAY(NEORV32_TRNG_INIT)
