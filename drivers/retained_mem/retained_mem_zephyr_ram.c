/*
 * Copyright (c) 2023, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_retained_ram

#include <string.h>
#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/retained_mem.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(retained_mem_zephyr_ram, CONFIG_RETAINED_MEM_LOG_LEVEL);

struct zephyr_retained_mem_ram_config {
	uint8_t *address;
	size_t size;
#ifdef CONFIG_RETAINED_MEM_MUTEXES
	struct k_mutex *lock;
#endif
};

static inline void zephyr_retained_mem_ram_lock_take(const struct device *dev)
{
#ifdef CONFIG_RETAINED_MEM_MUTEXES
	const struct zephyr_retained_mem_ram_config *config = dev->config;

	if (!k_is_pre_kernel()) {
		k_mutex_lock(config->lock, K_FOREVER);
	}
#else
	ARG_UNUSED(dev);
#endif
}

static inline void zephyr_retained_mem_ram_lock_release(const struct device *dev)
{
#ifdef CONFIG_RETAINED_MEM_MUTEXES
	const struct zephyr_retained_mem_ram_config *config = dev->config;

	if (!k_is_pre_kernel()) {
		k_mutex_unlock(config->lock);
	}
#else
	ARG_UNUSED(dev);
#endif
}

static ssize_t zephyr_retained_mem_ram_size(const struct device *dev)
{
	const struct zephyr_retained_mem_ram_config *config = dev->config;

	return (ssize_t)config->size;
}

static int zephyr_retained_mem_ram_read(const struct device *dev, off_t offset, uint8_t *buffer,
					size_t size)
{
	const struct zephyr_retained_mem_ram_config *config = dev->config;

	zephyr_retained_mem_ram_lock_take(dev);

	memcpy(buffer, (config->address + offset), size);

	zephyr_retained_mem_ram_lock_release(dev);

	return 0;
}

static int zephyr_retained_mem_ram_write(const struct device *dev, off_t offset,
					 const uint8_t *buffer, size_t size)
{
	const struct zephyr_retained_mem_ram_config *config = dev->config;
	int err = 0;

	zephyr_retained_mem_ram_lock_take(dev);

	memcpy((config->address + offset), buffer, size);

#if defined(CONFIG_CACHE_MANAGEMENT) && defined(CONFIG_DCACHE)
	err = sys_cache_data_flush_range(config->address + offset, size);
#endif /* defined(CONFIG_CACHE_MANAGEMENT) && defined(CONFIG_DCACHE) */

	zephyr_retained_mem_ram_lock_release(dev);

	return err;
}

static int zephyr_retained_mem_ram_clear(const struct device *dev)
{
	const struct zephyr_retained_mem_ram_config *config = dev->config;
	int err = 0;

	zephyr_retained_mem_ram_lock_take(dev);

	memset(config->address, 0, config->size);

#if defined(CONFIG_CACHE_MANAGEMENT) && defined(CONFIG_DCACHE)
	err = sys_cache_data_flush_range(config->address, config->size);
#endif /* defined(CONFIG_CACHE_MANAGEMENT) && defined(CONFIG_DCACHE) */

	zephyr_retained_mem_ram_lock_release(dev);

	return err;
}

static DEVICE_API(retained_mem, zephyr_retained_mem_ram_api) = {
	.size = zephyr_retained_mem_ram_size,
	.read = zephyr_retained_mem_ram_read,
	.write = zephyr_retained_mem_ram_write,
	.clear = zephyr_retained_mem_ram_clear,
};

#define ZEPHYR_RETAINED_MEM_RAM_DEVICE(inst)							\
	IF_ENABLED(CONFIG_RETAINED_MEM_MUTEXES,							\
		   (static K_MUTEX_DEFINE(zephyr_retained_mem_ram_config_lock##inst);))		\
	static const struct zephyr_retained_mem_ram_config					\
			zephyr_retained_mem_ram_config_##inst = {				\
		.address = (uint8_t *)DT_REG_ADDR(DT_PARENT(DT_INST(inst, DT_DRV_COMPAT))),	\
		.size = DT_REG_SIZE(DT_PARENT(DT_INST(inst, DT_DRV_COMPAT))),			\
		IF_ENABLED(CONFIG_RETAINED_MEM_MUTEXES,						\
			(.lock = &zephyr_retained_mem_ram_config_lock##inst,))			\
	};											\
	DEVICE_DT_INST_DEFINE(inst,								\
			      NULL,								\
			      NULL,								\
			      NULL,								\
			      &zephyr_retained_mem_ram_config_##inst,				\
			      PRE_KERNEL_1,							\
			      CONFIG_RETAINED_MEM_INIT_PRIORITY,				\
			      &zephyr_retained_mem_ram_api);

DT_INST_FOREACH_STATUS_OKAY(ZEPHYR_RETAINED_MEM_RAM_DEVICE)
