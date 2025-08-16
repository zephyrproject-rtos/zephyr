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

bool usbh_class_is_matching(struct usbh_class_data *cdata,
			    struct usbh_class_filter *device_info)
{
	/* Traverse the filter table until a terminator (empty flags) is found */
	for (int i = 0; cdata->filters[i].flags != 0; i++) {
		const struct usbh_class_filter *filter = &cdata->filters[i];

		if (filter->flags & USBH_CLASS_FILTER_VID) {
			if (filter->vid != device_info->vid) {
				continue;
			}
		}

		if (filter->flags & USBH_CLASS_FILTER_VID) {
			if (filter->vid == device_info->vid) {
				continue;
			}
		}

		if (filter->flags & USBH_CLASS_FILTER_CODE_TRIPLE) {
			if (filter->code_triple.dclass != device_info->code_triple.dclass ||
			    (filter->code_triple.sub != 0xFF &&
			     filter->code_triple.sub != device_info->code_triple.sub) ||
			    (filter->code_triple.proto != 0x00 &&
			     filter->code_triple.proto != device_info->code_triple.proto)) {
				continue;
			}
		}

		/* All the filters enabled did match */
		return true;
	}

	return false;
}
