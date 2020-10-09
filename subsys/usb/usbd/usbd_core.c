/*
 * Copyright (c) 2016-2018 Intel Corporation
 * Copyright (c) 2020 Nordic Semiconductor ASA
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <usb/usbd.h>
#include <sys/byteorder.h>
#include <usbd_internal.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(usbd, CONFIG_USBD_LOG_LEVEL);

K_MUTEX_DEFINE(usbd_enable_lock);

/* USB device support context definition */
struct usbd_contex usbd_ctx = {
	.ep_bm = BIT(16) | BIT(0),
	.alternate = {0},
	.dev_desc = {
		.bLength = sizeof(struct usb_device_descriptor),
		.bDescriptorType = USB_DESC_DEVICE,
		.bcdUSB = sys_cpu_to_le16(USB_SRN_2_0),
		.bDeviceClass = USB_BCC_MISCELLANEOUS,
		.bDeviceSubClass = 0x02,
		.bDeviceProtocol = 0x01,
		.bMaxPacketSize0 = USB_CONTROL_EP_MPS,
		.idVendor = sys_cpu_to_le16((uint16_t)CONFIG_USBD_DEVICE_VID),
		.idProduct = sys_cpu_to_le16((uint16_t)CONFIG_USBD_DEVICE_PID),
		.bcdDevice = sys_cpu_to_le16(USB_BCD_DRN),
		.iManufacturer = USBD_DESC_MANUFACTURER_IDX,
		.iProduct = USBD_DESC_PRODUCT_IDX,
		.iSerialNumber = USBD_DESC_SERIAL_NUMBER_IDX,
		.bNumConfigurations = 1,
	},
	.cfg_desc = {
		.bLength = sizeof(struct usb_cfg_descriptor),
		.bDescriptorType = USB_DESC_CONFIGURATION,
		/* wTotalLength will be updated during initialization */
		.wTotalLength = 0,
		.bNumInterfaces = 0,
		.bConfigurationValue = 1,
		.iConfiguration = 0,
		.bmAttributes = USB_SCD_ATTRIBUTES,
		.bMaxPower = CONFIG_USBD_MAX_POWER,
	},
	.lang_desc = {
		.bLength = sizeof(struct usb_string_descriptor),
		.bDescriptorType = USB_DESC_STRING,
		.bString = sys_cpu_to_le16(0x0409),
	},
	.mfr_desc = {
		.bLength = USB_STRING_DESCRIPTOR_LENGTH(
				CONFIG_USBD_DEVICE_MANUFACTURER),
		.bDescriptorType = USB_DESC_STRING,
		.bString = CONFIG_USBD_DEVICE_MANUFACTURER,
	},
	.product_desc = {
		.bLength = USB_STRING_DESCRIPTOR_LENGTH(
				CONFIG_USBD_DEVICE_PRODUCT),
		.bDescriptorType = USB_DESC_STRING,
		.bString = CONFIG_USBD_DEVICE_PRODUCT,
	},
	.sn_desc = {
		.bLength = USB_STRING_DESCRIPTOR_LENGTH(CONFIG_USBD_DEVICE_SN),
		.bDescriptorType = USB_DESC_STRING,
		.bString = CONFIG_USBD_DEVICE_SN,
	},
};

/**
 * @brief Calculate the length of the class descriptor
 *
 * Calculate the length of the class descriptor.
 * The descriptor must be terminated by a termination descriptor
 * (bLength = 0 and bDescriptorType = 0).
 *
 * @param[in] cctx Class context struct of the class instance
 *
 * @return Length of the class descriptor
 */
size_t usbd_cctx_desc_len(struct usbd_class_ctx *cctx)
{
	struct usb_desc_header *head;
	uint8_t *ptr;
	size_t len = 0;

	if (cctx->class_desc != NULL) {
		head = cctx->class_desc;
		ptr = (uint8_t *)head;

		while (head->bLength != 0) {
			len += head->bLength;
			ptr += head->bLength;
			head = (struct usb_desc_header *)ptr;
		}
	}

	return len;
}

/**
 * @brief Get class context by bInterfaceNumber value
 *
 * The function searches the class instance list for the interface number.
 *
 * @param[in] i_n Interface number
 *
 * @return Class context pointer or NULL
 */
