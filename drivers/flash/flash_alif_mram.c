/*
 * Copyright (c) 2025 Alif Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT alif_mram_flash_controller

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(flash_alif_mram, CONFIG_FLASH_LOG_LEVEL);

#define SOC_NV_FLASH_NODE DT_INST(0, soc_nv_flash)

#define MRAM_BASE        DT_REG_ADDR(SOC_NV_FLASH_NODE)
#define MRAM_SIZE        DT_REG_SIZE(SOC_NV_FLASH_NODE)
#define MRAM_ERASE_UNIT  DT_PROP(SOC_NV_FLASH_NODE, erase_block_size)
#define MRAM_WORD_SIZE   DT_PROP(SOC_NV_FLASH_NODE, write_block_size)
/* Erased MRAM reads as zeros, matching the Alif SETOOLS MRAM erase */
#define MRAM_ERASE_VALUE 0x00U

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1, "Only one MRAM controller supported");
BUILD_ASSERT(MRAM_WORD_SIZE == 16U, "MRAM is programmed in 128-bit words");
BUILD_ASSERT((MRAM_ERASE_UNIT % MRAM_WORD_SIZE) == 0U,
	     "Erase unit must be a multiple of the MRAM word size");
BUILD_ASSERT((MRAM_SIZE % MRAM_ERASE_UNIT) == 0U, "MRAM size must be a multiple of erase unit");

#define MRAM_ADDR(offset) ((volatile uint8_t *)(MRAM_BASE + (uintptr_t)(offset)))

/*
 * The writable part of MRAM must be mapped as Device memory, so every access
 * goes through volatile 32-bit loads and stores instead of memcpy().
 */

struct flash_alif_mram_data {
	struct k_mutex lock;
};

static const struct flash_parameters flash_alif_mram_parameters = {
	.write_block_size = MRAM_WORD_SIZE,
	.erase_value = MRAM_ERASE_VALUE,
	.caps = {
		.no_explicit_erase = true,
	},
};

static void mram_write_word(volatile uint8_t *dst, const uint8_t *src)
{
	volatile uint32_t *dst32 = (volatile uint32_t *)dst;
	unsigned int key;

	/* All four stores must reach the MRAM controller as one 128-bit word */
	key = irq_lock();

	dst32[0] = UNALIGNED_GET((const uint32_t *)&src[0]);
	dst32[1] = UNALIGNED_GET((const uint32_t *)&src[4]);
	dst32[2] = UNALIGNED_GET((const uint32_t *)&src[8]);
	dst32[3] = UNALIGNED_GET((const uint32_t *)&src[12]);

	irq_unlock(key);
}

static void mram_read(uint8_t *dst, const volatile uint8_t *src, size_t len)
{
	size_t remaining = len;

	while ((remaining > 0U) && (((uintptr_t)src & 0x3U) != 0U)) {
		*dst = *src;
		dst++;
		src++;
		remaining--;
	}

	while (remaining >= sizeof(uint32_t)) {
		UNALIGNED_PUT(*(const volatile uint32_t *)src, (uint32_t *)dst);
		src += sizeof(uint32_t);
		dst += sizeof(uint32_t);
		remaining -= sizeof(uint32_t);
	}

	while (remaining > 0U) {
		*dst = *src;
		dst++;
		src++;
		remaining--;
	}
}

static bool flash_alif_mram_range_is_valid(off_t offset, size_t len)
{
	return (offset >= 0) && ((size_t)offset <= MRAM_SIZE) &&
	       (len <= (MRAM_SIZE - (size_t)offset));
}

static int flash_alif_mram_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	struct flash_alif_mram_data *dev_data = dev->data;

	if (len == 0U) {
		return 0;
	}

	if (!flash_alif_mram_range_is_valid(offset, len)) {
		LOG_ERR("Invalid range: offset %ld, len %zu", (long)offset, len);
		return -EINVAL;
	}

	k_mutex_lock(&dev_data->lock, K_FOREVER);
	mram_read(data, MRAM_ADDR(offset), len);
	k_mutex_unlock(&dev_data->lock);

	return 0;
}

