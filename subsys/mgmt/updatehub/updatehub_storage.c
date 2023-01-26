/*
 * Copyright (c) 2023 O.S.Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(updatehub, CONFIG_UPDATEHUB_LOG_LEVEL);

#include <zephyr/dfu/mcuboot.h>

#include "updatehub_storage.h"

int updatehub_storage_is_partition_good(struct updatehub_storage_context *ctx)
{
	bool ret;

	if (ctx == NULL) {
		return -EINVAL;
	}

	ret = boot_is_img_confirmed() == 0;

	return ret ? 0 : -EIO;
}

int updatehub_storage_init(struct updatehub_storage_context *ctx,
			   const uint32_t partition_id)
{
	if (ctx == NULL) {
		return -EINVAL;
	}

	if (boot_erase_img_bank(partition_id)) {
		return -EIO;
	}

	return flash_img_init(&ctx->flash_ctx);
}

int updatehub_storage_write(struct updatehub_storage_context *ctx,
			    const uint8_t *data, const size_t size,
			    const bool flush)
{
	if (ctx == NULL || (size > 0 && data == NULL)) {
		return -EINVAL;
	}

	LOG_DBG("Flash: Address: 0x%08x, Size: %d, Flush: %s",
		ctx->flash_ctx.stream.bytes_written, size,
		flush ? "True" : "False");

	return flash_img_buffered_write(&ctx->flash_ctx, data, size, flush);
}

int updatehub_storage_check(struct updatehub_storage_context *ctx,
			    const uint32_t partition_id,
			    const uint8_t *hash, const size_t size)
{
	if (ctx == NULL || hash == NULL || size == 0) {
		return -EINVAL;
	}

	const struct flash_img_check fic = { .match = hash, .clen = size };

	return flash_img_check(&ctx->flash_ctx, &fic, partition_id);
}

int updatehub_storage_mark_partition_to_upgrade(struct updatehub_storage_context *ctx,
						const uint32_t partition_id)
{
	if (ctx == NULL) {
		return -EINVAL;
	}

	return boot_request_upgrade_multi(partition_id, BOOT_UPGRADE_TEST);
}

int updatehub_storage_mark_partition_as_confirmed(const uint32_t partition_id)
{
	return boot_write_img_confirmed();
}
