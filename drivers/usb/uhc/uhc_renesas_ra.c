/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_ra_uhc

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/dlist.h>
#include "uhc_common.h"
#include "r_usb_host.h"

LOG_MODULE_REGISTER(uhc_renesas_ra, CONFIG_UHC_DRIVER_LOG_LEVEL);

#define UHC_RENESA_RA_MAX_UDEV 5

/* The default control pipe, and the nine the controller can give an endpoint */
#define UHC_RENESAS_RA_PIPES 10

enum uhc_renesas_ra_event_type {
	/* Shim driver event to trigger next transfer */
	UHC_RENESAS_RA_EVT_XFER,
	/* Device speed check, typically performed after a RESET signal is issued */
	UHC_RENESAS_RA_EVT_POLL_DEVICE_SPEED,
};

struct uhc_renesas_ra_evt {
	enum uhc_renesas_ra_event_type type;
};

/*
 * What one pipe of the controller is carrying. Every device's endpoint zero
 * shares the control pipe, which takes one transfer at a time, and each of the
 * other pipes belongs to a single endpoint, so several transfers are in flight
 * at once and a completion has to be matched to the endpoint reporting it.
 */
struct uhc_renesas_ra_pipe {
	struct uhc_transfer *xfer;
	uint8_t addr;
	uint8_t ep;
	/* The stage it has reached has still to be handed to the controller */
	bool issue;
};

struct uhc_renesas_ra_data {
	struct uhc_renesas_ra_pipe pipe[UHC_RENESAS_RA_PIPES];
	struct k_thread thread_data;
	struct k_msgq msgq;
	struct st_usbh_instance_ctrl uhc_ctrl;
	struct st_usb_cfg uhc_cfg;
	sys_dlist_t xfers;
	uint8_t devadd[UHC_RENESA_RA_MAX_UDEV];
	/* The controller pipe each endpoint is open on, 0 if it is not */
	uint8_t ep[UHC_RENESA_RA_MAX_UDEV][16][2];
};

struct uhc_renesas_ra_config {
	const struct pinctrl_dev_config *pcfg;
	k_thread_stack_t *drv_stack;
	size_t drv_stack_size;
};

extern void r_usbh_isr(void);

static void uhc_renesas_ra_interrupt_handler(void *arg)
{
	ARG_UNUSED(arg);
	r_usbh_isr();
}

static void uhc_renesas_ra_xfer_request(const struct device *dev)
{
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);
	struct uhc_renesas_ra_evt event = {.type = UHC_RENESAS_RA_EVT_XFER};
	int ret;

	ret = k_msgq_put(&priv->msgq, &event, K_NO_WAIT);
	__ASSERT_NO_MSG(ret == 0);
}

/*
 * The pipe a transfer is in flight on, or NULL if it is not. Endpoint zero is
 * always the first pipe, whichever device the transfer is for.
 */
static struct uhc_renesas_ra_pipe *uhc_renesas_ra_pipe_find(const struct device *dev,
							    const uint8_t addr, const uint8_t ep)
{
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);

	if (USB_EP_GET_IDX(ep) == 0) {
		return priv->pipe[0].xfer != NULL ? &priv->pipe[0] : NULL;
	}

	for (size_t i = 1; i < UHC_RENESAS_RA_PIPES; i++) {
		struct uhc_renesas_ra_pipe *pipe = &priv->pipe[i];

		if (pipe->xfer != NULL && pipe->addr == addr && pipe->ep == ep) {
			return pipe;
		}
	}

	return NULL;
}

/* A pipe to carry a transfer that is not in flight yet, if one is free */
static struct uhc_renesas_ra_pipe *uhc_renesas_ra_pipe_claim(const struct device *dev,
							     struct uhc_transfer *const xfer)
{
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);

	if (USB_EP_GET_IDX(xfer->ep) == 0) {
		return priv->pipe[0].xfer == NULL ? &priv->pipe[0] : NULL;
	}

	for (size_t i = 1; i < UHC_RENESAS_RA_PIPES; i++) {
		if (priv->pipe[i].xfer == NULL) {
			return &priv->pipe[i];
		}
	}

	return NULL;
}

static int uhc_renesas_ra_lock(const struct device *dev)
{
	return uhc_lock_internal(dev, K_FOREVER);
}

static int uhc_renesas_ra_unlock(const struct device *dev)
{
	return uhc_unlock_internal(dev);
}

