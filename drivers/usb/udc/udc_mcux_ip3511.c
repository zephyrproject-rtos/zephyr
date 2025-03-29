/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT nxp_lpcip3511

#include <soc.h>
#include <string.h>
#include <stdio.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/drivers/pinctrl.h>

#include "udc_common.h"
#include "usb.h"
#include "usb_device_config.h"
#include "usb_device_mcux_drv_port.h"
#include "usb_device_lpcip3511.h"
#include "usb_phy.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(udc_mcux, CONFIG_UDC_DRIVER_LOG_LEVEL);

/*
 * There is no real advantage to change control endpoint size
 * but we can use it for testing UDC driver API and higher layers.
 */
#define USB_MCUX_MPS0		UDC_MPS0_64
#define USB_MCUX_EP0_SIZE	64

#define PRV_DATA_HANDLE(_handle) CONTAINER_OF(_handle, struct udc_mcux_data, mcux_device)

struct udc_mcux_config {
	const usb_device_controller_interface_struct_t *mcux_if;
	void (*irq_enable_func)(const struct device *dev);
	void (*irq_disable_func)(const struct device *dev);
	size_t num_of_eps;
	struct udc_ep_config *ep_cfg_in;
	struct udc_ep_config *ep_cfg_out;
	uintptr_t base;
	const struct pinctrl_dev_config *pincfg;
	usb_phy_config_struct_t *phy_config;
};

struct udc_mcux_data {
	const struct device *dev;
	usb_device_struct_t mcux_device;
	struct k_work work;
	struct k_fifo fifo;
	uint8_t controller_id; /* 0xFF is invalid value */
};

/* Structure for driver's events */
struct udc_mcux_event {
	sys_snode_t node;
	const struct device *dev;
	usb_device_callback_message_struct_t mcux_msg;
};

K_MEM_SLAB_DEFINE(udc_event_slab, sizeof(struct udc_mcux_event),
		  CONFIG_UDC_NXP_EVENT_COUNT, sizeof(void *));

static void udc_mcux_lock(const struct device *dev)
{
	udc_lock_internal(dev, K_FOREVER);
}

static void udc_mcux_unlock(const struct device *dev)
{
	udc_unlock_internal(dev);
}

static int udc_mcux_control(const struct device *dev, usb_device_control_type_t command,
				void *param)
{
	const struct udc_mcux_config *config = dev->config;
	const usb_device_controller_interface_struct_t *mcux_if = config->mcux_if;
	struct udc_mcux_data *priv = udc_get_private(dev);
	usb_status_t status;

	status = mcux_if->deviceControl(priv->mcux_device.controllerHandle,
			command, param);

	if (status != kStatus_USB_Success) {
		return -ENOMEM;
	}

	return 0;
}

/* If ep is busy, return busy. Otherwise feed the buf to controller */
static int udc_mcux_ep_feed(const struct device *dev,
			struct udc_ep_config *const cfg,
			struct net_buf *const buf)
{
	const struct udc_mcux_config *config = dev->config;
	const usb_device_controller_interface_struct_t *mcux_if = config->mcux_if;
	struct udc_mcux_data *priv = udc_get_private(dev);
	usb_status_t status = kStatus_USB_Success;
	uint8_t *data;
	uint32_t len;
	usb_device_endpoint_status_struct_t ep_status;

	ep_status.endpointAddress = cfg->addr;
	udc_mcux_control(dev, kUSB_DeviceControlGetEndpointStatus, &ep_status);
	if (ep_status.endpointStatus == kUSB_DeviceEndpointStateStalled) {
		return -EACCES; /* stalled */
	}

	udc_mcux_lock(dev);
	if (!udc_ep_is_busy(cfg)) {
		udc_ep_set_busy(cfg, true);
		udc_mcux_unlock(dev);

		if (USB_EP_DIR_IS_OUT(cfg->addr)) {
			len = net_buf_tailroom(buf);
			data = net_buf_tail(buf);
			status = mcux_if->deviceRecv(priv->mcux_device.controllerHandle,
					cfg->addr, data, len);
		} else {
			len = buf->len;
			data = buf->data;
			status = mcux_if->deviceSend(priv->mcux_device.controllerHandle,
					cfg->addr, data, len);
		}

		udc_mcux_lock(dev);
		if (status != kStatus_USB_Success) {
			udc_ep_set_busy(cfg, false);
		}
		udc_mcux_unlock(dev);
	} else {
		udc_mcux_unlock(dev);
		return -EBUSY;
	}

