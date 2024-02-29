/*
 * Copyright (c) 2021-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/slist.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/usb/usbd.h>

#include "usbd_device.h"
#include "usbd_config.h"
#include "usbd_class.h"
#include "usbd_class_api.h"
#include "usbd_endpoint.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usbd_init, CONFIG_USBD_LOG_LEVEL);

/* TODO: Allow to disable automatic assignment of endpoint features */

/* Assign endpoint address and update wMaxPacketSize */
static int assign_ep_addr(const struct device *dev,
			  struct usb_ep_descriptor *const ed,
			  uint32_t *const config_ep_bm,
			  uint32_t *const class_ep_bm)
{
	int ret = -ENODEV;

	for (unsigned int idx = 1; idx < 16U; idx++) {
		uint16_t mps = ed->wMaxPacketSize;
		uint8_t ep;

		if (USB_EP_DIR_IS_IN(ed->bEndpointAddress)) {
			ep = USB_EP_DIR_IN | idx;
		} else {
			ep = idx;
		}

		if (usbd_ep_bm_is_set(config_ep_bm, ep) ||
		    usbd_ep_bm_is_set(class_ep_bm, ep)) {
			continue;
		}


		ret = udc_ep_try_config(dev, ep,
					ed->bmAttributes, &mps,
					ed->bInterval);

		if (ret == 0) {
			LOG_DBG("ep 0x%02x -> 0x%02x", ed->bEndpointAddress, ep);
			ed->bEndpointAddress = ep;
			ed->wMaxPacketSize = mps;
			usbd_ep_bm_set(class_ep_bm, ed->bEndpointAddress);
			usbd_ep_bm_set(config_ep_bm, ed->bEndpointAddress);

			return 0;
		}
	}

	return ret;
}

/* Unassign all endpoint of a class instance based on class_ep_bm */
static int unassign_eps(struct usbd_contex *const uds_ctx,
			uint32_t *const config_ep_bm,
			uint32_t *const class_ep_bm)
{
	for (unsigned int idx = 1; idx < 16U && *class_ep_bm; idx++) {
		uint8_t ep_in = USB_EP_DIR_IN | idx;
		uint8_t ep_out = idx;

		if (usbd_ep_bm_is_set(class_ep_bm, ep_in)) {
			if (!usbd_ep_bm_is_set(config_ep_bm, ep_in)) {
				LOG_ERR("Endpoing 0x%02x not assigned", ep_in);
				return -EINVAL;
			}

			usbd_ep_bm_clear(config_ep_bm, ep_in);
			usbd_ep_bm_clear(class_ep_bm, ep_in);
		}

		if (usbd_ep_bm_is_set(class_ep_bm, ep_out)) {
			if (!usbd_ep_bm_is_set(config_ep_bm, ep_out)) {
				LOG_ERR("Endpoing 0x%02x not assigned", ep_out);
				return -EINVAL;
			}

			usbd_ep_bm_clear(config_ep_bm, ep_out);
			usbd_ep_bm_clear(class_ep_bm, ep_out);
		}
	}

	return 0;
}

/*
 * Configure all interfaces and endpoints of a class instance
 *
 * The total number of interfaces is stored in the configuration descriptor's
 * value bNumInterfaces. This value is reset at the beginning of configuration
 * initialization and is increased according to the number of interfaces.
 * The respective bInterfaceNumber must be assigned to all interfaces
 * of a class instance.
 *
 * Like bInterfaceNumber the endpoint addresses must be assigned
 * for all registered instances and respective endpoint descriptors.
 * We use config_ep_bm variable as map for assigned endpoint for an
 * USB device configuration.
 */
static int init_configuration_inst(struct usbd_contex *const uds_ctx,
				   const enum usbd_speed speed,
				   struct usbd_class_iter *const iter,
				   uint32_t *const config_ep_bm,
				   uint8_t *const nif)
{
	struct usb_desc_header **dhp;
	struct usb_if_descriptor *ifd = NULL;
	struct usb_ep_descriptor *ed;
	uint32_t class_ep_bm = 0;
	uint8_t tmp_nif;
	int ret;

	LOG_DBG("Initializing configuration for %u speed", speed);
	dhp = usbd_class_get_desc(iter->c_nd, speed);
	if (dhp == NULL) {
		return 0;
	}

	tmp_nif = *nif;
	iter->iface_bm = 0U;
	iter->ep_active = 0U;

	while (*dhp != NULL && (*dhp)->bLength != 0) {

		if ((*dhp)->bDescriptorType == USB_DESC_INTERFACE) {
			ifd = (struct usb_if_descriptor *)(*dhp);

			iter->ep_active |= class_ep_bm;

			if (ifd->bAlternateSetting == 0) {
				ifd->bInterfaceNumber = tmp_nif;
				iter->iface_bm |= BIT(tmp_nif);
				tmp_nif++;
			} else {
				ifd->bInterfaceNumber = tmp_nif - 1;
				/*
				 * Unassign endpoints from last alternate,
				 * to work properly it requires that the
				 * characteristics of endpoints in alternate
				 * interfaces are ascending.
				 */
				unassign_eps(uds_ctx, config_ep_bm, &class_ep_bm);
			}

			class_ep_bm = 0;
			LOG_INF("interface %u alternate %u",
				ifd->bInterfaceNumber, ifd->bAlternateSetting);
		}

		if ((*dhp)->bDescriptorType == USB_DESC_ENDPOINT) {
			ed = (struct usb_ep_descriptor *)(*dhp);
			ret = assign_ep_addr(uds_ctx->dev, ed,
					     config_ep_bm, &class_ep_bm);
			if (ret) {
				return ret;
			}

			LOG_INF("\tep 0x%02x mps %u interface ep-bm 0x%08x",
				ed->bEndpointAddress, ed->wMaxPacketSize, class_ep_bm);
		}

		dhp++;
	}

	if (tmp_nif <= *nif) {
		return -EINVAL;
	}

	*nif = tmp_nif;
	iter->ep_active |= class_ep_bm;

	LOG_INF("Instance iface-bm 0x%08x ep-bm 0x%08x",
		iter->iface_bm, iter->ep_active);

	return 0;
}

