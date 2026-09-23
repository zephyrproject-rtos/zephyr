/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/usb/usb_hub.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <zephyr/drivers/usb/uhc.h>

#include "usbh_ch9.h"
#include "usbh_class.h"
#include "usbh_desc.h"
#include "usbh_device.h"
#include "usbh_host.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usbh_hub, CONFIG_USBH_LOG_LEVEL);

#define HUB_STATUS_BUF_MAX ((CONFIG_USBH_HUB_MAX_PORTS + 1U + 7U) / 8U)

/*
 * Per downstream port child pointer. hub->ports[] is indexed by port - 1 for
 * 1-based USB hub port numbers.
 */
struct usbh_hub_port {
	struct usb_device *child;
};

struct usbh_hub_data {
	struct usb_device *hub;
	struct usbh_context *ctx;
	struct usb_hub_descriptor desc;
	uint8_t nports;
	uint8_t status_ep;
	uint8_t status_interval;
	uint8_t change_bitmap[HUB_STATUS_BUF_MAX];
	uint8_t change_len;
	struct uhc_transfer *status_xfer;
	struct k_work event_work;
	struct k_mutex status_lock;
	struct usbh_hub_port ports[CONFIG_USBH_HUB_MAX_PORTS];
	bool active;
};

static int hub_status_xfer_cb(struct usb_device *const udev, struct uhc_transfer *const xfer);

static int usbh_hub_class_probe(struct usbh_class_data *const c_data, struct usb_device *const udev,
				const uint8_t iface);

static bool hub_class_already_bound(const struct usb_device *const udev)
{
	STRUCT_SECTION_FOREACH(usbh_class_node, c_node) {
		const struct usbh_class_data *const c_data = c_node->c_data;

		if (c_node->state != USBH_CLASS_STATE_BOUND || c_data->udev != udev) {
			continue;
		}

		if (c_data->api != NULL && c_data->api->probe == usbh_hub_class_probe) {
			return true;
		}
	}

	return false;
}

static int hub_accept_probe(const struct usb_device *const udev, const uint8_t iface)
{
	if (iface == USBH_CLASS_IFNUM_DEVICE) {
		if (udev->dev_desc.bDeviceClass != USB_BCC_HUB) {
			return -ENOTSUP;
		}
	} else if (udev->dev_desc.bDeviceClass != 0) {
		/*
		 * Single-function hubs publish class 0x09 on the device
		 * descriptor; interface-level bind would duplicate the driver
		 * when CONFIG_USBH_HUB_INSTANCES_COUNT > 1.
		 */
		return -ENOTSUP;
	}

	if (hub_class_already_bound(udev)) {
		return -ENOTSUP;
	}

	return 0;
}

static struct usbh_hub_data *hub_data_from_c_data(const struct usbh_class_data *const c_data)
{
	__ASSERT(c_data != NULL && c_data->priv != NULL, "hub class missing private data");

	return c_data->priv;
}

static struct usbh_hub_data *hub_data_from_hub_udev(const struct usb_device *const hub_udev)
{
	STRUCT_SECTION_FOREACH(usbh_class_node, c_node) {
		struct usbh_class_data *const c_data = c_node->c_data;

		if (c_node->state != USBH_CLASS_STATE_BOUND || c_data->udev != hub_udev) {
			continue;
		}

		if (c_data->api != NULL && c_data->api->probe == usbh_hub_class_probe) {
			return hub_data_from_c_data(c_data);
		}
	}

	return NULL;
}

static bool hub_port_in_range(const struct usbh_hub_data *const hub, const unsigned int port)
{
	if (port < 1U || port > CONFIG_USBH_HUB_MAX_PORTS) {
		return false;
	}

	if (hub->nports != 0U && port > hub->nports) {
		return false;
	}

	return true;
}

static const struct usbh_hub_port *hub_port_ro(const struct usbh_hub_data *const hub,
					       const unsigned int port)
{
	if (!hub_port_in_range(hub, port)) {
		return NULL;
	}

	return &hub->ports[port - 1U];
}

