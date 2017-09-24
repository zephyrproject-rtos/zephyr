/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_DOMAIN "fota/flash_block"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_IMG_MANAGER_LEVEL
#include <logging/sys_log.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <flash.h>
#include <board.h>
#include <dfu/flash_img.h>

BUILD_ASSERT_MSG((CONFIG_IMG_BLOCK_BUF_SIZE % FLASH_WRITE_BLOCK_SIZE == 0),
		 "CONFIG_IMG_BLOCK_BUF_SIZE is not a multiple of "
		 "FLASH_WRITE_BLOCK_SIZE");

static bool flash_verify(struct device *dev, off_t offset,
			 u8_t *data, size_t len)
{
	size_t size;
	u32_t temp;
	int rc;

	while (len) {
		size = (len >= sizeof(temp)) ? sizeof(temp) : len;
		rc = flash_read(dev, offset, &temp, size);
		if (rc) {
			SYS_LOG_ERR("flash_read error %d offset=0x%08x",
				    rc, offset);
			break;
		}

		if (memcmp(data, &temp, size)) {
			SYS_LOG_ERR("offset=0x%08x VERIFY FAIL. "
				    "expected: 0x%08x, actual: 0x%08x",
				    offset, temp, *(__packed u32_t*)data);
			break;
		}
		len -= size;
		offset += size;
		data += size;
	}

	return (len == 0) ? true : false;
}

/* buffer data into block writes */
static int flash_block_write(struct flash_img_context *ctx, off_t offset,
			     u8_t *data, size_t len, bool finished)
{
	int processed = 0;
	int rc = 0;

	while ((len - processed) >
	       (CONFIG_IMG_BLOCK_BUF_SIZE - ctx->buf_bytes)) {
		memcpy(ctx->buf + ctx->buf_bytes, data + processed,
		       (CONFIG_IMG_BLOCK_BUF_SIZE - ctx->buf_bytes));

		flash_write_protection_set(ctx->dev, false);
		rc = flash_write(ctx->dev, offset + ctx->bytes_written,
				 ctx->buf, CONFIG_IMG_BLOCK_BUF_SIZE);
		flash_write_protection_set(ctx->dev, true);
		if (rc) {
			SYS_LOG_ERR("flash_write error %d offset=0x%08x",
				    rc, offset + ctx->bytes_written);
			return rc;
		}

		if (!flash_verify(ctx->dev, offset + ctx->bytes_written,
				  ctx->buf, CONFIG_IMG_BLOCK_BUF_SIZE)) {
			return -EIO;
		}

		ctx->bytes_written += CONFIG_IMG_BLOCK_BUF_SIZE;
		processed += (CONFIG_IMG_BLOCK_BUF_SIZE - ctx->buf_bytes);
		ctx->buf_bytes = 0;
	}

	/* place rest of the data into ctx->buf */
	if (processed < len) {
		memcpy(ctx->buf + ctx->buf_bytes,
		       data + processed, len - processed);
		ctx->buf_bytes += len - processed;
	}

	if (finished && ctx->buf_bytes > 0) {
		/* pad the rest of ctx->buf and write it out */
		memset(ctx->buf + ctx->buf_bytes, 0xFF,
		       CONFIG_IMG_BLOCK_BUF_SIZE - ctx->buf_bytes);

		flash_write_protection_set(ctx->dev, false);
		rc = flash_write(ctx->dev, offset + ctx->bytes_written,
				 ctx->buf, CONFIG_IMG_BLOCK_BUF_SIZE);
		flash_write_protection_set(ctx->dev, true);
		if (rc) {
			SYS_LOG_ERR("flash_write error %d offset=0x%08x",
				    rc, offset + ctx->bytes_written);
			return rc;
		}

		if (!flash_verify(ctx->dev, offset + ctx->bytes_written,
				  ctx->buf, CONFIG_IMG_BLOCK_BUF_SIZE)) {
			return -EIO;
		}

		ctx->bytes_written = ctx->bytes_written + ctx->buf_bytes;
		ctx->buf_bytes = 0;
	}

	return rc;
}

size_t flash_img_bytes_written(struct flash_img_context *ctx)
{
	return ctx->bytes_written;
}

void flash_img_init(struct flash_img_context *ctx, struct device *dev)
{
	ctx->dev = dev;
	ctx->bytes_written = 0;
	ctx->buf_bytes = 0;
}

int flash_img_buffered_write(struct flash_img_context *ctx, u8_t *data,
			     size_t len, bool flush)
{
	return flash_block_write(ctx, FLASH_AREA_IMAGE_1_OFFSET, data, len,
				 flush);
}