static int uhc_renesas_ra_control_status_xfer(const struct device *dev,
					      struct uhc_transfer *const xfer)
{
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);
	uint8_t inv_ep = (xfer->ep ^ USB_EP_DIR_MASK);
	int err;

	err = R_USBH_XferStart(&priv->uhc_ctrl, xfer->udev->addr, inv_ep, NULL, 0);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	return 0;
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
		net_buf_remove_mem(buf, len);
		LOG_ERR("ep 0x%02x state status error", xfer->ep);
		return -EIO;
	}

	return 0;
}

static int uhc_renesas_ra_data_xfer(const struct device *dev, struct uhc_transfer *const xfer)
{
	if (USB_EP_DIR_IS_IN(xfer->ep)) {
		return uhc_renesas_ra_data_receive(dev, xfer);
	} else {
		return uhc_renesas_ra_data_send(dev, xfer);
	}
}

static int uhc_renesas_ra_edpt_open(const struct device *dev, struct uhc_transfer *const xfer)
{
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);
	usb_desc_endpoint_t ep_desc = {
		.bDescriptorType = USB_DT_ENDPOINT,
		.bEndpointAddress = xfer->ep,
		.bmAttributes = {.xfer = xfer->type, .sync = 0, .usage = 0},
		.wMaxPacketSize = xfer->mps,
		.bInterval = xfer->interval,
	};
	uint8_t pipe_num;
	fsp_err_t err;
	int ret;

	err = R_USBH_EdptOpen(&priv->uhc_ctrl, xfer->udev->addr, &ep_desc, &pipe_num);
	if (err == FSP_SUCCESS) {
		LOG_INF("ep 0x%02x has been opened", xfer->ep);
		priv->ep[xfer->udev->addr][USB_EP_GET_IDX(xfer->ep)][!!USB_EP_GET_DIR(xfer->ep)] =
			pipe_num;
		ret = 0;
	} else if (err == FSP_ERR_USB_BUSY) {
		LOG_ERR("No available pipe to configure for this EP");
		ret = -EBUSY;
	} else {
		LOG_ERR("Open ep 0x%02x failed", xfer->ep);
		ret = -EIO;
	}

	return ret;
}

static int uhc_renesas_ra_control_xfer(const struct device *dev, struct uhc_transfer *const xfer)
{
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);
	fsp_err_t err;
	int ret = 0;

	LOG_DBG("issue ep 0x%02x stage %d", xfer->ep, xfer->stage);

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
		ret = uhc_renesas_ra_control_status_xfer(dev, xfer);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int uhc_renesas_ra_transfer_append(const struct device *dev, struct uhc_transfer *const xfer)
{
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);
	int ret;

	if (xfer->interval > 0) {
		sys_dlist_append(&priv->xfers, &xfer->node);
	} else {
		ret = uhc_xfer_append(dev, xfer);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

static int uhc_renesas_ra_issue(const struct device *dev, struct uhc_transfer *const xfer)
{
	if (USB_EP_GET_IDX(xfer->ep) == 0) {
		/*
		 * A control transfer is handed over a stage at a time, and the
		 * driver keeps track of which one it is on.
		 */
		return uhc_renesas_ra_control_xfer(dev, xfer);
	}

	return uhc_renesas_ra_data_xfer(dev, xfer);
}

static void uhc_renesas_ra_pipe_release(struct uhc_renesas_ra_pipe *const pipe)
{
	pipe->xfer = NULL;
	pipe->issue = false;
}

/* Give a transfer a pipe and hand its first stage to the controller */
static void uhc_renesas_ra_start(const struct device *dev, struct uhc_transfer *const xfer)
{
	struct uhc_renesas_ra_pipe *pipe;
	int ret;

	pipe = uhc_renesas_ra_pipe_claim(dev, xfer);
	if (pipe == NULL) {
		/* Every pipe is carrying something: this one waits its turn */
		return;
	}

	pipe->xfer = xfer;
	pipe->addr = xfer->udev->addr;
	pipe->ep = xfer->ep;
	pipe->issue = false;

	ret = uhc_renesas_ra_issue(dev, xfer);
	if (ret != 0) {
		uhc_renesas_ra_pipe_release(pipe);
		uhc_xfer_return(dev, xfer, ret);
	}
}

/*
 * A transfer the caller has given up on. If the controller has it, stop the
 * pipe, so that it neither completes the transfer nor fills a buffer that has
 * been handed back.
 */
static void uhc_renesas_ra_drop(const struct device *dev, struct uhc_transfer *const xfer)
{
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);
	struct uhc_renesas_ra_pipe *pipe;
	fsp_err_t err;

	pipe = uhc_renesas_ra_pipe_find(dev, xfer->udev->addr, xfer->ep);
	if (pipe != NULL && pipe->xfer == xfer) {
		uhc_renesas_ra_pipe_release(pipe);

		err = R_USBH_XferAbort(&priv->uhc_ctrl, xfer->udev->addr, xfer->ep);
		if (err != FSP_SUCCESS) {
			LOG_WRN("Failed to abort ep 0x%02x: %d", xfer->ep, err);
		}
	}

	uhc_xfer_return(dev, xfer, -ECONNRESET);
}

