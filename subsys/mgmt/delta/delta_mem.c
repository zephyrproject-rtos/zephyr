/*
 * Copyright (c) 2026 Clovis Corde
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/mgmt/delta/delta.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(delta_mem, CONFIG_DELTA_UPDATE_LOG_LEVEL);

int delta_mem_read_source(struct delta_memory *mem, uint8_t *buf, size_t size)
{
	int ret = 0;

	ret = flash_area_open(PARTITION_ID(slot0_partition), &mem->flash.flash_area);
	if (ret != 0) {
		LOG_ERR("Can not open the flash area for slot 0");
		return -ENODEV;
	}

	ret = flash_area_read(mem->flash.flash_area, mem->offset.source, buf, size);
	if (ret != 0) {
		LOG_ERR("Can not read %zu bytes at offset: %" PRIu64, size,
			mem->offset.source);
		flash_area_close(mem->flash.flash_area);
		return -EINVAL;
	}
	flash_area_close(mem->flash.flash_area);

	return ret;
}

int delta_mem_read_patch(struct delta_memory *mem, uint8_t *buf, size_t size)
{
	int ret = 0;

	if (mem->patch_storage == DELTA_PATCH_STORAGE_FLASH) {
		ret = flash_area_open(PARTITION_ID(patch_partition), &mem->flash.flash_area);
		if (ret != 0) {
			LOG_ERR("Can not open the patch partition");
			return -ENODEV;
		}

		if (mem->offset.patch + size > mem->flash.flash_area->fa_size) {
			LOG_ERR("Read operation exceeds patch partition size");
			flash_area_close(mem->flash.flash_area);
			return -EINVAL;
		}

		ret = flash_area_read(mem->flash.flash_area, mem->offset.patch, buf, size);
		if (ret != 0) {
			LOG_ERR("Can not read %zu bytes at offset: %" PRIu64, size,
				mem->offset.patch);
			flash_area_close(mem->flash.flash_area);
			return -EINVAL;
		}
		flash_area_close(mem->flash.flash_area);
	} else {
		if (mem->offset.patch + size > mem->ram.patch_size) {
			LOG_ERR("Read operation exceeds RAM patch buffer size");
			return -EINVAL;
		}

		memcpy(buf, mem->ram.patch + mem->offset.patch, size);
	}

	return ret;
}

int delta_mem_write(struct delta_memory *mem, uint8_t *buf, size_t size, bool flush)
{
	int ret = 0;

	ret = flash_img_buffered_write(&mem->flash.img_ctx, buf, size, flush);
	if (ret != 0) {
		LOG_ERR("Flash write error");
		return -EIO;
	}

	return ret;
}

int delta_mem_seek(struct delta_memory *mem, size_t source_offset, size_t patch_offset,
		   size_t target_offset)
{
	mem->offset.source = source_offset;
	mem->offset.patch = patch_offset;
	mem->offset.target = target_offset;

	return 0;
}
