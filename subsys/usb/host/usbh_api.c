/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/sys/dlist.h>
#include <zephyr/sys/util.h>
#include "usbh_host.h"
#include "usbh_internal.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uhs_api, CONFIG_USBH_LOG_LEVEL);

int usbh_init(struct usbh_context *uhs_ctx)
{
	int ret;

	usbh_host_lock(uhs_ctx);

	if (!device_is_ready(uhs_ctx->dev)) {
		LOG_ERR("USB host controller is not ready");
		ret = -ENODEV;
		goto init_exit;
	}

	if (uhc_is_initialized(uhs_ctx->dev)) {
		LOG_WRN("USB host controller is already initialized");
		/*
		 * usbh_init_device_intl() was skipped, so sys_dlist_init() may never
		 * have run. An all-zero sys_dlist_t breaks sys_dlist_peek_head() /
		 * usbh_device_get_any() (head NULL, is_empty false, peek returns NULL).
		 * Re-init only when the list is clearly uninitialized.
		 */
		if (uhs_ctx->udevs.head == NULL) {
			sys_dlist_init(&uhs_ctx->udevs);
		}
		ret = -EALREADY;
		goto init_exit;
	}

	ret = usbh_init_device_intl(uhs_ctx);

init_exit:
	usbh_host_unlock(uhs_ctx);
	return ret;
}

int usbh_enable(struct usbh_context *uhs_ctx)
{
	int ret;

	usbh_host_lock(uhs_ctx);

	if (!uhc_is_initialized(uhs_ctx->dev)) {
		LOG_WRN("USB host controller is not initialized");
		ret = -EPERM;
		goto enable_exit;
	}

	if (uhc_is_enabled(uhs_ctx->dev)) {
		LOG_WRN("USB host controller is already enabled");
		ret = -EALREADY;
		goto enable_exit;
	}

	ret = uhc_enable(uhs_ctx->dev);
	if (ret != 0) {
		LOG_ERR("Failed to enable controller");
		goto enable_exit;
	}

enable_exit:
	usbh_host_unlock(uhs_ctx);
	return ret;
}

int usbh_disable(struct usbh_context *uhs_ctx)
{
	int ret;

	if (!uhc_is_enabled(uhs_ctx->dev)) {
		LOG_WRN("USB host controller is already disabled");
		return 0;
	}

	usbh_host_lock(uhs_ctx);

	ret = uhc_disable(uhs_ctx->dev);
	if (ret) {
		LOG_ERR("Failed to disable USB controller");
	}

	usbh_host_unlock(uhs_ctx);

	return ret;
}

int usbh_shutdown(struct usbh_context *const uhs_ctx)
{
	int ret;

	usbh_host_lock(uhs_ctx);

	ret = uhc_shutdown(uhs_ctx->dev);
	if (ret) {
		LOG_ERR("Failed to shutdown USB device");
	}

	usbh_host_unlock(uhs_ctx);

	return ret;
}
