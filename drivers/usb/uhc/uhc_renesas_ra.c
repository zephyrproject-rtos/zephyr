/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_ra_uhc

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/dlist.h>
#include <zephyr/sys/util.h>
#include <r_usb_host.h>

#include "uhc_common.h"

LOG_MODULE_REGISTER(uhc_renesas_ra, CONFIG_UHC_DRIVER_LOG_LEVEL);

enum uhc_renesas_ra_event_type {
	/* Shim driver event to trigger next transfer */
	UHC_RENESAS_RA_EVT_XFER,
	/* Device speed check, typically performed after a RESET signal is issued */
	UHC_RENESAS_RA_EVT_POLL_PORT_SPEED,
	/* Submit a xfer after complete */
	UHC_RENESAS_RA_EVT_XFER_SUBMIT,
	/* Cancel a queued/in-flight xfer on the given endpoint */
	UHC_RENESAS_RA_EVT_DEQUEUE,
	/* Device attach event, deferred from ISR context */
	UHC_RENESAS_RA_EVT_DEVICE_ATTACH,
	/* Device detach event, deferred from ISR context */
	UHC_RENESAS_RA_EVT_DEVICE_DETACH,
};

struct uhc_renesas_ra_evt {
	enum uhc_renesas_ra_event_type type;
	usbh_event_t hal_event;
	uint8_t dev_addr;
	uint8_t ep;
};

struct uhc_renesas_ra_device {
	struct usb_device device;
	bool configured;
};

struct uhc_renesas_ra_pipe {
	sys_dlist_t xfers_list;
};

struct uhc_renesas_ra_endpoint {
	uint8_t pipenum;
};

struct uhc_renesas_ra_data {
	struct k_thread thread_data;
	struct k_msgq msgq;
	struct st_usbh_instance_ctrl uhc_ctrl;
	struct st_usb_cfg uhc_cfg;
	/*
	 * Indexed by (device address - 1): device addresses range from 1 to 10
	 * inclusive, and address 0 (the default address) is handled separately.
	 */
	struct uhc_renesas_ra_device usbd_device[10];
	struct uhc_renesas_ra_pipe dcp;
	struct uhc_renesas_ra_pipe pipe[10];
	struct uhc_renesas_ra_endpoint ep[10][16][2];
	usb_speed_t speed;
};

struct uhc_renesas_ra_config {
	const struct pinctrl_dev_config *pcfg;
	k_thread_stack_t *drv_stack;
	size_t drv_stack_size;
};

extern void r_usbh_isr(void);

static void uhc_renesas_ra_interrupt_handler(const void *arg)
{
	ARG_UNUSED(arg);
	r_usbh_isr();
}

static int uhc_renesas_ra_xfer_request(const struct device *dev, uint8_t dev_addr, uint8_t ep)
{
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);
	struct uhc_renesas_ra_evt event = {
		.type = UHC_RENESAS_RA_EVT_XFER,
		.dev_addr = dev_addr,
		.ep = ep,
	};

	return k_msgq_put(&priv->msgq, &event, K_NO_WAIT);
}

static int uhc_renesas_ra_event_poll_port_speed(const struct device *dev)
{
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);
	struct uhc_renesas_ra_evt evt = {
		.type = UHC_RENESAS_RA_EVT_POLL_PORT_SPEED,
	};

	return k_msgq_put(&priv->msgq, &evt, K_NO_WAIT);
}

static int uhc_renesas_ra_lock(const struct device *dev)
{
	return uhc_lock_internal(dev, K_FOREVER);
}

static int uhc_renesas_ra_unlock(const struct device *dev)
{
	return uhc_unlock_internal(dev);
}

static int uhc_renesas_ra_data_send(const struct device *dev, struct uhc_transfer *const xfer)
{
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);
	struct net_buf *buf = xfer->buf;
	int err;

	err = R_USBH_XferStart(&priv->uhc_ctrl, xfer->udev->addr, xfer->ep, buf->data, buf->len);
	if (err != FSP_SUCCESS) {
		LOG_ERR("ep 0x%02x state data error", xfer->ep);
		return -EIO;
	}

	return 0;
}

static int uhc_renesas_ra_data_receive(const struct device *dev, struct uhc_transfer *const xfer)
{
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);
	struct net_buf *buf = xfer->buf;
	size_t len = net_buf_tailroom(buf);
	void *buffer_tail = net_buf_tail(buf);
	int err;

	err = R_USBH_XferStart(&priv->uhc_ctrl, xfer->udev->addr, xfer->ep, buffer_tail, len);
	if (err != FSP_SUCCESS) {
		LOG_ERR("ep 0x%02x state status error", xfer->ep);
		return -EIO;
	}

	return 0;
}

