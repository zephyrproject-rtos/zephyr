/*
 * Copyright (c) 2022 Google Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_bbram

#include <errno.h>

#include <zephyr/drivers/bbram.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
LOG_MODULE_REGISTER(bbram, CONFIG_BBRAM_LOG_LEVEL);

#define STM32_BKP_REG_BYTES		 4
#define STM32_BKP_REG_OFFSET		 0x50
#define STM32_BKP_REG_INDEX(offset)	 ((offset) >> 2)
#define STM32_BKP_REG_BYTE_INDEX(offset) ((offset)&0x3UL)
#define STM32_BKP_REG(i)		 (((volatile uint32_t *)config->base_addr)[(i)])

/** Device config */
struct bbram_stm32_config {
	const struct device *parent;
	/* BBRAM base address */
	uintptr_t base_addr;
	/* BBRAM size in bytes. */
	int size;
};

static int bbram_stm32_read(const struct device *dev, size_t offset, size_t size, uint8_t *data)
{
	const struct bbram_stm32_config *config = dev->config;
	uint32_t reg, begin, to_copy;

	if (size < 1 || offset + size > config->size) {
		return -EFAULT;
	}

	for (size_t read = 0; read < size; read += to_copy) {
		reg = STM32_BKP_REG(STM32_BKP_REG_INDEX(offset + read));
		begin = STM32_BKP_REG_BYTE_INDEX(offset + read);
		to_copy = MIN(STM32_BKP_REG_BYTES - begin, size - read);
		bytecpy(data + read, (uint8_t *)&reg + begin, to_copy);
	}

	return 0;
}

static int bbram_stm32_write(const struct device *dev, size_t offset, size_t size,
			     const uint8_t *data)
{
	const struct bbram_stm32_config *config = dev->config;
	uint32_t reg, begin, to_copy;

	if (size < 1 || offset + size > config->size) {
		return -EFAULT;
	}

	for (size_t written = 0; written < size; written += to_copy) {
		reg = STM32_BKP_REG(STM32_BKP_REG_INDEX(offset + written));
		begin = STM32_BKP_REG_BYTE_INDEX(offset + written);
		to_copy = MIN(STM32_BKP_REG_BYTES - begin, size - written);
		bytecpy((uint8_t *)&reg + begin, data + written, to_copy);
		STM32_BKP_REG(STM32_BKP_REG_INDEX(offset + written)) = reg;
	}

	return 0;
}

static int bbram_stm32_get_size(const struct device *dev, size_t *size)
{
	const struct bbram_stm32_config *config = dev->config;

	*size = config->size;

	return 0;
}

static const struct bbram_driver_api bbram_stm32_driver_api = {
	.read = bbram_stm32_read,
	.write = bbram_stm32_write,
	.get_size = bbram_stm32_get_size,
};

static int bbram_stm32_init(const struct device *dev)
{
	const struct bbram_stm32_config *config = dev->config;

	if (!device_is_ready(config->parent)) {
		LOG_ERR("Device %s is not ready", config->parent->name);
		return -ENODEV;
	}

	return 0;
}

#define BBRAM_INIT(inst)                                                                           \
	static const struct bbram_stm32_config bbram_cfg_##inst = {                                \
		.parent = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                     \
		.base_addr = DT_REG_ADDR(DT_INST_PARENT(inst)) + STM32_BKP_REG_OFFSET,             \
		.size = DT_INST_PROP(inst, st_backup_regs) * STM32_BKP_REG_BYTES,                  \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, bbram_stm32_init, NULL, NULL, &bbram_cfg_##inst, PRE_KERNEL_1, \
			      CONFIG_BBRAM_INIT_PRIORITY, &bbram_stm32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BBRAM_INIT);