	return (status == kStatus_USB_Success ? 0 : -EIO);
}

/* return success if the ep is busy or stalled. */
static int udc_mcux_ep_try_feed(const struct device *dev,
			struct udc_ep_config *const cfg)
{
	struct net_buf *feed_buf;

	feed_buf = udc_buf_peek(cfg);
	if (feed_buf) {
		int ret = udc_mcux_ep_feed(dev, cfg, feed_buf);

		return ((ret == -EBUSY || ret == -EACCES || ret == 0) ? 0 : -EIO);
	}

	return 0;
}

/*
 * Allocate buffer and initiate a new control OUT transfer.
 */
static int udc_mcux_ctrl_feed_dout(const struct device *dev,
				   const size_t length)
{
	struct net_buf *buf;
	struct udc_ep_config *cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT);
	int ret;

	buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT, length);
	if (buf == NULL) {
		return -ENOMEM;
	}

	k_fifo_put(&cfg->fifo, buf);

	ret = udc_mcux_ep_feed(dev, cfg, buf);

	if (ret) {
		net_buf_unref(buf);
		return ret;
	}

	return 0;
}

static int udc_mcux_handler_setup(const struct device *dev, struct usb_setup_packet *setup)
{
	int err;
	struct net_buf *buf;

	LOG_DBG("setup packet");
	buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT,
			sizeof(struct usb_setup_packet));
	if (buf == NULL) {
		LOG_ERR("Failed to allocate for setup");
		return -EIO;
	}

	udc_ep_buf_set_setup(buf);
	memcpy(buf->data, setup, 8);
	net_buf_add(buf, 8);

	if (setup->RequestType.type == USB_REQTYPE_TYPE_STANDARD &&
	    setup->RequestType.direction == USB_REQTYPE_DIR_TO_DEVICE &&
	    setup->bRequest == USB_SREQ_SET_ADDRESS &&
	    setup->wLength == 0) {
		udc_mcux_control(dev, kUSB_DeviceControlPreSetDeviceAddress,
			&setup->wValue);
	}

	/* Update to next stage of control transfer */
	udc_ctrl_update_stage(dev, buf);

	if (!buf->len) {
		return -EIO;
	}

	if (udc_ctrl_stage_is_data_out(dev)) {
		/*  Allocate and feed buffer for data OUT stage */
		LOG_DBG("s:%p|feed for -out-", buf);
		err = udc_mcux_ctrl_feed_dout(dev, udc_data_stage_length(buf));
		if (err == -ENOMEM) {
			err = udc_submit_ep_event(dev, buf, err);
		}
	} else if (udc_ctrl_stage_is_data_in(dev)) {
		err = udc_ctrl_submit_s_in_status(dev);
	} else {
		err = udc_ctrl_submit_s_status(dev);
	}

	return err;
}

static int udc_mcux_handler_ctrl_out(const struct device *dev, struct net_buf *buf,
				uint8_t *mcux_buf, uint16_t mcux_len)
{
	int err = 0;
	uint32_t len;

	len = MIN(net_buf_tailroom(buf), mcux_len);
	net_buf_add(buf, len);
	if (udc_ctrl_stage_is_status_out(dev)) {
		/* Update to next stage of control transfer */
		udc_ctrl_update_stage(dev, buf);
		/* Status stage finished, notify upper layer */
		err = udc_ctrl_submit_status(dev, buf);
	} else {
		/* Update to next stage of control transfer */
		udc_ctrl_update_stage(dev, buf);
	}

	if (udc_ctrl_stage_is_status_in(dev)) {
		err = udc_ctrl_submit_s_out_status(dev, buf);
	}

	return err;
}