/*
 * Initialize a device configuration
 *
 * Iterate on a list of all classes in a configuration
 */
static int init_configuration(struct usbd_contex *const uds_ctx,
			      const enum usbd_speed speed,
			      struct usbd_config_node *const cfg_nd)
{
	struct usb_cfg_descriptor *cfg_desc = cfg_nd->desc;
	struct usbd_class_iter *iter;
	uint32_t config_ep_bm = 0;
	size_t cfg_len = 0;
	uint8_t nif = 0;
	int ret;

	SYS_SLIST_FOR_EACH_CONTAINER(&cfg_nd->class_list, iter, node) {

		ret = init_configuration_inst(uds_ctx, speed, iter,
					      &config_ep_bm, &nif);
		if (ret != 0) {
			LOG_ERR("Failed to assign endpoint addresses");
			return ret;
		}

		ret = usbd_class_init(iter->c_nd);
		if (ret != 0) {
			LOG_ERR("Failed to initialize class instance");
			return ret;
		}

		LOG_INF("Init class node %p, descriptor length %zu",
			iter->c_nd, usbd_class_desc_len(iter->c_nd, speed));
		cfg_len += usbd_class_desc_len(iter->c_nd, speed);
	}

	/* Update wTotalLength and bNumInterfaces of configuration descriptor */
	sys_put_le16(sizeof(struct usb_cfg_descriptor) + cfg_len,
		     (uint8_t *)&cfg_desc->wTotalLength);
	cfg_desc->bNumInterfaces = nif;

	LOG_INF("bNumInterfaces %u wTotalLength %u",
		cfg_desc->bNumInterfaces,
		cfg_desc->wTotalLength);

	/* Finally reset configuration's endpoint assignment */
	SYS_SLIST_FOR_EACH_CONTAINER(&cfg_nd->class_list, iter, node) {
		iter->ep_assigned = iter->ep_active;
		ret = unassign_eps(uds_ctx, &config_ep_bm, &iter->ep_active);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

static void usbd_init_update_fs_mps0(struct usbd_contex *const uds_ctx)
{
	struct udc_device_caps caps = udc_caps(uds_ctx->dev);
	struct usb_device_descriptor *desc = uds_ctx->fs_desc;

	switch (caps.mps0) {
	case UDC_MPS0_8:
		desc->bMaxPacketSize0 = 8;
		break;
	case UDC_MPS0_16:
		desc->bMaxPacketSize0 = 16;
		break;
	case UDC_MPS0_32:
		desc->bMaxPacketSize0 = 32;
		break;
	case UDC_MPS0_64:
		desc->bMaxPacketSize0 = 64;
		break;
	}
}

int usbd_init_configurations(struct usbd_contex *const uds_ctx)
{
	struct usbd_config_node *cfg_nd;

	usbd_init_update_fs_mps0(uds_ctx);

	SYS_SLIST_FOR_EACH_CONTAINER(&uds_ctx->hs_configs, cfg_nd, node) {
		int ret;

		ret = init_configuration(uds_ctx, USBD_SPEED_HS, cfg_nd);
		if (ret) {
			LOG_ERR("Failed to init HS configuration %u",
				usbd_config_get_value(cfg_nd));
			return ret;
		}

		LOG_INF("HS bNumConfigurations %u",
			usbd_get_num_configs(uds_ctx, USBD_SPEED_HS));
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&uds_ctx->fs_configs, cfg_nd, node) {
		int ret;

		ret = init_configuration(uds_ctx, USBD_SPEED_FS, cfg_nd);
		if (ret) {
			LOG_ERR("Failed to init FS configuration %u",
				usbd_config_get_value(cfg_nd));
			return ret;
		}

		LOG_INF("FS bNumConfigurations %u",
			usbd_get_num_configs(uds_ctx, USBD_SPEED_FS));
	}

	return 0;
}
