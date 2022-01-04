/*
 * Copyright (c) 2021 Google Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it8xxx2_bbram

#include <drivers/bbram.h>
#include <errno.h>
#include <sys/util.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(bbram, CONFIG_BBRAM_LOG_LEVEL);

/** Device config */
struct bbram_it8xxx2_config {
	/** BBRAM base address */
	uintptr_t base_addr;
	/** BBRAM size (Unit:bytes) */
	int size;
};

static int bbram_it8xxx2_read(const struct device *dev, size_t offset, size_t size, uint8_t *data)
{
	const struct bbram_it8xxx2_config *config = dev->config;

	if (size < 1 || offset + size > config->size) {
		return -EFAULT;
	}

	bytecpy(data, ((uint8_t *)config->base_addr + offset), size);
	return 0;
}

static int bbram_it8xxx2_write(const struct device *dev, size_t offset, size_t size,
			       const uint8_t *data)
{
	const struct bbram_it8xxx2_config *config = dev->config;

	if (size < 1 || offset + size > config->size) {
		return -EFAULT;
	}

	bytecpy(((uint8_t *)config->base_addr + offset), data, size);
	return 0;
}

static const struct bbram_driver_api bbram_it8xxx2_driver_api = {
	.read = bbram_it8xxx2_read,
	.write = bbram_it8xxx2_write,
};

static int bbram_it8xxx2_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

#define BBRAM_INIT(inst)                                                                           \
	static const struct bbram_it8xxx2_config bbram_cfg_##inst = {                              \
		.base_addr = DT_INST_REG_ADDR(inst),                                               \
		.size = DT_INST_REG_SIZE(inst),                                                    \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, bbram_it8xxx2_init, NULL, NULL, &bbram_cfg_##inst,             \
			      PRE_KERNEL_1, CONFIG_BBRAM_INIT_PRIORITY,                            \
			      &bbram_it8xxx2_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BBRAM_INIT);
