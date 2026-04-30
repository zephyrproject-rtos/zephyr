/*
 * Copyright (c) 2026 Clovis Corde
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/mgmt/delta/delta.h>
#include <zephyr/mgmt/delta/delta_bsdiff.h>

LOG_MODULE_REGISTER(delta_apply, CONFIG_DELTA_UPDATE_LOG_LEVEL);

/**
 * @brief Initialize the delta context.
 *
 * @param ctx Pointer to the delta context structure.
 * @return 0 on success, negative error code on failure.
 */
static int delta_apply_init(struct delta_ctx *ctx)
{
	int ret;

	if (ctx == NULL) {
		return -EINVAL;
	}

	/* The patch is stored in flash in this example */
	ctx->memory.patch_storage = DELTA_PATCH_STORAGE_FLASH;

	ret = flash_img_init(&ctx->memory.flash.img_ctx);
	if (ret != 0) {
		LOG_ERR("flash_img_init failed, ret: %d", ret);
		return ret;
	}

	/* Set the backend algorithm */
	ctx->backend = &delta_backend_bsdiff_api;

	ret = delta_mem_seek(&ctx->memory,
			     boot_get_image_start_offset(PARTITION_ID(slot0_partition)), 0, 0);
	if (ret != 0) {
		LOG_ERR("delta api seek offset failed, ret = %d", ret);
		return ret;
	}

	return 0;
}

/**
 * @brief Apply the patch using the delta algorithm defined in backend and
 *        write the magic to define the new firmware as a bootable image.
 *
 * @param ctx Pointer to the delta context structure.
 * @return 0 on success, negative error code on failure.
 */
int delta_apply(struct delta_ctx *ctx)
{
	int ret;

	if (ctx == NULL) {
		return -EINVAL;
	}

	ret = delta_apply_init(ctx);
	if (ret != 0) {
		LOG_ERR("The delta API initialization failed, ret = %d", ret);
		return ret;
	}

	/* Validate the header of the delta patch  */
	if (!ctx->backend->validate_header(ctx)) {
		LOG_ERR("Validate header patch failed");
		return -EINVAL;
	}

	/* Apply the patch using a delta algorithm */
	ret = ctx->backend->patch(ctx);
	if (ret != 0) {
		LOG_ERR("Apply patch failed");
		return ret;
	}

	ret = boot_request_upgrade(BOOT_UPGRADE_TEST);
	if (ret != 0) {
		LOG_ERR("Boot request error: %d", ret);
		return ret;
	}

	LOG_INF("The new FW was successfully written, now rebooting...");

	/* Flush the logs before reboot */
	LOG_PANIC();

	sys_reboot(SYS_REBOOT_COLD);

	return ret;
}