struct usbd_class_ctx *usbd_cctx_get_by_iface(uint8_t i_n)
{
	struct usbd_class_ctx *cctx;
	struct usb_desc_header *head;
	struct usb_if_descriptor *if_desc;
	uint8_t *ptr;

	SYS_SLIST_FOR_EACH_CONTAINER(&usbd_ctx.class_list, cctx, node) {
		if (cctx->class_desc == NULL) {
			continue;
		}

		head = cctx->class_desc;
		ptr = (uint8_t *)head;

		while (head->bLength != 0) {
			switch (head->bDescriptorType) {
			case USB_DESC_INTERFACE:
				if_desc = (struct usb_if_descriptor *)head;
				if (if_desc->bInterfaceNumber == i_n) {
					return cctx;
				}

			default:
				break;
			}

			ptr += head->bLength;
			head = (struct usb_desc_header *)ptr;
		}
	}

	return NULL;
}

/**
 * @brief Get class context by endpoint address
 *
 * The function searches the class instance list for the endpoint address.
 *
 * @param[in] ep Endpoint address
 *
 * @return Class context pointer or NULL
 */
struct usbd_class_ctx *usbd_cctx_get_by_ep(uint8_t ep)
{
	struct usbd_class_ctx *cctx;
	uint8_t ep_idx = USB_EP_GET_IDX(ep);
	uint32_t ep_bm;

	if (USB_EP_DIR_IS_IN(ep)) {
		ep_bm = BIT(ep_idx + 16);
	} else {
		ep_bm = BIT(ep_idx);
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&usbd_ctx.class_list, cctx, node) {
		if (cctx->ep_bm & ep_bm) {
			return cctx;
		}
	}

	return NULL;
}

/**
 * @brief Get class context by request
 *
 * The function searches the class instance list and
 * compares the vendor request table with request value.
 * The function is only used if the request type is Vendor and
 * request recipient is Device.
 * Accordingly only the first class instance can be found.
 *
 * @param[in] ep Endpoint address
 *
 * @return Class context pointer or NULL
 */
struct usbd_class_ctx *usbd_cctx_get_by_req(uint8_t request)
{
	struct usbd_class_ctx *cctx;

	SYS_SLIST_FOR_EACH_CONTAINER(&usbd_ctx.class_list, cctx, node) {
		if (cctx->v_reqs == NULL) {
			continue;
		}

		for (int i = 0; i < cctx->v_reqs->len; i++) {
			/*
			 * First instance always wins.
			 * There is no other way to determine the recipient.
			 */
			if (cctx->v_reqs->reqs[i] == request) {
				return cctx;
			}
		}
	}

	return NULL;
}

/**
 * @brief Configure and enable endpoint
 *
 * This function configures and enables an endpoint.
 * The endpoint callback is a common callback from
 * the transfer buffer subsystem.
 *
 * @note Must be revised after the change of USB driver API.
 *
 * @param[in] ep Endpoint descriptor
 *
 * @return 0 on success, other values on fail.
 */
static int set_endpoint(const struct usb_ep_descriptor *ep_desc)
{
	struct usb_dc_ep_cfg_data ep_cfg;
	int ret;

	ep_cfg.ep_addr = ep_desc->bEndpointAddress;
	ep_cfg.ep_mps = sys_le16_to_cpu(ep_desc->wMaxPacketSize);
	ep_cfg.ep_type = ep_desc->bmAttributes & USB_EP_TRANSFER_TYPE_MASK;

	ret = usb_dc_ep_configure(&ep_cfg);
	if (ret == -EALREADY) {
		LOG_WRN("Endpoint 0x%02x already configured", ep_cfg.ep_addr);
		ret = 0;
	} else if (ret) {
		LOG_ERR("Failed to configure endpoint 0x%02x", ep_cfg.ep_addr);
		return ret;
	}

	ret = usb_dc_ep_enable(ep_cfg.ep_addr);
	if (ret == -EALREADY) {
		LOG_WRN("Endpoint 0x%02x already enabled", ep_cfg.ep_addr);
		ret = 0;
	} else if (ret) {
		LOG_ERR("Failed to enable endpoint 0x%02x", ep_cfg.ep_addr);
		return ret;
	}

	if (usb_dc_ep_set_callback(ep_cfg.ep_addr, usbd_tbuf_ep_cb)) {
		ret = -EIO;
	}

	LOG_WRN("Configured ep 0x%x type %u MPS %u",
		ep_cfg.ep_addr, ep_cfg.ep_type, ep_cfg.ep_mps);

	return ret;
}