/*
 * Hand over every stage the controller is waiting to be given, then start what
 * there is a free pipe for. A transfer already on a pipe keeps it until it
 * finishes, so a bulk endpoint that is only listening does not hold up the
 * others.
 */
static int uhc_renesas_ra_submit_pending(const struct device *dev)
{
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);
	struct uhc_data *const data = dev->data;
	struct uhc_transfer *xfer, *tmp;

	for (size_t i = 0; i < UHC_RENESAS_RA_PIPES; i++) {
		struct uhc_renesas_ra_pipe *pipe = &priv->pipe[i];
		struct uhc_transfer *staged = pipe->xfer;
		int ret;

		if (staged == NULL || !pipe->issue) {
			continue;
		}

		pipe->issue = false;

		ret = uhc_renesas_ra_issue(dev, staged);
		if (ret != 0) {
			uhc_renesas_ra_pipe_release(pipe);
			uhc_xfer_return(dev, staged, ret);
		}
	}

	SYS_DLIST_FOR_EACH_CONTAINER_SAFE(&data->ctrl_xfers, xfer, tmp, node) {
		if (xfer->err == -ECONNRESET) {
			uhc_renesas_ra_drop(dev, xfer);
			continue;
		}

		if (uhc_renesas_ra_pipe_find(dev, xfer->udev->addr, xfer->ep) == NULL) {
			uhc_renesas_ra_start(dev, xfer);
		}
	}

	SYS_DLIST_FOR_EACH_CONTAINER_SAFE(&priv->xfers, xfer, tmp, node) {
		if (xfer->err == -ECONNRESET) {
			uhc_renesas_ra_drop(dev, xfer);
			continue;
		}

		if (uhc_renesas_ra_pipe_find(dev, xfer->udev->addr, xfer->ep) == NULL) {
			uhc_renesas_ra_start(dev, xfer);
		}
	}

	return 0;
}

static void uhc_control_stage_update(const struct device *dev,
				     struct uhc_renesas_ra_pipe *const pipe)
{
	struct uhc_transfer *xfer = pipe->xfer;

	/*
	 * Only move the stage on here; starting it belongs to the driver thread.
	 * This runs in the controller's own interrupt, and handing a transfer to
	 * the controller clears that interrupt pending in the ICU and the NVIC,
	 * which discards anything that arrived while the handler was running.
	 */
	switch (xfer->stage) {
	case UHC_CONTROL_STAGE_SETUP:
		/* S-[in]-status, S-[out]-status or S-[status] */
		xfer->stage = (xfer->buf != NULL) ? UHC_CONTROL_STAGE_DATA
						  : UHC_CONTROL_STAGE_STATUS;
		pipe->issue = true;
		break;
	case UHC_CONTROL_STAGE_DATA:
		/* S-in-[status] or S-out-[status] */
		xfer->stage = UHC_CONTROL_STAGE_STATUS;
		pipe->issue = true;
		break;
	case UHC_CONTROL_STAGE_STATUS:
		/* Transfer is completed */
		uhc_renesas_ra_pipe_release(pipe);
		uhc_xfer_return(dev, xfer, 0);
		break;
	default:
		break;
	}
}

