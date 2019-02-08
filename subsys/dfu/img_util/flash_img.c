/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME fota_flash_block
#define LOG_LEVEL CONFIG_IMG_MANAGER_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <zephyr/types.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <dfu/flash_img.h>
#include <inttypes.h>

BUILD_ASSERT_MSG((CONFIG_IMG_BLOCK_BUF_SIZE % DT_FLASH_WRITE_BLOCK_SIZE == 0),
		 "CONFIG_IMG_BLOCK_BUF_SIZE is not a multiple of "
		 "DT_FLASH_WRITE_BLOCK_SIZE");

static bool flash_verify(const struct flash_area *fa, off_t offset,
			 u8_t *data, size_t len)
{
	size_t size;
	u32_t temp;
	int rc;

	while (len) {
		size = (len >= sizeof(temp)) ? sizeof(temp) : len;
		rc = flash_area_read(fa, offset, &temp, size);
		if (rc) {
			LOG_ERR("flash_read error %d offset=0x%08"PRIx32,
				rc, (u32_t)offset);
			break;
		}

		if (memcmp(data, &temp, size)) {
			LOG_ERR("offset=0x%08"PRIx32" VERIFY FAIL. "
				"expected: 0x%08x, actual: 0x%08x",
				(u32_t)offset, temp, UNALIGNED_GET(data));
			break;
		}
		len -= size;
		offset += size;
		data += size;
	}

	return (len == 0) ? true : false;
}

static int flash_sync(struct flash_img_context *ctx)
{
	int rc = 0;

	if (ctx->buf_bytes < CONFIG_IMG_BLOCK_BUF_SIZE) {
		(void)memset(ctx->buf + ctx->buf_bytes, 0xFF,
			     CONFIG_IMG_BLOCK_BUF_SIZE - ctx->buf_bytes);
	}

	rc = flash_area_write(ctx->flash_area, ctx->bytes_written, ctx->buf,
			      CONFIG_IMG_BLOCK_BUF_SIZE);
	if (rc) {
		LOG_ERR("flash_write error %d offset=0x%08" PRIx32, rc,
			(u32_t)ctx->bytes_written);
		return rc;
	}

	if (!flash_verify(ctx->flash_area, ctx->bytes_written, ctx->buf,
			  CONFIG_IMG_BLOCK_BUF_SIZE)) {
		return -EIO;
	}

	ctx->bytes_written += ctx->buf_bytes;
	ctx->buf_bytes = 0;

	return rc;
}

int flash_img_buffered_write(struct flash_img_context *ctx, u8_t *data,
			     size_t len, bool flush)
{
	int processed = 0;
	int rc = 0;
	int buf_empty_bytes;

	while ((len - processed) >
	       (buf_empty_bytes = CONFIG_IMG_BLOCK_BUF_SIZE - ctx->buf_bytes)) {
		memcpy(ctx->buf + ctx->buf_bytes, data + processed,
		       buf_empty_bytes);

		ctx->buf_bytes = CONFIG_IMG_BLOCK_BUF_SIZE;
		rc = flash_sync(ctx);

		if (rc) {
			return rc;
		}

		processed += buf_empty_bytes;
	}

	/* place rest of the data into ctx->buf */
	if (processed < len) {
		memcpy(ctx->buf + ctx->buf_bytes,
		       data + processed, len - processed);
		ctx->buf_bytes += len - processed;
	}

	if (!flush) {
		return rc;
	}

	if (ctx->buf_bytes > 0) {
		/* pad the rest of ctx->buf and write it out */
		rc = flash_sync(ctx);

		if (rc) {
			return rc;
		}
	}

	flash_area_close(ctx->flash_area);
	ctx->flash_area = NULL;

	return rc;
}

size_t flash_img_bytes_written(struct flash_img_context *ctx)
{
	return ctx->bytes_written;
}

int flash_img_init(struct flash_img_context *ctx)
{
	ctx->bytes_written = 0;
	ctx->buf_bytes = 0;
	return flash_area_open(DT_FLASH_AREA_IMAGE_1_ID,
			       (const struct flash_area **)&(ctx->flash_area));
}
