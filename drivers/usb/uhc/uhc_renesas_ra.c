/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_ra_uhc

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include "uhc_common.h"
#include "r_usb_host.h"

LOG_MODULE_REGISTER(uhc_renesas_ra, CONFIG_UHC_DRIVER_LOG_LEVEL);

#define UHC_RENESA_RA_MAX_UDEV 5

enum uhc_renesas_ra_event_type {
	/* Shim driver event to trigger next transfer */
	UHC_RENESAS_RA_EVT_XFER,
	/* Device speed check, typically performed after a RESET signal is issued */
	UHC_RENESAS_RA_EVT_POLL_DEVICE_SPEED,
};

struct uhc_renesas_ra_evt {
	enum uhc_renesas_ra_event_type type;
};

struct uhc_renesas_ra_data {
	const struct device *dev;
	struct uhc_transfer *last_xfer;
	struct k_thread thread_data;
	struct k_msgq msgq;
	struct st_usbh_instance_ctrl uhc_ctrl;
	struct st_usb_cfg uhc_cfg;
	uint8_t devadd[UHC_RENESA_RA_MAX_UDEV];
	atomic_t ep[UHC_RENESA_RA_MAX_UDEV];
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
	fsp_err_t err;
	int ret;

	err = R_USBH_EdptOpen(&priv->uhc_ctrl, xfer->udev->addr, &ep_desc);
	if (err == FSP_SUCCESS) {
		LOG_INF("ep 0x%02x has been opened", xfer->ep);
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

static int uhc_renesas_ra_schedule_xfer(const struct device *dev)
{
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);

	if (priv->last_xfer == NULL) {
		/* Allocate buffer to establish a new transaction */
		priv->last_xfer = uhc_xfer_get_next(dev);
		if (priv->last_xfer == NULL) {
			LOG_DBG("Nothing to transfer");
			return 0;
		}
	}

	if (USB_EP_GET_IDX(priv->last_xfer->ep) == 0) {
		/*
		 * If there is a control xfer, the driver should maintaince control xfer stage
		 * update properly to not conrrupt the control transfer seq
		 */
		return uhc_renesas_ra_control_xfer(dev, priv->last_xfer);
	}

	return uhc_renesas_ra_data_xfer(dev, priv->last_xfer);
}

static void uhc_control_stage_update(const struct device *dev)
{
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);
	struct uhc_transfer *xfer = priv->last_xfer;

	switch (xfer->stage) {
	case UHC_CONTROL_STAGE_SETUP: {
		if (xfer->buf != NULL) {
			/* S-[in]-status or S-[out]-status */
			xfer->stage = UHC_CONTROL_STAGE_DATA;
			uhc_renesas_ra_xfer_request(dev);
		} else {
			/* S-[status] */
			xfer->stage = UHC_CONTROL_STAGE_STATUS;
			uhc_renesas_ra_control_status_xfer(dev, xfer);
		}
		break;
	}
	case UHC_CONTROL_STAGE_DATA:
		/* S-in-[status] or S-out-[status] */
		xfer->stage = UHC_CONTROL_STAGE_STATUS;
		uhc_renesas_ra_control_status_xfer(dev, xfer);
		break;
	case UHC_CONTROL_STAGE_STATUS:
		/* Transfer is completed */
		uhc_xfer_return(dev, xfer, 0);
		priv->last_xfer = NULL;
		break;
	default:
		break;
	}
}

static int uhc_renesas_ra_event_xfer_complete(const struct device *dev, usbh_event_t *hal_evt)
{
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);
	struct uhc_transfer *const xfer = priv->last_xfer;
	int ret = 0;

	switch (hal_evt->complete.result) {
	case USB_XFER_RESULT_STALLED:
	case USB_XFER_RESULT_TIMEOUT:
	case USB_XFER_RESULT_FAILED:
		uhc_xfer_return(dev, priv->last_xfer, -EPIPE);
		ret = -EAGAIN;
		break;
	case USB_XFER_RESULT_SUCCESS: {
		uint8_t ep = hal_evt->complete.ep_addr;

		if (USB_EP_GET_IDX(ep) == 0) {
			if (ep == USB_CONTROL_EP_IN &&
			    priv->last_xfer->stage == UHC_CONTROL_STAGE_DATA) {
				net_buf_add(xfer->buf, hal_evt->complete.len);
			}

			uhc_control_stage_update(dev);
			break;
		}

		if (USB_EP_DIR_IS_IN(ep)) {
			if (hal_evt->complete.len > 0) {
				net_buf_add(xfer->buf, hal_evt->complete.len);
			}
		}

		uhc_xfer_return(dev, xfer, 0);
		priv->last_xfer = NULL;
		break;
	}
	default:
		/* USB_XFER_RESULT_INVALID */
		ret = -EINVAL;
	}

	return ret;
}

