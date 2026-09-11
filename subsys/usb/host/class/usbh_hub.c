/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * USB host hub class, USB 2.0 specification chapter 11.
 *
 * The status change endpoint of the hub is polled with an interrupt IN
 * transfer. Its completion, reported on the host stack thread, only records
 * the changed ports and schedules the hub work. The work runs on a dedicated
 * work queue because handling a port change requires synchronous control
 * requests, whose completions are handled by the host stack thread.
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/usb/class/usb_hub.h>

#include <usbh_ch9.h>
#include <usbh_class.h>
#include <usbh_device.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usbh_hub, CONFIG_USBH_HUB_LOG_LEVEL);

/* Connection debounce interval, spec. 7.1.7.3 */
#define HUB_DEBOUNCE_MS       100U
/* Interval between two port status reads while waiting for reset completion */
#define HUB_RESET_POLL_MS     10U
/* Maximum time to wait for the hub to complete a port reset, spec. 11.5.1.5 */
#define HUB_RESET_TIMEOUT_MS  500U
/* Reset recovery interval, spec. 7.1.7.5 */
#define HUB_RESET_RECOVERY_MS 10U
/* Size of the hub descriptor buffer, spec. 11.23.2.1 */
#define HUB_DESC_BUF_SIZE     (sizeof(struct usb_hub_descriptor) + 2U * 32U)

struct usbh_hub_data {
	struct usbh_class_data *c_data;
	struct usbh_context *ctx;
	struct usb_device *udev;
	struct usb_device *children[CONFIG_USBH_HUB_MAX_PORTS + 1];
	struct k_work work;
	/* Protects int_xfer, int_queued and active */
	struct k_mutex lock;
	struct uhc_transfer *int_xfer;
	atomic_t changes;
	uint8_t ep_in;
	uint8_t num_ports;
	bool int_queued;
	bool active;
};

static K_KERNEL_STACK_DEFINE(hub_wq_stack, CONFIG_USBH_HUB_STACK_SIZE);
static struct k_work_q hub_wq;

static int hub_int_queue(struct usbh_hub_data *const hub)
{
	int ret;

	/* Assumes that hub->lock is locked */
	if (!hub->active || hub->int_xfer == NULL || hub->int_queued) {
		return 0;
	}

	net_buf_reset(hub->int_xfer->buf);
	ret = usbh_xfer_enqueue(hub->udev, hub->int_xfer);
	if (ret != 0) {
		LOG_ERR("Failed to queue status change transfer %d", ret);
		return ret;
	}

	hub->int_queued = true;

	return 0;
}

static void hub_int_free(struct usbh_hub_data *const hub, struct uhc_transfer *const xfer)
{
	/* Do not use the hub udev here, it may already be freed */
	if (xfer->buf != NULL) {
		uhc_xfer_buf_free(hub->ctx->dev, xfer->buf);
	}

	uhc_xfer_free(hub->ctx->dev, xfer);
}

static int hub_int_cb(struct usb_device *const udev, struct uhc_transfer *const xfer)
{
	struct usbh_hub_data *const hub = xfer->priv;
	uint32_t changes = 0U;

	ARG_UNUSED(udev);

	(void)k_mutex_lock(&hub->lock, K_FOREVER);

	hub->int_queued = false;

	if (!hub->active) {
		hub_int_free(hub, xfer);
		hub->int_xfer = NULL;
		k_mutex_unlock(&hub->lock);
		return 0;
	}

	if (xfer->err != 0) {
		LOG_WRN("Status change transfer failed %d", xfer->err);
		/* Re-queue and wait for the next change or the hub removal */
		(void)hub_int_queue(hub);
		k_mutex_unlock(&hub->lock);
		return 0;
	}

	for (size_t i = 0U; i < MIN(xfer->buf->len, sizeof(changes)); i++) {
		changes |= (uint32_t)xfer->buf->data[i] << (8U * i);
	}

	k_mutex_unlock(&hub->lock);

	LOG_DBG("Status change 0x%08x", changes);
	(void)atomic_or(&hub->changes, changes);
	(void)k_work_submit_to_queue(&hub_wq, &hub->work);

	return 0;
}

