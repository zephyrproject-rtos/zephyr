/*
 * Copyright (c) 2026 Aleksandr Senin <al@meshium.net>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT gd_gd32_backup_sram

#include <errno.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/drivers/retained_mem.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/gd32.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

#ifdef CONFIG_RETAINED_MEM_MUTEXES
struct gd32_bkpsram_data {
	struct k_mutex lock;
};
#endif

struct gd32_bkpsram_config {
	uint8_t *address;
	size_t size;
	uint32_t clkid;
	const struct device *vin_supply;
};

static inline void gd32_bkpsram_lock_take(const struct device *dev)
{
#ifdef CONFIG_RETAINED_MEM_MUTEXES
	struct gd32_bkpsram_data *data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);
#else
	ARG_UNUSED(dev);
#endif
}

static inline void gd32_bkpsram_lock_release(const struct device *dev)
{
#ifdef CONFIG_RETAINED_MEM_MUTEXES
	struct gd32_bkpsram_data *data = dev->data;

	k_mutex_unlock(&data->lock);
#else
	ARG_UNUSED(dev);
#endif
}

static int gd32_bkpsram_init(const struct device *dev)
{
	const struct gd32_bkpsram_config *cfg = dev->config;

#ifdef CONFIG_RETAINED_MEM_MUTEXES
	struct gd32_bkpsram_data *data = dev->data;

	k_mutex_init(&data->lock);
#endif

	(void)clock_control_on(GD32_CLOCK_CONTROLLER, (clock_control_subsys_t)&cfg->clkid);

#if defined(CONFIG_REGULATOR)
	if (cfg->vin_supply != NULL) {
		if (!device_is_ready(cfg->vin_supply)) {
			return -ENODEV;
		}

		int ret = regulator_enable(cfg->vin_supply);

		if (ret < 0) {
			return ret;
		}
	}
#else
	ARG_UNUSED(cfg);
#endif

	return 0;
}

static ssize_t gd32_bkpsram_size(const struct device *dev)
{
	const struct gd32_bkpsram_config *cfg = dev->config;

	return (ssize_t)cfg->size;
}

static int gd32_bkpsram_read(const struct device *dev, off_t offset, uint8_t *buffer, size_t size)
{
	const struct gd32_bkpsram_config *cfg = dev->config;

	gd32_bkpsram_lock_take(dev);
	memcpy(buffer, (cfg->address + offset), size);
	gd32_bkpsram_lock_release(dev);

	return 0;
}

static int gd32_bkpsram_write(const struct device *dev, off_t offset, const uint8_t *buffer,
			      size_t size)
{
	const struct gd32_bkpsram_config *cfg = dev->config;

	gd32_bkpsram_lock_take(dev);
	memcpy((cfg->address + offset), buffer, size);
	gd32_bkpsram_lock_release(dev);

	return 0;
}

static int gd32_bkpsram_clear(const struct device *dev)
{
	const struct gd32_bkpsram_config *cfg = dev->config;

	gd32_bkpsram_lock_take(dev);
	memset(cfg->address, 0, cfg->size);
	gd32_bkpsram_lock_release(dev);

	return 0;
}

static DEVICE_API(retained_mem, gd32_bkpsram_api) = {
	.size = gd32_bkpsram_size,
	.read = gd32_bkpsram_read,
	.write = gd32_bkpsram_write,
	.clear = gd32_bkpsram_clear,
};

/* clang-format off */
#if defined(CONFIG_REGULATOR)
#define GD32_BKPSRAM_VIN_SUPPLY(inst) \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, vin_supply), \
		    (DEVICE_DT_GET(DT_INST_PHANDLE(inst, vin_supply))), (NULL))
#else
#define GD32_BKPSRAM_VIN_SUPPLY(inst) NULL
#endif

#define GD32_BKPSRAM_DEVICE(inst) \
	static const struct gd32_bkpsram_config gd32_bkpsram_config_##inst = { \
		.address = (uint8_t *)DT_REG_ADDR(DT_INST(inst, DT_DRV_COMPAT)), \
		.size = DT_REG_SIZE(DT_INST(inst, DT_DRV_COMPAT)), \
		.clkid = DT_INST_CLOCKS_CELL(inst, id), \
		.vin_supply = GD32_BKPSRAM_VIN_SUPPLY(inst), \
	}; \
	IF_ENABLED(CONFIG_RETAINED_MEM_MUTEXES, \
		(static struct gd32_bkpsram_data gd32_bkpsram_data_##inst;)) \
	DEVICE_DT_INST_DEFINE(inst, gd32_bkpsram_init, NULL, \
		COND_CODE_1(CONFIG_RETAINED_MEM_MUTEXES, \
			(&gd32_bkpsram_data_##inst), \
			(NULL)), \
		&gd32_bkpsram_config_##inst, POST_KERNEL, \
		CONFIG_RETAINED_MEM_GD32_BKPSRAM_INIT_PRIORITY, &gd32_bkpsram_api)
/* clang-format on */

DT_INST_FOREACH_STATUS_OKAY(GD32_BKPSRAM_DEVICE)