static int uhc_renesas_ra_data_xfer(const struct device *dev, struct uhc_transfer *const xfer)
{
	if (USB_EP_DIR_IS_IN(xfer->ep)) {
		return uhc_renesas_ra_data_receive(dev, xfer);
	}

	return uhc_renesas_ra_data_send(dev, xfer);
}

static int uhc_renesas_ra_control_xfer(const struct device *dev, struct uhc_transfer *const xfer)
{
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);
	fsp_err_t err;
	int ret = 0;

	switch (xfer->stage) {
	case UHC_CONTROL_STAGE_SETUP:
		err = R_USBH_SetupSend(&priv->uhc_ctrl, xfer->udev->addr, xfer->setup_pkt);
		if (err != FSP_SUCCESS) {
			LOG_ERR("ep 0x%02x state setup error", xfer->ep);
			ret = -EIO;
		}
		break;
	case UHC_CONTROL_STAGE_DATA:
		ret = uhc_renesas_ra_data_xfer(dev, xfer);
		break;
	case UHC_CONTROL_STAGE_STATUS:
		err = R_USBH_XferStart(&priv->uhc_ctrl, xfer->udev->addr,
				       (xfer->ep ^ USB_EP_DIR_MASK), NULL, 0);
		if (err != FSP_SUCCESS) {
			ret = -EIO;
		}
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static uint32_t get_pipe_num(const struct device *dev, uint8_t dev_addr, uint8_t ep)
{
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);

	return priv->ep[dev_addr - 1][USB_EP_GET_IDX(ep)][!!USB_EP_GET_DIR(ep)].pipenum;
}

static bool is_configured_ep(const struct device *dev, uint8_t dev_addr, uint8_t ep)
{
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);

	if (USB_EP_GET_IDX(ep) == 0) {
		return true;
	}

	if (priv->ep[dev_addr - 1][USB_EP_GET_IDX(ep)][!!USB_EP_GET_DIR(ep)].pipenum != 0) {
		return true;
	}

	return false;
}

static void uhc_renesas_ra_transfer_append(const struct device *dev,
					   struct uhc_transfer *const xfer)
{
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);

	if (USB_EP_GET_IDX(xfer->ep) == 0) {
		sys_dlist_append(&priv->dcp.xfers_list, &xfer->node);
		return;
	}

	sys_dlist_append(&priv->pipe[get_pipe_num(dev, xfer->udev->addr, xfer->ep)].xfers_list,
			 &xfer->node);
}

static struct uhc_transfer *uhc_renesas_ra_transfer_get_next(const struct device *dev,
							     uint8_t dev_addr, uint8_t ep)
{
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);
	struct uhc_transfer *xfer = NULL;
	sys_dnode_t *node = NULL;

	if (USB_EP_GET_IDX(ep) == 0) {
		node = sys_dlist_peek_head(&priv->dcp.xfers_list);
	} else {
		node = sys_dlist_peek_head(&priv->pipe[get_pipe_num(dev, dev_addr, ep)].xfers_list);
	}

	xfer = (node == NULL) ? NULL : SYS_DLIST_CONTAINER(node, xfer, node);

	return xfer;
}

static int uhc_renesas_ra_schedule_xfer(const struct device *dev, uint8_t dev_addr, uint8_t ep)
{
	struct uhc_transfer *xfer;

	xfer = uhc_renesas_ra_transfer_get_next(dev, dev_addr, ep);
	if (xfer == NULL) {
		return -ENOMEM;
	}

	if (USB_EP_GET_IDX(xfer->ep) == 0) {
		/*
		 * If there is a control xfer, the driver should maintaince control xfer stage
		 * update properly to not conrrupt the control transfer seq
		 */
		return uhc_renesas_ra_control_xfer(dev, xfer);
	}

	return uhc_renesas_ra_data_xfer(dev, xfer);
}

