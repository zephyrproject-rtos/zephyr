/*
 * Copyright (c) 2024, Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/storage/nvmp.h>
#include <zephyr/drivers/flash.h>
#include <string.h>

LOG_MODULE_REGISTER(nvmp_flash, CONFIG_NVMP_LOG_LEVEL);

struct flash_store {
	const struct device *dev;
	size_t offset;
	uint8_t erase_value;
};

static int nvmp_flash_open(const struct nvmp_info *info)
{
	const struct flash_store *st = (const struct flash_store *)info->store;
	int rc = 0;

	if (!device_is_ready(st->dev)) {
		rc = -ENODEV;
	}

	if (IS_ENABLED(CONFIG_NVMP_FLASH_RUNTIME_VERIFY)) {
		const struct flash_parameters *flash_parameters =
			flash_get_parameters(st->dev);
		const size_t wbs = flash_parameters->write_block_size;
		const uint8_t erase_value = flash_parameters->erase_value;
		struct flash_pages_info page = {
			.start_offset = st->offset,
		};

		while (page.start_offset < (st->offset + info->size)) {
			rc = flash_get_page_info_by_offs(
				st->dev, page.start_offset, &page);
			if (rc != 0) {
				LOG_ERR("Failed to get flash page info");
				break;
			}

			if ((info->block_size % page.size) != 0U) {
				LOG_ERR("Block size configuration error");
				rc = -EINVAL;
				break;
			}

			page.start_offset += page.size;
		}

		if ((info->write_block_size % wbs) != 0U) {
			LOG_ERR("Write block size configuration error");
			rc = -EINVAL;
		}

		if (st->erase_value != erase_value) {
			LOG_ERR("Erase value configuration error");
			rc = -EINVAL;
		}
	}

	return rc;
}

static int nvmp_flash_read(const struct nvmp_info *info, size_t start,
			   void *data, size_t len)
{
	const struct flash_store *st = (const struct flash_store *)info->store;

	LOG_DBG("read %d byte at 0x%x", len, start);
	start += st->offset;
	return flash_read(st->dev, (off_t)start, data, len);
}

static int nvmp_flash_write(const struct nvmp_info *info, size_t start,
			    const void *data, size_t len)
{
	const struct flash_store *st = (const struct flash_store *)info->store;

	LOG_DBG("write %d byte at 0x%x", len, start);
	start += st->offset;
	return flash_write(st->dev, (off_t)start, data, len);
}

static int nvmp_flash_erase(const struct nvmp_info *info, size_t start,
			    size_t len)
{
	const struct flash_store *st = (const struct flash_store *)info->store;

	LOG_DBG("write %d byte at 0x%x", len, start);
	start += st->offset;
	return flash_erase(st->dev, (off_t)start, len);
}

static int nvmp_flash_clear(const struct nvmp_info *info, void *data,
			    size_t len)
{
	const struct flash_store *st = (const struct flash_store *)info->store;

	memset(data, st->erase_value, len);
	return 0;
}

static int nvmp_flash_close(const struct nvmp_info *info)
{
	return 0;
}

#include "nvmp_define.h"

#define NVMP_FLASH_GPDEV(inst)                                                  \
	COND_CODE_1(DT_NODE_HAS_COMPAT(inst, soc_nv_flash),                     \
		    (DEVICE_DT_GET(DT_PARENT(inst))), (DEVICE_DT_GET(inst)))
#define NVMP_FLASH_DEV(inst) NVMP_FLASH_GPDEV(DT_GPARENT(inst))
#define NVMP_FLASH_GPERASEVALUE(inst)                                           \
	COND_CODE_1(DT_NODE_HAS_COMPAT(inst, soc_nv_flash),                     \
		    (DT_PROP_OR(DT_PARENT(inst), erase_value, (0xF0))),         \
		    (DT_PROP_OR(inst, erase_value, (0xF0))))
#define NVMP_FLASH_ERASEVALUE(inst) NVMP_FLASH_GPERASEVALUE(DT_GPARENT(inst))
#define NVMP_FLASH_BSIZE(inst)                                                  \
	DT_PROP_OR(inst, block_size,                                            \
		   (DT_PROP_OR(DT_GPARENT(inst), erase_block_size,              \
			       (NVMP_SIZE(inst)))))
#define NVMP_FLASH_WBSIZE(inst)                                                 \
	DT_PROP_OR(inst, write_block_size,                                      \
		   (DT_PROP_OR(DT_GPARENT(inst), write_block_size,              \
			       (NVMP_FLASH_BSIZE(inst)))))

#define NVMP_FLASH_ITEM_DEFINE(inst)                                            \
	BUILD_ASSERT((NVMP_FLASH_ERASEVALUE(inst) == 0xFF) ||                   \
		     (NVMP_FLASH_ERASEVALUE(inst) == 0x00),                     \
		     "Invalid erase value, check dts definition");              \
	const struct flash_store flash_store_##inst = {                         \
		.dev = NVMP_FLASH_DEV(inst),                                    \
		.offset = NVMP_OFF(inst),                                       \
		.erase_value = NVMP_FLASH_ERASEVALUE(inst),			\
	};                                                                      \
	NVMP_INFO_DEFINE(inst, (void *)&flash_store_##inst, NVMP_SIZE(inst),    \
			 NVMP_FLASH_BSIZE(inst), NVMP_FLASH_WBSIZE(inst),       \
			 nvmp_flash_open, nvmp_flash_read,                      \
			 NVMP_PRO(inst) ? NULL : nvmp_flash_write,              \
			 NVMP_PRO(inst) ? NULL : nvmp_flash_erase,              \
			 nvmp_flash_clear, nvmp_flash_close);

#define NVMP_FLASH_DEFINE(inst) DT_FOREACH_CHILD(inst, NVMP_FLASH_ITEM_DEFINE)

DT_FOREACH_STATUS_OKAY(zephyr_flash_nvmp_fixed_partitions, NVMP_FLASH_DEFINE)
