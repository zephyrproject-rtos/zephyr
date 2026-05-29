/*
 * Copyright (c) 2023, Nordic Semiconductor ASA
 * Copyright (c) 2023, Bjarki Arge Andreasen
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_buram

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_silabs.h>
#include <zephyr/drivers/retained_mem.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(retained_mem_silabs_buram, CONFIG_RETAINED_MEM_LOG_LEVEL);

struct silabs_buram_data {
#ifdef CONFIG_RETAINED_MEM_MUTEXES
	struct k_mutex lock;
#endif
};

struct silabs_buram_config {
	uint8_t *address;
	size_t size;
	const struct device *clock_dev;
	const struct silabs_clock_control_cmu_config clock_cfg;
};

static inline void silabs_buram_lock_take(const struct device *dev)
{
#ifdef CONFIG_RETAINED_MEM_MUTEXES
	struct silabs_buram_data *data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);
#else
	ARG_UNUSED(dev);
#endif
}

static inline void silabs_buram_lock_release(const struct device *dev)
{
#ifdef CONFIG_RETAINED_MEM_MUTEXES
	struct silabs_buram_data *data = dev->data;

	k_mutex_unlock(&data->lock);
#else
	ARG_UNUSED(dev);
#endif
}

static int silabs_buram_init(const struct device *dev)
{
	const struct silabs_buram_config *config = dev->config;
	int err;

#ifdef CONFIG_RETAINED_MEM_MUTEXES
	struct silabs_buram_data *data = dev->data;

	k_mutex_init(&data->lock);
#endif

	if (!config->clock_dev) {
		/* BURAM is automatically clocked */
		return 0;
	}

	err = clock_control_on(config->clock_dev, (clock_control_subsys_t)&config->clock_cfg);
	if (err < 0 && err != -EALREADY) {
		return err;
	}

	return 0;
}

static ssize_t silabs_buram_size(const struct device *dev)
{
	const struct silabs_buram_config *config = dev->config;

	return (ssize_t)config->size;
}

static int silabs_buram_read(const struct device *dev, off_t offset, uint8_t *buffer, size_t size)
{
	const struct silabs_buram_config *config = dev->config;

	silabs_buram_lock_take(dev);

	memcpy(buffer, (config->address + offset), size);

	silabs_buram_lock_release(dev);

	return 0;
}

static int silabs_buram_write(const struct device *dev, off_t offset, const uint8_t *buffer,
			      size_t size)
{
	const struct silabs_buram_config *config = dev->config;

	silabs_buram_lock_take(dev);

	memcpy((config->address + offset), buffer, size);

	silabs_buram_lock_release(dev);

	return 0;
}

static int silabs_buram_clear(const struct device *dev)
{
	const struct silabs_buram_config *config = dev->config;

	silabs_buram_lock_take(dev);

	memset(config->address, 0, config->size);

	silabs_buram_lock_release(dev);

	return 0;
}

static DEVICE_API(retained_mem, silabs_buram_api) = {
	.size = silabs_buram_size,
	.read = silabs_buram_read,
	.write = silabs_buram_write,
	.clear = silabs_buram_clear,
};

#define SILABS_BURAM_DEVICE(inst)                                                                  \
	static struct silabs_buram_data silabs_buram_data_##inst;                                  \
	static const struct silabs_buram_config silabs_buram_config_##inst = {                     \
		.address = (uint8_t *)DT_INST_REG_ADDR(inst),                                      \
		.size = DT_INST_REG_SIZE(inst),                                                    \
		.clock_dev = COND_CODE_1(DT_INST_CLOCKS_HAS_IDX(inst, 0),                          \
					 (DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst))),               \
					 (NULL)),                                                  \
		.clock_cfg = COND_CODE_1(DT_INST_CLOCKS_HAS_IDX(inst, 0),                          \
					 (SILABS_DT_INST_CLOCK_CFG(inst)),                         \
					 ({0})),                                                   \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, &silabs_buram_init, NULL, &silabs_buram_data_##inst,           \
			      &silabs_buram_config_##inst, POST_KERNEL,                            \
			      CONFIG_RETAINED_MEM_INIT_PRIORITY, &silabs_buram_api);

DT_INST_FOREACH_STATUS_OKAY(SILABS_BURAM_DEVICE)