/**
 * @brief Disable an endpoint
 *
 * This function discard endpoint buffer, disables an endpoint, and
 * cancels ongoing transfers.
 *
 * @note Must be revised after the change of USB driver API.
 *
 * @param[in] ep Endpoint descriptor
 *
 * @return 0 on success, other values on fail.
 */
static int reset_endpoint(const struct usb_ep_descriptor *ep_desc)
{
	struct usb_dc_ep_cfg_data ep_cfg;
	int ret;

	ep_cfg.ep_addr = ep_desc->bEndpointAddress;
	ep_cfg.ep_type = ep_desc->bmAttributes & USB_EP_TRANSFER_TYPE_MASK;

	LOG_WRN("Reset endpoint 0x%02x type %u",
		ep_cfg.ep_addr, ep_cfg.ep_type);

	usb_dc_ep_flush(ep_cfg.ep_addr);
	ret = usb_dc_ep_disable(ep_cfg.ep_addr);
	if (ret == -EALREADY) {
		LOG_WRN("Endpoint 0x%02x already disabled", ep_cfg.ep_addr);
		ret = 0;
	} else if (ret) {
		LOG_ERR("Failed to disable endpoint 0x%02x", ep_cfg.ep_addr);
	}

	usbd_tbuf_cancel(ep_cfg.ep_addr);

	return ret;
}

/**
 * @brief Setup all endpoints for an interface of class instance
 *
 * This function enables or disables all endpoints
 * that belong to an isntance of a class. The function also
 * respects the alternate setting for an interface.
 *
 * @param[in] cctx Class context struct of the class instance
 * @param[in] i_n Interface number
 * @param[in] bool Should be true to enable and false to disable endpoints
 *
 * @return 0 on success, other values on fail.
 */
static int setup_iface_eps(struct usbd_class_ctx *cctx,
			   uint8_t i_n, bool enable)
{
	struct usb_desc_header *head;
	struct usb_if_descriptor *if_desc = NULL;
	struct usb_ep_descriptor *ep_desc = NULL;
	int ret = 0;
	uint8_t *ptr;
	uint8_t a_n;

	a_n = usbd_ctx.alternate[i_n];
	head = cctx->class_desc;

	while (head->bLength != 0 && ret == 0) {
		ptr = (uint8_t *)head;

		switch (head->bDescriptorType) {
		case USB_DESC_INTERFACE:
			if_desc = (struct usb_if_descriptor *)head;
			break;
		case USB_DESC_ENDPOINT:
			ep_desc = (struct usb_ep_descriptor *)head;

			if (if_desc->bInterfaceNumber == i_n &&
			    if_desc->bAlternateSetting == a_n) {
				if (enable) {
					ret = set_endpoint(ep_desc);
				} else {
					ret = reset_endpoint(ep_desc);
				}
			}

			break;
		default:
			break;
		}

		ptr += head->bLength;
		head = (struct usb_desc_header *)ptr;
	}

	return ret;
}

/**
 * @brief Restart OUT transfers for specific interface
 *
 * This function restarts transfers for all OUT endpoints
 * that belong to an interface of a class instance.
 *
 * The reason for this function is the intermediate layer
 * that handles transfers through the USB driver API,
 * which actually uses a kind of static buffers.
 *
 * @note Must be removed after USB device driver API rework.
 *
 * @param[in] cctx Class context struct of the class instance
 * @param[in] i_n Interface number
 * @param[in] bool Should be true to force restart on all interfaces
 *
 * @return 0 on success, other values on fail.
 */