static enum usb_device_speed hub_port_speed(const uint16_t status)
{
	if ((status & USB_HUB_PORT_STAT_LOW_SPEED) != 0U) {
		return USB_SPEED_SPEED_LS;
	}

	if ((status & USB_HUB_PORT_STAT_HIGH_SPEED) != 0U) {
		return USB_SPEED_SPEED_HS;
	}

	return USB_SPEED_SPEED_FS;
}

static void hub_port_disconnect(struct usbh_hub_data *const hub, const uint8_t port)
{
	struct usb_device *const child = hub->children[port];

	if (child == NULL) {
		return;
	}

	LOG_INF("Device address %u removed from port %u", child->addr, port);
	hub->children[port] = NULL;
	usbh_device_disconnect(hub->ctx, child);
}

static int hub_port_reset(struct usbh_hub_data *const hub, const uint8_t port,
			  uint16_t *const status)
{
	uint16_t change;
	int ret;

	ret = usbh_req_set_hcfs_prst(hub->udev, port);
	if (ret != 0) {
		LOG_ERR("Failed to reset port %u %d", port, ret);
		return ret;
	}

	for (uint32_t t = 0U; t < HUB_RESET_TIMEOUT_MS; t += HUB_RESET_POLL_MS) {
		k_msleep(HUB_RESET_POLL_MS);

		ret = usbh_req_get_hcs(hub->udev, port, status, &change);
		if (ret != 0) {
			return ret;
		}

		if ((change & USB_HUB_PORT_CHANGE_RESET) != 0U) {
			ret = usbh_req_clear_hcfs(hub->udev, port, USB_HCFS_C_PORT_RESET);
			if (ret != 0) {
				return ret;
			}

			if ((*status & USB_HUB_PORT_STAT_CONNECTION) == 0U) {
				return -ENODEV;
			}

			if ((*status & USB_HUB_PORT_STAT_ENABLE) == 0U) {
				LOG_ERR("Port %u not enabled after reset", port);
				return -EIO;
			}

			k_msleep(HUB_RESET_RECOVERY_MS);

			return 0;
		}
	}

	LOG_ERR("Port %u reset timeout", port);

	return -ETIMEDOUT;
}

static void hub_port_connect(struct usbh_hub_data *const hub, const uint8_t port)
{
	struct usb_device *child;
	uint16_t status, change;
	int ret;

	k_msleep(HUB_DEBOUNCE_MS);

	ret = usbh_req_get_hcs(hub->udev, port, &status, &change);
	if (ret != 0) {
		return;
	}

	if ((status & USB_HUB_PORT_STAT_CONNECTION) == 0U ||
	    (change & USB_HUB_PORT_CHANGE_CONNECTION) != 0U) {
		/* Unstable connection, handled with the next change */
		LOG_DBG("Port %u connection not stable", port);
		return;
	}

	ret = hub_port_reset(hub, port, &status);
	if (ret != 0) {
		return;
	}

	child = usbh_device_alloc(hub->ctx);
	if (child == NULL) {
		LOG_ERR("Failed to allocate device for port %u", port);
		return;
	}

	child->speed = hub_port_speed(status);
	LOG_INF("New device on port %u, speed %u", port, child->speed);

	ret = usbh_device_connect(hub->ctx, child);
	if (ret != 0) {
		LOG_ERR("Failed to connect device on port %u %d", port, ret);
		return;
	}

	hub->children[port] = child;
}

