/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "usbd_msg.h"

#include <zephyr/kernel.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/class/usbd_dfu.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/dfu/flash_img.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dfu_flash, CONFIG_USBD_DFU_LOG_LEVEL);

/*
 * This file implements the flash backend for the USB DFU implementation. The
 * flash backend can serve up to two image slots, which are typically defined
 * for the in-tree boards in the Zephyr project.
 */

struct usbd_dfu_flash_data {
	struct flash_img_context fi_ctx;
	uint32_t last_block;
	const uint8_t id;
	union {
		uint32_t uploaded;
		uint32_t downloaded;
	};
};

static int dfu_flash_read(void *const priv,
			  const uint32_t block, const uint16_t size,
			  uint8_t buf[static CONFIG_USBD_DFU_TRANSFER_SIZE])
{
	struct usbd_dfu_flash_data *const data = priv;
	const struct flash_area *fa;
	uint32_t to_upload;
	int len;
	int ret;

	if (size == 0) {
		/* There is nothing to upload */
		return 0;
	}

	if (block == 0) {
		data->last_block = 0;
		data->uploaded = 0;
	} else {
		if (data->last_block + 1U != block) {
			return -EINVAL;
		}

	}

	ret = flash_area_open(data->id, &fa);
	if (ret) {
		return ret;
	}

	if (block == 0) {
		LOG_DBG("Flash area size %u", fa->fa_size);
	}

	to_upload = fa->fa_size - data->uploaded;
	if (to_upload < size) {
		len = to_upload;
	} else {
		len = size;
	}

	ret = flash_area_read(fa, data->uploaded, buf, len);
	flash_area_close(fa);
	if (ret) {
		return ret;
	}

	data->last_block = block;
	data->uploaded += size;
	LOG_DBG("uploaded %u block %u len %u", data->uploaded, block, len);

	return len;
}

static int dfu_flash_write(void *const priv,
			   const uint32_t block, const uint16_t size,
			   const uint8_t buf[static CONFIG_USBD_DFU_TRANSFER_SIZE])
{
	struct usbd_dfu_flash_data *const data = priv;
	const bool flush = size == 0 ? true : false;
	int ret;

	if (block == 0) {
		if (flash_img_init(&data->fi_ctx)) {
			return -EINVAL;
		}

		data->last_block = 0;
		data->downloaded = 0;

		if (size == 0) {
			/* There is nothing to download */
			return 0;
		}
	} else {
		if (data->last_block + 1U != block) {
			return -EINVAL;
		}

	}

	ret = flash_img_buffered_write(&data->fi_ctx, buf, size, flush);
	if (ret) {
		return ret;
	}

	data->last_block = block;
	data->downloaded += size;
	LOG_DBG("downloaded %u (%u) block %u size %u", data->downloaded,
		flash_img_bytes_written(&data->fi_ctx), block, size);

	return 0;
}

static bool dfu_flash_next(void *const priv,
			   const enum usb_dfu_state state, const enum usb_dfu_state next)
{
	if (state == DFU_MANIFEST_SYNC && next == DFU_IDLE) {
		LOG_DBG("Download finished");
	}

	return true;
}

#if FIXED_PARTITION_EXISTS(slot0_partition) && defined(CONFIG_USBD_DFU_FLASH_SLOT0)
static struct usbd_dfu_flash_data slot0_data = {
	.id = FIXED_PARTITION_ID(slot0_partition),
};

USBD_DFU_DEFINE_IMG(slot0_image, "slot0_image", &slot0_data,
		    dfu_flash_read, dfu_flash_write, dfu_flash_next);
#endif

#if FIXED_PARTITION_EXISTS(slot1_partition) && defined(CONFIG_USBD_DFU_FLASH_SLOT1)
static struct usbd_dfu_flash_data slot1_data = {
	.id = FIXED_PARTITION_ID(slot1_partition),
};

USBD_DFU_DEFINE_IMG(slot1_image, "slot1_image", &slot1_data,
		    dfu_flash_read, dfu_flash_write, dfu_flash_next);
#endif