static struct usbh_hub_port *hub_port_mut(struct usbh_hub_data *const hub, const unsigned int port)
{
	if (!hub_port_in_range(hub, port)) {
		return NULL;
	}

	return &hub->ports[port - 1U];
}

static struct usb_device *hub_port_child_get_locked(const struct usbh_hub_data *const hub,
						    const unsigned int port)
{
	const struct usbh_hub_port *const entry = hub_port_ro(hub, port);

	if (entry == NULL) {
		return NULL;
	}

	return entry->child;
}

static void hub_port_child_set_locked(struct usbh_hub_data *const hub, const unsigned int port,
				      struct usb_device *const child)
{
	struct usbh_hub_port *const entry = hub_port_mut(hub, port);

	if (entry == NULL) {
		return;
	}

	entry->child = child;
}

static struct usb_device *hub_port_child_get(struct usbh_hub_data *const hub,
					     const unsigned int port)
{
	struct usb_device *child;

	k_mutex_lock(&hub->status_lock, K_FOREVER);
	child = hub_port_child_get_locked(hub, port);
	k_mutex_unlock(&hub->status_lock);

	return child;
}

static void hub_port_child_set(struct usbh_hub_data *const hub, const unsigned int port,
			       struct usb_device *const child)
{
	k_mutex_lock(&hub->status_lock, K_FOREVER);
	hub_port_child_set_locked(hub, port, child);
	k_mutex_unlock(&hub->status_lock);
}

void usbh_hub_child_detached(struct usb_device *parent, const uint8_t hub_port,
			     struct usb_device *const child)
{
	struct usbh_hub_data *const hub = hub_data_from_hub_udev(parent);

	if (hub == NULL || child == NULL) {
		return;
	}

	k_mutex_lock(&hub->status_lock, K_FOREVER);
	if (hub_port_child_get_locked(hub, hub_port) == child) {
		hub_port_child_set_locked(hub, hub_port, NULL);
	}
	k_mutex_unlock(&hub->status_lock);
}

static void hub_port_disconnect(struct usbh_hub_data *const hub, const unsigned int port)
{
	k_mutex_lock(&hub->status_lock, K_FOREVER);
	hub_port_child_set_locked(hub, port, NULL);
	k_mutex_unlock(&hub->status_lock);

	usbh_device_disconnect_by_port(hub->ctx, hub->hub, (uint8_t)port);
}

static bool hub_port_changed(const struct usbh_hub_data *const hub, const unsigned int port)
{
	unsigned long bits = 0U;

	for (uint8_t i = 0U; i < hub->change_len; i++) {
		bits |= ((unsigned long)hub->change_bitmap[i]) << (i * 8U);
	}

	return (bits & BIT(port)) != 0U;
}

static int hub_find_status_ep(const struct usb_device *const udev, const uint8_t iface,
			      uint8_t *const ep_out, uint8_t *const interval_out)
{
	const struct usb_if_descriptor *ifd;
	const struct usb_desc_header *head;
	const void *desc_end;

	ifd = usbh_desc_get_iface(udev, iface);
	if (ifd == NULL) {
		return -ENOENT;
	}

	desc_end = usbh_desc_cfg_end(udev->cfg_desc);
	head = usbh_desc_get_next(ifd, desc_end);
	while (head != NULL) {
		if (head->bDescriptorType == USB_DESC_INTERFACE) {
			break;
		}

		if (head->bDescriptorType == USB_DESC_ENDPOINT &&
		    head->bLength >= sizeof(struct usb_ep_descriptor)) {
			const struct usb_ep_descriptor *const ep = (const void *)head;
			const uint8_t attr = ep->bmAttributes & USB_EP_TRANSFER_TYPE_MASK;

			if (attr == USB_EP_TYPE_INTERRUPT &&
			    USB_EP_DIR_IS_IN(ep->bEndpointAddress)) {
				*ep_out = ep->bEndpointAddress;
				*interval_out = ep->bInterval;
				return 0;
			}
		}

		head = usbh_desc_get_next(head, desc_end);
	}

	return -ENOENT;
}