static void hub_port_event(struct usbh_hub_data *const hub, const uint8_t port)
{
	uint16_t status, change;
	int ret;

	ret = usbh_req_get_hcs(hub->udev, port, &status, &change);
	if (ret != 0) {
		LOG_ERR("Failed to get port %u status %d", port, ret);
		return;
	}

	LOG_DBG("Port %u status 0x%04x change 0x%04x", port, status, change);

	if ((change & USB_HUB_PORT_CHANGE_CONNECTION) != 0U) {
		ret = usbh_req_clear_hcfs(hub->udev, port, USB_HCFS_C_PORT_CONNECTION);
		if (ret != 0) {
			return;
		}

		hub_port_disconnect(hub, port);

		if ((status & USB_HUB_PORT_STAT_CONNECTION) != 0U) {
			hub_port_connect(hub, port);
		}
	}

	if ((change & USB_HUB_PORT_CHANGE_ENABLE) != 0U) {
		(void)usbh_req_clear_hcfs(hub->udev, port, USB_HCFS_C_PORT_ENABLE);
		if ((status & USB_HUB_PORT_STAT_ENABLE) == 0U) {
			LOG_WRN("Port %u disabled by the hub", port);
			hub_port_disconnect(hub, port);
		}
	}

	if ((change & USB_HUB_PORT_CHANGE_SUSPEND) != 0U) {
		(void)usbh_req_clear_hcfs(hub->udev, port, USB_HCFS_C_PORT_SUSPEND);
	}

	if ((change & USB_HUB_PORT_CHANGE_OVER_CURRENT) != 0U) {
		(void)usbh_req_clear_hcfs(hub->udev, port, USB_HCFS_C_PORT_OVER_CURRENT);
		LOG_WRN("Port %u over-current change, status 0x%04x", port, status);
	}

	if ((change & USB_HUB_PORT_CHANGE_RESET) != 0U) {
		(void)usbh_req_clear_hcfs(hub->udev, port, USB_HCFS_C_PORT_RESET);
	}
}

static void hub_status_event(struct usbh_hub_data *const hub)
{
	uint16_t status, change;
	int ret;

	ret = usbh_req_get_hcs(hub->udev, 0, &status, &change);
	if (ret != 0) {
		LOG_ERR("Failed to get hub status %d", ret);
		return;
	}

	LOG_DBG("Hub status 0x%04x change 0x%04x", status, change);

	if ((change & USB_HUB_CHANGE_LOCAL_POWER) != 0U) {
		(void)usbh_req_clear_hcfs(hub->udev, 0, USB_HCFS_C_HUB_LOCAL_POWER);
	}

	if ((change & USB_HUB_CHANGE_OVER_CURRENT) != 0U) {
		(void)usbh_req_clear_hcfs(hub->udev, 0, USB_HCFS_C_HUB_OVER_CURRENT);
		LOG_WRN("Hub over-current change, status 0x%04x", status);
	}
}

static void hub_work_handler(struct k_work *const work)
{
	struct usbh_hub_data *const hub = CONTAINER_OF(work, struct usbh_hub_data, work);
	uint32_t changes = atomic_clear(&hub->changes);

	if (!hub->active) {
		return;
	}

	if (IS_BIT_SET(changes, 0)) {
		hub_status_event(hub);
	}

	for (uint8_t port = 1U; port <= hub->num_ports; port++) {
		if (!hub->active) {
			return;
		}

		if (IS_BIT_SET(changes, port)) {
			hub_port_event(hub, port);
		}
	}

	(void)k_mutex_lock(&hub->lock, K_FOREVER);
	(void)hub_int_queue(hub);
	k_mutex_unlock(&hub->lock);
}

