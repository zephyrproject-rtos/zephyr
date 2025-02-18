/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2025 Realtek Semiconductor Corporation, SIBG-SD7
 * Author: Elmo Lan <elmo_lan@realtek.com>
 */

#define DT_DRV_COMPAT realtek_rts5912_bbram

#include <zephyr/drivers/bbram.h>
#include <errno.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rts5912_bbram, CONFIG_BBRAM_LOG_LEVEL);

struct bbram_rts5912_config {
	uintptr_t base;
	int size;
};

static int bbram_rts5912_get_size(const struct device *dev, size_t *size)
{
	const struct bbram_rts5912_config *config = dev->config;

	*size = config->size;
	LOG_INF("size: 0x%08x", *size);
	return 0;
}

static int bbram_rts5912_read(const struct device *dev, size_t offset, size_t size, uint8_t *data)
{
	const struct bbram_rts5912_config *config = dev->config;

	uint32_t word;
	size_t start_offset, bytes_to_read;

	if (size < 1 || offset + size > config->size) {
		LOG_ERR("Invalid params");
		return -EFAULT;
	}

	/* The bbram in RTS5912 only accepts word-access.
	 * Read it as a word, then split it into bytes.
	 */
	for (int i = 0; i < size;) {
		word = *(uint32_t *)(config->base + ROUND_DOWN((offset + i), 4));
		start_offset = (offset + i) % 4;
		bytes_to_read = MIN(4 - start_offset, size - i);
		memcpy(&data[i], ((uint8_t *)&word) + start_offset, bytes_to_read);
		i += bytes_to_read;
	}

	return 0;
}

static int bbram_rts5912_write(const struct device *dev, size_t offset, size_t size,
			       const uint8_t *data)
{
	const struct bbram_rts5912_config *config = dev->config;

	uint32_t word;
	size_t start_offset, bytes_to_write;

	if (size < 1 || offset + size > config->size) {
		LOG_ERR("Invalid params");
		return -EFAULT;
	}

	/* The BBRAM in RTS5912 only accepts word-access. Read it as a word,
	 * then modify the byte field, and finally write it back as a word
	 */
	for (int i = 0; i < size;) {
		word = *(uint32_t *)(config->base + ROUND_DOWN((offset + i), 4));
		start_offset = (offset + i) % 4;
		bytes_to_write = MIN(4 - start_offset, size - i);
		memcpy(((uint8_t *)&word) + start_offset, &data[i], bytes_to_write);
		*(uint32_t *)(config->base + ROUND_DOWN((offset + i), 4)) = word;
		i += bytes_to_write;
	}

	return 0;
}

static DEVICE_API(bbram, bbram_rts5912_driver_api) = {
	.get_size = bbram_rts5912_get_size,
	.read = bbram_rts5912_read,
	.write = bbram_rts5912_write,
};

#define BBRAM_INIT(inst)                                                                           \
	static const struct bbram_rts5912_config bbram_cfg_##inst = {                              \
		.base = DT_INST_REG_ADDR(inst),                                                    \
		.size = DT_INST_REG_SIZE(inst),                                                    \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, NULL, &bbram_cfg_##inst, PRE_KERNEL_1,             \
			      CONFIG_BBRAM_INIT_PRIORITY, &bbram_rts5912_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BBRAM_INIT);
