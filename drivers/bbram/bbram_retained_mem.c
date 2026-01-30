/*
 * Copyright 2026 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_bbram_retained_mem

#include <zephyr/drivers/bbram.h>
#include <zephyr/drivers/retained_mem.h>
#include <string.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bbram, CONFIG_BBRAM_LOG_LEVEL);

/** Device config */
struct bbram_retained_mem_config {
	/** Retained memory device */
	const struct device *parent;
	/** Offset within retained memory */
	size_t offset;
	/** BBRAM size in bytes */
	size_t size;
};

static int bbram_retained_mem_get_size(const struct device *dev, size_t *size)
{
	const struct bbram_retained_mem_config *config = dev->config;

	*size = config->size;
	return 0;
}

static int bbram_retained_mem_read(const struct device *dev, size_t offset, size_t size,
				   uint8_t *data)
{
	const struct bbram_retained_mem_config *config = dev->config;

	if (size < 1 || offset + size > config->size) {
		return -EFAULT;
	}

	return retained_mem_read(config->parent, (config->offset + offset), data, size);
}

static int bbram_retained_mem_write(const struct device *dev, size_t offset, size_t size,
				    const uint8_t *data)
{
	const struct bbram_retained_mem_config *config = dev->config;

	if (size < 1 || offset + size > config->size) {
		return -EFAULT;
	}

	return retained_mem_write(config->parent, (config->offset + offset), data, size);
}

static DEVICE_API(bbram, bbram_retained_mem_driver_api) = {
	.get_size = bbram_retained_mem_get_size,
	.read = bbram_retained_mem_read,
	.write = bbram_retained_mem_write,
};

#define BBRAM_INIT(inst)                                                                           \
	static struct bbram_retained_mem_config bbram_retained_mem_config_##inst = {               \
		.parent = DEVICE_DT_GET(DT_PARENT(DT_DRV_INST(inst))),                             \
		.offset = DT_INST_REG_ADDR(inst),                                                  \
		.size = DT_INST_REG_SIZE(inst),                                                    \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, NULL, &bbram_retained_mem_config_##inst,           \
			      PRE_KERNEL_1, CONFIG_BBRAM_INIT_PRIORITY,                            \
			      &bbram_retained_mem_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BBRAM_INIT);