static int uhc_renesas_ra_event_xfer_complete(const struct device *dev, usbh_event_t *hal_evt)
{
	const uint8_t ep = hal_evt->complete.ep_addr;
	struct uhc_renesas_ra_pipe *pipe;
	struct uhc_transfer *xfer;
	int ret = 0;

	pipe = uhc_renesas_ra_pipe_find(dev, hal_evt->dev_addr, ep);
	if (pipe == NULL) {
		LOG_WRN("No transfer in progress on ep 0x%02x", ep);
		return -EINVAL;
	}

	xfer = pipe->xfer;

	switch (hal_evt->complete.result) {
	case USB_XFER_RESULT_STALLED:
	case USB_XFER_RESULT_TIMEOUT:
	case USB_XFER_RESULT_FAILED:
		uhc_renesas_ra_pipe_release(pipe);
		uhc_xfer_return(dev, xfer, -EPIPE);
		ret = -EAGAIN;
		break;
	case USB_XFER_RESULT_SUCCESS:
		if (USB_EP_GET_IDX(ep) == 0) {
			if (ep == USB_CONTROL_EP_IN && xfer->stage == UHC_CONTROL_STAGE_DATA) {
				net_buf_add(xfer->buf, hal_evt->complete.len);
			}

			uhc_control_stage_update(dev, pipe);
			break;
		}

		if (USB_EP_DIR_IS_IN(ep)) {
			if (hal_evt->complete.len > 0) {
				net_buf_add(xfer->buf, hal_evt->complete.len);
			}
		}

		uhc_renesas_ra_pipe_release(pipe);
		uhc_xfer_return(dev, xfer, 0);
		break;
	default:
		/* USB_XFER_RESULT_INVALID */
		uhc_renesas_ra_pipe_release(pipe);
		uhc_xfer_return(dev, xfer, -EINVAL);
		ret = -EINVAL;
	}

	/* A stage to hand over, or a pipe something else was waiting for */
	uhc_renesas_ra_xfer_request(dev);

	return ret;
}

static bool uhc_renesas_ra_chk_edpt_open(const struct device *dev, uint8_t addr, uint8_t ep)
{
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);

	if (USB_EP_GET_IDX(ep) == 0) {
		/* Control endpoint is always open after device address is assigned */
		return true;
	}

	return priv->ep[addr][USB_EP_GET_IDX(ep)][!!USB_EP_GET_DIR(ep)] != 0U;
}

static int uhc_renesas_ra_ep_enqueue(const struct device *dev, struct uhc_transfer *const xfer)
{
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);
	usb_speed_t speed;
	fsp_err_t err;
	int ret;

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
		LOG_DBG("Device speed %d is not supported by controller", xfer->udev->speed);
		return -ENOTSUP;
	}

	if (xfer->udev->addr >= UHC_RENESA_RA_MAX_UDEV) {
		LOG_ERR("Device address %u is beyond what the driver tracks", xfer->udev->addr);
		return -ENOTSUP;
	}

	/*
	 * The controller has to be told a device's speed and the maximum packet
	 * size of its control endpoint, which the host stack keeps at eight
	 * until it has read the device descriptor at address zero.
	 *
	 * It only has to be told about a device once. Doing it again puts the
	 * default control pipe back to NAK and rewrites the address registers,
	 * which would be underneath whatever else is in flight.
	 *
	 * TODO: Configure split transaction once the host stack knows hubs
	 */
	if (priv->devadd[xfer->udev->addr] == 0) {
		err = R_USBH_PortOpen(&priv->uhc_ctrl, xfer->udev->addr, speed,
				      xfer->udev->dev_desc.bMaxPacketSize0, 0, 0);
		if (err != FSP_SUCCESS) {
			return -EIO;
		}

		priv->devadd[xfer->udev->addr] = 1;
	}

	ret = uhc_renesas_ra_transfer_append(dev, xfer);
	if (ret != 0) {
		LOG_DBG("Failed to append transfer: %d", ret);
		return ret;
	}

	if (!uhc_renesas_ra_chk_edpt_open(dev, xfer->udev->addr, xfer->ep)) {
		ret = uhc_renesas_ra_edpt_open(dev, xfer);
		if (ret != 0) {
			uhc_xfer_return(dev, xfer, ret);
			return ret;
		}
	}

	uhc_renesas_ra_xfer_request(dev);

	return 0;
}

static int uhc_renesas_ra_ep_dequeue(const struct device *dev, struct uhc_transfer *const xfer)
{
	unsigned int key;

	/*
	 * Mark it and leave the rest to the driver thread, which hands it back
	 * once the caller has cleared the queued flag: uhc_xfer_free() refuses
	 * a transfer that is still queued.
	 */
	key = irq_lock();
	xfer->err = -ECONNRESET;
	irq_unlock(key);

	uhc_renesas_ra_xfer_request(dev);

	return 0;
}

static int uhc_renesas_ra_device_attach(const struct device *dev, usbh_event_t *event)
{
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);
	struct uhc_renesas_ra_evt evt = {.type = UHC_RENESAS_RA_EVT_POLL_DEVICE_SPEED};
	fsp_err_t err;

	err = R_USBH_PortReset(&priv->uhc_ctrl);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	return k_msgq_put(&priv->msgq, &evt, K_NO_WAIT);
}

