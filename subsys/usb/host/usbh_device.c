/*
 * Copyright (c) 2023,2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/usb/usbh.h>
#include <zephyr/sys/byteorder.h>

#include "usbh_device.h"
#include "usbh_ch9.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usbh_dev, CONFIG_USBH_LOG_LEVEL);

K_MEM_SLAB_DEFINE_STATIC(usb_device_slab, sizeof(struct usb_device),
			 CONFIG_USBH_USB_DEVICE_MAX, sizeof(void *));

K_HEAP_DEFINE(usb_device_heap, CONFIG_USBH_USB_DEVICE_HEAP);

struct usb_device *usbh_device_alloc(struct usbh_contex *const uhs_ctx)
{
	struct usb_device *udev;

	if (k_mem_slab_alloc(&usb_device_slab, (void **)&udev, K_NO_WAIT)) {
		LOG_ERR("Failed to allocate USB device memory");
		return NULL;
	}

	memset(udev, 0, sizeof(struct usb_device));
	udev->ctx = uhs_ctx;
	sys_dlist_append(&uhs_ctx->udevs, &udev->node);
	k_mutex_init(&udev->mutex);

	return udev;
}

void usbh_device_free(struct usb_device *const udev)
{
	struct usbh_contex *const uhs_ctx = udev->ctx;

	sys_bitarray_clear_bit(uhs_ctx->addr_ba, udev->addr);
	sys_dlist_remove(&udev->node);
	if (udev->cfg_desc != NULL) {
		k_heap_free(&usb_device_heap, udev->cfg_desc);
	}

	k_mem_slab_free(&usb_device_slab, (void *)udev);
}

struct usb_device *usbh_device_get_any(struct usbh_contex *const uhs_ctx)
{
	sys_dnode_t *const node = sys_dlist_peek_head(&uhs_ctx->udevs);
	struct usb_device *udev;

	udev = SYS_DLIST_CONTAINER(node, udev, node);

	return udev;
}

static int validate_device_mps0(const struct usb_device *const udev)
{
	const uint8_t mps0 = udev->dev_desc.bMaxPacketSize0;

	if (udev->speed == USB_SPEED_SPEED_SS || udev->speed == USB_SPEED_SPEED_LS) {
		LOG_ERR("USB device speed not supported");
		return -ENOTSUP;
	}

	if (udev->speed == USB_SPEED_SPEED_HS) {
		if (mps0 != 64) {
			LOG_ERR("HS device has wrong bMaxPacketSize0 %u", mps0);
			return -EINVAL;
		}
	}

	if (udev->speed == USB_SPEED_SPEED_FS) {
		if (mps0 != 8 && mps0 != 16 && mps0 != 32 && mps0 != 64) {
			LOG_ERR("FS device has wrong bMaxPacketSize0 %u", mps0);
			return -EINVAL;
		}
	}

	return 0;
}

static int alloc_device_address(struct usb_device *const udev, uint8_t *const addr)
{
	struct usbh_contex *const uhs_ctx = udev->ctx;
	int val;
	int err;

	for (unsigned int i = 1; i < 128; i++) {
		err = sys_bitarray_test_and_set_bit(uhs_ctx->addr_ba, i, &val);
		if (err) {
			return err;
		}

		if (val == 0) {
			*addr = i;
			return 0;
		}
	}

	return -ENOENT;
}

enum ep_op {
	EP_OP_TEST, /* Verify endpont descriptor */
	EP_OP_UP,   /* Enable endpoint and update endpoint pointers */
	EP_OP_DOWN, /* Disable endpoint and update endpoint pointers */
};

static void assign_ep_desc_ptr(struct usb_device *const udev,
			       const uint8_t ep, void *const ptr)
{
	uint8_t idx = USB_EP_GET_IDX(ep) & 0xF;

	if (USB_EP_DIR_IS_IN(ep)) {
		udev->ep_in[idx].desc = ptr;
	} else {
		udev->ep_out[idx].desc = ptr;
	}
}

