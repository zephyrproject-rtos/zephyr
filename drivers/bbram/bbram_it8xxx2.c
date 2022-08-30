/*
 * Copyright (c) 2021 Google Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it8xxx2_bbram

#include <zephyr/drivers/bbram.h>
#include <errno.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bbram, CONFIG_BBRAM_LOG_LEVEL);

#define BRAM_VALID_MAGIC        0x4252414D  /* "BRAM" */
#define BRAM_VALID_MAGIC_FIELD0 (BRAM_VALID_MAGIC & 0xff)
#define BRAM_VALID_MAGIC_FIELD1 ((BRAM_VALID_MAGIC >> 8) & 0xff)
#define BRAM_VALID_MAGIC_FIELD2 ((BRAM_VALID_MAGIC >> 16) & 0xff)
#define BRAM_VALID_MAGIC_FIELD3 ((BRAM_VALID_MAGIC >> 24) & 0xff)

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
	const struct bbram_it8xxx2_config *config = dev->config;
	uint8_t *base_addr = (uint8_t *)config->base_addr;
	uint8_t *bram_valid_flag0 = base_addr + BRAM_IDX_VALID_FLAGS0;
	uint8_t *bram_valid_flag1 = base_addr + BRAM_IDX_VALID_FLAGS1;
	uint8_t *bram_valid_flag2 = base_addr + BRAM_IDX_VALID_FLAGS2;
	uint8_t *bram_valid_flag3 = base_addr + BRAM_IDX_VALID_FLAGS3;

	if ((*bram_valid_flag0 != BRAM_VALID_MAGIC_FIELD0) ||
	    (*bram_valid_flag1 != BRAM_VALID_MAGIC_FIELD1) ||
	    (*bram_valid_flag2 != BRAM_VALID_MAGIC_FIELD2) ||
	    (*bram_valid_flag3 != BRAM_VALID_MAGIC_FIELD3)) {
		/*
		 * Magic does not match, so BRAM must be uninitialized. Clear
		 * entire Bank0 BRAM, and set magic value.
		 */
		for (int i = 0; i < BRAM_IDX_VALID_FLAGS0; i++) {
			*(base_addr + i) = 0;
		}

		*bram_valid_flag0 = BRAM_VALID_MAGIC_FIELD0;
		*bram_valid_flag1 = BRAM_VALID_MAGIC_FIELD1;
		*bram_valid_flag2 = BRAM_VALID_MAGIC_FIELD2;
		*bram_valid_flag3 = BRAM_VALID_MAGIC_FIELD3;
	}

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
