/*
 * SPDX-FileCopyrightText: Copyright Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/usb/uhc.h>

#include "usbh_device.h"
#include "usbh_ch9.h"
#include "usbh_host.h"
#include "usbh_class.h"

#include <zephyr/logging/log.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/toolchain.h>

LOG_MODULE_REGISTER(usbh_dev, CONFIG_USBH_LOG_LEVEL);

__weak void usbh_device_configured_notify(struct usb_device *udev)
{
	ARG_UNUSED(udev);
}

__weak void usbh_device_removed_notify(struct usb_device *udev)
{
	ARG_UNUSED(udev);
}

K_MEM_SLAB_DEFINE_STATIC(usb_device_slab, sizeof(struct usb_device), CONFIG_USBH_USB_DEVICE_MAX,
			 sizeof(void *));

K_HEAP_DEFINE(usb_device_heap, CONFIG_USBH_USB_DEVICE_HEAP);

struct usb_device *usbh_device_alloc(struct usbh_context *const uhs_ctx)
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
	struct usbh_context *const uhs_ctx = udev->ctx;

	if (udev->addr != 0U) {
		sys_bitarray_clear_bit(uhs_ctx->addr_ba, udev->addr);
	}
	sys_dlist_remove(&udev->node);
	if (udev->cfg_desc != NULL) {
		k_heap_free(&usb_device_heap, udev->cfg_desc);
	}

	k_mem_slab_free(&usb_device_slab, (void *)udev);
}

struct usb_device *usbh_device_get_any(struct usbh_context *const uhs_ctx)
{
	sys_dnode_t *node = sys_dlist_peek_head(&uhs_ctx->udevs);
	struct usb_device *udev;

	/*
	 * If udevs was never sys_dlist_init()'d (head stays NULL), peek_head is NULL
	 * even after usbh_device_alloc() — SYS_DLIST_STATIC_INIT in USBH_CONTROLLER_DEFINE
	 * and usbh_init() normally prevent that. Single-root: ctx->root still points at
	 * the connected device.
	 */
	if (node == NULL && uhs_ctx->root != NULL) {
		return uhs_ctx->root;
	}

	udev = SYS_DLIST_CONTAINER(node, udev, node);

	return udev;
}

struct usb_device *usbh_device_get(struct usbh_context *const uhs_ctx, const uint8_t addr)
{
	struct usb_device *udev;

	SYS_DLIST_FOR_EACH_CONTAINER(&uhs_ctx->udevs, udev, node) {
		if (addr == udev->addr) {
			return udev;
		}
	}

	return NULL;
}

static int validate_device_mps0(const struct usb_device *const udev)
{
	const uint8_t mps0 = udev->dev_desc.bMaxPacketSize0;

	if (udev->speed == USB_SPEED_SPEED_LS) {
		if (mps0 != 8) {
			LOG_ERR("LS device has wrong bMaxPacketSize0 %u", mps0);
			return -EINVAL;
		}

		return 0;
	}

	if (udev->speed == USB_SPEED_SPEED_SS) {
		if (mps0 != 9) {
			LOG_ERR("SS device has wrong bMaxPacketSize0 %u", mps0);
			return -EINVAL;
		}

		return 0;
	}

	if (udev->speed == USB_SPEED_SPEED_HS) {
		if (mps0 != 64) {
			LOG_ERR("HS device has wrong bMaxPacketSize0 %u", mps0);
			return -EINVAL;
		}

		return 0;
	}

	if (udev->speed == USB_SPEED_SPEED_FS) {
		if (mps0 != 8 && mps0 != 16 && mps0 != 32 && mps0 != 64) {
			LOG_ERR("FS device has wrong bMaxPacketSize0 %u", mps0);
			return -EINVAL;
		}
	}

	return 0;
}

/*
 * USB2 enumeration initial EP0 max packet guess before GET_DESCRIPTOR
 * (HS=64, FS=64, LS=8 per USB 2.0 spec).
 */