static int udc_mcux_handler_ctrl_in(const struct device *dev, struct net_buf *buf,
				uint8_t *mcux_buf, uint16_t mcux_len)
{
	int err = 0;
	uint32_t len;

	len = MIN(buf->len, mcux_len);
	buf->data += len;
	buf->len -= len;

	if (udc_ctrl_stage_is_status_in(dev) ||
	udc_ctrl_stage_is_no_data(dev)) {
		/* Status stage finished, notify upper layer */
		err = udc_ctrl_submit_status(dev, buf);
	}

	/* Update to next stage of control transfer */
	udc_ctrl_update_stage(dev, buf);

	if (udc_ctrl_stage_is_status_out(dev)) {
		/*
		 * IN transfer finished, release buffer,
		 * control OUT buffer should be already fed.
		 */
		net_buf_unref(buf);
		err = udc_mcux_ctrl_feed_dout(dev, 0u);
	}

	return err;
}

static int udc_mcux_handler_non_ctrl_in(const struct device *dev, uint8_t ep,
			struct net_buf *buf, uint8_t *mcux_buf, uint16_t mcux_len)
{
	int err;
	uint32_t len;

	len = MIN(buf->len, mcux_len);
	buf->data += len;
	buf->len -= len;

	err = udc_submit_ep_event(dev, buf, 0);
	udc_mcux_ep_try_feed(dev, udc_get_ep_cfg(dev, ep));

	return err;
}

static int udc_mcux_handler_non_ctrl_out(const struct device *dev, uint8_t ep,
			struct net_buf *buf, uint8_t *mcux_buf, uint16_t mcux_len)
{
	int err;
	uint32_t len;

	len = MIN(net_buf_tailroom(buf), mcux_len);
	net_buf_add(buf, len);

	err = udc_submit_ep_event(dev, buf, 0);
	udc_mcux_ep_try_feed(dev, udc_get_ep_cfg(dev, ep));

	return err;
}

static int udc_mcux_handler_out(const struct device *dev, uint8_t ep,
				uint8_t *mcux_buf, uint16_t mcux_len)
{
	struct udc_ep_config *const cfg = udc_get_ep_cfg(dev, ep);
	int err;
	struct net_buf *buf;

	buf = udc_buf_get(cfg);

	udc_mcux_lock(dev);
	udc_ep_set_busy(cfg, false);
	udc_mcux_unlock(dev);

	if (buf == NULL) {
		udc_submit_event(dev, UDC_EVT_ERROR, -ENOBUFS);
		return -ENOBUFS;
	}

	if (ep == USB_CONTROL_EP_OUT) {
		err = udc_mcux_handler_ctrl_out(dev, buf, mcux_buf, mcux_len);
	} else {
		err = udc_mcux_handler_non_ctrl_out(dev, ep, buf, mcux_buf, mcux_len);
	}

	return err;
}

/* return true - zlp is feed; false - no zlp */
static bool udc_mcux_handler_zlt(const struct device *dev, uint8_t ep, struct net_buf *buf,
				uint16_t mcux_len)
{
	const struct udc_mcux_config *config = dev->config;
	const usb_device_controller_interface_struct_t *mcux_if = config->mcux_if;
	struct udc_mcux_data *priv = udc_get_private(dev);

	/* The whole transfer is already done by MCUX controller driver. */
	if (mcux_len >= buf->len) {
		if (udc_ep_buf_has_zlp(buf)) {
			usb_status_t status;

			udc_ep_buf_clear_zlp(buf);
			status = mcux_if->deviceSend(priv->mcux_device.controllerHandle,
					ep, NULL, 0);
			if (status != kStatus_USB_Success) {
				udc_submit_event(dev, UDC_EVT_ERROR, -EIO);
				return false;
			}
			return true;
		}
	}

	return false;
}

static int udc_mcux_handler_in(const struct device *dev, uint8_t ep,
				uint8_t *mcux_buf, uint16_t mcux_len)
{
	struct udc_ep_config *const cfg = udc_get_ep_cfg(dev, ep);
	int err;
	struct net_buf *buf;

	buf = udc_buf_peek(cfg);
	if (buf == NULL) {
		udc_submit_event(dev, UDC_EVT_ERROR, -ENOBUFS);
		return -ENOBUFS;
	}

	if (udc_mcux_handler_zlt(dev, ep, buf, mcux_len)) {
		return 0;
	}

	buf = udc_buf_get(cfg);

	udc_mcux_lock(dev);
	udc_ep_set_busy(cfg, false);
	udc_mcux_unlock(dev);

	if (buf == NULL) {
		udc_submit_event(dev, UDC_EVT_ERROR, -ENOBUFS);
		return -ENOBUFS;
	}
	if (ep == USB_CONTROL_EP_IN) {
		err = udc_mcux_handler_ctrl_in(dev, buf, mcux_buf, mcux_len);
	} else {
		err = udc_mcux_handler_non_ctrl_in(dev, ep, buf, mcux_buf, mcux_len);
	}

	return err;
}