static void uhc_control_stage_update(const struct device *dev, usbh_event_t *hal_evt,
				     struct uhc_transfer *xfer)
{
	switch (xfer->stage) {
	case UHC_CONTROL_STAGE_SETUP: {
		if (xfer->buf != NULL) {
			/* S-[in]-status or S-[out]-status */
			xfer->stage = UHC_CONTROL_STAGE_DATA;
			uhc_renesas_ra_xfer_request(dev, xfer->udev->addr, xfer->ep);
		} else {
			/* S-[status] */
			xfer->stage = UHC_CONTROL_STAGE_STATUS;
			uhc_renesas_ra_xfer_request(dev, xfer->udev->addr, xfer->ep);
		}
		break;
	}
	case UHC_CONTROL_STAGE_DATA:
		/* S-in-[status] or S-out-[status] */
		if (xfer->ep == USB_CONTROL_EP_IN) {
			net_buf_add(xfer->buf, hal_evt->complete.len);
		}
		xfer->stage = UHC_CONTROL_STAGE_STATUS;
		uhc_renesas_ra_xfer_request(dev, xfer->udev->addr, xfer->ep);
		break;
	case UHC_CONTROL_STAGE_STATUS:
		/* Transfer is completed */
		uhc_xfer_return(dev, xfer, 0);
		break;
	default:
		break;
	}
}

static bool is_configured_udev(const struct device *dev, uint8_t device_addr)
{
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);

	if (device_addr == 0) {
		return true;
	}

	if (device_addr - 1 >= ARRAY_SIZE(priv->usbd_device) ||
	    !priv->usbd_device[device_addr - 1].configured) {
		return false;
	}

	return true;
}

static int uhc_renesas_ra_configure_udev(const struct device *dev, struct usb_device *udev,
					 uint8_t mxps0)
{
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);
	usb_speed_t speed;
	fsp_err_t err;
	int ret = 0;

	switch (udev->speed) {
	case USB_SPEED_SPEED_LS:
		speed = USB_SPEED_LS;
		break;
	case USB_SPEED_SPEED_FS:
		speed = USB_SPEED_FS;
		break;
	case USB_SPEED_SPEED_HS:
		speed = USB_SPEED_HS;
		break;
	default:
		LOG_DBG("Device speed %d is not supported by controller", udev->speed);
		return -ENOTSUP;
	}

	/*
	 * TODO: hub_addr/hub_port are hardcoded to 0 (root port) as a temporary
	 * workaround until struct usb_device gains a hub topology field, which
	 * split transactions to devices behind a high-speed hub need.
	 */
	err = R_USBH_PortOpen(&priv->uhc_ctrl, udev->addr, speed, mxps0, 0, 0);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	priv->usbd_device[udev->addr - 1].device = *udev;
	priv->usbd_device[udev->addr - 1].configured = true;

	return ret;
}

static int uhc_renesas_ra_open_pipe(const struct device *dev, uint8_t dev_addr, uint8_t ep,
				    struct usb_ep_descriptor *desc)
{
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);
	uint8_t pipe_num;
	fsp_err_t err;
	int ret;

	err = R_USBH_EdptOpen(&priv->uhc_ctrl, dev_addr, (usb_desc_endpoint_t *)desc, &pipe_num);
	if (err == FSP_SUCCESS) {
		ret = 0;
	} else if (err == FSP_ERR_USB_BUSY) {
		ret = -EBUSY;
	} else {
		ret = -EIO;
	}

	if (ret == 0) {
		priv->ep[dev_addr - 1][USB_EP_GET_IDX(ep)][!!USB_EP_GET_DIR(ep)].pipenum = pipe_num;
	}

	return ret;
}