static int uhc_renesas_ra_ep_enqueue(const struct device *dev, struct uhc_transfer *const xfer)
{
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);
	fsp_err_t err;
	int ret, i;

	ret = uhc_xfer_append(dev, xfer);
	if (ret != 0) {
		return ret;
	}

	for (i = 0; i < UHC_RENESA_RA_MAX_UDEV; i++) {
		if (priv->devadd[i] == xfer->udev->addr) {
			break;
		}

		if (priv->devadd[i] == 0) {
			err = R_USBH_PortOpen(&priv->uhc_ctrl, xfer->udev->addr);
			if (err != FSP_SUCCESS) {
				return -EIO;
			}

			priv->devadd[i] = xfer->udev->addr;
			break;
		}
	}

	if (USB_EP_GET_IDX(xfer->ep) != 0 &&
	    !atomic_test_bit(&priv->ep[i], USB_EP_GET_IDX(xfer->ep))) {
		ret = uhc_renesas_ra_edpt_open(dev, xfer);
		if (ret != 0) {
			uhc_xfer_return(dev, xfer, ret);
			return ret;
		}

		atomic_set_bit(&priv->ep[i], USB_EP_GET_IDX(xfer->ep));
	}

	uhc_renesas_ra_xfer_request(dev);

	return 0;
}

static int uhc_renesas_ra_ep_dequeue(const struct device *dev, struct uhc_transfer *const xfer)
{
	struct uhc_renesas_ra_data *priv = uhc_get_private(dev);
	struct uhc_transfer *const last_xfer = priv->last_xfer;
	fsp_err_t err;

	if (last_xfer != xfer) {
		sys_dlist_remove(&xfer->node);
		uhc_xfer_free(dev, xfer);
	}

	err = R_USBH_XferAbort(&priv->uhc_ctrl, last_xfer->udev->addr, last_xfer->ep);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

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

	err = R_USBH_PortOpen(&priv->uhc_ctrl, 0);
	if (err != FSP_SUCCESS) {
		return -EIO;
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
			ret = uhc_renesas_ra_schedule_xfer(dev);
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
		atomic_clear(&priv->ep[i]);
	}

#if DT_HAS_COMPAT_STATUS_OKAY(renesas_ra_usbhs)
	if (priv->uhc_cfg.hs_irq != FSP_INVALID_VECTOR) {
		irq_enable(priv->uhc_cfg.hs_irq);
	}
#endif

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

	if (priv->last_xfer != NULL) {
		uhc_xfer_free(dev, priv->last_xfer);
		priv->last_xfer = NULL;
	}

	if (R_USBH_Close(&priv->uhc_ctrl) != FSP_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static const struct uhc_api uhc_renesas_ra_api = {
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

#if DT_HAS_COMPAT_STATUS_OKAY(renesas_ra_usbhs)
	if (priv->uhc_cfg.hs_irq != FSP_INVALID_VECTOR) {
		R_ICU->IELSR[priv->uhc_cfg.hs_irq] = BSP_PRV_IELS_ENUM(EVENT_USBHS_USB_INT_RESUME);
	}
#endif

	if (priv->uhc_cfg.irq != FSP_INVALID_VECTOR) {
		R_ICU->IELSR[priv->uhc_cfg.irq] = BSP_PRV_IELS_ENUM(EVENT_USBFS_INT);
	}

	if (priv->uhc_cfg.irq_r != FSP_INVALID_VECTOR) {
		R_ICU->IELSR[priv->uhc_cfg.irq_r] = BSP_PRV_IELS_ENUM(EVENT_USBFS_RESUME);
	}

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