static void udc_mcux_event_submit(const struct device *dev,
				  const usb_device_callback_message_struct_t *mcux_msg)
{
	struct udc_mcux_data *priv = udc_get_private(dev);
	struct udc_mcux_event *ev;
	int ret;

	ret = k_mem_slab_alloc(&udc_event_slab, (void **)&ev, K_NO_WAIT);
	if (ret) {
		udc_submit_event(dev, UDC_EVT_ERROR, ret);
		LOG_ERR("Failed to allocate slab");
		return;
	}

	ev->dev = dev;
	ev->mcux_msg = *mcux_msg;
	k_fifo_put(&priv->fifo, ev);
	k_work_submit_to_queue(udc_get_work_q(), &priv->work);
}

static void udc_mcux_work_handler(struct k_work *item)
{
	struct udc_mcux_event *ev;
	struct udc_mcux_data *priv;
	usb_device_callback_message_struct_t *mcux_msg;
	int err;
	uint8_t ep;

	priv = CONTAINER_OF(item, struct udc_mcux_data, work);
	while ((ev = k_fifo_get(&priv->fifo, K_NO_WAIT)) != NULL) {
		mcux_msg = &ev->mcux_msg;

		if (mcux_msg->code == kUSB_DeviceNotifyBusReset) {
			struct udc_ep_config *cfg;

			udc_mcux_control(ev->dev, kUSB_DeviceControlSetDefaultStatus, NULL);
			cfg = udc_get_ep_cfg(ev->dev, USB_CONTROL_EP_OUT);
			if (cfg->stat.enabled) {
				udc_ep_disable_internal(ev->dev, USB_CONTROL_EP_OUT);
			}
			cfg = udc_get_ep_cfg(ev->dev, USB_CONTROL_EP_IN);
			if (cfg->stat.enabled) {
				udc_ep_disable_internal(ev->dev, USB_CONTROL_EP_IN);
			}
			if (udc_ep_enable_internal(ev->dev, USB_CONTROL_EP_OUT,
						USB_EP_TYPE_CONTROL,
						USB_MCUX_EP0_SIZE, 0)) {
				LOG_ERR("Failed to enable control endpoint");
			}

			if (udc_ep_enable_internal(ev->dev, USB_CONTROL_EP_IN,
						USB_EP_TYPE_CONTROL,
						USB_MCUX_EP0_SIZE, 0)) {
				LOG_ERR("Failed to enable control endpoint");
			}
			udc_submit_event(ev->dev, UDC_EVT_RESET, 0);
		} else {
			ep  = mcux_msg->code;

			if (mcux_msg->isSetup) {
				struct usb_setup_packet *setup =
					(struct usb_setup_packet *)mcux_msg->buffer;

				err = udc_mcux_handler_setup(ev->dev, setup);
			} else if (USB_EP_DIR_IS_IN(ep)) {
				err = udc_mcux_handler_in(ev->dev, ep, mcux_msg->buffer,
							  mcux_msg->length);
			} else {
				err = udc_mcux_handler_out(ev->dev, ep, mcux_msg->buffer,
							   mcux_msg->length);
			}

			if (unlikely(err)) {
				udc_submit_event(ev->dev, UDC_EVT_ERROR, err);
			}
		}

		k_mem_slab_free(&udc_event_slab, (void *)ev);
	}
}