static void usbh_ep0_mps0_guess_from_speed(struct usb_device *const udev)
{
	switch (udev->speed) {
	case USB_SPEED_SPEED_HS:
	case USB_SPEED_SPEED_FS:
		udev->dev_desc.bMaxPacketSize0 = 64;
		break;
	case USB_SPEED_SPEED_LS:
		udev->dev_desc.bMaxPacketSize0 = 8;
		break;
	case USB_SPEED_SPEED_SS:
		udev->dev_desc.bMaxPacketSize0 = 9;
		break;
	default:
		udev->dev_desc.bMaxPacketSize0 = 8;
		break;
	}
}

/*
 * 64-byte first GET_DESCRIPTOR in USB2 enumeration — enough for
 * bMaxPacketSize0; full idProduct etc. come from the post-SET_ADDRESS read.
 */
#define USBH_GET_DESC_BUFSIZE 64U

static int usbh_first_get_descriptor_device(struct usb_device *const udev,
					    struct usb_device_descriptor *const desc)
{
	struct net_buf *buf;
	int ret;

	buf = usbh_xfer_buf_alloc(udev, USBH_GET_DESC_BUFSIZE);
	if (buf == NULL) {
		return -ENOMEM;
	}

	ret = usbh_req_desc(udev, USB_DESC_DEVICE, 0, 0, USBH_GET_DESC_BUFSIZE, buf);
	if (ret == 0 && buf->len >= 8U) {
		const size_t n = MIN((size_t)buf->len, sizeof(*desc));

		memcpy(desc, buf->data, n);
		if (n >= 4U) {
			desc->bcdUSB = sys_le16_to_cpu(desc->bcdUSB);
		}
		if (n >= sizeof(struct usb_device_descriptor)) {
			desc->idVendor = sys_le16_to_cpu(desc->idVendor);
			desc->idProduct = sys_le16_to_cpu(desc->idProduct);
			desc->bcdDevice = sys_le16_to_cpu(desc->bcdDevice);
		}
	} else if (ret == 0) {
		ret = -EPROTO;
	}

	usbh_xfer_buf_free(udev, buf);

	return ret;
}

static int alloc_device_address(struct usb_device *const udev, uint8_t *const addr)
{
	struct usbh_context *const uhs_ctx = udev->ctx;
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

static void release_device_address(struct usbh_context *const uhs_ctx, uint8_t addr)
{
	if (addr != 0U && uhs_ctx != NULL && uhs_ctx->addr_ba != NULL) {
		(void)sys_bitarray_clear_bit(uhs_ctx->addr_ba, addr);
	}
}

/*
 * Enumeration settle / retry delays. usbh_device_init() holds udev->mutex for
 * the whole enumeration; k_msleep yields the CPU while still serializing access
 * to the partially enumerated device. Control transfers complete on usbh_thread.
 */
static void usbh_enumeration_delay_ms(uint32_t ms)
{
	if (ms > 0U) {
		k_msleep(ms);
	}
}

bool usbh_device_still_connected(const struct usb_device *udev)
{
	struct usbh_context *ctx;
	bool connected;

	if (udev == NULL) {
		return false;
	}

	ctx = udev->ctx;
	if (ctx == NULL) {
		return false;
	}

	usbh_host_lock(ctx);
	connected = (ctx->root == udev && udev->state == USB_STATE_CONFIGURED && udev->addr != 0U);
	usbh_host_unlock(ctx);

	return connected;
}

static int usbh_assign_device_address(struct usb_device *const udev, uint8_t *const addr_out)
{
	struct usbh_context *const uhs_ctx = udev->ctx;
	const struct device *hcd = uhs_ctx->dev;
	uint8_t addr;
	int prev;
	int err;

	if (addr_out == NULL) {
		return -EINVAL;
	}

	err = uhc_assign_address(hcd, udev, &addr);
	if (err == -ENOTSUP) {
		err = alloc_device_address(udev, &addr);
		if (err != 0) {
			return err;
		}

		err = usbh_req_set_address(udev, addr);
		if (err != 0) {
			release_device_address(uhs_ctx, addr);
			return err;
		}
	} else if (err != 0) {
		return err;
	} else {
		if (addr == 0U) {
			return -EIO;
		}

		err = sys_bitarray_test_and_set_bit(uhs_ctx->addr_ba, addr, &prev);
		if (err != 0) {
			return err;
		}

		if (prev != 0) {
			return -EBUSY;
		}

		LOG_DBG("usbh: HCD assigned USB address %u", addr);
	}

	udev->addr = addr;
	udev->state = USB_STATE_ADDRESSED;
	*addr_out = addr;

	return 0;
}

enum ep_op {
	EP_OP_TEST, /* Verify endpoint descriptor */
	EP_OP_UP,   /* Enable endpoint and update endpoint pointers */
	EP_OP_DOWN, /* Disable endpoint and update endpoint pointers */
};

static void assign_ep_desc_ptr(struct usb_device *const udev, const uint8_t ep, void *const ptr)
{
	uint8_t idx = USB_EP_GET_IDX(ep) & 0xF;

	if (USB_EP_DIR_IS_IN(ep)) {
		udev->ep_in[idx].desc = ptr;
	} else {
		udev->ep_out[idx].desc = ptr;
	}
}

static int handle_ep_op(struct usb_device *const udev, const enum ep_op op, const uint8_t ep,
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

static int device_interface_modify(struct usb_device *const udev, const enum ep_op op,
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

			LOG_INF("Modify interface %u ep 0x%02x by op %u", iface,
				ep_desc->bEndpointAddress, op);
		}

		dhp = (void *)((uint8_t *)dhp + dhp->bLength);
	}

	return found_iface ? 0 : -ENODATA;
}

