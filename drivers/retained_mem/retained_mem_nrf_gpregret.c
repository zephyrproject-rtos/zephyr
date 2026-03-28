/*
 * Copyright (c) 2023, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrf_gpregret

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/retained_mem.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(retained_mem_nrf_gpregret, CONFIG_RETAINED_MEM_LOG_LEVEL);

#ifdef CONFIG_RETAINED_MEM_MUTEXES
struct nrf_gpregret_data {
	struct k_mutex lock;
};
#endif

struct nrf_gpregret_config {
	uint8_t *addr;
	size_t size;
};

static inline void nrf_gpregret_lock_take(const struct device *dev)
{
#ifdef CONFIG_RETAINED_MEM_MUTEXES
	struct nrf_gpregret_data *data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);
#else
	ARG_UNUSED(dev);
#endif
}

static inline void nrf_gpregret_lock_release(const struct device *dev)
{
#ifdef CONFIG_RETAINED_MEM_MUTEXES
	struct nrf_gpregret_data *data = dev->data;

	k_mutex_unlock(&data->lock);
#else
	ARG_UNUSED(dev);
#endif
}

static int nrf_gpregret_init(const struct device *dev)
{
#ifdef CONFIG_RETAINED_MEM_MUTEXES
	struct nrf_gpregret_data *data = dev->data;

	k_mutex_init(&data->lock);
#endif

	return 0;
}

static ssize_t nrf_gpregret_size(const struct device *dev)
{
	const struct nrf_gpregret_config *config = dev->config;

	return (ssize_t)config->size;
}

static int nrf_gpregret_read(const struct device *dev, off_t offset, uint8_t *buffer, size_t size)
{
	const struct nrf_gpregret_config *config = dev->config;

	nrf_gpregret_lock_take(dev);

	memcpy(buffer, (config->addr + offset), size);

	nrf_gpregret_lock_release(dev);

	return 0;
}

static int nrf_gpregret_write(const struct device *dev, off_t offset, const uint8_t *buffer,
			      size_t size)
{
	const struct nrf_gpregret_config *config = dev->config;

	nrf_gpregret_lock_take(dev);

	memcpy((config->addr + offset), buffer, size);

	nrf_gpregret_lock_release(dev);

	return 0;
}

static int nrf_gpregret_clear(const struct device *dev)
{
	const struct nrf_gpregret_config *config = dev->config;

	nrf_gpregret_lock_take(dev);

	memset(config->addr, 0, config->size);

	nrf_gpregret_lock_release(dev);

	return 0;
}

static DEVICE_API(retained_mem, nrf_gpregret_api) = {
	.size = nrf_gpregret_size,
	.read = nrf_gpregret_read,
	.write = nrf_gpregret_write,
	.clear = nrf_gpregret_clear,
};

#define NRF_GPREGRET_DEVICE(inst)						\
	IF_ENABLED(CONFIG_RETAINED_MEM_MUTEXES,					\
		   (static struct nrf_gpregret_data nrf_gpregret_data_##inst;)	\
	)									\
	static const struct nrf_gpregret_config nrf_gpregret_config_##inst = {	\
		.addr = (uint8_t *)DT_INST_REG_ADDR(inst),			\
		.size = DT_INST_REG_SIZE(inst),					\
	};									\
	DEVICE_DT_INST_DEFINE(inst,						\
			      &nrf_gpregret_init,				\
			      NULL,						\
			      COND_CODE_1(CONFIG_RETAINED_MEM_MUTEXES,		\
					  (&nrf_gpregret_data_##inst), (NULL)	\
			      ),						\
			      &nrf_gpregret_config_##inst,			\
			      POST_KERNEL,					\
			      CONFIG_RETAINED_MEM_INIT_PRIORITY,		\
			      &nrf_gpregret_api);

DT_INST_FOREACH_STATUS_OKAY(NRF_GPREGRET_DEVICE)