/* NXP MCUX controller driver notify transfers/status through this interface */
usb_status_t USB_DeviceNotificationTrigger(void *handle, void *msg)
{
	usb_device_callback_message_struct_t *mcux_msg = msg;
	usb_device_notification_t mcux_notify;
	struct udc_mcux_data *priv;
	const struct device *dev;
	usb_status_t mcux_status = kStatus_USB_Success;

	if ((NULL == msg) || (NULL == handle)) {
		return kStatus_USB_InvalidHandle;
	}

	mcux_notify = (usb_device_notification_t)mcux_msg->code;
	priv = (struct udc_mcux_data *)(PRV_DATA_HANDLE(handle));
	dev = priv->dev;

	switch (mcux_notify) {
	case kUSB_DeviceNotifyBusReset:
		udc_mcux_event_submit(dev, mcux_msg);
		break;
	case kUSB_DeviceNotifyError:
		udc_submit_event(dev, UDC_EVT_ERROR, -EIO);
		break;
	case kUSB_DeviceNotifySuspend:
		udc_set_suspended(dev, true);
		udc_submit_event(dev, UDC_EVT_SUSPEND, 0);
		break;
	case kUSB_DeviceNotifyResume:
		udc_set_suspended(dev, false);
		udc_submit_event(dev, UDC_EVT_RESUME, 0);
		break;
	case kUSB_DeviceNotifyLPMSleep:
		break;
	case kUSB_DeviceNotifyDetach:
		udc_submit_event(dev, UDC_EVT_VBUS_REMOVED, 0);
		break;
	case kUSB_DeviceNotifyAttach:
		udc_submit_event(dev, UDC_EVT_VBUS_READY, 0);
		break;
	case kUSB_DeviceNotifySOF:
		udc_submit_event(dev, UDC_EVT_SOF, 0);
		break;
	default:
		udc_mcux_event_submit(dev, mcux_msg);
		break;
	}

	return mcux_status;
}

static void udc_mcux_isr(const struct device *dev)
{
	struct udc_mcux_data *priv = udc_get_private(dev);

	USB_DeviceLpcIp3511IsrFunction((void *)(&priv->mcux_device));
}

/* Return actual USB device speed */
static enum udc_bus_speed udc_mcux_device_speed(const struct device *dev)
{
	int err;
	uint8_t mcux_speed;

	err = udc_mcux_control(dev, kUSB_DeviceControlGetSpeed, &mcux_speed);
	if (err) {
		/*
		 * In the current version of all NXP USB device drivers,
		 * no error is returned if the parameter is correct.
		 */
		return UDC_BUS_SPEED_FS;
	}

	switch (mcux_speed) {
	case USB_SPEED_HIGH:
		return UDC_BUS_SPEED_HS;
	case USB_SPEED_LOW:
		__ASSERT(false, "Low speed mode not supported");
		__fallthrough;
	case USB_SPEED_FULL:
		__fallthrough;
	default:
		return UDC_BUS_SPEED_FS;
	}
}

static int udc_mcux_ep_enqueue(const struct device *dev,
			       struct udc_ep_config *const cfg,
			       struct net_buf *const buf)
{
	udc_buf_put(cfg, buf);
	if (cfg->stat.halted) {
		LOG_DBG("ep 0x%02x halted", cfg->addr);
		return 0;
	}

	return udc_mcux_ep_try_feed(dev, cfg);
}

static int udc_mcux_ep_dequeue(const struct device *dev,
			       struct udc_ep_config *const cfg)
{
	struct net_buf *buf;

	cfg->stat.halted = false;
	buf = udc_buf_get_all(cfg);
	if (buf) {
		udc_submit_ep_event(dev, buf, -ECONNABORTED);
	}

	udc_mcux_lock(dev);
	udc_ep_set_busy(cfg, false);
	udc_mcux_unlock(dev);

	return 0;
}

static int udc_mcux_ep_set_halt(const struct device *dev,
				struct udc_ep_config *const cfg)
{
	return udc_mcux_control(dev, kUSB_DeviceControlEndpointStall, &cfg->addr);
}

static int udc_mcux_ep_clear_halt(const struct device *dev,
				  struct udc_ep_config *const cfg)
{
	(void)udc_mcux_control(dev, kUSB_DeviceControlEndpointUnstall, &cfg->addr);
	/* transfer is enqueued after stalled */
	return udc_mcux_ep_try_feed(dev, cfg);
}

static int udc_mcux_ep_enable(const struct device *dev,
			      struct udc_ep_config *const cfg)
{
	usb_device_endpoint_init_struct_t ep_init;

	LOG_DBG("Enable ep 0x%02x", cfg->addr);