static int flash_alif_mram_write(const struct device *dev, off_t offset, const void *data,
				 size_t len)
{
	struct flash_alif_mram_data *dev_data = dev->data;
	const uint8_t *src = data;
	volatile uint8_t *dst;
	uint8_t word[MRAM_WORD_SIZE];
	size_t head;

	if (len == 0U) {
		return 0;
	}

	if (!flash_alif_mram_range_is_valid(offset, len)) {
		LOG_ERR("Invalid range: offset %ld, len %zu", (long)offset, len);
		return -EINVAL;
	}

	k_mutex_lock(&dev_data->lock, K_FOREVER);

	dst = MRAM_ADDR(ROUND_DOWN(offset, MRAM_WORD_SIZE));
	head = (size_t)offset % MRAM_WORD_SIZE;

	/* Read-modify-write a partial leading word */
	if (head != 0U) {
		size_t count = MIN(MRAM_WORD_SIZE - head, len);

		mram_read(word, dst, MRAM_WORD_SIZE);
		memcpy(&word[head], src, count);
		mram_write_word(dst, word);

		dst += MRAM_WORD_SIZE;
		src += count;
		len -= count;
	}

	while (len >= MRAM_WORD_SIZE) {
		mram_write_word(dst, src);

		dst += MRAM_WORD_SIZE;
		src += MRAM_WORD_SIZE;
		len -= MRAM_WORD_SIZE;
	}

	/* Read-modify-write a partial trailing word */
	if (len > 0U) {
		mram_read(word, dst, MRAM_WORD_SIZE);
		memcpy(word, src, len);
		mram_write_word(dst, word);
	}

	k_mutex_unlock(&dev_data->lock);

	return 0;
}

static int flash_alif_mram_erase(const struct device *dev, off_t offset, size_t size)
{
	struct flash_alif_mram_data *dev_data = dev->data;
	uint8_t word[MRAM_WORD_SIZE];
	volatile uint8_t *dst;

	if (size == 0U) {
		return 0;
	}

	if (!flash_alif_mram_range_is_valid(offset, size)) {
		LOG_ERR("Invalid range: offset %ld, size %zu", (long)offset, size);
		return -EINVAL;
	}

	if ((((size_t)offset % MRAM_ERASE_UNIT) != 0U) || ((size % MRAM_ERASE_UNIT) != 0U)) {
		LOG_ERR("Offset %ld and size %zu must be multiples of %d", (long)offset, size,
			MRAM_ERASE_UNIT);
		return -EINVAL;
	}

	memset(word, MRAM_ERASE_VALUE, sizeof(word));

	k_mutex_lock(&dev_data->lock, K_FOREVER);

	dst = MRAM_ADDR(offset);

	for (size_t done = 0U; done < size; done += MRAM_WORD_SIZE) {
		mram_write_word(&dst[done], word);
	}

	k_mutex_unlock(&dev_data->lock);

	return 0;
}

static const struct flash_parameters *flash_alif_mram_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_alif_mram_parameters;
}

static int flash_alif_mram_get_size(const struct device *dev, uint64_t *size)
{
	ARG_UNUSED(dev);

	*size = MRAM_SIZE;

	return 0;
}

#ifdef CONFIG_FLASH_PAGE_LAYOUT
static const struct flash_pages_layout flash_alif_mram_pages_layout = {
	.pages_count = MRAM_SIZE / MRAM_ERASE_UNIT,
	.pages_size = MRAM_ERASE_UNIT,
};

static void flash_alif_mram_page_layout(const struct device *dev,
					const struct flash_pages_layout **layout,
					size_t *layout_size)
{
	ARG_UNUSED(dev);

	*layout = &flash_alif_mram_pages_layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static DEVICE_API(flash, flash_alif_mram_api) = {
	.read = flash_alif_mram_read,
	.write = flash_alif_mram_write,
	.erase = flash_alif_mram_erase,
	.get_parameters = flash_alif_mram_get_parameters,
	.get_size = flash_alif_mram_get_size,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = flash_alif_mram_page_layout,
#endif
};

static int flash_alif_mram_init(const struct device *dev)
{
	struct flash_alif_mram_data *dev_data = dev->data;

	k_mutex_init(&dev_data->lock);

	return 0;
}

static struct flash_alif_mram_data flash_alif_mram_data_0;

DEVICE_DT_INST_DEFINE(0, flash_alif_mram_init, NULL, &flash_alif_mram_data_0, NULL, POST_KERNEL,
		      CONFIG_FLASH_INIT_PRIORITY, &flash_alif_mram_api);