static int hub_read_descriptor(struct usbh_hub_data *const hub)
{
	struct usb_hub_descriptor desc;
	struct net_buf *buf;
	int ret;

	buf = usbh_xfer_buf_alloc(hub->udev, HUB_DESC_BUF_SIZE);
	if (buf == NULL) {
		return -ENOMEM;
	}

	ret = usbh_req_desc_hub(hub->udev, HUB_DESC_BUF_SIZE, buf);
	if (ret == 0 && buf->len < sizeof(desc)) {
		ret = -EIO;
	}

	if (ret != 0) {
		LOG_ERR("Failed to read hub descriptor %d", ret);
		goto out;
	}

	memcpy(&desc, buf->data, sizeof(desc));
	desc.wHubCharacteristics = sys_le16_to_cpu(desc.wHubCharacteristics);

	if (desc.bDescriptorType != USB_DESC_HUB || desc.bNbrPorts == 0U) {
		LOG_ERR("Invalid hub descriptor, type 0x%02x ports %u",
			desc.bDescriptorType, desc.bNbrPorts);
		ret = -EINVAL;
		goto out;
	}

	hub->num_ports = (uint8_t)MIN(desc.bNbrPorts, CONFIG_USBH_HUB_MAX_PORTS);
	if (hub->num_ports != desc.bNbrPorts) {
		LOG_WRN("Only %u of %u ports are used", hub->num_ports, desc.bNbrPorts);
	}

	LOG_INF("Hub with %u ports, characteristics 0x%04x, power on to power good %u ms",
		desc.bNbrPorts, desc.wHubCharacteristics, desc.bPwrOn2PwrGood * 2U);

	/* Power on all ports and wait for the power to be good */
	for (uint8_t port = 1U; port <= hub->num_ports; port++) {
		ret = usbh_req_set_hcfs_ppwr(hub->udev, port);
		if (ret != 0) {
			LOG_ERR("Failed to power port %u %d", port, ret);
			goto out;
		}
	}

	k_msleep(desc.bPwrOn2PwrGood * 2U);

out:
	usbh_xfer_buf_free(hub->udev, buf);

	return ret;
}

static int hub_find_int_ep(struct usbh_hub_data *const hub)
{
	for (uint8_t i = 1U; i < ARRAY_SIZE(hub->udev->ep_in); i++) {
		const struct usb_ep_descriptor *const desc = hub->udev->ep_in[i].desc;

		if (desc != NULL &&
		    (desc->bmAttributes & USB_EP_TRANSFER_TYPE_MASK) == USB_EP_TYPE_INTERRUPT) {
			hub->ep_in = desc->bEndpointAddress;
			return 0;
		}
	}

	return -ENOENT;
}

static int usbh_hub_probe(struct usbh_class_data *const c_data,
			  struct usb_device *const udev, const uint8_t iface)
{
	struct usbh_hub_data *const hub = c_data->priv;
	struct net_buf *buf;
	int ret;

	/*
	 * A hub is a single function device, bind at the device level only so
	 * that a second instance does not bind to its interface as well.
	 */
	if ((iface != USBH_CLASS_IFNUM_DEVICE) || (udev->dev_desc.bDeviceClass != USB_BCC_HUB)) {
		return -ENOTSUP;
	}

	hub->ctx = udev->ctx;
	hub->udev = udev;
	hub->int_xfer = NULL;
	hub->int_queued = false;
	atomic_clear(&hub->changes);
	memset(hub->children, 0, sizeof(hub->children));

	ret = hub_find_int_ep(hub);
	if (ret != 0) {
		LOG_ERR("No status change endpoint");
		return -ENOTSUP;
	}

	ret = hub_read_descriptor(hub);
	if (ret != 0) {
		return ret;
	}

	hub->int_xfer = usbh_xfer_alloc(udev, hub->ep_in, hub_int_cb, hub);
	if (hub->int_xfer == NULL) {
		return -ENOMEM;
	}

	buf = usbh_xfer_buf_alloc(udev, hub->int_xfer->mps);
	if (buf == NULL) {
		usbh_xfer_free(udev, hub->int_xfer);
		hub->int_xfer = NULL;
		return -ENOMEM;
	}