static void hub_status_xfer_done(struct usbh_hub_data *const hub, struct uhc_transfer *xfer)
{
	if (hub == NULL || hub->ctx == NULL || xfer == NULL) {
		return;
	}

	if (xfer->buf != NULL) {
		uhc_xfer_buf_free(hub->ctx->dev, xfer->buf);
		xfer->buf = NULL;
	}

	(void)uhc_xfer_free(hub->ctx->dev, xfer);
}

static int hub_submit_status_xfer(struct usbh_hub_data *const hub)
{
	struct net_buf *buf;
	int ret;

	if (!hub->active || hub->hub == NULL || hub->status_ep == 0U) {
		return -ENODEV;
	}

	if (hub->status_xfer != NULL) {
		return 0;
	}

	buf = usbh_xfer_buf_alloc(hub->hub, MAX(hub->change_len, 1U));
	if (buf == NULL) {
		return -ENOMEM;
	}

	hub->status_xfer = usbh_xfer_alloc(hub->hub, hub->status_ep, hub_status_xfer_cb, hub);
	if (hub->status_xfer == NULL) {
		usbh_xfer_buf_free(hub->hub, buf);
		return -ENOMEM;
	}

	ret = usbh_xfer_buf_add(hub->hub, hub->status_xfer, buf);
	if (ret != 0) {
		hub_status_xfer_done(hub, hub->status_xfer);
		hub->status_xfer = NULL;
		return ret;
	}

	ret = usbh_xfer_enqueue(hub->hub, hub->status_xfer);
	if (ret != 0) {
		hub_status_xfer_done(hub, hub->status_xfer);
		hub->status_xfer = NULL;
	}

	return ret;
}

static void hub_cancel_status_xfer(struct usbh_hub_data *const hub)
{
	struct uhc_transfer *xfer;
	struct usb_device *udev;

	if (hub->status_xfer == NULL || hub->hub == NULL) {
		return;
	}

	xfer = hub->status_xfer;
	udev = hub->hub;
	hub->status_xfer = NULL;

	if (usbh_xfer_dequeue(udev, xfer) != 0) {
		hub_status_xfer_done(hub, xfer);
	}
}

static int hub_status_xfer_cb(struct usb_device *const udev, struct uhc_transfer *const xfer)
{
	struct usbh_hub_data *const hub = xfer->priv;
	bool resubmit = false;

	if (hub == NULL) {
		return 0;
	}

	k_mutex_lock(&hub->status_lock, K_FOREVER);

	hub->status_xfer = NULL;

	if (!hub->active || hub->hub != udev) {
		hub_status_xfer_done(hub, xfer);
		k_mutex_unlock(&hub->status_lock);
		return 0;
	}

	if (xfer->err == 0 && xfer->buf != NULL && xfer->buf->len > 0U) {
		const size_t copy_len = MIN((size_t)xfer->buf->len, sizeof(hub->change_bitmap));

		memcpy(hub->change_bitmap, xfer->buf->data, copy_len);
		hub->change_len = (uint8_t)copy_len;
		k_work_submit(&hub->event_work);
	} else if (xfer->err != 0 && xfer->err != -ECONNRESET) {
		LOG_WRN("Hub status transfer failed: %d", xfer->err);
	}

	hub_status_xfer_done(hub, xfer);

	if (hub->active && hub->hub != NULL) {
		resubmit = true;
	}

	k_mutex_unlock(&hub->status_lock);

	if (resubmit) {
		(void)hub_submit_status_xfer(hub);
	}

	return 0;
}

static void hub_speed_from_portstatus(const uint16_t portstatus, struct usb_device *const child)
{
	if ((portstatus & USB_PORT_STAT_LOW_SPEED) != 0U) {
		child->speed = USB_SPEED_SPEED_LS;
	} else if ((portstatus & USB_PORT_STAT_HIGH_SPEED) != 0U) {
		child->speed = USB_SPEED_SPEED_HS;
	} else {
		child->speed = USB_SPEED_SPEED_FS;
	}
}

