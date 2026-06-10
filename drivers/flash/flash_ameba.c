/*
 * Copyright (c) 2026 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT realtek_ameba_flash_controller

/* Include <soc.h> before <ameba_soc.h> to avoid redefining unlikely() macro */
#include <soc.h>
#include <ameba_soc.h>

#include <zephyr/drivers/flash.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(flash_ameba, CONFIG_FLASH_LOG_LEVEL);

#define SOC_NV_FLASH_COMPAT(node_id) \
	COND_CODE_1(DT_NODE_HAS_COMPAT(node_id, soc_nv_flash), (node_id), ())
#define SOC_NV_FLASH_NODE \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(0, SOC_NV_FLASH_COMPAT)

#define FLASH_WRITE_BLK_SZ DT_PROP(SOC_NV_FLASH_NODE, write_block_size)
#define FLASH_ERASE_BLK_SZ DT_PROP(SOC_NV_FLASH_NODE, erase_block_size)

#define FLASH_SEM_TIMEOUT (k_is_in_isr() ? K_NO_WAIT : K_FOREVER)

struct flash_ameba_dev_config {
	uint32_t base_addr;
};

struct flash_ameba_dev_data {
#ifdef CONFIG_MULTITHREADING
	struct k_sem sem;
#endif
};

static const struct flash_parameters flash_ameba_parameters = {
	.write_block_size = FLASH_WRITE_BLK_SZ,
	.erase_value = 0xff,
};

#ifdef CONFIG_MULTITHREADING
static inline int flash_ameba_sem_take(const struct device *dev)
{
	struct flash_ameba_dev_data *data = dev->data;

	return k_sem_take(&data->sem, FLASH_SEM_TIMEOUT);
}

static inline void flash_ameba_sem_give(const struct device *dev)
{
	struct flash_ameba_dev_data *data = dev->data;

	k_sem_give(&data->sem);
}
#else

#define flash_ameba_sem_take(dev) (0)
#define flash_ameba_sem_give(dev) ((void)(dev))

#endif /* CONFIG_MULTITHREADING */

static int flash_ameba_read(const struct device *dev, off_t address, void *buffer, size_t length)
{
	int ret;

	if (buffer == NULL) {
		return -EINVAL;
	}

	ret = flash_ameba_sem_take(dev);
	if (ret) {
		return ret;
	}

	FLASH_ReadStream(address, length, buffer);

	flash_ameba_sem_give(dev);
	return 0;
}

static int flash_ameba_write(const struct device *dev, off_t address, const void *buffer,
			     size_t length)
{
	int ret;
	uint8_t ram_buf[CONFIG_FLASH_AMEBA_BUF_SIZE];
	size_t offset = 0;
	size_t rest_len = length;

	ret = flash_ameba_sem_take(dev);
	if (ret) {
		return ret;
	}

	if (IS_FLASH_ADDR((uint32_t)buffer)) {
		while (rest_len > 0) {
			size_t single_write_len = MIN(rest_len, CONFIG_FLASH_AMEBA_BUF_SIZE);

			memcpy(ram_buf, (const uint8_t *)buffer + offset, single_write_len);
			FLASH_WriteStream(address + offset, single_write_len, ram_buf);
			offset += single_write_len;
			rest_len -= single_write_len;
		}
	} else {
		FLASH_WriteStream(address, length, (uint8_t *)buffer);
	}

	flash_ameba_sem_give(dev);
	return 0;
}

static int flash_ameba_erase(const struct device *dev, off_t offset, size_t len)
{
	struct flash_pages_info info;
	uint32_t start_sector_offset, end_sector_offset;
	int ret;

	ret = flash_ameba_sem_take(dev);
	if (ret) {
		return ret;
	}

	ret = flash_get_page_info_by_offs(dev, offset, &info);
	if (ret) {
		goto out;
	}
	start_sector_offset = info.start_offset;

	ret = flash_get_page_info_by_offs(dev, offset + len - 1, &info);
	if (ret) {
		goto out;
	}
	end_sector_offset = info.start_offset;

	while (start_sector_offset <= end_sector_offset) {
		FLASH_EraseXIP(EraseSector, start_sector_offset);
		start_sector_offset += FLASH_ERASE_BLK_SZ;
	}

out:
	flash_ameba_sem_give(dev);

	return ret;
}

#ifdef CONFIG_FLASH_PAGE_LAYOUT
static const struct flash_pages_layout flash_ameba_pages_layout = {
	.pages_count = DT_REG_SIZE(SOC_NV_FLASH_NODE) / FLASH_ERASE_BLK_SZ,
	.pages_size = DT_PROP(SOC_NV_FLASH_NODE, erase_block_size),
};

static void flash_ameba_page_layout(const struct device *dev,
				    const struct flash_pages_layout **layout,
				    size_t *layout_size)
{
	*layout = &flash_ameba_pages_layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static const struct flash_parameters *flash_ameba_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_ameba_parameters;
}

static int flash_ameba_init(const struct device *dev)
{
	struct flash_ameba_dev_data *const dev_data = dev->data;

#ifdef CONFIG_MULTITHREADING
	k_sem_init(&dev_data->sem, 1, 1);
#endif /* CONFIG_MULTITHREADING */

	return 0;
}

static int flash_ameba_get_size(const struct device *dev, uint64_t *size)
{
	ARG_UNUSED(dev);

	*size = (uint64_t)DT_REG_SIZE(SOC_NV_FLASH_NODE);

	return 0;
}

static DEVICE_API(flash, flash_ameba_driver_api) = {
	.read = flash_ameba_read,
	.write = flash_ameba_write,
	.erase = flash_ameba_erase,
	.get_parameters = flash_ameba_get_parameters,
	.get_size = flash_ameba_get_size,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = flash_ameba_page_layout,
#endif
};

static struct flash_ameba_dev_data flash_ameba_data;

static const struct flash_ameba_dev_config flash_ameba_config = {
	.base_addr = DT_INST_REG_ADDR(0),
};

DEVICE_DT_INST_DEFINE(0, flash_ameba_init, NULL, &flash_ameba_data, &flash_ameba_config,
		      POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY, &flash_ameba_driver_api);
