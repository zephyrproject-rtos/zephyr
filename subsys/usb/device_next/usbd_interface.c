/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/usb/udc.h>
#include <zephyr/usb/usbd.h>

#include "usbd_device.h"
#include "usbd_class.h"
#include "usbd_class_api.h"
#include "usbd_endpoint.h"
#include "usbd_ch9.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usbd_iface, CONFIG_USBD_LOG_LEVEL);

enum ep_op {
	EP_OP_TEST, /* Test if interface alternate available */
	EP_OP_UP,   /* Enable endpoint and update endpoints bitmap */
	EP_OP_DOWN, /* Disable endpoint and update endpoints bitmap */
};

static int handle_ep_op(struct usbd_contex *const uds_ctx,
			const enum ep_op op,
			struct usb_ep_descriptor *const ed,
			uint32_t *const ep_bm)
{
	const uint8_t ep = ed->bEndpointAddress;
	int ret;

	switch (op) {
	case EP_OP_TEST:
		ret = 0;
		break;
	case EP_OP_UP:
		ret = usbd_ep_enable(uds_ctx->dev, ed, ep_bm);
		break;
	case EP_OP_DOWN:
		ret = usbd_ep_disable(uds_ctx->dev, ep, ep_bm);
		break;
	}

	if (ret) {
		LOG_ERR("Failed to handle op %d, ep 0x%02x, bm 0x%08x, %d",
			op, ep, *ep_bm, ret);
	}

	return ret;
}

static int usbd_interface_modify(struct usbd_contex *const uds_ctx,
				 struct usbd_class_iter *const iter,
				 const enum ep_op op,
				 const uint8_t iface,
				 const uint8_t alt)
{
	struct usb_desc_header **dhp;
	bool found_iface = false;
	int ret;

	dhp = usbd_class_get_desc(iter->c_nd, usbd_bus_speed(uds_ctx));
	if (dhp == NULL) {
		return -EINVAL;
	}

	while (*dhp != NULL && (*dhp)->bLength != 0) {
		struct usb_if_descriptor *ifd;
		struct usb_ep_descriptor *ed;

		if ((*dhp)->bDescriptorType == USB_DESC_INTERFACE) {
			ifd = (struct usb_if_descriptor *)(*dhp);

			if (found_iface) {
				break;
			}

			if (ifd->bInterfaceNumber == iface &&
			    ifd->bAlternateSetting == alt) {
				found_iface = true;
				LOG_DBG("Found interface %u %p", iface, iter);
				if (ifd->bNumEndpoints == 0) {
					LOG_INF("No endpoints, skip interface");
					break;
				}
			}
		}

		if ((*dhp)->bDescriptorType == USB_DESC_ENDPOINT && found_iface) {
			ed = (struct usb_ep_descriptor *)(*dhp);
			ret = handle_ep_op(uds_ctx, op, ed, &iter->ep_active);
			if (ret) {
				return ret;
			}

			LOG_INF("Modify interface %u ep 0x%02x by op %u ep_bm %x",
				iface, ed->bEndpointAddress,
				op, iter->ep_active);
		}

		dhp++;
	}

	/* TODO: rollback ep_bm on error? */

	return found_iface ? 0 : -ENODATA;
}

int usbd_interface_shutdown(struct usbd_contex *const uds_ctx,
			    struct usbd_config_node *const cfg_nd)
{
	struct usbd_class_iter *iter;

	SYS_SLIST_FOR_EACH_CONTAINER(&cfg_nd->class_list, iter, node) {
		uint32_t *ep_bm = &iter->ep_active;

		for (int idx = 1; idx < 16 && *ep_bm; idx++) {
			uint8_t ep_in = USB_EP_DIR_IN | idx;
			uint8_t ep_out = idx;
			int ret;

			if (usbd_ep_bm_is_set(ep_bm, ep_in)) {
				ret = usbd_ep_disable(uds_ctx->dev, ep_in, ep_bm);
				if (ret) {
					return ret;
				}
			}

			if (usbd_ep_bm_is_set(ep_bm, ep_out)) {
				ret = usbd_ep_disable(uds_ctx->dev, ep_out, ep_bm);
				if (ret) {
					return ret;
				}
			}
		}
	}

	return 0;
}

int usbd_interface_default(struct usbd_contex *const uds_ctx,
			   struct usbd_config_node *const cfg_nd)
{
	struct usb_cfg_descriptor *desc = cfg_nd->desc;
	const uint8_t new_cfg = desc->bConfigurationValue;

	/* Set default alternate for all interfaces */
	for (int i = 0; i < desc->bNumInterfaces; i++) {
		struct usbd_class_iter *class;
		int ret;

		class = usbd_class_get_by_config(uds_ctx, new_cfg, i);
		if (class == NULL) {
			return -ENODATA;
		}

		ret = usbd_interface_modify(uds_ctx, class, EP_OP_UP, i, 0);
		if (ret) {
			return ret;
		}
	}

	return 0;
}

int usbd_interface_set(struct usbd_contex *const uds_ctx,
		       const uint8_t iface,
		       const uint8_t alt)
{
	struct usbd_class_iter *class;
	uint8_t cur_alt;
	int ret;

	class = usbd_class_get_by_iface(uds_ctx, iface);
	if (class == NULL) {
		return -ENOENT;
	}

	ret = usbd_get_alt_value(uds_ctx, iface, &cur_alt);
	if (ret) {
		return ret;
	}

	LOG_INF("Set Interfaces %u, alternate %u -> %u", iface, cur_alt, alt);
	if (alt == cur_alt) {
		return 0;
	}

	/* Test if interface or interface alternate exist */
	ret = usbd_interface_modify(uds_ctx, class, EP_OP_TEST, iface, alt);
	if (ret) {
		return -ENOENT;
	}

	/* Shutdown current interface alternate */
	ret = usbd_interface_modify(uds_ctx, class, EP_OP_DOWN, iface, cur_alt);
	if (ret) {
		return ret;
	}

	/* Setup new interface alternate */
	ret = usbd_interface_modify(uds_ctx, class, EP_OP_UP, iface, alt);
	if (ret) {
		/* TODO: rollback on error? */
		return ret;
	}

	usbd_class_update(class->c_nd, iface, alt);
	usbd_set_alt_value(uds_ctx, iface, alt);

	return 0;
}