static void hub_port_connect(struct usbh_hub_data *const hub, const unsigned int port)
{
	struct usbh_context *const ctx = hub->hub->ctx;
	struct usb_device *child;
	uint16_t portstatus = 0U;
	uint16_t portchange = 0U;
	int err;

	hub_port_disconnect(hub, port);

	err = usbh_req_get_hcfs_port_status(hub->hub, (uint8_t)port, &portstatus, &portchange);
	if (err != 0) {
		LOG_ERR("Port %u GET_STATUS failed: %d", port, err);
		return;
	}

	if ((portstatus & USB_PORT_STAT_CONNECTION) == 0U) {
		return;
	}

	err = usbh_hub_port_reset(hub->hub, (uint8_t)port, CONFIG_USBH_HUB_PORT_RESET_MS);
	if (err != 0) {
		LOG_ERR("Port %u reset failed: %d", port, err);
		return;
	}

	err = usbh_req_get_hcfs_port_status(hub->hub, (uint8_t)port, &portstatus, &portchange);
	if (err != 0) {
		LOG_ERR("Port %u GET_STATUS after reset failed: %d", port, err);
		return;
	}

	if ((portstatus & USB_PORT_STAT_CONNECTION) == 0U) {
		return;
	}

	child = usbh_device_alloc_port(ctx, hub->hub, (uint8_t)port);
	if (child == NULL) {
		LOG_ERR("Port %u: failed to allocate child device", port);
		return;
	}

	child->state = USB_STATE_DEFAULT;
	hub_speed_from_portstatus(portstatus, child);

	/* Publish child on this port before enumeration completes. */
	hub_port_child_set(hub, port, child);

	LOG_INF("Hub port %u: enumerating child (speed=%u)", port, child->speed);

	err = usbh_device_init(child);
	if (err != 0) {
		LOG_ERR("Port %u: child enumeration failed: %d", port, err);
		hub_port_disconnect(hub, port);
	}
}

static void hub_port_event(struct usbh_hub_data *const hub, const unsigned int port)
{
	struct usb_device *child;
	uint16_t portstatus = 0U;
	uint16_t portchange = 0U;
	int err;

	err = usbh_req_get_hcfs_port_status(hub->hub, (uint8_t)port, &portstatus, &portchange);
	if (err != 0) {
		LOG_ERR("Port %u GET_STATUS failed: %d", port, err);
		return;
	}

	if ((portchange & USB_PORT_STAT_C_CONNECTION) != 0U) {
		(void)usbh_req_clear_hcfs_port_feature(hub->hub, (uint8_t)port,
						       USB_HCFS_C_PORT_CONNECTION);
	}

	if ((portchange & USB_PORT_STAT_C_ENABLE) != 0U) {
		(void)usbh_req_clear_hcfs_port_feature(hub->hub, (uint8_t)port,
						       USB_HCFS_C_PORT_ENABLE);
	}

	if ((portchange & USB_PORT_STAT_C_RESET) != 0U) {
		(void)usbh_req_clear_hcfs_port_feature(hub->hub, (uint8_t)port,
						       USB_HCFS_C_PORT_RESET);
	}

	if ((portchange & USB_PORT_STAT_C_OVERCURRENT) != 0U) {
		(void)usbh_req_clear_hcfs_port_feature(hub->hub, (uint8_t)port,
						       USB_HCFS_C_PORT_OVER_CURRENT);
	}

	/* Child may be a downstream hub or a function device. */
	child = hub_port_child_get(hub, port);

	if ((portstatus & USB_PORT_STAT_CONNECTION) == 0U) {
		if (child != NULL) {
			LOG_INF("Hub port %u: device disconnected", port);
			hub_port_disconnect(hub, port);
		}
		return;
	}

	if (child == NULL || child->state != USB_STATE_CONFIGURED) {
		hub_port_connect(hub, port);
	}
}

static void hub_event_handler(struct k_work *work)
{
	struct usbh_hub_data *const hub = CONTAINER_OF(work, struct usbh_hub_data, event_work);

	if (!hub->active || hub->hub == NULL) {
		return;
	}

	for (unsigned int port = 1U; port <= hub->nports; port++) {
		if (hub_port_changed(hub, port)) {
			hub_port_event(hub, port);
		}
	}
}