static int uhc_renesas_ra_open_dcp(const struct device *dev, struct uhc_transfer *const xfer)
{
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);
	usb_speed_t speed;
	fsp_err_t err;

	switch (xfer->udev->speed) {
	case USB_SPEED_SPEED_LS:
		speed = USB_SPEED_LS;
		break;
	case USB_SPEED_SPEED_FS:
		speed = USB_SPEED_FS;
		break;
	case USB_SPEED_SPEED_HS:
		speed = USB_SPEED_HS;
		break;
	default:
		return -ENOTSUP;
	}

	/*
	 * TODO: hub_addr/hub_port are hardcoded to 0 (root port) as a temporary
	 * workaround until struct usb_device gains a hub topology field, which
	 * split transactions to devices behind a high-speed hub need.
	 */
	err = R_USBH_PortOpen(&priv->uhc_ctrl, xfer->udev->addr, speed, xfer->mps, 0, 0);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static int uhc_renesas_ra_ep_enqueue(const struct device *dev, struct uhc_transfer *const xfer)
{
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);
	int ret;

	if (xfer->udev->addr != 0 && xfer->udev->addr - 1 >= ARRAY_SIZE(priv->usbd_device)) {
		LOG_ERR("Device address %u exceeds supported range", xfer->udev->addr);
		return -ENOTSUP;
	}

	if (USB_EP_GET_IDX(xfer->ep) == 0) {
		ret = uhc_renesas_ra_open_dcp(dev, xfer);
		if (ret != 0) {
			return ret;
		}
	}

	if (!is_configured_udev(dev, xfer->udev->addr)) {
		ret = uhc_renesas_ra_configure_udev(dev, xfer->udev, xfer->mps);
		if (ret != 0) {
			return ret;
		}
	}

	if (!is_configured_ep(dev, xfer->udev->addr, xfer->ep)) {
		struct usb_ep_descriptor desc = {
			.bEndpointAddress = xfer->ep,
			.Attributes = {.transfer = xfer->type},
			.wMaxPacketSize = xfer->mps,
			.bInterval = xfer->interval,
		};

		ret = uhc_renesas_ra_open_pipe(dev, xfer->udev->addr, xfer->ep, &desc);
		if (ret != 0) {
			return ret;
		}
	}

	uhc_renesas_ra_transfer_append(dev, xfer);

	return uhc_renesas_ra_xfer_request(dev, xfer->udev->addr, xfer->ep);
}

static int uhc_renesas_ra_ep_dequeue(const struct device *dev, struct uhc_transfer *const xfer)
{
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);
	struct uhc_renesas_ra_evt evt = {
		.type = UHC_RENESAS_RA_EVT_DEQUEUE,
		.dev_addr = xfer->udev->addr,
		.ep = xfer->ep,
	};

	uhc_lock_internal(dev, K_FOREVER);

	/*
	 * Only mark the xfer here. The actual abort and uhc_xfer_return() happen in
	 * the driver thread, which is the single owner of the transfer lists and the
	 * only context allowed to call uhc_xfer_return() for a given xfer.
	 */
	xfer->err = -ECONNRESET;
	if (k_msgq_put(&priv->msgq, &evt, K_NO_WAIT) != 0) {
		LOG_WRN("Failed to enqueue dequeue event");
	}

	uhc_unlock_internal(dev);

	return 0;
}

static int uhc_renesas_ra_poll_port_speed(const struct device *dev)
{
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);
	usb_speed_t speed = USB_SPEED_INVALID;
	fsp_err_t err;

	for (size_t i = 0; i < CONFIG_UHC_RENESAS_RA_OSC_WAIT_RETRIES; i++) {
		k_msleep(20);

		err = R_USBH_GetDeviceSpeed(&priv->uhc_ctrl, &speed);
		if (err != FSP_SUCCESS) {
			return -EIO;
		}

		if (speed != USB_SPEED_INVALID) {
			break;
		}
	}

	uhc_submit_event(dev, UHC_EVT_RESETED, 0);

	if (priv->speed != speed) {
		uhc_submit_event(dev, UHC_EVT_DEV_REMOVED, 0);

		/* Speed negociation completed. Update device speed */
		switch (speed) {
		case USB_SPEED_LS:
			uhc_submit_event(dev, UHC_EVT_DEV_CONNECTED_LS, 0);
			break;
		case USB_SPEED_FS:
			uhc_submit_event(dev, UHC_EVT_DEV_CONNECTED_FS, 0);
			break;
		case USB_SPEED_HS:
			uhc_submit_event(dev, UHC_EVT_DEV_CONNECTED_HS, 0);
			break;
		default:
			return -EINVAL;
		}

		priv->speed = speed;
	}

	return 0;
}

static int uhc_renesas_ra_submit_xfer(const struct device *dev, struct uhc_renesas_ra_evt *evt)
{
	usbh_event_t *hal_evt = &evt->hal_event;
	uint8_t dev_addr = hal_evt->dev_addr;
	struct uhc_transfer *xfer;
	int ret = 0;

	xfer = uhc_renesas_ra_transfer_get_next(dev, dev_addr, hal_evt->complete.ep_addr);
	if (xfer == NULL) {
		return 0;
	}

	switch (hal_evt->complete.result) {
	case USB_XFER_RESULT_STALLED:
		ret = -ENOTSUP;
		break;
	case USB_XFER_RESULT_TIMEOUT:
	case USB_XFER_RESULT_FAILED:
		ret = -EAGAIN;
		break;
	case USB_XFER_RESULT_SUCCESS: {
		if (USB_EP_GET_IDX(xfer->ep) == 0) {
			uhc_control_stage_update(dev, hal_evt, xfer);
			return 0;
		}

		if (USB_EP_DIR_IS_IN(xfer->ep)) {
			size_t length = MIN(net_buf_tailroom(xfer->buf), hal_evt->complete.len);

			if (length) {
				net_buf_add(xfer->buf, length);
			}
		}
		break;
	}
	default:
		/* USB_XFER_RESULT_INVALID */
		ret = -EINVAL;
	}

	uhc_xfer_return(dev, xfer, ret);

	return ret;
}