	ret = usbh_xfer_buf_add(udev, hub->int_xfer, buf);
	if (ret != 0) {
		usbh_xfer_buf_free(udev, buf);
		usbh_xfer_free(udev, hub->int_xfer);
		hub->int_xfer = NULL;
		return ret;
	}

	(void)k_mutex_lock(&hub->lock, K_FOREVER);
	hub->active = true;
	ret = hub_int_queue(hub);
	k_mutex_unlock(&hub->lock);
	if (ret != 0) {
		hub->active = false;
		hub_int_free(hub, hub->int_xfer);
		hub->int_xfer = NULL;
		return ret;
	}

	/* Scan all ports once, devices may already be connected */
	(void)atomic_or(&hub->changes, GENMASK(hub->num_ports, 1));
	(void)k_work_submit_to_queue(&hub_wq, &hub->work);

	LOG_INF("Hub at address %u bound, status change endpoint 0x%02x",
		udev->addr, hub->ep_in);

	return 0;
}

static int usbh_hub_removed(struct usbh_class_data *const c_data)
{
	struct usbh_hub_data *const hub = c_data->priv;
	struct k_work_sync sync;

	(void)k_mutex_lock(&hub->lock, K_FOREVER);
	hub->active = false;
	k_mutex_unlock(&hub->lock);

	/* Wait for a running port event handler, it is bound to fail quickly */
	(void)k_work_cancel_sync(&hub->work, &sync);

	(void)k_mutex_lock(&hub->lock, K_FOREVER);
	if (hub->int_xfer != NULL) {
		if (hub->int_queued) {
			/* The transfer is returned and freed by the completion callback */
			(void)usbh_xfer_dequeue(hub->udev, hub->int_xfer);
		} else {
			hub_int_free(hub, hub->int_xfer);
			hub->int_xfer = NULL;
		}
	}
	k_mutex_unlock(&hub->lock);

	for (uint8_t port = 1U; port <= hub->num_ports; port++) {
		hub_port_disconnect(hub, port);
	}

	LOG_INF("Hub at address %u removed", hub->udev->addr);
	hub->udev = NULL;

	return 0;
}

static int usbh_hub_init(struct usbh_class_data *const c_data)
{
	struct usbh_hub_data *const hub = c_data->priv;

	static bool wq_started;

	if (!wq_started) {
		static const struct k_work_queue_config wq_cfg = {.name = "usbh_hub"};

		k_work_queue_start(&hub_wq, hub_wq_stack, K_KERNEL_STACK_SIZEOF(hub_wq_stack),
				   K_PRIO_COOP(9), &wq_cfg);
		wq_started = true;
	}

	hub->c_data = c_data;
	k_mutex_init(&hub->lock);
	k_work_init(&hub->work, hub_work_handler);

	return 0;
}

static struct usbh_class_api usbh_hub_class_api = {
	.init = usbh_hub_init,
	.probe = usbh_hub_probe,
	.removed = usbh_hub_removed,
};

/* Full-speed hub, high-speed hub with single TT, high-speed hub with multiple TTs */
static struct usbh_class_filter usbh_hub_filters[] = {
	{.flags = USBH_CLASS_MATCH_CODE_TRIPLE, .class = USB_BCC_HUB, .sub = 0U, .proto = 0U},
	{.flags = USBH_CLASS_MATCH_CODE_TRIPLE, .class = USB_BCC_HUB, .sub = 0U, .proto = 1U},
	{.flags = USBH_CLASS_MATCH_CODE_TRIPLE, .class = USB_BCC_HUB, .sub = 0U, .proto = 2U},
	{0},
};

#define USBH_HUB_DEFINE(n, _)								\
	static struct usbh_hub_data usbh_hub_data_##n;					\
	USBH_DEFINE_CLASS(usbh_hub_##n, &usbh_hub_class_api,				\
			  &usbh_hub_data_##n, usbh_hub_filters);

LISTIFY(CONFIG_USBH_HUB_INSTANCES_COUNT, USBH_HUB_DEFINE, (;), _)