int usbh_device_interface_set(struct usb_device *const udev, const uint8_t iface, const uint8_t alt,
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
	err = 0;

error:
	k_mutex_unlock(&udev->mutex);

	return err;
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

	while ((void *)dhp < desc_end) {
		if ((uint8_t *)dhp + sizeof(struct usb_desc_header) > (uint8_t *)desc_end ||
		    (uint8_t *)dhp + dhp->bLength > (uint8_t *)desc_end ||
		    dhp->bLength <= sizeof(struct usb_desc_header)) {
			LOG_ERR("Invalid descriptor size %d.", dhp->bLength);
			return -EINVAL;
		}

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
				if (iad != NULL &&
				    iad->bFirstInterface == if_desc->bInterfaceNumber) {
					udev->ifaces[tmp_nif].iad = iad;
				}

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

static int usbh_config_descriptor_index(struct usb_device *const udev, const uint8_t cfg_value,
					uint8_t *const index_out)
{
	struct usb_cfg_descriptor hdr;

	for (uint8_t idx = 0U; idx < udev->dev_desc.bNumConfigurations; idx++) {
		const int err = usbh_req_desc_cfg(udev, idx, sizeof(hdr), &hdr);

		if (err != 0) {
			continue;
		}

		if (hdr.bDescriptorType != USB_DESC_CONFIGURATION) {
			continue;
		}

		if (hdr.bConfigurationValue == cfg_value) {
			*index_out = idx;
			return 0;
		}
	}

	return -ENOENT;
}

__weak int usbh_device_configuration_prefers(const struct usb_device *udev, const void *cfg_desc,
					     uint16_t len)
{
	ARG_UNUSED(udev);
	ARG_UNUSED(cfg_desc);
	ARG_UNUSED(len);

	return -ENOENT;
}

static int usbh_device_pick_configuration(struct usb_device *const udev, uint8_t *const cfg_out)
{
	uint8_t fallback = 0U;
	bool have_fallback = false;

	for (uint8_t idx = 0U; idx < udev->dev_desc.bNumConfigurations; idx++) {
		struct usb_cfg_descriptor hdr;
		int err = usbh_req_desc_cfg(udev, idx, sizeof(hdr), &hdr);

		if (err != 0 || hdr.bDescriptorType != USB_DESC_CONFIGURATION) {
			continue;
		}

		{
			const uint16_t total = sys_le16_to_cpu(hdr.wTotalLength);

			if (total > sizeof(hdr)) {
				uint8_t *const blob =
					k_heap_alloc(&usb_device_heap, total, K_NO_WAIT);

				if (blob != NULL) {
					err = usbh_req_desc_cfg(udev, idx, total,
								(struct usb_cfg_descriptor *)blob);
					if (err == 0) {
						err = usbh_device_configuration_prefers(udev, blob,
											total);
					}
					k_heap_free(&usb_device_heap, blob);

					if (err == 0) {
						*cfg_out = hdr.bConfigurationValue;
						LOG_INF("Selected configuration %u (class "
							"preferred)",
							*cfg_out);
						return 0;
					}
				} else {
					LOG_WRN("Failed to allocate %u bytes for configuration %u "
						"scan",
						total, hdr.bConfigurationValue);
				}
			}
		}
		if (!have_fallback) {
			fallback = hdr.bConfigurationValue;
			have_fallback = true;
		}
	}

	if (have_fallback) {
		*cfg_out = fallback;
		LOG_INF("Selected configuration %u (first available)", *cfg_out);
		return 0;
	}

	return -ENOENT;
}

static int usbh_device_set_configuration_locked(struct usb_device *const udev, const uint8_t num)
{
	struct usb_cfg_descriptor cfg_desc;
	uint8_t idx;
	int err = 0;

	if (udev->actual_cfg == num) {
		LOG_INF("Already active device configuration");
		return 0;
	}

	if (num == 0) {
		if (udev->cfg_desc != NULL) {
			k_heap_free(&usb_device_heap, udev->cfg_desc);
			udev->cfg_desc = NULL;
		}
		reset_configuration(udev);
		err = usbh_req_set_cfg(udev, num);
		if (err) {
			LOG_ERR("Set Configuration %u request failed", num);
		}

		return err;
	}

	err = usbh_config_descriptor_index(udev, num, &idx);
	if (err != 0) {
		LOG_ERR("Configuration value %u not found on device", num);
		return err;
	}

	err = usbh_req_desc_cfg(udev, idx, sizeof(cfg_desc), &cfg_desc);
	if (err) {
		LOG_ERR("Failed to read configuration %u descriptor", num);
		return err;
	}

	if (cfg_desc.bDescriptorType != USB_DESC_CONFIGURATION) {
		LOG_ERR("Failed to read configuration descriptor");
		return -EINVAL;
	}

	if (cfg_desc.bNumInterfaces == 0) {
		LOG_ERR("Configuration %u has no interfaces", cfg_desc.bNumInterfaces);
		return -EINVAL;
	}

	if (cfg_desc.bNumInterfaces >= UHC_INTERFACES_MAX) {
		LOG_ERR("Unsupported number of interfaces");
		return -EINVAL;
	}

	if (udev->cfg_desc != NULL) {
		k_heap_free(&usb_device_heap, udev->cfg_desc);
		udev->cfg_desc = NULL;
	}

	udev->cfg_desc = k_heap_alloc(&usb_device_heap, cfg_desc.wTotalLength, K_NO_WAIT);
	if (udev->cfg_desc == NULL) {
		LOG_ERR("Failed to allocate memory for configuration descriptor");
		return -ENOMEM;
	}

	err = usbh_req_set_cfg(udev, num);
	if (err) {
		LOG_ERR("Set Configuration %u request failed", num);
		k_heap_free(&usb_device_heap, udev->cfg_desc);
		udev->cfg_desc = NULL;
		return err;
	}

#if CONFIG_USBH_POST_SET_CONFIGURATION_MS > 0
	/*
	 * Optional settle before the full GET_DESCRIPTOR(CONFIGURATION): a few
	 * devices are slow to accept EP0 immediately after configuration change.
	 */
	usbh_enumeration_delay_ms((uint32_t)CONFIG_USBH_POST_SET_CONFIGURATION_MS);
#endif

	memset(udev->cfg_desc, 0, cfg_desc.wTotalLength);
	if (udev->state == USB_STATE_CONFIGURED) {
		reset_configuration(udev);
	}

	err = usbh_req_desc_cfg(udev, idx, cfg_desc.wTotalLength, udev->cfg_desc);
	if (err) {
		LOG_ERR("Failed to read configuration descriptor");
		k_heap_free(&usb_device_heap, udev->cfg_desc);
		udev->cfg_desc = NULL;
		return err;
	}

	if (memcmp(udev->cfg_desc, &cfg_desc, sizeof(cfg_desc))) {
		LOG_ERR("Configuration descriptor read mismatch");
		k_heap_free(&usb_device_heap, udev->cfg_desc);
		udev->cfg_desc = NULL;
		return -EINVAL;
	}

	LOG_INF("Configuration %u bNumInterfaces %u", cfg_desc.bConfigurationValue,
		cfg_desc.bNumInterfaces);

	err = parse_configuration_descriptor(udev);
	if (err) {
		k_heap_free(&usb_device_heap, udev->cfg_desc);
		udev->cfg_desc = NULL;
		return err;
	}

	/*
	 * After SET_CONFIGURATION and descriptor parse, the HCD programs
	 * endpoints (Configure Endpoint, bandwidth). Host stack has filled
	 * udev->ifaces[] and ep_in/ep_out[] with alternate 0 endpoints.
	 */
	LOG_INF("usbh: HCD device_configured — cfg=%u addr=%u speed=%u (xHCI Configure Endpoint "
		"next)",
		num, udev->addr, (unsigned int)udev->speed);
	err = uhc_add_endpoints(((struct usbh_context *)udev->ctx)->dev, udev);
	if (err) {
		k_heap_free(&usb_device_heap, udev->cfg_desc);
		udev->cfg_desc = NULL;
		return err;
	}

	udev->actual_cfg = num;
	udev->state = USB_STATE_CONFIGURED;
	LOG_INF("usbh: enumeration milestone — USB_STATE_CONFIGURED cfg=%u addr=%u speed=%u "
		"bNumInterfaces=%u (class drivers may run)",
		num, udev->addr, (unsigned int)udev->speed, (unsigned int)cfg_desc.bNumInterfaces);

	return 0;
}

int usbh_device_set_configuration(struct usb_device *const udev, const uint8_t num)
{
	int err;

	err = k_mutex_lock(&udev->mutex, K_NO_WAIT);
	if (err) {
		LOG_ERR("Failed to lock USB device");
		return err;
	}

	err = usbh_device_set_configuration_locked(udev, num);
	k_mutex_unlock(&udev->mutex);

	return err;
}

int usbh_device_set_address(struct usb_device *const udev, const uint8_t new_addr)
{
	uint8_t assigned;
	int err;

	if (new_addr == 0U) {
		err = usbh_req_set_address(udev, 0);
		if (err != 0) {
			LOG_ERR("Failed to set device address to 0");
			return err;
		}

		udev->addr = 0;
		udev->state = USB_STATE_DEFAULT;
		return 0;
	}

	err = usbh_assign_device_address(udev, &assigned);
	if (err != 0) {
		LOG_ERR("Failed to assign device address (requested %u)", new_addr);
		return err;
	}

	if (assigned != new_addr) {
		LOG_WRN("usbh: HCD assigned address %u (requested %u)", assigned, new_addr);
	}

	return 0;
}

int usbh_device_init(struct usb_device *const udev)
{
	struct usbh_context *const uhs_ctx = udev->ctx;
	uint8_t new_addr = 0U;
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
	LOG_DBG("usbh: about to uhc_bus_reset()");
	err = uhc_bus_reset(uhs_ctx->dev);
	if (err) {
		LOG_ERR("Failed to signal bus reset");
		k_mutex_unlock(&udev->mutex);
		return err;
	}

#if IS_ENABLED(CONFIG_USBH_USB2_ENUM_SCHEME)
	const bool usb2_enum_scheme = (udev->speed != USB_SPEED_SPEED_SS);
#else
	const bool usb2_enum_scheme = false;
#endif

	if (usb2_enum_scheme) {
		/*
		 * USB2 enumeration: 64-byte GET_DESCRIPTOR, optional second reset,
		 * SET_ADDRESS, settle, full GET_DESCRIPTOR.
		 */
		LOG_DBG("usbh: USB2 enum "
			"(GET_DESCRIPTOR/64 → SET_ADDRESS → settle → GET_DESCRIPTOR/18)");

		usbh_ep0_mps0_guess_from_speed(udev);

		{
			unsigned int gd_tries = (unsigned int)CONFIG_USBH_GET_DESCRIPTOR_TRIES;
			unsigned int gd;

			for (gd = 0U; gd < gd_tries; gd++) {
				if (gd > 0U && CONFIG_USBH_ENUM_RETRY_DELAY_MS > 0) {
					usbh_enumeration_delay_ms(
						(uint32_t)CONFIG_USBH_ENUM_RETRY_DELAY_MS);
				}
				err = usbh_first_get_descriptor_device(udev, &udev->dev_desc);
				if (err == 0) {
					break;
				}
				LOG_WRN("usbh: GET_DESCRIPTOR/64 try %u/%u failed err=%d", gd + 1U,
					gd_tries, err);
			}
		}
		if (err) {
			LOG_ERR("Failed first GET_DESCRIPTOR (64), err=%d", err);
			goto error;
		}

		err = validate_device_mps0(udev);
		if (err) {
			goto error;
		}

#if IS_ENABLED(CONFIG_USBH_USB2_ENUM_SECOND_RESET)
		LOG_DBG("usbh: second uhc_bus_reset()");
		err = uhc_bus_reset(uhs_ctx->dev);
		if (err) {
			LOG_ERR("Second bus reset failed");
			goto error;
		}
#endif

		{
			unsigned int sa_tries = (unsigned int)CONFIG_USBH_SET_ADDRESS_TRIES;
			unsigned int sa;

			for (sa = 0U; sa < sa_tries; sa++) {
				if (sa > 0U && CONFIG_USBH_SET_ADDRESS_RETRY_DELAY_MS > 0) {
					usbh_enumeration_delay_ms(
						(uint32_t)CONFIG_USBH_SET_ADDRESS_RETRY_DELAY_MS);
				}
				LOG_DBG("usbh: assign address try %u/%u", sa + 1U, sa_tries);
				err = usbh_assign_device_address(udev, &new_addr);
				if (err == 0) {
					break;
				}
				LOG_WRN("usbh: assign address try %u/%u failed err=%d", sa + 1U,
					sa_tries, err);
			}
		}
		if (err) {
			LOG_ERR("Failed to assign device address");
			udev->addr = 0;
			goto error;
		}

#if CONFIG_USBH_POST_SET_ADDRESS_MS > 0
		LOG_DBG("usbh: POST_SET_ADDRESS delay %u ms (mutex held)",
			(unsigned int)CONFIG_USBH_POST_SET_ADDRESS_MS);
		usbh_enumeration_delay_ms((uint32_t)CONFIG_USBH_POST_SET_ADDRESS_MS);
		LOG_DBG("usbh: POST_SET_ADDRESS settle done");
#endif

		LOG_DBG("usbh: GET_DESCRIPTOR device full (after SET_ADDRESS)");
		err = usbh_req_desc_dev(udev, sizeof(udev->dev_desc), &udev->dev_desc);
		if (err) {
			LOG_ERR("Failed to read full device descriptor");
			goto error;
		}

		LOG_INF("usbh: device idVendor=0x%04x idProduct=0x%04x", udev->dev_desc.idVendor,
			udev->dev_desc.idProduct);

		if (!udev->dev_desc.bNumConfigurations) {
			LOG_ERR("Device has no configurations, bNumConfigurations %d",
				udev->dev_desc.bNumConfigurations);
			goto error;
		}

		LOG_INF("New device with address %u state %u", udev->addr, udev->state);

		{
			uint8_t cfg;

			err = usbh_device_pick_configuration(udev, &cfg);
			if (err != 0) {
				LOG_ERR("Failed to pick device configuration");
				goto error;
			}

			err = usbh_device_set_configuration_locked(udev, cfg);
		}
	} else {
		LOG_DBG("usbh: uhc_bus_reset() returned ok; legacy GET_DESCRIPTOR(8)");
		LOG_DBG("usbh: bus reset done, starting GET_DESCRIPTOR (8)");

		/*
		 * Limit mps0 until the device descriptor is read (LS=8, SS=9).
		 */
		if (udev->speed == USB_SPEED_SPEED_SS) {
			udev->dev_desc.bMaxPacketSize0 = 9;
		} else {
			udev->dev_desc.bMaxPacketSize0 = 8;
		}
		LOG_DBG("usbh: GET_DESCRIPTOR device 8-byte (EP0)");
		err = usbh_req_desc_dev(udev, 8, &udev->dev_desc);
		if (err) {
			LOG_ERR("Failed to read device descriptor");
			goto error;
		}

		LOG_DBG("usbh: GET_DESCRIPTOR(8) ok; validate mps0");
		err = validate_device_mps0(udev);
		if (err) {
			goto error;
		}

		LOG_DBG("usbh: GET_DESCRIPTOR device full");
		err = usbh_req_desc_dev(udev, sizeof(udev->dev_desc), &udev->dev_desc);
		if (err) {
			LOG_ERR("Failed to read device descriptor");
			goto error;
		}

		LOG_INF("usbh: device idVendor=0x%04x idProduct=0x%04x", udev->dev_desc.idVendor,
			udev->dev_desc.idProduct);

		if (!udev->dev_desc.bNumConfigurations) {
			LOG_ERR("Device has no configurations, bNumConfigurations %d",
				udev->dev_desc.bNumConfigurations);
			goto error;
		}

		LOG_DBG("usbh: assign device address");
		err = usbh_assign_device_address(udev, &new_addr);
		if (err) {
			LOG_ERR("Failed to assign device address");
			udev->addr = 0;
			goto error;
		}

		LOG_INF("New device with address %u state %u", udev->addr, udev->state);

		{
			uint8_t cfg;

			err = usbh_device_pick_configuration(udev, &cfg);
			if (err != 0) {
				LOG_ERR("Failed to pick device configuration");
				goto error;
			}

			err = usbh_device_set_configuration_locked(udev, cfg);
		}
	}

	if (err) {
		LOG_ERR("Failed to configure new device with address %u", udev->addr);
	}

error:
	k_mutex_unlock(&udev->mutex);

	/*
	 * After the outer enumeration lock is released: apps may touch the device
	 * from preemptible context; notify here (not inside set_configuration) so
	 * the mutex is never held while the app runs.
	 *
	 * Require CONFIGURED + non-zero cfg: avoids spurious notify if a path ever
	 * returned success without fully configuring (e.g. fixed descriptor mismatch).
	 */
	if (err == 0 && udev->state == USB_STATE_CONFIGURED && udev->actual_cfg != 0U) {
		const char *const tname = k_thread_name_get(k_current_get());

		LOG_DBG("trace: usbh calling configured_notify thread=%s prio=%d udev=%p "
			"addr=%u state=%d cfg=%u",
			tname != NULL ? tname : "?", k_thread_priority_get(k_current_get()),
			(void *)udev, udev->addr, (int)udev->state, udev->actual_cfg);
		/*
		 * Publish device fields to other CPUs before the app wakes (notify may give
		 * a one-shot semaphore; main must not see stale state and drop the wakeup).
		 */
		barrier_dmem_fence_full();
		usbh_class_probe_device(udev);
		usbh_device_configured_notify(udev);
	} else {
		const char *const tn = k_thread_name_get(k_current_get());

		LOG_DBG("trace: usbh skip notify err=%d state=%d actual_cfg=%u thread=%s", err,
			(int)udev->state, udev->actual_cfg, tn != NULL ? tn : "?");
	}

	return err;
}