static void uhc_renesas_ra_dequeue_pipe_cancelled(const struct device *dev, sys_dlist_t *list)
{
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);
	struct uhc_transfer *xfer, *tmp;
	bool first = true;

	SYS_DLIST_FOR_EACH_CONTAINER_SAFE(list, xfer, tmp, node) {
		bool is_head = first;

		first = false;

		if (xfer->err != -ECONNRESET) {
			continue;
		}

		if (is_head) {
			/* May already be in-flight in hardware, abort it first */
			(void)R_USBH_XferAbort(&priv->uhc_ctrl, xfer->udev->addr, xfer->ep);
		}

		uhc_xfer_return(dev, xfer, -ECONNRESET);
	}
}

static int uhc_renesas_ra_dequeue_cancelled(const struct device *dev, uint8_t dev_addr, uint8_t ep)
{
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);

	if (USB_EP_GET_IDX(ep) == 0) {
		uhc_renesas_ra_dequeue_pipe_cancelled(dev, &priv->dcp.xfers_list);
	} else {
		uhc_renesas_ra_dequeue_pipe_cancelled(
			dev, &priv->pipe[get_pipe_num(dev, dev_addr, ep)].xfers_list);
	}

	return 0;
}

static void uhc_renesas_ra_device_attach(const struct device *dev, usbh_event_t *event)
{
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);
	fsp_err_t err;

	priv->speed = event->attach.speed;

	switch (event->attach.speed) {
	case USB_SPEED_LS:
		uhc_submit_event(dev, UHC_EVT_DEV_CONNECTED_LS, 0);
		break;
	case USB_SPEED_FS:
		uhc_submit_event(dev, UHC_EVT_DEV_CONNECTED_FS, 0);
		break;
	case USB_SPEED_HS:
		uhc_submit_event(dev, UHC_EVT_DEV_CONNECTED_HS, 0);
		break;
	default:
		LOG_WRN("Spurious attach event");
		return;
	}

	err = R_USBH_PortReset(&priv->uhc_ctrl);
	if (err != FSP_SUCCESS) {
		return;
	}

	uhc_renesas_ra_event_poll_port_speed(dev);
}

static void uhc_renesas_ra_device_detach(const struct device *dev, usbh_event_t *hal_evt)
{
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);
	fsp_err_t err;

	for (int i = 1; i <= ARRAY_SIZE(priv->usbd_device); i++) {
		if (!is_configured_udev(dev, i)) {
			continue;
		}

		err = R_USBH_DeviceRelease(&priv->uhc_ctrl, i);
		if (!(err == FSP_SUCCESS || err == FSP_ERR_ABORTED)) {
			LOG_WRN("Error releasing device: %d", err);
			return;
		}
	}

	memset(&priv->usbd_device, 0, sizeof(priv->usbd_device));
	memset(&priv->ep, 0, sizeof(priv->ep));

	uhc_submit_event(dev, UHC_EVT_DEV_REMOVED, 0);
}

