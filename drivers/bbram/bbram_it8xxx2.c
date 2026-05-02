/*
 * Copyright (c) 2021 Google Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it8xxx2_bbram

#include <errno.h>

#include <zephyr/drivers/bbram.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#ifndef CONFIG_BBRAM_IT8XXX2_EMUL
#include <chip_chipregs.h>
#else
/* Emulation register values */
enum bram_indices {
	BRAM_IDX_VALID_FLAGS0,
	BRAM_IDX_VALID_FLAGS1,
	BRAM_IDX_VALID_FLAGS2,
	BRAM_IDX_VALID_FLAGS3,
};
#endif

#include "it8xxx2.h"

LOG_MODULE_REGISTER(it8xxx2_bbram, CONFIG_BBRAM_LOG_LEVEL);

#define BRAM_VALID_MAGIC        0x4252414D /* "BRAM" */
#define BRAM_VALID_MAGIC_FIELD0 (BRAM_VALID_MAGIC & 0xff)
#define BRAM_VALID_MAGIC_FIELD1 ((BRAM_VALID_MAGIC >> 8) & 0xff)
#define BRAM_VALID_MAGIC_FIELD2 ((BRAM_VALID_MAGIC >> 16) & 0xff)
#define BRAM_VALID_MAGIC_FIELD3 ((BRAM_VALID_MAGIC >> 24) & 0xff)

static int bbram_it8xxx2_read(const struct device *dev, size_t offset, size_t size, uint8_t *data)
{
	const struct bbram_it8xxx2_config *config = dev->config;

	if (size < 1 || offset + size > config->size) {
		return -EINVAL;
	}

	bytecpy(data, ((uint8_t *)config->base_addr + offset), size);
	return 0;
}

static int bbram_it8xxx2_write(const struct device *dev, size_t offset, size_t size,
			       const uint8_t *data)
{
	const struct bbram_it8xxx2_config *config = dev->config;

	if (size < 1 || offset + size > config->size) {
		return -EINVAL;
	}

	bytecpy(((uint8_t *)config->base_addr + offset), data, size);
	return 0;
}

static int bbram_it8xxx2_size(const struct device *dev, size_t *size)
{
	const struct bbram_it8xxx2_config *config = dev->config;

	*size = config->size;
	return 0;
}

static DEVICE_API(bbram, bbram_it8xxx2_driver_api) = {
	.read = bbram_it8xxx2_read,
	.write = bbram_it8xxx2_write,
	.get_size = bbram_it8xxx2_size,
};

static int bbram_it8xxx2_init(const struct device *dev)
{
	const struct bbram_it8xxx2_config *config = dev->config;
	uint8_t *base_addr = (uint8_t *)config->base_addr;
	uint8_t *bram_valid_flag0 = base_addr + BRAM_IDX_VALID_FLAGS0;
	uint8_t *bram_valid_flag1 = base_addr + BRAM_IDX_VALID_FLAGS1;
	uint8_t *bram_valid_flag2 = base_addr + BRAM_IDX_VALID_FLAGS2;
	uint8_t *bram_valid_flag3 = base_addr + BRAM_IDX_VALID_FLAGS3;
	int size = config->size;

	if ((*bram_valid_flag0 != BRAM_VALID_MAGIC_FIELD0) ||
	    (*bram_valid_flag1 != BRAM_VALID_MAGIC_FIELD1) ||
	    (*bram_valid_flag2 != BRAM_VALID_MAGIC_FIELD2) ||
	    (*bram_valid_flag3 != BRAM_VALID_MAGIC_FIELD3)) {
		/*
		 * Magic does not match, so BRAM must be uninitialized. Clear
		 * entire Bank0 and Bank1 BRAM, and set magic value.
		 */
		for (int i = 0; i < size; i++) {
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
	BBRAM_IT8XXX2_DECL_CONFIG(inst);                                                           \
	DEVICE_DT_INST_DEFINE(inst, bbram_it8xxx2_init, NULL, NULL, &bbram_cfg_##inst,             \
			      PRE_KERNEL_1, CONFIG_BBRAM_INIT_PRIORITY,                            \
			      &bbram_it8xxx2_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BBRAM_INIT);