static int uhc_renesas_ra_poll_device_speed(const struct device *dev)
{
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);
	usb_speed_t speed = USB_SPEED_INVALID;
	fsp_err_t err;

	for (size_t i = 0; i < CONFIG_UHC_RENESAS_RA_OSC_WAIT_RETRIES; i++) {
		err = R_USBH_GetDeviceSpeed(&priv->uhc_ctrl, &speed);
		if (err != FSP_SUCCESS) {
			return -EIO;
		}

		if (speed != USB_SPEED_INVALID) {
			break;
		}

		k_msleep(5);
	}

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

	LOG_INF("Device speed detected: speed type %d", speed);

	return 0;
}

static int uhc_renesas_ra_port_release(const struct device *dev)
{
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);
	fsp_err_t err;

	err = R_USBH_DeviceRelease(&priv->uhc_ctrl, 0x01);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	for (size_t i = 0; i < UHC_RENESA_RA_MAX_UDEV; i++) {
		priv->devadd[i] = 0;
		for (size_t j = 0; j < 16; j++) {
			priv->ep[i][j][0] = 0;
			priv->ep[i][j][1] = 0;
		}
	}

	for (size_t i = 0; i < UHC_RENESAS_RA_PIPES; i++) {
		struct uhc_renesas_ra_pipe *pipe = &priv->pipe[i];
		struct uhc_transfer *xfer = pipe->xfer;

		if (xfer == NULL) {
			continue;
		}

		/* Hand it back rather than free it: it is still queued */
		uhc_renesas_ra_pipe_release(pipe);
		uhc_xfer_return(dev, xfer, -ECONNRESET);
	}

	uhc_submit_event(dev, UHC_EVT_DEV_REMOVED, 0);

	return 0;
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

		switch (event.type) {
		case UHC_RENESAS_RA_EVT_XFER:
			ret = uhc_renesas_ra_submit_pending(dev);
			if (unlikely(ret)) {
				LOG_WRN("Schedule xfer failed with error %d", ret);
			}
			break;
		case UHC_RENESAS_RA_EVT_POLL_DEVICE_SPEED:
			ret = uhc_renesas_ra_poll_device_speed(dev);
			if (unlikely(ret)) {
				LOG_WRN("Poll device speed failed with error %d", ret);
			}
			break;
		default:
			break;
		}
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
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);
	fsp_err_t err;

	err = R_USBH_PortReset(&priv->uhc_ctrl);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	uhc_submit_event(dev, UHC_EVT_RESETED, 0);

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
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);
	fsp_err_t err;

	err = R_USBH_Open(&priv->uhc_ctrl, &priv->uhc_cfg);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	LOG_INF("Initialized");

	for (int i = 0; i < UHC_RENESA_RA_MAX_UDEV; i++) {
		for (int j = 0; j < 16; j++) {
			priv->ep[i][j][0] = 0;
			priv->ep[i][j][1] = 0;
		}
	}

	if (priv->uhc_cfg.hs_irq != FSP_INVALID_VECTOR) {
		irq_enable(priv->uhc_cfg.hs_irq);
	}

	if (priv->uhc_cfg.irq != FSP_INVALID_VECTOR) {
		irq_enable(priv->uhc_cfg.irq);
	}

	if (priv->uhc_cfg.irq_r != FSP_INVALID_VECTOR) {
		irq_enable(priv->uhc_cfg.irq_r);
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

	for (size_t i = 0; i < UHC_RENESAS_RA_PIPES; i++) {
		struct uhc_renesas_ra_pipe *pipe = &priv->pipe[i];
		struct uhc_transfer *xfer = pipe->xfer;

		if (xfer == NULL) {
			continue;
		}

		/* Hand it back rather than free it: it is still queued */
		uhc_renesas_ra_pipe_release(pipe);
		uhc_xfer_return(dev, xfer, -ECONNRESET);
	}

	if (R_USBH_Close(&priv->uhc_ctrl) != FSP_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static const struct uhc_driver_api uhc_renesas_ra_api = {
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

static void uhc_renesas_ra_callback(usbh_callback_arg_t *p_args)
{
	const struct device *dev = p_args->p_context;

	LOG_DBG("evt %d ep 0x%02x res %d len %u", p_args->event.event_id,
		p_args->event.complete.ep_addr, p_args->event.complete.result,
		p_args->event.complete.len);

	switch (p_args->event.event_id) {
	case USBH_EVENT_XFER_COMPLETE:
		uhc_renesas_ra_event_xfer_complete(dev, &p_args->event);
		break;
	case USBH_EVENT_DEVICE_ATTACH:
		uhc_renesas_ra_device_attach(dev, &p_args->event);
		break;
	case USBH_EVENT_DEVICE_REMOVE:
		uhc_renesas_ra_port_release(dev);
		break;
	default:
		break;
	}
}

static int uhc_ra_driver_preinit(const struct device *dev)
{
	const struct uhc_renesas_ra_config *config = dev->config;
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);
	int ret;

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("USB pinctrl setup failed (%d)", ret);
	}

	if (priv->uhc_cfg.hs_irq != FSP_INVALID_VECTOR) {
		R_ICU->IELSR[priv->uhc_cfg.hs_irq] = BSP_PRV_IELS_ENUM(EVENT_USBHS_USB_INT_RESUME);
		BSP_ASSIGN_EVENT_TO_CURRENT_CORE(BSP_PRV_IELS_ENUM(EVENT_USBHS_USB_INT_RESUME));
	}

	if (priv->uhc_cfg.irq != FSP_INVALID_VECTOR) {
		R_ICU->IELSR[priv->uhc_cfg.irq] = BSP_PRV_IELS_ENUM(EVENT_USBFS_INT);
		BSP_ASSIGN_EVENT_TO_CURRENT_CORE(BSP_PRV_IELS_ENUM(EVENT_USBFS_INT));
	}

	if (priv->uhc_cfg.irq_r != FSP_INVALID_VECTOR) {
		R_ICU->IELSR[priv->uhc_cfg.irq_r] = BSP_PRV_IELS_ENUM(EVENT_USBFS_RESUME);
		BSP_ASSIGN_EVENT_TO_CURRENT_CORE(BSP_PRV_IELS_ENUM(EVENT_USBFS_RESUME));
	}

	sys_dlist_init(&priv->xfers);

	k_msgq_alloc_init(&priv->msgq, sizeof(struct uhc_renesas_ra_evt),
			  CONFIG_UHC_RENESAS_RA_MAX_MSGQ);
	k_thread_create(&priv->thread_data, config->drv_stack, config->drv_stack_size,
			uhc_renesas_ra_thread, (void *)dev, NULL, NULL,
			K_PRIO_COOP(CONFIG_UHC_RENESAS_RA_THREAD_PRIORITY), K_ESSENTIAL, K_NO_WAIT);
	k_thread_name_set(&priv->thread_data, dev->name);

	return ret;
}

#define IS_USB_HIGH_SPEED(n)                                                                       \
	DT_NODE_HAS_COMPAT(n, renesas_ra_usbhs)                                                    \
	? DT_ENUM_IDX_OR(n, maximum_speed, 2) == 2 : false

#define USB_MODULE_NUMBER(n) ((DT_REG_ADDR(n)) == R_USB_FS0_BASE ? 0 : 1)

#define RENESAS_RA_USB_IRQ_CONNECT(idx, n)                                                         \
	IRQ_CONNECT(DT_IRQ_BY_IDX(DT_INST_PARENT(n), idx, irq),                                    \
		    DT_IRQ_BY_IDX(DT_INST_PARENT(n), idx, priority),                               \
		    uhc_renesas_ra_interrupt_handler, DEVICE_DT_INST_GET(n), 0)

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
		.devadd = {0},                                                                     \
	};                                                                                         \
                                                                                                   \
	static struct uhc_data uhc_data_##n = {                                                    \
		.mutex = Z_MUTEX_INITIALIZER(uhc_data_##n.mutex),                                  \
		.priv = &uhc_priv_data_##n,                                                        \
	};                                                                                         \
                                                                                                   \
	static int uhc_ra_driver_init_##n(const struct device *dev)                                \
	{                                                                                          \
		LISTIFY(DT_NUM_IRQS(DT_INST_PARENT(n)), RENESAS_RA_USB_IRQ_CONNECT, (;), n);       \
		return uhc_ra_driver_preinit(dev);                                                 \
	}                                                                                          \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, uhc_ra_driver_init_##n, NULL, &uhc_data_##n, &uhc_config_##n,     \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,                     \
			      &uhc_renesas_ra_api);

/* clang-format on */

DT_INST_FOREACH_STATUS_OKAY(UHC_RENESAS_RA_DEVICE_DEFINE)