static void uhc_renesas_ra_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	const struct device *dev = p1;
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);

	LOG_DBG("UHC_RENESAS_RA thread started");

	while (true) {
		struct uhc_renesas_ra_evt event;
		int ret;

		k_msgq_get(&priv->msgq, &event, K_FOREVER);

		uhc_lock_internal(dev, K_FOREVER);

		switch (event.type) {
		case UHC_RENESAS_RA_EVT_XFER:
			ret = uhc_renesas_ra_schedule_xfer(dev, event.dev_addr, event.ep);
			if (unlikely(ret)) {
				LOG_WRN("Failed to schedule xfer");
			}
			break;
		case UHC_RENESAS_RA_EVT_POLL_PORT_SPEED:
			ret = uhc_renesas_ra_poll_port_speed(dev);
			if (unlikely(ret)) {
				LOG_WRN("Poll device speed failed with error %d", ret);
			}
			break;
		case UHC_RENESAS_RA_EVT_XFER_SUBMIT:
			ret = uhc_renesas_ra_submit_xfer(dev, &event);
			if (unlikely(ret)) {
				LOG_WRN("Submit xfer failed %d", ret);
			}
			break;
		case UHC_RENESAS_RA_EVT_DEQUEUE:
			ret = uhc_renesas_ra_dequeue_cancelled(dev, event.dev_addr, event.ep);
			if (unlikely(ret)) {
				LOG_WRN("Failed to dequeue cancelled xfer");
			}
			break;
		case UHC_RENESAS_RA_EVT_DEVICE_ATTACH:
			uhc_renesas_ra_device_attach(dev, &event.hal_event);
			break;
		case UHC_RENESAS_RA_EVT_DEVICE_DETACH:
			uhc_renesas_ra_device_detach(dev, &event.hal_event);
			break;
		default:
			LOG_WRN("Spurious event");
			break;
		}

		uhc_unlock_internal(dev);
	}
}

/* Enable SOF generator */
static int uhc_renesas_ra_sof_enable(const struct device *dev)
{
	/* Already enabled by a uhc_enable() call */
	ARG_UNUSED(dev);

	return 0;
}

static int uhc_renesas_ra_bus_suspend(const struct device *dev)
{
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);
	fsp_err_t err;

	err = R_USBH_BusSuspend(&priv->uhc_ctrl);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	uhc_submit_event(dev, UHC_EVT_SUSPENDED, 0);

	return 0;
}

static int uhc_renesas_ra_bus_reset(const struct device *dev)
{
	/*
	 * TODO: Move the reset performance from the USBH_EVENT_DEVICE_ATTACH event to this
	 * function. Then, the reset signal will be issued by usbh_core.
	 *
	 * This can be done once the high-speed negotiation issue in usbh_core has been fixed.
	 * See https://github.com/zephyrproject-rtos/zephyr/pull/110022
	 */

	return 0;
}

static int uhc_renesas_ra_bus_resume(const struct device *dev)
{
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);
	fsp_err_t err;

	err = R_USBH_BusResume(&priv->uhc_ctrl);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	uhc_submit_event(dev, UHC_EVT_RESUMED, 0);

	return 0;
}

static int uhc_renesas_ra_init(const struct device *dev)
{
	const struct uhc_renesas_ra_config *config = dev->config;
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);
	fsp_err_t err;
	int ret;

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("USB pinctrl setup failed (%d)", ret);
		return ret;
	}

	if (priv->uhc_cfg.hs_irq != FSP_INVALID_VECTOR) {
		irq_connect_dynamic(priv->uhc_cfg.hs_irq, priv->uhc_cfg.hsipl,
				    uhc_renesas_ra_interrupt_handler, dev, 0);
		R_ICU->IELSR[priv->uhc_cfg.hs_irq] = BSP_PRV_IELS_ENUM(EVENT_USBHS_USB_INT_RESUME);
		BSP_ASSIGN_EVENT_TO_CURRENT_CORE(BSP_PRV_IELS_ENUM(EVENT_USBHS_USB_INT_RESUME));
		irq_enable(priv->uhc_cfg.hs_irq);
	}

	if (priv->uhc_cfg.irq != FSP_INVALID_VECTOR) {
		irq_connect_dynamic(priv->uhc_cfg.irq, priv->uhc_cfg.ipl,
				    uhc_renesas_ra_interrupt_handler, dev, 0);
		R_ICU->IELSR[priv->uhc_cfg.irq] = BSP_PRV_IELS_ENUM(EVENT_USBFS_INT);
		BSP_ASSIGN_EVENT_TO_CURRENT_CORE(BSP_PRV_IELS_ENUM(EVENT_USBFS_INT));
		irq_enable(priv->uhc_cfg.irq);
	}

	if (priv->uhc_cfg.irq_r != FSP_INVALID_VECTOR) {
		irq_connect_dynamic(priv->uhc_cfg.irq_r, priv->uhc_cfg.ipl_r,
				    uhc_renesas_ra_interrupt_handler, dev, 0);
		R_ICU->IELSR[priv->uhc_cfg.irq_r] = BSP_PRV_IELS_ENUM(EVENT_USBFS_RESUME);
		BSP_ASSIGN_EVENT_TO_CURRENT_CORE(BSP_PRV_IELS_ENUM(EVENT_USBFS_RESUME));
		irq_enable(priv->uhc_cfg.irq_r);
	}

	err = R_USBH_Open(&priv->uhc_ctrl, &priv->uhc_cfg);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static int uhc_renesas_ra_enable(const struct device *dev)
{
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);
	fsp_err_t err;

	err = R_USBH_Enable(&priv->uhc_ctrl);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static int uhc_renesas_ra_disable(const struct device *dev)
{
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);
	fsp_err_t err;

	err = R_USBH_Disable(&priv->uhc_ctrl);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static int uhc_renesas_ra_shutdown(const struct device *dev)
{
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);

	if (R_USBH_Close(&priv->uhc_ctrl) != FSP_SUCCESS) {
		return -EIO;
	}

	if (priv->uhc_cfg.hs_irq != FSP_INVALID_VECTOR) {
		R_ICU->IELSR[priv->uhc_cfg.irq] = 0;
		irq_disable(priv->uhc_cfg.hs_irq);
	}

	if (priv->uhc_cfg.irq != FSP_INVALID_VECTOR) {
		R_ICU->IELSR[priv->uhc_cfg.irq] = 0;
		irq_disable(priv->uhc_cfg.irq);
	}

	if (priv->uhc_cfg.irq_r != FSP_INVALID_VECTOR) {
		R_ICU->IELSR[priv->uhc_cfg.irq_r] = 0;
		irq_disable(priv->uhc_cfg.irq_r);
	}

	return 0;
}