static int handle_ep_op(struct usb_device *const udev,
			const enum ep_op op, const uint8_t ep,
			struct usb_ep_descriptor *const ep_desc)
{
	switch (op) {
	case EP_OP_TEST:
		break;
	case EP_OP_UP:
		if (ep_desc == NULL) {
			return -ENOTSUP;
		}

		assign_ep_desc_ptr(udev, ep_desc->bEndpointAddress, ep_desc);
		break;
	case EP_OP_DOWN:
		assign_ep_desc_ptr(udev, ep, NULL);
		break;
	}

	return 0;
}

static int device_interface_modify(struct usb_device *const udev,
				   const enum ep_op op,
				   const uint8_t iface, const uint8_t alt)
{
	struct usb_cfg_descriptor *cfg_desc = udev->cfg_desc;
	struct usb_if_descriptor *if_desc = NULL;
	struct usb_ep_descriptor *ep_desc;
	struct usb_desc_header *dhp;
	bool found_iface = false;
	void *desc_end;
	int err;

	dhp = udev->ifaces[iface].dhp;
	desc_end = (void *)((uint8_t *)udev->cfg_desc + cfg_desc->wTotalLength);

	while (dhp != NULL && (void *)dhp < desc_end) {
		if (dhp->bDescriptorType == USB_DESC_INTERFACE) {
			if_desc = (struct usb_if_descriptor *)dhp;

			if (found_iface) {
				break;
			}

			if (if_desc->bInterfaceNumber == iface &&
			    if_desc->bAlternateSetting == alt) {
				found_iface = true;
				LOG_DBG("Found interface %u alternate %u", iface, alt);
				if (if_desc->bNumEndpoints == 0) {
					LOG_DBG("No endpoints, skip interface");
					break;
				}
			}
		}

		if (dhp->bDescriptorType == USB_DESC_ENDPOINT && found_iface) {
			ep_desc = (struct usb_ep_descriptor *)dhp;
			err = handle_ep_op(udev, op, ep_desc->bEndpointAddress, ep_desc);
			if (err) {
				return err;
			}

			LOG_INF("Modify interface %u ep 0x%02x by op %u",
				iface, ep_desc->bEndpointAddress, op);
		}

		dhp = (void *)((uint8_t *)dhp + dhp->bLength);
	}


	return found_iface ? 0 : -ENODATA;
}

int usbh_device_interface_set(struct usb_device *const udev,
			      const uint8_t iface, const uint8_t alt,
			      const bool dry)
{
	uint8_t cur_alt;
	int err;

	if (iface > UHC_INTERFACES_MAX) {
		LOG_ERR("Unsupported number of interfaces");
		return -EINVAL;
	}

	err = k_mutex_lock(&udev->mutex, K_NO_WAIT);
	if (err) {
		LOG_ERR("Failed to lock USB device");
		return err;
	}

	if (!dry) {
		err = usbh_req_set_alt(udev, iface, alt);
		if (err) {
			LOG_ERR("Set Interface %u alternate %u request failed", iface, alt);
			goto error;
		}
	}

	cur_alt = udev->ifaces[iface].alternate;
	LOG_INF("Set Interfaces %u, alternate %u -> %u", iface, cur_alt, alt);
	if (alt == cur_alt) {
		LOG_DBG("Already active interface alternate");
		goto error;
	}

	/* Test if interface and interface alternate exist */
	err = device_interface_modify(udev, EP_OP_TEST, iface, alt);
	if (err) {
		LOG_ERR("No interface %u with alternate %u", iface, alt);
		goto error;
	}

	/* Shutdown current interface alternate */
	err = device_interface_modify(udev, EP_OP_DOWN, iface, cur_alt);
	if (err) {
		LOG_ERR("Failed to shutdown interface %u alternate %u", iface, alt);
		goto error;
	}

	/* Setup new interface alternate */
	err = device_interface_modify(udev, EP_OP_UP, iface, alt);
	if (err) {
		LOG_ERR("Failed to setup interface %u alternate %u", iface, cur_alt);
		goto error;
	}

	udev->ifaces[iface].alternate = alt;

error:
	k_mutex_unlock(&udev->mutex);

	return 0;
}