	ep_init.zlt             = 0U;
	ep_init.interval        = cfg->interval;
	ep_init.endpointAddress = cfg->addr;
	/* HAL expects wMaxPacketSize value directly in maxPacketSize field */
	ep_init.maxPacketSize   = cfg->mps;

	switch (cfg->attributes & USB_EP_TRANSFER_TYPE_MASK) {
	case USB_EP_TYPE_CONTROL:
		ep_init.transferType = USB_ENDPOINT_CONTROL;
		break;
	case USB_EP_TYPE_BULK:
		ep_init.transferType = USB_ENDPOINT_BULK;
		break;
	case USB_EP_TYPE_INTERRUPT:
		ep_init.transferType = USB_ENDPOINT_INTERRUPT;
		break;
	case USB_EP_TYPE_ISO:
		ep_init.transferType = USB_ENDPOINT_ISOCHRONOUS;
		break;
	default:
		return -EINVAL;
	}

	return udc_mcux_control(dev, kUSB_DeviceControlEndpointInit, &ep_init);
}

static int udc_mcux_ep_disable(const struct device *dev,
			       struct udc_ep_config *const cfg)
{
	LOG_DBG("Disable ep 0x%02x", cfg->addr);

	return udc_mcux_control(dev, kUSB_DeviceControlEndpointDeinit, &cfg->addr);
}

static int udc_mcux_host_wakeup(const struct device *dev)
{
	return -ENOTSUP;
}

static int udc_mcux_set_address(const struct device *dev, const uint8_t addr)
{
	uint8_t temp_addr = addr;

	return udc_mcux_control(dev, kUSB_DeviceControlSetDeviceAddress, &temp_addr);
}

static int udc_mcux_enable(const struct device *dev)
{
	return udc_mcux_control(dev, kUSB_DeviceControlRun, NULL);
}

static int udc_mcux_disable(const struct device *dev)
{
	return udc_mcux_control(dev, kUSB_DeviceControlStop, NULL);
}

static int udc_mcux_init(const struct device *dev)
{
	const struct udc_mcux_config *config = dev->config;
	const usb_device_controller_interface_struct_t *mcux_if = config->mcux_if;
	struct udc_mcux_data *priv = udc_get_private(dev);
	usb_status_t status;

	if (priv->controller_id == 0xFFu) {
		return -ENOMEM;
	}

#ifdef CONFIG_DT_HAS_NXP_USBPHY_ENABLED
	if (config->phy_config != NULL) {
		USB_EhciPhyInit(priv->controller_id, 0u, config->phy_config);
	}
#endif

	/* Init MCUX USB device driver. */
	status = mcux_if->deviceInit(priv->controller_id,
		&priv->mcux_device, &(priv->mcux_device.controllerHandle));
	if (status != kStatus_USB_Success) {
		return -ENOMEM;
	}

	/* enable USB interrupt */
	config->irq_enable_func(dev);

	LOG_DBG("Initialized USB controller %x", (uint32_t)config->base);

	return 0;
}

static int udc_mcux_shutdown(const struct device *dev)
{
	const struct udc_mcux_config *config = dev->config;
	const usb_device_controller_interface_struct_t *mcux_if = config->mcux_if;
	struct udc_mcux_data *priv = udc_get_private(dev);
	usb_status_t status;

	/* Disable interrupt */
	config->irq_disable_func(dev);

	/* De-init MCUX USB device driver. */
	status = mcux_if->deviceDeinit(priv->mcux_device.controllerHandle);
	if (status != kStatus_USB_Success) {
		return -ENOMEM;
	}

	return 0;
}