static int hub_read_descriptor(struct usbh_hub_data *const hub)
{
	uint8_t raw[USB_DT_HUB_NONVAR_SIZE + 2U * HUB_STATUS_BUF_MAX];
	struct usb_hub_descriptor *hdr = (void *)raw;
	size_t desc_len;
	int err;

	err = usbh_req_desc_hub(hub->hub, USB_DT_HUB_NONVAR_SIZE, hdr);
	if (err != 0) {
		return err;
	}

	if (hdr->bDescriptorType != USB_DT_HUB || hdr->bNbrPorts == 0U ||
	    hdr->bNbrPorts > CONFIG_USBH_HUB_MAX_PORTS) {
		return -EINVAL;
	}

	desc_len = usb_hub_descriptor_size(hdr->bNbrPorts);
	if (desc_len > sizeof(raw)) {
		return -EINVAL;
	}

	err = usbh_req_desc_hub(hub->hub, desc_len, raw);
	if (err != 0) {
		return err;
	}

	(void)memcpy(&hub->desc, raw, USB_DT_HUB_NONVAR_SIZE);
	hub->desc.wHubCharacteristics = sys_le16_to_cpu(hub->desc.wHubCharacteristics);
	hub->nports = hub->desc.bNbrPorts;
	hub->change_len = (uint8_t)((hub->nports + 1U + 7U) / 8U);

	LOG_INF("Hub: %u ports, pwr_on=%u ms", hub->nports, hub->desc.bPwrOn2PwrGood * 2U);

	return 0;
}

static int hub_power_on_ports(struct usbh_hub_data *const hub)
{
	const uint16_t lpsm = hub->desc.wHubCharacteristics & HUB_CHAR_LPSM;
	int err;

	if (lpsm == HUB_CHAR_NO_LPSM) {
		return 0;
	}

	for (unsigned int port = 1U; port <= hub->nports; port++) {
		err = usbh_req_set_hcfs_port_feature(hub->hub, (uint8_t)port, USB_HCFS_PORT_POWER);
		if (err != 0) {
			LOG_WRN("Port %u power on failed: %d", port, err);
		}
	}

	if (hub->desc.bPwrOn2PwrGood > 0U) {
		k_msleep((uint32_t)hub->desc.bPwrOn2PwrGood * 2U);
	}

	return 0;
}

static void hub_scan_existing_ports(struct usbh_hub_data *const hub)
{
	for (unsigned int port = 1U; port <= hub->nports; port++) {
		uint16_t portstatus = 0U;
		uint16_t portchange = 0U;

		if (usbh_req_get_hcfs_port_status(hub->hub, (uint8_t)port, &portstatus,
						  &portchange) != 0) {
			continue;
		}

		if ((portstatus & USB_PORT_STAT_CONNECTION) != 0U) {
			hub_port_connect(hub, port);
		}
	}
}

static int hub_resolve_iface(const struct usb_device *const udev, const uint8_t iface,
			     uint8_t *const hub_iface)
{
	const struct usb_cfg_descriptor *const cfg = udev->cfg_desc;

	if (iface != USBH_CLASS_IFNUM_DEVICE) {
		const struct usb_if_descriptor *ifd = usbh_desc_get_iface(udev, iface);

		if (ifd == NULL || ifd->bInterfaceClass != USB_BCC_HUB) {
			return -ENOENT;
		}

		*hub_iface = iface;
		return 0;
	}

	for (uint8_t i = 0U; i < cfg->bNumInterfaces; i++) {
		const struct usb_if_descriptor *ifd = usbh_desc_get_iface(udev, i);

		if (ifd != NULL && ifd->bInterfaceClass == USB_BCC_HUB) {
			*hub_iface = i;
			return 0;
		}
	}

	return -ENOENT;
}

