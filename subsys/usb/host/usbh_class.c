/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/usb/usbh.h>
#include <zephyr/logging/log.h>

#include "usbh_class.h"
#include "usbh_class_api.h"

LOG_MODULE_REGISTER(usbh_class, CONFIG_USBH_LOG_LEVEL);

int usbh_register_all_classes(struct usbh_context *uhs_ctx)
{
	int registered_count = 0;

	STRUCT_SECTION_FOREACH(usbh_class_data, cdata_existing) {
		struct usbh_class_data *cdata_registered;
		bool already_registered = false;

		/* Check if already registered */
		SYS_SLIST_FOR_EACH_CONTAINER(&uhs_ctx->class_list, cdata_registered, node) {
			if (cdata_existing == cdata_registered) {
				already_registered = true;
				break;
			}
		}

		if (!already_registered) {
			sys_slist_append(&uhs_ctx->class_list, &cdata_existing->node);
			registered_count++;
			LOG_DBG("Auto-registered class: %s", cdata_existing->name);
		}
	}

	LOG_INF("Auto-registered %d classes to controller %s",
		registered_count, uhs_ctx->name);

	return 0;
}

int usbh_init_registered_classes(struct usbh_context *uhs_ctx)
{
	struct usbh_class_data *cdata;
	int ret;

	SYS_SLIST_FOR_EACH_CONTAINER(&uhs_ctx->class_list, cdata, node) {
		ret = usbh_class_init(cdata);
		if (ret != 0) {
			LOG_ERR("Failed to initialize class instance");
			return ret;
		}
	}

	return 0;
}
