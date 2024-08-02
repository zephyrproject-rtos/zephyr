/*
 * Copyright (c) 2017, 2020 Nordic Semiconductor ASA
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME STREAM_FLASH
#define LOG_LEVEL CONFIG_STREAM_FLASH_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_STREAM_FLASH_LOG_LEVEL);

#include <zephyr/types.h>
#include <string.h>
#include <zephyr/drivers/flash.h>

#include <zephyr/storage/stream_flash.h>

#ifdef CONFIG_STREAM_FLASH_PROGRESS
#include <zephyr/settings/settings.h>

static int settings_direct_loader(const char *key, size_t len,
				  settings_read_cb read_cb, void *cb_arg,
				  void *param)
{
	struct stream_flash_ctx *ctx = (struct stream_flash_ctx *) param;

	/* Handle the subtree if it is an exact key match. */
	if (settings_name_next(key, NULL) == 0) {
		size_t bytes_written = 0;
		ssize_t cb_len = read_cb(cb_arg, &bytes_written,
				      sizeof(bytes_written));

		if (cb_len != sizeof(ctx->bytes_written)) {
			LOG_ERR("Unable to read bytes_written from storage");
			return cb_len;
		}

		/* Check that loaded progress is not outdated. */
		if (bytes_written >= ctx->bytes_written) {
			ctx->bytes_written = bytes_written;
		} else {
			LOG_WRN("Loaded outdated bytes_written %zu < %zu",
				bytes_written, ctx->bytes_written);
			return 0;
		}

#ifdef CONFIG_STREAM_FLASH_ERASE
		int rc;
		struct flash_pages_info page;
		off_t offset = (off_t) (ctx->offset + ctx->bytes_written) - 1;

		/* Update the last erased page to avoid deleting already
		 * written data.
		 */
		if (ctx->bytes_written > 0) {
			rc = flash_get_page_info_by_offs(ctx->fdev, offset,
							 &page);
			if (rc != 0) {
				LOG_ERR("Error %d while getting page info", rc);
				return rc;
			}
			ctx->last_erased_page_start_offset = page.start_offset;
		} else {
			ctx->last_erased_page_start_offset = -1;
		}
#endif /* CONFIG_STREAM_FLASH_ERASE */
	}

	return 0;
}

#endif /* CONFIG_STREAM_FLASH_PROGRESS */

#ifdef CONFIG_STREAM_FLASH_ERASE

int stream_flash_erase_page(struct stream_flash_ctx *ctx, off_t off)
{
#if defined(CONFIG_FLASH_HAS_EXPLICIT_ERASE)
	int rc;
	struct flash_pages_info page;
#if defined(CONFIG_FLASH_HAS_NO_EXPLICIT_ERASE)
	/* There are both types of devices */
	const struct flash_parameters *fparams = flash_get_parameters(ctx->fdev);

	/* Stream flash does not rely on erase, it does it when device needs it */
	if (!(flash_params_get_erase_cap(fparams) & FLASH_ERASE_C_EXPLICIT)) {
		return 0;
	}
#endif
	rc = flash_get_page_info_by_offs(ctx->fdev, off, &page);
	if (rc != 0) {
		LOG_ERR("Error %d while getting page info", rc);
		return rc;
	}

	if (ctx->last_erased_page_start_offset == page.start_offset) {
		return 0;
	}

	LOG_DBG("Erasing page at offset 0x%08lx", (long)page.start_offset);

	rc = flash_erase(ctx->fdev, page.start_offset, page.size);

	if (rc != 0) {
		LOG_ERR("Error %d while erasing page", rc);
	} else {
		ctx->last_erased_page_start_offset = page.start_offset;
	}

	return rc;
#else
	return 0;
#endif
}

#endif /* CONFIG_STREAM_FLASH_ERASE */

static int flash_sync(struct stream_flash_ctx *ctx)
{
	int rc = 0;
	size_t write_addr = ctx->offset + ctx->bytes_written;
	size_t buf_bytes_aligned;
	size_t fill_length;
	uint8_t filler;


	if (ctx->buf_bytes == 0) {
		return 0;
	}

	if (IS_ENABLED(CONFIG_STREAM_FLASH_ERASE)) {

		rc = stream_flash_erase_page(ctx,
					     write_addr + ctx->buf_bytes - 1);
		if (rc < 0) {
			LOG_ERR("stream_flash_erase_page err %d offset=0x%08zx",
				rc, write_addr);
			return rc;
		}
	}

	fill_length = ctx->write_block_size;
	if (ctx->buf_bytes % fill_length) {
		fill_length -= ctx->buf_bytes % fill_length;
		filler = ctx->erase_value;

		memset(ctx->buf + ctx->buf_bytes, filler, fill_length);
	} else {
		fill_length = 0;
	}

	buf_bytes_aligned = ctx->buf_bytes + fill_length;
	rc = flash_write(ctx->fdev, write_addr, ctx->buf, buf_bytes_aligned);

	if (rc != 0) {
		LOG_ERR("flash_write error %d offset=0x%08zx", rc,
			write_addr);
		return rc;
	}

	if (ctx->callback) {
		/* Invert to ensure that caller is able to discover a faulty
		 * flash_read() even if no error code is returned.
		 */
		for (int i = 0; i < ctx->buf_bytes; i++) {
			ctx->buf[i] = ~ctx->buf[i];
		}

		rc = flash_read(ctx->fdev, write_addr, ctx->buf,
				ctx->buf_bytes);
		if (rc != 0) {
			LOG_ERR("flash read failed: %d", rc);
			return rc;
		}

		rc = ctx->callback(ctx->buf, ctx->buf_bytes, write_addr);
		if (rc != 0) {
			LOG_ERR("callback failed: %d", rc);
			return rc;
		}
	}

	ctx->bytes_written += ctx->buf_bytes;
	ctx->buf_bytes = 0U;

	return rc;
}

