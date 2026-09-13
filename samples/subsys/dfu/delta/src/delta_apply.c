/*
 * Copyright (c) 2026 Clovis Corde
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/delta/delta.h>
#include <zephyr/delta/backend_bsdiffhs.h>
#include <zephyr/dfu/flash_img.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/sys/util.h>

#include "delta_apply.h"

LOG_MODULE_REGISTER(delta_apply, LOG_LEVEL_INF);

/* Glue between lib/delta and the MCUboot flash layout. */
struct delta_ctx {
	const struct flash_area *source_area;
	const struct flash_area *patch_area;
	struct flash_img_context flash_img;
};

/* Patch read-ahead window handed to the backend. */
#define WORK_BUF_SIZE 128

static uint8_t work_buf[WORK_BUF_SIZE];

static int read_source(void *vctx, size_t off, void *buf, size_t len)
{
	struct delta_ctx *ctx = vctx;

	return flash_area_read(ctx->source_area, (off_t)off, buf, len);
}

static int read_patch(void *vctx, size_t off, void *buf, size_t len)
{
	struct delta_ctx *ctx = vctx;

	return flash_area_read(ctx->patch_area, (off_t)off, buf, len);
}

static int write_target(void *vctx, size_t off, const void *buf, size_t len)
{
	struct delta_ctx *ctx = vctx;

	ARG_UNUSED(off);
	return flash_img_buffered_write(&ctx->flash_img, buf, len, false);
}

static int finalize_target(void *vctx)
{
	struct delta_ctx *ctx = vctx;

	/* Flush the last partially filled block to the upload slot. */
	return flash_img_buffered_write(&ctx->flash_img, NULL, 0, true);
}

int delta_apply_and_reboot(void)
{
	struct delta_ctx ctx;
	int ret;

	ret = flash_area_open(PARTITION_ID(slot0_partition), &ctx.source_area);
	if (ret != 0) {
		LOG_ERR("flash_area_open(slot0) failed: %d", ret);
		return ret;
	}

	ret = flash_area_open(PARTITION_ID(patch_partition), &ctx.patch_area);
	if (ret != 0) {
		LOG_ERR("flash_area_open(patch) failed: %d", ret);
		flash_area_close(ctx.source_area);
		return ret;
	}

	ret = flash_img_init(&ctx.flash_img);
	if (ret != 0) {
		LOG_ERR("flash_img_init failed: %d", ret);
		flash_area_close(ctx.patch_area);
		flash_area_close(ctx.source_area);
		return ret;
	}

	struct delta_io io = {
		.read_source = read_source,
		.read_patch = read_patch,
		.write_target = write_target,
		.finalize = finalize_target,
		.ctx = &ctx,
	};
	struct delta_config cfg = {
		.work_buf = work_buf,
		.work_sz = sizeof(work_buf),
		.source_size = ctx.source_area->fa_size,
		.patch_size = ctx.patch_area->fa_size,
		.max_target_size = ctx.flash_img.flash_area->fa_size,
	};

	ret = delta_apply_patch(&io, &delta_backend_bsdiffhs, &cfg);

	flash_area_close(ctx.patch_area);
	flash_area_close(ctx.source_area);
	if (ret != 0) {
		LOG_ERR("delta apply failed: %d", ret);
		return ret;
	}

	ret = boot_request_upgrade(BOOT_UPGRADE_TEST);
	if (ret != 0) {
		LOG_ERR("boot_request_upgrade failed: %d", ret);
		return ret;
	}

	LOG_INF("New firmware written, rebooting...");
	LOG_PANIC();
	sys_reboot(SYS_REBOOT_COLD);

	return 0;
}
