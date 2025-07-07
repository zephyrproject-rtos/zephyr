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

static int handle_ep_op(struct usbd_context *const uds_ctx,
			const enum ep_op op,
			struct usb_ep_descriptor *const ed,
			const uint16_t m_mps,
			uint32_t *const ep_bm)
{
	const uint8_t ep = ed->bEndpointAddress;
	int ret = -ENOTSUP;

	switch (op) {
	case EP_OP_TEST:
		ret = 0;
		break;
	case EP_OP_UP:
		ret = usbd_ep_enable(uds_ctx->dev, ed, m_mps, ep_bm);
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

/*
 * This function is intended to be called from the interface descriptor
 * position, where it can iterate over the endpoint descriptor and any
 * alternate settings. It returns wMaxPacketSize for the largest possible total
 * data payload for a specific endpoint within an interface descriptor.
 */
static uint16_t interface_find_mps(struct usbd_context *const uds_ctx,
				   struct usb_desc_header **const dhp,
				   const uint8_t iface,
				   const uint8_t ep)
{
	struct usb_desc_header **tmp = dhp;
	uint16_t m_mps = 0;

	while (*tmp != NULL && (*tmp)->bLength != 0) {
		struct usb_if_descriptor *ifd;
		struct usb_ep_descriptor *ed;
		uint16_t mps;

		if ((*tmp)->bDescriptorType == USB_DESC_INTERFACE) {
			ifd = (struct usb_if_descriptor *)(*tmp);

			if (ifd->bInterfaceNumber != iface) {
				break;
			}
		}

		if ((*tmp)->bDescriptorType == USB_DESC_ENDPOINT) {
			ed = (struct usb_ep_descriptor *)(*tmp);
			mps = sys_le16_to_cpu(ed->wMaxPacketSize);

			if (ep == ed->bEndpointAddress &&
			    USB_MPS_TO_TPL(mps) > USB_MPS_TO_TPL(m_mps)) {
				m_mps = mps;
			}
		}

		tmp++;
	}


	return m_mps;
}

static int usbd_interface_modify(struct usbd_context *const uds_ctx,
				 struct usbd_class_node *const c_nd,
				 const enum ep_op op,
				 const uint8_t iface,
				 const uint8_t alt)
{
	struct usb_desc_header **dhp;
	bool found_iface = false;
	int ret;

	dhp = usbd_class_get_desc(c_nd->c_data, usbd_bus_speed(uds_ctx));
	if (dhp == NULL) {
		return -EINVAL;
	}

	while (*dhp != NULL && (*dhp)->bLength != 0) {
		struct usb_if_descriptor *ifd;
		struct usb_ep_descriptor *ed;
		uint16_t m_mps = 0;

		if ((*dhp)->bDescriptorType == USB_DESC_INTERFACE) {
			ifd = (struct usb_if_descriptor *)(*dhp);

			if (found_iface) {
				break;
			}

			if (ifd->bInterfaceNumber == iface &&
			    ifd->bAlternateSetting == alt) {
				found_iface = true;
				LOG_DBG("Found interface %u %p", iface, c_nd);
				if (ifd->bNumEndpoints == 0) {
					LOG_INF("No endpoints, skip interface");
					break;
				}
			}
		}

		if ((*dhp)->bDescriptorType == USB_DESC_ENDPOINT && found_iface) {
			ed = (struct usb_ep_descriptor *)(*dhp);

			if (op == EP_OP_UP) {
				m_mps = interface_find_mps(uds_ctx, dhp,
							   iface, ed->bEndpointAddress);
			}

			ret = handle_ep_op(uds_ctx, op, ed, m_mps, &c_nd->ep_active);
			if (ret) {
				return ret;
			}

			LOG_INF("Modify interface %u ep 0x%02x by op %u ep_bm %x",
				iface, ed->bEndpointAddress,
				op, c_nd->ep_active);
		}

		dhp++;
	}

	/* TODO: rollback ep_bm on error? */

	return found_iface ? 0 : -ENODATA;
}

int usbd_interface_shutdown(struct usbd_context *const uds_ctx,
			    struct usbd_config_node *const cfg_nd)
{
	struct usbd_class_node *c_nd;

	SYS_SLIST_FOR_EACH_CONTAINER(&cfg_nd->class_list, c_nd, node) {
		uint32_t *ep_bm = &c_nd->ep_active;

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

int usbd_interface_default(struct usbd_context *const uds_ctx,
			   const enum usbd_speed speed,
			   struct usbd_config_node *const cfg_nd)
{
	struct usb_cfg_descriptor *desc = cfg_nd->desc;
	const uint8_t new_cfg = desc->bConfigurationValue;

	/* Set default alternate for all interfaces */
	for (int i = 0; i < desc->bNumInterfaces; i++) {
		struct usbd_class_node *class;
		int ret;

		class = usbd_class_get_by_config(uds_ctx, speed, new_cfg, i);
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

int usbd_interface_set(struct usbd_context *const uds_ctx,
		       const uint8_t iface,
		       const uint8_t alt)
{
	struct usbd_class_node *class;
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
	/*
	 * Disable/enable endpoints even if the new alternate is the same as
	 * the current one, forcing the endpoint to reset to defaults.
	 */

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

	usbd_class_update(class->c_data, iface, alt);
	usbd_set_alt_value(uds_ctx, iface, alt);

	return 0;
}