static inline void udc_mcux_get_hal_driver_id(struct udc_mcux_data *priv,
			const struct udc_mcux_config *config)
{
	/*
	 * MCUX USB controller drivers use an ID to tell the HAL drivers
	 * which controller is being used. This part of the code converts
	 * the base address to the ID value.
	 */
#ifdef USB_BASE_ADDRS
	uintptr_t ip3511_fs_base[] = USB_BASE_ADDRS;
#endif
#ifdef USBHSD_BASE_ADDRS
	uintptr_t ip3511_hs_base[] = USBHSD_BASE_ADDRS;
#endif

	/* get the right controller id */
	priv->controller_id = 0xFFu; /* invalid value */
#ifdef USB_BASE_ADDRS
	for (uint8_t i = 0; i < ARRAY_SIZE(ip3511_fs_base); i++) {
		if (ip3511_fs_base[i] == config->base) {
			priv->controller_id = kUSB_ControllerLpcIp3511Fs0 + i;
			break;
		}
	}
#endif

#ifdef USBHSD_BASE_ADDRS
	if (priv->controller_id == 0xFF) {
		for (uint8_t i = 0; i < ARRAY_SIZE(ip3511_hs_base); i++) {
			if (ip3511_hs_base[i] == config->base) {
				priv->controller_id = kUSB_ControllerLpcIp3511Hs0 + i;
				break;
			}
		}
	}
#endif
}

static int udc_mcux_driver_preinit(const struct device *dev)
{
	const struct udc_mcux_config *config = dev->config;
	struct udc_data *data = dev->data;
	struct udc_mcux_data *priv = data->priv;
	int err;

	udc_mcux_get_hal_driver_id(priv, config);
	if (priv->controller_id == 0xFFu) {
		return -ENOMEM;
	}

	k_mutex_init(&data->mutex);
	k_fifo_init(&priv->fifo);
	k_work_init(&priv->work, udc_mcux_work_handler);

	for (int i = 0; i < config->num_of_eps; i++) {
		config->ep_cfg_out[i].caps.out = 1;
		if (i == 0) {
			config->ep_cfg_out[i].caps.control = 1;
			config->ep_cfg_out[i].caps.mps = 64;
		} else {
			config->ep_cfg_out[i].caps.bulk = 1;
			config->ep_cfg_out[i].caps.interrupt = 1;
			config->ep_cfg_out[i].caps.iso = 1;
			config->ep_cfg_out[i].caps.mps = 1024;
			if ((priv->controller_id == kUSB_ControllerLpcIp3511Hs0) ||
			    (priv->controller_id == kUSB_ControllerLpcIp3511Hs1)) {
				config->ep_cfg_out[i].caps.high_bandwidth = 1;
			}
		}

		config->ep_cfg_out[i].addr = USB_EP_DIR_OUT | i;
		err = udc_register_ep(dev, &config->ep_cfg_out[i]);
		if (err != 0) {
			LOG_ERR("Failed to register endpoint");
			return err;
		}
	}

	for (int i = 0; i < config->num_of_eps; i++) {
		config->ep_cfg_in[i].caps.in = 1;
		if (i == 0) {
			config->ep_cfg_in[i].caps.control = 1;
			config->ep_cfg_in[i].caps.mps = 64;
		} else {
			config->ep_cfg_in[i].caps.bulk = 1;
			config->ep_cfg_in[i].caps.interrupt = 1;
			config->ep_cfg_in[i].caps.iso = 1;
			config->ep_cfg_in[i].caps.mps = 1024;
			if ((priv->controller_id == kUSB_ControllerLpcIp3511Hs0) ||
			    (priv->controller_id == kUSB_ControllerLpcIp3511Hs1)) {
				config->ep_cfg_in[i].caps.high_bandwidth = 1;
			}
		}

		config->ep_cfg_in[i].addr = USB_EP_DIR_IN | i;
		err = udc_register_ep(dev, &config->ep_cfg_in[i]);
		if (err != 0) {
			LOG_ERR("Failed to register endpoint");
			return err;
		}
	}

	/* Requires udc_mcux_host_wakeup() implementation */
	data->caps.rwup = false;
	data->caps.mps0 = USB_MCUX_MPS0;
	if ((priv->controller_id == kUSB_ControllerLpcIp3511Hs0) ||
	    (priv->controller_id == kUSB_ControllerLpcIp3511Hs1)) {
		data->caps.hs = true;
	}
	priv->dev = dev;

	pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);

	return 0;
}