int stream_flash_buffered_write(struct stream_flash_ctx *ctx, const uint8_t *data,
				size_t len, bool flush)
{
	int processed = 0;
	int rc = 0;
	int buf_empty_bytes;

	if (!ctx) {
		return -EFAULT;
	}

	if (ctx->bytes_written + ctx->buf_bytes + len > ctx->available) {
		return -ENOMEM;
	}

	while ((len - processed) >=
	       (buf_empty_bytes = ctx->buf_len - ctx->buf_bytes)) {
		memcpy(ctx->buf + ctx->buf_bytes, data + processed,
		       buf_empty_bytes);

		ctx->buf_bytes = ctx->buf_len;
		rc = flash_sync(ctx);

		if (rc != 0) {
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

	if (flush && ctx->buf_bytes > 0) {
		rc = flash_sync(ctx);
	}

	return rc;
}

size_t stream_flash_bytes_written(struct stream_flash_ctx *ctx)
{
	return ctx->bytes_written;
}

struct _inspect_flash {
	size_t buf_len;
	size_t total_size;
};

static bool find_flash_total_size(const struct flash_pages_info *info,
				  void *data)
{
	struct _inspect_flash *ctx = (struct _inspect_flash *) data;

	if (ctx->buf_len > info->size) {
		LOG_ERR("Buffer size is bigger than page");
		ctx->total_size = 0;
		return false;
	}

	ctx->total_size += info->size;

	return true;
}

int stream_flash_init(struct stream_flash_ctx *ctx, const struct device *fdev,
		      uint8_t *buf, size_t buf_len, size_t offset, size_t size,
		      stream_flash_callback_t cb)
{
	const struct flash_parameters *params;
	if (!ctx || !fdev || !buf) {
		return -EFAULT;
	}

#ifdef CONFIG_STREAM_FLASH_PROGRESS
	int rc = settings_subsys_init();

	if (rc != 0) {
		LOG_ERR("Error %d initializing settings subsystem", rc);
		return rc;
	}
#endif

	struct _inspect_flash inspect_flash_ctx = {
		.buf_len = buf_len,
		.total_size = 0
	};

	params = flash_get_parameters(fdev);
	ctx->write_block_size = params->write_block_size;

	if (buf_len % ctx->write_block_size) {
		LOG_ERR("Buffer size is not aligned to minimal write-block-size");
		return -EFAULT;
	}

	/* Calculate the total size of the flash device */
	flash_page_foreach(fdev, find_flash_total_size, &inspect_flash_ctx);

	/* The flash size counted should never be equal zero */
	if (inspect_flash_ctx.total_size == 0) {
		return -EFAULT;
	}

	if ((offset + size) > inspect_flash_ctx.total_size ||
	    offset % ctx->write_block_size) {
		LOG_ERR("Incorrect parameter");
		return -EFAULT;
	}


	ctx->fdev = fdev;
	ctx->buf = buf;
	ctx->buf_len = buf_len;
	ctx->bytes_written = 0;
	ctx->buf_bytes = 0U;
	ctx->offset = offset;
	ctx->available = (size == 0 ? inspect_flash_ctx.total_size - offset :
				      size);
	ctx->callback = cb;

#ifdef CONFIG_STREAM_FLASH_ERASE
	ctx->last_erased_page_start_offset = -1;
#endif
	ctx->erase_value = params->erase_value;

	return 0;
}

#ifdef CONFIG_STREAM_FLASH_PROGRESS

int stream_flash_progress_load(struct stream_flash_ctx *ctx,
			       const char *settings_key)
{
	if (!ctx || !settings_key) {
		return -EFAULT;
	}

	int rc = settings_load_subtree_direct(settings_key,
					      settings_direct_loader,
					      (void *) ctx);

	if (rc != 0) {
		LOG_ERR("Error %d while loading progress for \"%s\"",
			rc, settings_key);
	}

	return rc;
}

int stream_flash_progress_save(struct stream_flash_ctx *ctx,
			       const char *settings_key)
{
	if (!ctx || !settings_key) {
		return -EFAULT;
	}

	int rc = settings_save_one(settings_key,
				   &ctx->bytes_written,
				   sizeof(ctx->bytes_written));

	if (rc != 0) {
		LOG_ERR("Error %d while storing progress for \"%s\"",
			rc, settings_key);
	}

	return rc;
}

int stream_flash_progress_clear(struct stream_flash_ctx *ctx,
				const char *settings_key)
{
	if (!ctx || !settings_key) {
		return -EFAULT;
	}

	int rc = settings_delete(settings_key);

	if (rc != 0) {
		LOG_ERR("Error %d while deleting progress for \"%s\"",
			rc, settings_key);
	}

	return rc;
}

#endif  /* CONFIG_STREAM_FLASH_PROGRESS */