int cctx_restart_out_eps(struct usbd_class_ctx *cctx, uint8_t i_n,
			 bool force_all)
{
	struct usb_desc_header *head;
	struct usb_if_descriptor *if_desc = NULL;
	struct usb_ep_descriptor *ep_desc = NULL;
	struct net_buf *buf;
	uint8_t *ptr;
	uint8_t ep;
	uint16_t mps;
	uint8_t a_n;

	a_n = usbd_ctx.alternate[i_n];
	head = cctx->class_desc;

	while (head->bLength != 0) {
		ptr = (uint8_t *)head;

		switch (head->bDescriptorType) {
		case USB_DESC_INTERFACE:
			if_desc = (struct usb_if_descriptor *)head;
			if (force_all == true) {
				i_n = if_desc->bInterfaceNumber;
				a_n = usbd_ctx.alternate[i_n];
			}

			break;
		case USB_DESC_ENDPOINT:
			ep_desc = (struct usb_ep_descriptor *)head;
			ep = ep_desc->bEndpointAddress;
			mps = ep_desc->wMaxPacketSize;

			if (if_desc->bInterfaceNumber == i_n &&
			    if_desc->bAlternateSetting == a_n &&
			    USB_EP_DIR_IS_OUT(ep)) {
				LOG_WRN("restart ep 0x%02x, mps %u", ep, mps);
				buf = usbd_tbuf_alloc(ep, mps);
				if (buf == NULL) {
					return -ENOMEM;
				}

				usbd_tbuf_submit(buf, false);
			}

			break;
		default:
			break;
		}

		ptr += head->bLength;
		head = (struct usb_desc_header *)ptr;
	}

	return 0;
}

/**
 * @brief Setup all endpoints for a specific interface
 *
 * This function enables or disables all endpoints
 * that belong to a specific interface. The function
 * respects alternate setting.
 *
 * @param[in] i_n Interface number
 * @param[in] bool Should be true to enable and false to disable endpoints
 *
 * @return 0 on success, other values on fail.
 */
int usbd_cctx_cfg_eps(uint8_t i_n, bool enable)
{
	struct usbd_class_ctx *cctx;

	cctx = usbd_cctx_get_by_iface(i_n);

	if (cctx == NULL) {
		LOG_ERR("Failed to find context for interface %u", i_n);
		return -ENOTSUP;
	}

	return setup_iface_eps(cctx, i_n, enable);
}

/**
 * @brief Handle Disconnect event
 *
 * Disable all endpoints and cancel all transfers.
 */
static void usbd_handle_event_discon(void)
{
	int ret;

	atomic_clear_bit(&usbd_ctx.state, USBD_STATE_CONFIGURED);

	for (uint8_t i = 0; i < usbd_ctx.cfg_desc.bNumInterfaces; i++) {
		ret = usbd_cctx_cfg_eps(i, false);
		if (ret != 0) {
			LOG_ERR("Failed to disable interface %u endpoints", i);
		}
	}

	usb_dc_ep_disable(USB_CONTROL_EP_OUT);
	usbd_tbuf_cancel(USB_CONTROL_EP_OUT);
	usb_dc_ep_disable(USB_CONTROL_EP_IN);
	usbd_tbuf_cancel(USB_CONTROL_EP_IN);
	usbd_ctx.configuration = 0;
}

int usbd_cctx_register(const struct device *dev)
{
	struct usbd_class_ctx *cctx;

	if (dev == NULL) {
		return -ENODEV;
	}

	cctx = (struct usbd_class_ctx *)dev->config_info;

	if (atomic_test_bit(&usbd_ctx.state, USBD_STATE_ENABLED)) {
		LOG_ERR("USB device support is already enabled");
		return -EBUSY;
	}

	if (atomic_test_and_set_bit(&cctx->state, USBD_CCTX_REGISTERED)) {
		LOG_ERR("Class instance allready registered");
		return -EBUSY;
	}

	sys_slist_append(&usbd_ctx.class_list, &cctx->node);

	return 0;
}

int usbd_cctx_unregister(const struct device *dev)
{
	struct usbd_class_ctx *cctx;
	bool ret;

	if (dev == NULL) {
		return -ENODEV;
	}

	cctx = (struct usbd_class_ctx *)dev->config_info;

	if (atomic_test_bit(&usbd_ctx.state, USBD_STATE_ENABLED)) {
		LOG_ERR("USB device support is already enabled");
		return -EBUSY;
	}

	if (!atomic_test_and_clear_bit(&cctx->state, USBD_CCTX_REGISTERED)) {
		LOG_ERR("Class instance not registered");
		return -EBUSY;
	}

	ret = sys_slist_find_and_remove(&usbd_ctx.class_list, &cctx->node);
	if (ret == false) {
		LOG_ERR("Could not find class instance");
		return -ESRCH;
	}

	return 0;
}