static int parse_configuration_descriptor(struct usb_device *const udev)
{
	struct usb_cfg_descriptor *cfg_desc = udev->cfg_desc;
	struct usb_association_descriptor *iad = NULL;
	struct usb_if_descriptor *if_desc = NULL;
	struct usb_ep_descriptor *ep_desc;
	struct usb_desc_header *dhp;
	uint8_t tmp_nif = 0;
	void *desc_end;

	dhp = (void *)((uint8_t *)udev->cfg_desc + cfg_desc->bLength);
	desc_end = (void *)((uint8_t *)udev->cfg_desc + cfg_desc->wTotalLength);

	while ((dhp->bDescriptorType != 0 || dhp->bLength != 0) && (void *)dhp < desc_end) {
		if (dhp->bDescriptorType == USB_DESC_INTERFACE_ASSOC) {
			iad = (struct usb_association_descriptor *)dhp;
			LOG_DBG("bFirstInterface %u", iad->bFirstInterface);
		}

		if (dhp->bDescriptorType == USB_DESC_INTERFACE) {
			if_desc = (struct usb_if_descriptor *)dhp;
			LOG_DBG("bInterfaceNumber %u bAlternateSetting %u",
				if_desc->bInterfaceNumber, if_desc->bAlternateSetting);

			if (if_desc->bAlternateSetting == 0) {
				if (tmp_nif >= UHC_INTERFACES_MAX) {
					LOG_ERR("Unsupported number of interfaces");
					return -EINVAL;
				}

				udev->ifaces[tmp_nif].dhp = dhp;
				tmp_nif++;
			}
		}

		if (dhp->bDescriptorType == USB_DESC_ENDPOINT) {
			ep_desc = (struct usb_ep_descriptor *)dhp;

			ep_desc->wMaxPacketSize = sys_le16_to_cpu(ep_desc->wMaxPacketSize);
			LOG_DBG("bEndpointAddress 0x%02x wMaxPacketSize %u",
				ep_desc->bEndpointAddress, ep_desc->wMaxPacketSize);

			if (if_desc != NULL && if_desc->bAlternateSetting == 0) {
				assign_ep_desc_ptr(udev, ep_desc->bEndpointAddress, ep_desc);
			}
		}

		dhp = (void *)((uint8_t *)dhp + dhp->bLength);
	}

	if (cfg_desc->bNumInterfaces != tmp_nif) {
		LOG_ERR("The configuration has an incorrect number of interfaces");
		return -EINVAL;
	}

	return 0;
}

static void reset_configuration(struct usb_device *const udev)
{
	/* Reset all endpoint pointers */
	memset(udev->ep_in, 0, sizeof(udev->ep_in));
	memset(udev->ep_out, 0, sizeof(udev->ep_out));

	/* Reset all interface pointers */
	memset(udev->ifaces, 0, sizeof(udev->ifaces));

	udev->actual_cfg = 0;
	udev->state = USB_STATE_ADDRESSED;
}

