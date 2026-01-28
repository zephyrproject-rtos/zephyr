/*
 * Copyright (c) 2025, Nordic Semiconductor ASA, RBR Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_retained_bbram

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/bbram.h>
#include <zephyr/drivers/retained_mem.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(retained_mem_zephyr_bbram, CONFIG_RETAINED_MEM_LOG_LEVEL);

#ifdef CONFIG_RETAINED_MEM_MUTEXES
struct retained_bbram_data {
	struct k_mutex lock;
};
#endif

struct retained_bbram_config {
	const struct device *bbram;
};

static inline void retained_bbram_lock(const struct device *dev)
{
#ifdef CONFIG_RETAINED_MEM_MUTEXES
	struct retained_bbram_data *data = dev->data;

	if (!k_is_pre_kernel()) {
		k_mutex_lock(&data->lock, K_FOREVER);
	}
#else
	ARG_UNUSED(dev);
#endif
}

static inline void retained_bbram_unlock(const struct device *dev)
{
#ifdef CONFIG_RETAINED_MEM_MUTEXES
	struct retained_bbram_data *data = dev->data;

	if (!k_is_pre_kernel()) {
		k_mutex_unlock(&data->lock);
	}
#else
	ARG_UNUSED(dev);
#endif
}

static int retained_bbram_init(const struct device *dev)
{
#ifdef CONFIG_RETAINED_MEM_MUTEXES
	struct retained_bbram_data *data = dev->data;

	return k_mutex_init(&data->lock);
#else
	ARG_UNUSED(dev);

	return 0;
#endif
}

static ssize_t retained_bbram_size(const struct device *dev)
{
	const struct retained_bbram_config *config = dev->config;
	int err;
	size_t size;

	err = bbram_get_size(config->bbram, &size);
	if (err < 0) {
		return (ssize_t)err;
	}

	return (ssize_t)size;
}

static int retained_bbram_read(const struct device *dev, off_t offset, uint8_t *buffer, size_t size)
{
	const struct retained_bbram_config *config = dev->config;
	int err;

	retained_bbram_lock(dev);

	err = bbram_read(config->bbram, offset, size, buffer);

	retained_bbram_unlock(dev);

	return err;
}

static int retained_bbram_write(const struct device *dev, off_t offset, const uint8_t *buffer,
				size_t size)
{
	const struct retained_bbram_config *config = dev->config;

	retained_bbram_lock(dev);

	int err = bbram_write(config->bbram, offset, size, buffer);

	retained_bbram_unlock(dev);

	return err;
}

static int retained_bbram_clear(const struct device *dev)
{
	const struct retained_bbram_config *config = dev->config;
	uint8_t clear[CONFIG_RETAINED_MEM_ZEPHYR_BBRAM_CLEAR_CHUNK_SIZE];
	size_t total_size;
	size_t chunk_size = CONFIG_RETAINED_MEM_ZEPHYR_BBRAM_CLEAR_CHUNK_SIZE;
	int err = 0;

	memset(clear, 0, CONFIG_RETAINED_MEM_ZEPHYR_BBRAM_CLEAR_CHUNK_SIZE);

	retained_bbram_lock(dev);

	err = bbram_get_size(config->bbram, &total_size);
	if (err < 0) {
		goto done;
	}

	for (size_t offset = 0; offset < total_size; offset += chunk_size) {
		if (offset + chunk_size > total_size) {
			chunk_size = total_size - offset;
		}

		err = bbram_write(config->bbram, offset, chunk_size, clear);
		if (err < 0) {
			goto done;
		}
	}

done:
	retained_bbram_unlock(dev);
	return err;
}

static const struct retained_mem_driver_api retained_bbram_api = {
	.size = retained_bbram_size,
	.read = retained_bbram_read,
	.write = retained_bbram_write,
	.clear = retained_bbram_clear,
};

#define RETAINED_BBRAM_DEVICE(inst)                                                      \
	IF_ENABLED(CONFIG_RETAINED_MEM_MUTEXES, (                                            \
		static struct retained_bbram_data retained_bbram_data_##inst;                    \
	))                                                                                   \
	static const struct retained_bbram_config retained_bbram_config_##inst = {           \
		.bbram = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                    \
	};                                                                                   \
	DEVICE_DT_INST_DEFINE(                                                               \
		inst,                                                                            \
		&retained_bbram_init,                                                            \
		NULL,                                                                            \
		COND_CODE_1(CONFIG_RETAINED_MEM_MUTEXES, (&retained_bbram_data_##inst), (NULL)), \
		&retained_bbram_config_##inst,                                                   \
		POST_KERNEL,                                                                     \
		CONFIG_RETAINED_MEM_INIT_PRIORITY,                                               \
		&retained_bbram_api);

DT_INST_FOREACH_STATUS_OKAY(RETAINED_BBRAM_DEVICE)