/**
 * @brief Broadcast power event to all instances.
 *
 * WIP
 *
 * @param[in] status Event
 */
static void usbd_event_bcast(enum usb_dc_status_code status)
{
	struct usbd_class_ctx *cctx;
	enum usbd_pme_code event;

	switch (status) {
	case USB_DC_SUSPEND:
		event = USBD_PME_SUSPEND;
		break;
	case USB_DC_RESUME:
		event = USBD_PME_RESUME;
		break;
	case USB_DC_DISCONNECTED:
		event = USBD_PME_DETACHED;
		break;
	default:
		return;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&usbd_ctx.class_list, cctx, node) {
		if (cctx->ops.pm_event) {
			cctx->ops.pm_event(cctx, event);
		}
	}
}

/**
 * @brief Event handler
 *
 * WIP
 *
 * @param[in] status Event
 * @param[in] param Event parameter
 */
static void usbd_event_handler(enum usb_dc_status_code status,
			       const uint8_t *param)
{
	if (status == USB_DC_SUSPEND &&
	    atomic_test_bit(&usbd_ctx.state, USBD_STATE_CONFIGURED)) {
		LOG_ERR("Suspend Event");
		/* TODO: */
		/* usbd_tbuf_cancel(...); */
	}

	if (status == USB_DC_DISCONNECTED &&
	    atomic_test_bit(&usbd_ctx.state, USBD_STATE_CONFIGURED)) {
		LOG_ERR("Disconnect Event");
		usbd_handle_event_discon();
	}

	usbd_event_bcast(status);

	if (usbd_ctx.user_status_cb) {
		usbd_ctx.user_status_cb(status, param);
	}
}

static int usbd_init_notification(usb_dc_status_callback usr_cb)
{
	usbd_ctx.user_status_cb = usr_cb;
	usbd_ctx.status_cb = usbd_event_handler;
	usb_dc_set_status_callback(usbd_event_handler);

	return 0;
}

int usbd_disable(void)
{
	int ret;

	if (!atomic_test_and_clear_bit(&usbd_ctx.state, USBD_STATE_ENABLED)) {
		LOG_WRN("USB device support is already disabled");
		return 0;
	}

	k_mutex_lock(&usbd_enable_lock, K_FOREVER);
	/*
	 * Cancle trasnfers und disable endpoints for the case the
	 * driver does not emit USB_DC_DISCONNECTED event.
	 */
	usbd_handle_event_discon();

	ret = usb_dc_detach();
	if (ret != 0) {
		LOG_ERR("Failed to detach USB device");
	}

	usbd_ctx.status_cb = NULL;
	usbd_ctx.user_status_cb = NULL;
	k_mutex_unlock(&usbd_enable_lock);

	return 0;
}

int usbd_enable(usb_dc_status_callback status_cb)
{
	int ret;

	k_mutex_lock(&usbd_enable_lock, K_FOREVER);

	if (atomic_test_bit(&usbd_ctx.state, USBD_STATE_ENABLED)) {
		LOG_WRN("USB device support is already enabled");
		ret = -EALREADY;
		goto enable_exit;
	}

	ret = usbd_init_desc();
	if (ret != 0) {
		goto enable_exit;
	}

	ret = usbd_tbuf_init();
	if (ret != 0) {
		goto enable_exit;
	}

	ret = usb_dc_attach();
	if (ret != 0) {
		goto enable_exit;
	}

	ret = usbd_init_control_ep();
	if (ret != 0) {
		goto enable_exit;
	}

	ret = usbd_init_notification(status_cb);
	if (ret != 0) {
		goto enable_exit;
	}

	atomic_set_bit(&usbd_ctx.state, USBD_STATE_ENABLED);
	ret = 0;

enable_exit:
	k_mutex_unlock(&usbd_enable_lock);
	return ret;
}