static DEVICE_API(uhc, uhc_renesas_ra_api) = {
	.lock = uhc_renesas_ra_lock,
	.unlock = uhc_renesas_ra_unlock,
	.init = uhc_renesas_ra_init,
	.enable = uhc_renesas_ra_enable,
	.disable = uhc_renesas_ra_disable,
	.shutdown = uhc_renesas_ra_shutdown,
	.bus_reset = uhc_renesas_ra_bus_reset,
	.sof_enable = uhc_renesas_ra_sof_enable,
	.bus_suspend = uhc_renesas_ra_bus_suspend,
	.bus_resume = uhc_renesas_ra_bus_resume,
	.ep_enqueue = uhc_renesas_ra_ep_enqueue,
	.ep_dequeue = uhc_renesas_ra_ep_dequeue,
};

static void uhc_renesas_ra_event_xfer_complete(const struct device *dev, usbh_event_t *hal_evt)
{
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);
	struct uhc_renesas_ra_evt evt = {
		.type = UHC_RENESAS_RA_EVT_XFER_SUBMIT,
		.hal_event = *hal_evt,
	};
	int ret = k_msgq_put(&priv->msgq, &evt, K_NO_WAIT);

	if (ret != 0) {
		LOG_WRN("Failed to enqueue transfer completion event");
	}
}

static void uhc_renesas_ra_event_device_attach(const struct device *dev, usbh_event_t *hal_evt)
{
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);
	struct uhc_renesas_ra_evt evt = {
		.type = UHC_RENESAS_RA_EVT_DEVICE_ATTACH,
		.hal_event = *hal_evt,
	};

	if (k_msgq_put(&priv->msgq, &evt, K_NO_WAIT) != 0) {
		LOG_WRN("Failed to enqueue device attach event");
	}
}

static void uhc_renesas_ra_event_device_detach(const struct device *dev, usbh_event_t *hal_evt)
{
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);
	struct uhc_renesas_ra_evt evt = {
		.type = UHC_RENESAS_RA_EVT_DEVICE_DETACH,
		.hal_event = *hal_evt,
	};

	if (k_msgq_put(&priv->msgq, &evt, K_NO_WAIT) != 0) {
		LOG_WRN("Failed to enqueue device detach event");
	}
}

static void uhc_renesas_ra_callback(usbh_callback_arg_t *p_args)
{
	const struct device *dev = p_args->p_context;

	switch (p_args->event.event_id) {
	case USBH_EVENT_XFER_COMPLETE:
		uhc_renesas_ra_event_xfer_complete(dev, &p_args->event);
		break;
	case USBH_EVENT_DEVICE_ATTACH:
		uhc_renesas_ra_event_device_attach(dev, &p_args->event);
		break;
	case USBH_EVENT_DEVICE_REMOVE:
		uhc_renesas_ra_event_device_detach(dev, &p_args->event);
		break;
	default:
		break;
	}
}