int usbh_device_set_configuration(struct usb_device *const udev, const uint8_t num)
{
	struct usb_cfg_descriptor cfg_desc;
	uint8_t idx;
	int err;

	err = k_mutex_lock(&udev->mutex, K_NO_WAIT);
	if (err) {
		LOG_ERR("Failed to lock USB device");
		return err;
	}

	if (udev->actual_cfg == num) {
		LOG_INF("Already active device configuration");
		goto error;
	}

	if (num == 0) {
		reset_configuration(udev);
		err = usbh_req_set_cfg(udev, num);
		if (err) {
			LOG_ERR("Set Configuration %u request failed", num);
		}

		goto error;
	}

	idx = num - 1;

	err = usbh_req_desc_cfg(udev, idx, sizeof(cfg_desc), &cfg_desc);
	if (err) {
		LOG_ERR("Failed to read configuration %u descriptor", num);
		goto error;
	}

	if (cfg_desc.bDescriptorType != USB_DESC_CONFIGURATION) {
		LOG_ERR("Failed to read configuration descriptor");
		err = -EINVAL;
		goto error;
	}

	if (cfg_desc.bNumInterfaces == 0) {
		LOG_ERR("Configuration %u has no interfaces", cfg_desc.bNumInterfaces);
		err = -EINVAL;
		goto error;
	}

	if (cfg_desc.bNumInterfaces >= UHC_INTERFACES_MAX) {
		LOG_ERR("Unsupported number of interfaces");
		err = -EINVAL;
		goto error;
	}

	udev->cfg_desc = k_heap_alloc(&usb_device_heap,
				      cfg_desc.wTotalLength,
				      K_NO_WAIT);
	if (udev->cfg_desc == NULL) {
		LOG_ERR("Failed to allocate memory for configuration descriptor");
		err = -ENOMEM;
		goto error;
	}

	err = usbh_req_set_cfg(udev, num);
	if (err) {
		LOG_ERR("Set Configuration %u request failed", num);
		goto error;
	}

	memset(udev->cfg_desc, 0, cfg_desc.wTotalLength);
	if (udev->state == USB_STATE_CONFIGURED) {
		reset_configuration(udev);
	}

	err = usbh_req_desc_cfg(udev, idx, cfg_desc.wTotalLength, udev->cfg_desc);
	if (err) {
		LOG_ERR("Failed to read configuration descriptor");
		k_heap_free(&usb_device_heap, udev->cfg_desc);
		goto error;
	}

	if (memcmp(udev->cfg_desc, &cfg_desc, sizeof(cfg_desc))) {
		LOG_ERR("Configuration descriptor read mismatch");
		k_heap_free(&usb_device_heap, udev->cfg_desc);
		goto error;
	}

	LOG_INF("Configuration %u bNumInterfaces %u",
		cfg_desc.bConfigurationValue, cfg_desc.bNumInterfaces);

	err = parse_configuration_descriptor(udev);
	if (err) {
		k_heap_free(&usb_device_heap, udev->cfg_desc);
		goto error;
	}

	udev->actual_cfg = num;
	udev->state = USB_STATE_CONFIGURED;

error:
	k_mutex_unlock(&udev->mutex);

	return err;
}

int usbh_device_init(struct usb_device *const udev)
{
	struct usbh_contex *const uhs_ctx = udev->ctx;
	uint8_t new_addr;
	int err;

	if (udev->state != USB_STATE_DEFAULT) {
		LOG_ERR("USB device is not in default state");
		return -EALREADY;
	}

	err = k_mutex_lock(&udev->mutex, K_NO_WAIT);
	if (err) {
		LOG_ERR("Failed to lock USB device");
		return err;
	}

	/* FIXME: The port to which the device is connected should be reset. */
	err = uhc_bus_reset(uhs_ctx->dev);
	if (err) {
		LOG_ERR("Failed to signal bus reset");
		return err;
	}

	/*
	 * Limit mps0 to the minimum supported by full-speed devices until the
	 * device descriptor is read.
	 */
	udev->dev_desc.bMaxPacketSize0 = 8;
	err = usbh_req_desc_dev(udev, 8, &udev->dev_desc);
	if (err) {
		LOG_ERR("Failed to read device descriptor");
		goto error;
	}

	err = validate_device_mps0(udev);
	if (err) {
		goto error;
	}

	err = usbh_req_desc_dev(udev, sizeof(udev->dev_desc), &udev->dev_desc);
	if (err) {
		LOG_ERR("Failed to read device descriptor");
		goto error;
	}

	if (!udev->dev_desc.bNumConfigurations) {
		LOG_ERR("Device has no configurations, bNumConfigurations %d",
			udev->dev_desc.bNumConfigurations);
		goto error;
	}

	err = alloc_device_address(udev, &new_addr);
	if (err) {
		LOG_ERR("Failed to allocate device address");
		goto error;
	}

	err = usbh_req_set_address(udev, new_addr);
	if (err) {
		LOG_ERR("Failed to set device address");
		udev->addr = 0;

		goto error;
	}

	udev->addr = new_addr;
	udev->state = USB_STATE_ADDRESSED;

	LOG_INF("New device with address %u state %u", udev->addr, udev->state);

	err = usbh_device_set_configuration(udev, 1);
	if (err) {
		LOG_ERR("Failed to configure new device with address %u", udev->addr);
	}

error:
	k_mutex_unlock(&udev->mutex);

	return err;
}
