/*
 * Copyright (c) 2023, Nordic Semiconductor ASA
 * Copyright (c) 2023, Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_retained_reg

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/retained_mem.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(retained_mem_zephyr_reg, CONFIG_RETAINED_MEM_LOG_LEVEL);

struct zephyr_retained_mem_reg_data {
#ifdef CONFIG_RETAINED_MEM_MUTEXES
	struct k_mutex lock;
#endif
};

struct zephyr_retained_mem_reg_config {
	uint8_t *address;
	size_t size;
};

static inline void zephyr_retained_mem_reg_lock_take(const struct device *dev)
{
#ifdef CONFIG_RETAINED_MEM_MUTEXES
	struct zephyr_retained_mem_reg_data *data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);
#else
	ARG_UNUSED(dev);
#endif
}

static inline void zephyr_retained_mem_reg_lock_release(const struct device *dev)
{
#ifdef CONFIG_RETAINED_MEM_MUTEXES
	struct zephyr_retained_mem_reg_data *data = dev->data;

	k_mutex_unlock(&data->lock);
#else
	ARG_UNUSED(dev);
#endif
}

static int zephyr_retained_mem_reg_init(const struct device *dev)
{
#ifdef CONFIG_RETAINED_MEM_MUTEXES
	struct zephyr_retained_mem_reg_data *data = dev->data;

	k_mutex_init(&data->lock);
#endif

	return 0;
}

static k_ssize_t zephyr_retained_mem_reg_size(const struct device *dev)
{
	const struct zephyr_retained_mem_reg_config *config = dev->config;

	return (k_ssize_t)config->size;
}

static int zephyr_retained_mem_reg_read(const struct device *dev, k_off_t offset, uint8_t *buffer,
					size_t size)
{
	const struct zephyr_retained_mem_reg_config *config = dev->config;

	zephyr_retained_mem_reg_lock_take(dev);

	memcpy(buffer, (config->address + offset), size);

	zephyr_retained_mem_reg_lock_release(dev);

	return 0;
}

static int zephyr_retained_mem_reg_write(const struct device *dev, k_off_t offset,
					 const uint8_t *buffer, size_t size)
{
	const struct zephyr_retained_mem_reg_config *config = dev->config;

	zephyr_retained_mem_reg_lock_take(dev);

	memcpy((config->address + offset), buffer, size);

	zephyr_retained_mem_reg_lock_release(dev);

	return 0;
}

static int zephyr_retained_mem_reg_clear(const struct device *dev)
{
	const struct zephyr_retained_mem_reg_config *config = dev->config;

	zephyr_retained_mem_reg_lock_take(dev);

	memset(config->address, 0, config->size);

	zephyr_retained_mem_reg_lock_release(dev);

	return 0;
}

static const struct retained_mem_driver_api zephyr_retained_mem_reg_api = {
	.size = zephyr_retained_mem_reg_size,
	.read = zephyr_retained_mem_reg_read,
	.write = zephyr_retained_mem_reg_write,
	.clear = zephyr_retained_mem_reg_clear,
};

#define ZEPHYR_RETAINED_MEM_REG_DEVICE(inst)							\
	static struct zephyr_retained_mem_reg_data zephyr_retained_mem_reg_data_##inst;		\
	static const struct zephyr_retained_mem_reg_config					\
			zephyr_retained_mem_reg_config_##inst = {				\
		.address = (uint8_t *)DT_INST_REG_ADDR(inst),					\
		.size = DT_INST_REG_SIZE(inst),							\
	};											\
	DEVICE_DT_INST_DEFINE(inst,								\
			      &zephyr_retained_mem_reg_init,					\
			      NULL,								\
			      &zephyr_retained_mem_reg_data_##inst,				\
			      &zephyr_retained_mem_reg_config_##inst,				\
			      POST_KERNEL,							\
			      CONFIG_RETAINED_MEM_INIT_PRIORITY,				\
			      &zephyr_retained_mem_reg_api);

DT_INST_FOREACH_STATUS_OKAY(ZEPHYR_RETAINED_MEM_REG_DEVICE)