static int uhc_ra_driver_preinit(const struct device *dev)
{
	const struct uhc_renesas_ra_config *config = dev->config;
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);
	struct uhc_data *data = dev->data;

	k_mutex_init(&data->mutex);

	sys_dlist_init(&priv->dcp.xfers_list);

	for (int i = 0; i < ARRAY_SIZE(priv->pipe); i++) {
		sys_dlist_init(&priv->pipe[i].xfers_list);
	}

	memset(&priv->usbd_device, 0, sizeof(priv->usbd_device));
	memset(&priv->ep, 0, sizeof(priv->ep));

	k_msgq_alloc_init(&priv->msgq, sizeof(struct uhc_renesas_ra_evt),
			  CONFIG_UHC_RENESAS_RA_MAX_MSGQ);
	k_thread_create(&priv->thread_data, config->drv_stack, config->drv_stack_size,
			uhc_renesas_ra_thread, (void *)dev, NULL, NULL,
			K_PRIO_PREEMPT(CONFIG_UHC_RENESAS_RA_THREAD_PRIORITY), K_ESSENTIAL,
			K_NO_WAIT);
	k_thread_name_set(&priv->thread_data, dev->name);

	return 0;
}

#define IS_USB_HIGH_SPEED(n)                                                                       \
	DT_NODE_HAS_COMPAT(n, renesas_ra_usbhs)                                                    \
	? DT_ENUM_IDX_OR(n, maximum_speed, 2) == 2 : false

#define USB_MODULE_NUMBER(n) ((DT_REG_ADDR(n)) == R_USB_FS0_BASE ? 0 : 1)

#define RENESAS_RA_USB_IRQ_GET(id, name, cell)                                                     \
	COND_CODE_1(DT_IRQ_HAS_NAME(id, name), (DT_IRQ_BY_NAME(id, name, cell)),                   \
				((IRQn_Type) FSP_INVALID_VECTOR))

/* clang-format off */

#define UHC_RENESAS_RA_DEVICE_DEFINE(n)                                                            \
	PINCTRL_DT_DEFINE(DT_INST_PARENT(n));                                                      \
	K_THREAD_STACK_DEFINE(uhc_renesas_ra_stack_##n, CONFIG_UHC_RENESAS_RA_STACK_SIZE);         \
                                                                                                   \
	static const struct uhc_renesas_ra_config uhc_config_##n = {                               \
		.pcfg = PINCTRL_DT_DEV_CONFIG_GET(DT_INST_PARENT(n)),                              \
		.drv_stack = uhc_renesas_ra_stack_##n,                                             \
		.drv_stack_size = K_THREAD_STACK_SIZEOF(uhc_renesas_ra_stack_##n),                 \
	};                                                                                         \
                                                                                                   \
	static struct uhc_renesas_ra_data uhc_priv_data_##n = {                                    \
		.uhc_cfg = {                                                                       \
			.irq = RENESAS_RA_USB_IRQ_GET(DT_INST_PARENT(n), usbfs_i, irq),            \
			.irq_r = RENESAS_RA_USB_IRQ_GET(DT_INST_PARENT(n), usbfs_r, irq),          \
			.hs_irq = RENESAS_RA_USB_IRQ_GET(DT_INST_PARENT(n), usbhs_ir, irq),        \
			.ipl = RENESAS_RA_USB_IRQ_GET(DT_INST_PARENT(n), usbfs_i, priority),       \
			.ipl_r = RENESAS_RA_USB_IRQ_GET(DT_INST_PARENT(n), usbfs_r, priority),     \
			.hsipl = RENESAS_RA_USB_IRQ_GET(DT_INST_PARENT(n), usbhs_ir, priority),    \
			.module_number = USB_MODULE_NUMBER(DT_INST_PARENT(n)),                     \
			.high_speed = IS_USB_HIGH_SPEED(DT_INST_PARENT(n)),                        \
			.p_callback = uhc_renesas_ra_callback,                                     \
			.p_context = DEVICE_DT_INST_GET(n),                                        \
		},                                                                                 \
	};                                                                                         \
                                                                                                   \
	static struct uhc_data uhc_data_##n = {                                                    \
		.mutex = Z_MUTEX_INITIALIZER(uhc_data_##n.mutex),                                  \
		.priv = &uhc_priv_data_##n,                                                        \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, uhc_ra_driver_preinit, NULL, &uhc_data_##n, &uhc_config_##n,      \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,                     \
			      &uhc_renesas_ra_api);

/* clang-format on */

DT_INST_FOREACH_STATUS_OKAY(UHC_RENESAS_RA_DEVICE_DEFINE)