static int usbh_hub_class_probe(struct usbh_class_data *const c_data, struct usb_device *const udev,
				const uint8_t iface)
{
	struct usbh_hub_data *const hub = hub_data_from_c_data(c_data);
	uint8_t hub_iface;
	uint8_t status_ep;
	uint8_t interval = 0U;
	int err;

	if (udev == NULL || udev->ctx == NULL) {
		return -EINVAL;
	}

	err = hub_accept_probe(udev, iface);
	if (err != 0) {
		return err;
	}

	if (hub->active) {
		return -EBUSY;
	}

	err = hub_resolve_iface(udev, iface, &hub_iface);
	if (err != 0) {
		return err;
	}

	err = hub_find_status_ep(udev, hub_iface, &status_ep, &interval);
	if (err != 0) {
		LOG_ERR("Hub status endpoint not found: %d", err);
		return err;
	}

	memset(hub, 0, sizeof(*hub));
	k_mutex_init(&hub->status_lock);
	hub->hub = udev;
	hub->ctx = udev->ctx;
	hub->status_ep = status_ep;
	hub->status_interval = interval;
	k_work_init(&hub->event_work, hub_event_handler);

	err = hub_read_descriptor(hub);
	if (err != 0) {
		LOG_ERR("Failed to read hub descriptor: %d", err);
		return err;
	}

	err = hub_power_on_ports(hub);
	if (err != 0) {
		LOG_WRN("Hub port power failed: %d", err);
	}

	hub->active = true;

	err = hub_submit_status_xfer(hub);
	if (err != 0) {
		LOG_ERR("Failed to start hub status polling: %d", err);
		hub->active = false;
		return err;
	}

	hub_scan_existing_ports(hub);

	return 0;
}

static int usbh_hub_class_removed(struct usbh_class_data *const c_data)
{
	struct usbh_hub_data *const hub = hub_data_from_c_data(c_data);
	struct usb_device *const hub_udev = c_data->udev;
	struct k_work_sync sync;

	if (!hub->active || hub_udev == NULL) {
		return 0;
	}

	k_mutex_lock(&hub->status_lock, K_FOREVER);
	hub->active = false;
	(void)k_work_cancel_sync(&hub->event_work, &sync);
	hub_cancel_status_xfer(hub);
	k_mutex_unlock(&hub->status_lock);
	/* Wait for an in-flight status completion callback (runs on usbh thread). */
	k_mutex_lock(&hub->status_lock, K_FOREVER);
	k_mutex_unlock(&hub->status_lock);

	for (unsigned int port = 1U; port <= hub->nports; port++) {
		if (hub_port_child_get(hub, port) != NULL) {
			hub_port_disconnect(hub, port);
		}
	}

	memset(hub, 0, sizeof(*hub));

	return 0;
}

static struct usbh_class_api usbh_hub_class_api = {
	.probe = usbh_hub_class_probe,
	.removed = usbh_hub_class_removed,
};

static struct usbh_class_filter usbh_hub_class_filters[] = {
	{
		.flags = USBH_CLASS_MATCH_CODE_TRIPLE,
		.class = USB_BCC_HUB,
		.sub = 0,
		.proto = USB_HUB_PR_FS,
	},
	{
		.flags = USBH_CLASS_MATCH_CODE_TRIPLE,
		.class = USB_BCC_HUB,
		.sub = 0,
		.proto = USB_HUB_PR_HS_SINGLE_TT,
	},
	{
		.flags = USBH_CLASS_MATCH_CODE_TRIPLE,
		.class = USB_BCC_HUB,
		.sub = 0,
		.proto = USB_HUB_PR_HS_MULTI_TT,
	},
	{
		.flags = USBH_CLASS_MATCH_CODE_TRIPLE,
		.class = USB_BCC_HUB,
		.sub = 0,
		.proto = USB_HUB_PR_SS,
	},
	{0},
};

#define USBH_HUB_INSTANCE_DEFINE(n, _) \
	static struct usbh_hub_data usbh_hub_data_##n; \
 \
	USBH_DEFINE_CLASS(usbh_hub_class_##n, &usbh_hub_class_api, &usbh_hub_data_##n, \
			  usbh_hub_class_filters)

LISTIFY(CONFIG_USBH_HUB_INSTANCES_COUNT, USBH_HUB_INSTANCE_DEFINE, (;), _)