static const struct udc_api udc_mcux_api = {
	.device_speed = udc_mcux_device_speed,
	.ep_enqueue = udc_mcux_ep_enqueue,
	.ep_dequeue = udc_mcux_ep_dequeue,
	.ep_set_halt = udc_mcux_ep_set_halt,
	.ep_clear_halt = udc_mcux_ep_clear_halt,
	.ep_try_config = NULL,
	.ep_enable = udc_mcux_ep_enable,
	.ep_disable = udc_mcux_ep_disable,
	.host_wakeup = udc_mcux_host_wakeup,
	.set_address = udc_mcux_set_address,
	.enable = udc_mcux_enable,
	.disable = udc_mcux_disable,
	.init = udc_mcux_init,
	.shutdown = udc_mcux_shutdown,
	.lock = udc_mcux_lock,
	.unlock = udc_mcux_unlock,
};

/* IP3511 device driver interface */
static const usb_device_controller_interface_struct_t udc_mcux_if = {
	USB_DeviceLpc3511IpInit, USB_DeviceLpc3511IpDeinit, USB_DeviceLpc3511IpSend,
	USB_DeviceLpc3511IpRecv, USB_DeviceLpc3511IpCancel, USB_DeviceLpc3511IpControl
};

#define UDC_MCUX_PHY_DEFINE(n)								\
static usb_phy_config_struct_t phy_config_##n = {					\
	.D_CAL = DT_PROP_OR(DT_INST_PHANDLE(n, phy_handle), tx_d_cal, 0),		\
	.TXCAL45DP = DT_PROP_OR(DT_INST_PHANDLE(n, phy_handle), tx_cal_45_dp_ohms, 0),	\
	.TXCAL45DM = DT_PROP_OR(DT_INST_PHANDLE(n, phy_handle), tx_cal_45_dm_ohms, 0),	\
}

#define UDC_MCUX_PHY_DEFINE_OR(n)							\
	COND_CODE_1(DT_NODE_HAS_PROP(DT_DRV_INST(n), phy_handle),			\
		    (UDC_MCUX_PHY_DEFINE(n)), ())

#define UDC_MCUX_PHY_CFG_PTR_OR_NULL(n)							\
	COND_CODE_1(DT_NODE_HAS_PROP(DT_DRV_INST(n), phy_handle),			\
		    (&phy_config_##n), (NULL))

#define USB_MCUX_IP3511_DEVICE_DEFINE(n)						\
	UDC_MCUX_PHY_DEFINE_OR(n);							\
											\
	static void udc_irq_enable_func##n(const struct device *dev)			\
	{										\
		IRQ_CONNECT(DT_INST_IRQN(n),						\
			    DT_INST_IRQ(n, priority),					\
			    udc_mcux_isr,						\
			    DEVICE_DT_INST_GET(n), 0);					\
											\
		irq_enable(DT_INST_IRQN(n));						\
	}										\
											\
	static void udc_irq_disable_func##n(const struct device *dev)			\
	{										\
		irq_disable(DT_INST_IRQN(n));						\
	}										\
											\
	static struct udc_ep_config							\
		ep_cfg_out##n[DT_INST_PROP(n, num_bidir_endpoints)];			\
	static struct udc_ep_config							\
		ep_cfg_in##n[DT_INST_PROP(n, num_bidir_endpoints)];			\
											\
	PINCTRL_DT_INST_DEFINE(n);							\
											\
	static struct udc_mcux_config priv_config_##n = {				\
		.base = DT_INST_REG_ADDR(n),						\
		.irq_enable_func = udc_irq_enable_func##n,				\
		.irq_disable_func = udc_irq_disable_func##n,				\
		.num_of_eps = DT_INST_PROP(n, num_bidir_endpoints),			\
		.ep_cfg_in = ep_cfg_in##n,						\
		.ep_cfg_out = ep_cfg_out##n,						\
		.mcux_if = &udc_mcux_if,						\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),				\
		.phy_config = UDC_MCUX_PHY_CFG_PTR_OR_NULL(n),				\
	};										\
											\
	static struct udc_mcux_data priv_data_##n = {					\
	};										\
											\
	static struct udc_data udc_data_##n = {						\
		.mutex = Z_MUTEX_INITIALIZER(udc_data_##n.mutex),			\
		.priv = &priv_data_##n,							\
	};										\
											\
	DEVICE_DT_INST_DEFINE(n, udc_mcux_driver_preinit, NULL,				\
			      &udc_data_##n, &priv_config_##n,				\
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
			      &udc_mcux_api);

DT_INST_FOREACH_STATUS_OKAY(USB_MCUX_IP3511_DEVICE_DEFINE)
