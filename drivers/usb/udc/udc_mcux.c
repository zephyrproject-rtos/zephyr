/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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
#ifdef CONFIG_USB_UDC_NXP_PHY
#include "usb_phy.h"
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(udc_mcux, CONFIG_UDC_DRIVER_LOG_LEVEL);

#define USBFSOTG_BD_OWN		BIT(5)
#define USBFSOTG_BD_DATA1	BIT(4)
#define USBFSOTG_BD_KEEP	BIT(3)
#define USBFSOTG_BD_NINC	BIT(2)
#define USBFSOTG_BD_DTS		BIT(1)
#define USBFSOTG_BD_STALL	BIT(0)

#define USBFSOTG_SETUP_TOKEN	0x0D
#define USBFSOTG_IN_TOKEN	0x09
#define USBFSOTG_OUT_TOKEN	0x01

#define USBFSOTG_PERID		0x04
#define USBFSOTG_REV		0x33

/*
 * There is no real advantage to change control endpoint size
 * but we can use it for testing UDC driver API and higher layers.
 */
#define USB_MCUX_MPS0		UDC_MPS0_64
#define USB_MCUX_EP0_SIZE	64

#define PRV_DATA_HANDLE(_handle) CONTAINER_OF(_handle, struct udc_mcux_data, device_instance)

#if defined(CONFIG_USB_UDC_NXP_EHCI)
#include "usb_device_ehci.h"
/* EHCI device driver interface */
static const usb_device_controller_interface_struct_t mcux_ehci_usb_iface = {
	USB_DeviceEhciInit, USB_DeviceEhciDeinit, USB_DeviceEhciSend,
	USB_DeviceEhciRecv, USB_DeviceEhciCancel, USB_DeviceEhciControl
};
#endif

#if defined(CONFIG_USB_UDC_NXP_IP3511)
#include "fsl_device_registers.h"
#include "usb_device_lpcip3511.h"
/* LPCIP3511 device driver interface */
static const usb_device_controller_interface_struct_t mcux_ip3511_usb_iface = {
	USB_DeviceLpc3511IpInit, USB_DeviceLpc3511IpDeinit, USB_DeviceLpc3511IpSend,
	USB_DeviceLpc3511IpRecv, USB_DeviceLpc3511IpCancel, USB_DeviceLpc3511IpControl
};
#endif

struct udc_mcux_config {
	const usb_device_controller_interface_struct_t *mcux_if;
	void (*irq_enable_func)(const struct device *dev);
	void (*irq_disable_func)(const struct device *dev);
	size_t num_of_eps;
	struct udc_ep_config *ep_cfg_in;
	struct udc_ep_config *ep_cfg_out;
	uint32_t base;
	const struct pinctrl_dev_config *pincfg;
#ifdef CONFIG_USB_UDC_NXP_PHY
	uint8_t need_to_init_phy;
	uint8_t D_CAL;
	uint8_t TXCAL45DP;
	uint8_t TXCAL45DM;
#endif
};

struct udc_mcux_data {
	const struct device *dev;
	usb_device_struct_t device_instance;
	uint8_t controller_id;
};

/* TODO: implement the cache maintenance
 * solution1: Use the non-cached buf to do memcpy before/after giving buffer to usb controller.
 * solution2: Use cache API to flush/invalid cache. but it needs the given buffer is
 * cache line size aligned and the buffer range cover multiple of cache line size block.
 * Need to change the usb stack to implement it, will try to implement it later.
 */
#if defined(CONFIG_NOCACHE_MEMORY)
K_HEAP_DEFINE_NOCACHE(mcux_packet_alloc_pool, USB_DEVICE_CONFIG_ENDPOINTS * 2u * 1024u);

/* allocate non-cached buffer for usb */
static void *udc_mcux_nocache_alloc(uint32_t size)
{
	void *p = (void *)k_heap_alloc(&mcux_packet_alloc_pool, size, K_NO_WAIT);

	if (p != NULL) {
		(void)memset(p, 0, size);
	}

	return p;
}

/* free the allocated non-cached buffer */
static void udc_mcux_nocache_free(void *p)
{
	if (p == NULL) {
		return;
	}
	k_heap_free(&mcux_packet_alloc_pool, p);
}
#endif

/* If ep is busy, return busy. Otherwise feed the buf to controller */
static int usb_mcux_ep_feed(const struct device *dev,
			struct udc_ep_config *const cfg,
			struct net_buf *const buf)
{
	const struct udc_mcux_config *config = dev->config;
	struct udc_mcux_data *priv = udc_get_private(dev);
	usb_status_t status = kStatus_USB_Success;
	uint8_t *data;
	uint32_t len;
	int current_sr;

	current_sr = irq_lock();
	if (!udc_ep_is_busy(dev, cfg->addr)) {
		udc_ep_set_busy(dev, cfg->addr, true);
		irq_unlock(current_sr);

		if (USB_EP_DIR_IS_OUT(cfg->addr)) {
			len = net_buf_tailroom(buf); /* MIN(net_buf_tailroom(buf), cfg->mps); */
#if defined(CONFIG_NOCACHE_MEMORY)
			data = (len == 0 ? NULL : udc_mcux_nocache_alloc(len));
#else
			data = net_buf_tail(buf);
#endif
			status = config->mcux_if->deviceRecv(priv->device_instance.controllerHandle,
					cfg->addr, data, len);
		} else {
			len = buf->len; /* MIN(buf->len, cfg->mps); */
#if defined(CONFIG_NOCACHE_MEMORY)
			data = (len == 0 ? NULL : udc_mcux_nocache_alloc(len));
			memcpy(data, buf->data, len);
#else
			data = buf->data;
#endif
			status = config->mcux_if->deviceSend(priv->device_instance.controllerHandle,
					cfg->addr, data, len);
		}

		current_sr = irq_lock();
		if (status != kStatus_USB_Success) {
			udc_ep_set_busy(dev, cfg->addr, false);
		}
		irq_unlock(current_sr);
	} else {
		irq_unlock(current_sr);
		return -EBUSY;
	}

	return (status == kStatus_USB_Success ? 0 : -EIO);
}

/* return success if the ep is busy. */
static int usb_mcux_ep_try_feed(const struct device *dev,
			struct udc_ep_config *const cfg,
			struct net_buf *const buf)
{
	int ret = usb_mcux_ep_feed(dev, cfg, buf);

	return (((ret == -EBUSY) || (ret == 0)) ? 0 : -EIO);
}

static int usb_mcux_control(const struct device *dev, usb_device_control_type_t command,
				void *param)
{
	const struct udc_mcux_config *config = dev->config;
	struct udc_mcux_data *priv = udc_get_private(dev);
	usb_status_t status;

	status = config->mcux_if->deviceControl(priv->device_instance.controllerHandle,
			command, param);

	if (status != kStatus_USB_Success) {
		return -ENOMEM;
	}

	return 0;
}

/*
 * Allocate buffer and initiate a new control OUT transfer.
 */
static int usb_mcux_ctrl_feed_dout(const struct device *dev,
				   const size_t length)
{
	struct net_buf *buf;
	struct udc_ep_config *cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT);
	int ret;

	buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT, length);
	if (buf == NULL) {
		return -ENOMEM;
	}

	net_buf_put(&cfg->fifo, buf);

	ret = usb_mcux_ep_feed(dev, cfg, buf);

	if (ret) {
		net_buf_unref(buf);
		return ret;
	}

	return 0;
}

/*!
 * @brief Notify the device that the controller status changed.
 *
 * This function is used to notify the device that the controller status changed.
 *
 * @param handle    The device handle. It equals the value returned from USB_DeviceInit.
 * @param message   The device callback message handle.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
usb_status_t USB_DeviceNotificationTrigger(void *handle, void *msg)
{
	usb_device_callback_message_struct_t *mcux_msg =
		(usb_device_callback_message_struct_t *)msg;
	uint8_t ep;
	usb_device_notification_t mcux_notify;
	struct udc_mcux_data *priv;
	const struct device *dev;
	usb_status_t mcux_status = kStatus_USB_Success;
	int err;
	uint32_t len;

	if ((NULL == msg) || (NULL == handle)) {
		return kStatus_USB_InvalidHandle;
	}

	mcux_notify = (usb_device_notification_t)mcux_msg->code;
	priv = (struct udc_mcux_data *)(PRV_DATA_HANDLE(handle));
	dev = priv->dev;

	switch (mcux_notify) {
	case kUSB_DeviceNotifyBusReset:
		struct udc_ep_config *cfg;

		usb_mcux_control(dev, kUSB_DeviceControlSetDefaultStatus, NULL);
		cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT);
		if (cfg->stat.enabled) {
			udc_ep_disable_internal(dev, USB_CONTROL_EP_OUT);
		}
		cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_IN);
		if (cfg->stat.enabled) {
			udc_ep_disable_internal(dev, USB_CONTROL_EP_IN);
		}
		if (udc_ep_enable_internal(dev, USB_CONTROL_EP_OUT,
					USB_EP_TYPE_CONTROL,
					USB_MCUX_EP0_SIZE, 0)) {
			LOG_ERR("Failed to enable control endpoint");
			return -EIO;
		}

		if (udc_ep_enable_internal(dev, USB_CONTROL_EP_IN,
					USB_EP_TYPE_CONTROL,
					USB_MCUX_EP0_SIZE, 0)) {
			LOG_ERR("Failed to enable control endpoint");
			return -EIO;
		}
		udc_submit_event(dev, UDC_EVT_RESET, 0);
		break;
#if (defined(USB_DEVICE_CONFIG_ERROR_HANDLING) && (USB_DEVICE_CONFIG_ERROR_HANDLING > 0U))
	case kUSB_DeviceNotifyError:
		udc_submit_event(dev, UDC_EVT_ERROR, -EIO);
		break;
#endif
#if (defined(USB_DEVICE_CONFIG_LOW_POWER_MODE) && (USB_DEVICE_CONFIG_LOW_POWER_MODE > 0U))
	case kUSB_DeviceNotifySuspend:
		udc_set_suspended(dev, true);
		udc_submit_event(dev, UDC_EVT_SUSPEND, 0);
		break;
	case kUSB_DeviceNotifyResume:
		udc_set_suspended(dev, false);
		udc_submit_event(dev, UDC_EVT_RESUME, 0);
		break;
#if (defined(USB_DEVICE_CONFIG_LPM_L1) && (USB_DEVICE_CONFIG_LPM_L1 > 0U))
	case kUSB_DeviceNotifyLPMSleep:
		break;
#endif
#endif
#if (defined(USB_DEVICE_CONFIG_DETACH_ENABLE) && (USB_DEVICE_CONFIG_DETACH_ENABLE > 0U))
	case kUSB_DeviceNotifyDetach:
		udc_submit_event(dev, UDC_EVT_VBUS_REMOVED, 0);
		break;
	case kUSB_DeviceNotifyAttach:
		udc_submit_event(dev, UDC_EVT_VBUS_READY, 0);
		break;
#endif
	default:
		struct net_buf *buf = NULL;
		int current_sr;

		ep  = mcux_msg->code;
		if (!mcux_msg->isSetup) {
			buf = udc_buf_get(dev, ep);
			if (buf == NULL) {
				return kStatus_USB_Error;
			}

			current_sr = irq_lock();
			udc_ep_set_busy(dev, ep, false);
			irq_unlock(current_sr);
		}

		if (mcux_msg->isSetup) {
			struct usb_setup_packet *setup;

			LOG_DBG("setup packet");
			buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT,
					sizeof(struct usb_setup_packet));
			if (buf == NULL) {
				LOG_ERR("Failed to allocate for setup");
				return kStatus_USB_Error;
			}

			udc_ep_buf_set_setup(buf);
			memcpy(buf->data, mcux_msg->buffer, 8);
			net_buf_add(buf, 8);

			setup = (struct usb_setup_packet *)mcux_msg->buffer;
			if ((setup->RequestType.type == USB_REQTYPE_TYPE_STANDARD) &&
				(!setup->wLength) &&
				(!(((setup->bmRequestType) >> 7) & 0x01U)) &&
				(setup->bRequest == USB_SREQ_SET_ADDRESS)) {
				usb_mcux_control(dev, kUSB_DeviceControlPreSetDeviceAddress,
					&setup->wValue);
			}

			/* Update to next stage of control transfer */
			udc_ctrl_update_stage(dev, buf);

			if (!buf->len) {
				return kStatus_USB_Error;
			}

			if (udc_ctrl_stage_is_data_out(dev)) {
				/*  Allocate and feed buffer for data OUT stage */
				LOG_DBG("s:%p|feed for -out-", buf);
				err = usb_mcux_ctrl_feed_dout(dev, udc_data_stage_length(buf));
				if (err == -ENOMEM) {
					err = udc_submit_ep_event(dev, buf, err);
				}
			} else if (udc_ctrl_stage_is_data_in(dev)) {
				err = udc_ctrl_submit_s_in_status(dev);
			} else {
				err = udc_ctrl_submit_s_status(dev);
			}
		} else if (ep == USB_CONTROL_EP_IN) {
			len = MIN(buf->len, mcux_msg->length);
			buf->data += len;
			buf->len -= len;
#if defined(CONFIG_NOCACHE_MEMORY)
			udc_mcux_nocache_free(mcux_msg->buffer);
#endif

			if (udc_ctrl_stage_is_status_in(dev) ||
			udc_ctrl_stage_is_no_data(dev)) {
				/* Status stage finished, notify upper layer */
				udc_ctrl_submit_status(dev, buf);
			}

			/* Update to next stage of control transfer */
			udc_ctrl_update_stage(dev, buf);

			if (udc_ctrl_stage_is_status_out(dev)) {
				/*
				 * IN transfer finished, release buffer,
				 * control OUT buffer should be already fed.
				 */
				net_buf_unref(buf);
				err = usb_mcux_ctrl_feed_dout(dev, 0u);
			}
		} else if (ep == USB_CONTROL_EP_OUT) {
			LOG_DBG("DataOut ep 0x%02x",  ep);
			len = MIN(net_buf_tailroom(buf), mcux_msg->length);
#if defined(CONFIG_NOCACHE_MEMORY)
			memcpy(net_buf_tail(buf), mcux_msg->buffer, len);
			udc_mcux_nocache_free(mcux_msg->buffer);
#endif
			net_buf_add(buf, len);
			if (udc_ctrl_stage_is_status_out(dev)) {
				/* Update to next stage of control transfer */
				udc_ctrl_update_stage(dev, buf);
				/* Status stage finished, notify upper layer */
				udc_ctrl_submit_status(dev, buf);
			} else {
				/* Update to next stage of control transfer */
				udc_ctrl_update_stage(dev, buf);
			}

			if (udc_ctrl_stage_is_status_in(dev)) {
				err = udc_ctrl_submit_s_out_status(dev, buf);
			}
		} else {
			if (USB_EP_DIR_IS_IN(ep)) {
				len = MIN(buf->len, mcux_msg->length);
				buf->data += len;
				buf->len -= len;
			} else {
				len = MIN(net_buf_tailroom(buf), mcux_msg->length);
#if defined(CONFIG_NOCACHE_MEMORY)
				memcpy(net_buf_tail(buf), mcux_msg->buffer, len);
#endif
				net_buf_add(buf, len);
			}

#if defined(CONFIG_NOCACHE_MEMORY)
			udc_mcux_nocache_free(mcux_msg->buffer);
#endif
			err = udc_submit_ep_event(dev, buf, 0);
			buf = udc_buf_peek(dev, ep);
			if (buf) {
				usb_mcux_ep_try_feed(dev, udc_get_ep_cfg(dev, ep), buf);
			}
		}
		break;
	}

	return mcux_status;
}

#ifdef CONFIG_USB_UDC_NXP_EHCI
static void udc_mcux_ehci_isr(const struct device *dev)
{
	struct udc_mcux_data *priv = udc_get_private(dev);

	USB_DeviceEhciIsrFunction((void *)(&priv->device_instance));
}
#endif

#ifdef CONFIG_USB_UDC_NXP_IP3511
static void udc_mcux_ip3511_isr(const struct device *dev)
{
	struct udc_mcux_data *priv = udc_get_private(dev);

	USB_DeviceLpcIp3511IsrFunction((void *)(&priv->device_instance));
}
#endif

static int usb_mcux_ep_enqueue(const struct device *dev,
			       struct udc_ep_config *const cfg,
			       struct net_buf *const buf)
{
	struct net_buf *feed_buf;

	udc_buf_put(cfg, buf);
	if (cfg->stat.halted) {
		LOG_DBG("ep 0x%02x halted", cfg->addr);
		return 0;
	}

	feed_buf = udc_buf_peek(dev, cfg->addr);
	if (feed_buf) {
		return usb_mcux_ep_try_feed(dev, cfg, feed_buf);
	}

	return 0;
}

static int usb_mcux_ep_dequeue(const struct device *dev,
			       struct udc_ep_config *const cfg)
{
	struct net_buf *buf;
	int current_sr;

	cfg->stat.halted = false;
	buf = udc_buf_get_all(dev, cfg->addr);
	if (buf) {
		udc_submit_ep_event(dev, buf, -ECONNABORTED);
	}

	current_sr = irq_lock();
	udc_ep_set_busy(dev, cfg->addr, false);
	irq_unlock(current_sr);

	return 0;
}

static int usb_mcux_ep_set_halt(const struct device *dev,
				struct udc_ep_config *const cfg)
{
	return usb_mcux_control(dev, kUSB_DeviceControlEndpointStall, &cfg->addr);
}

static int usb_mcux_ep_clear_halt(const struct device *dev,
				  struct udc_ep_config *const cfg)
{
	return usb_mcux_control(dev, kUSB_DeviceControlEndpointUnstall, &cfg->addr);
}

static int usb_mcux_ep_enable(const struct device *dev,
			      struct udc_ep_config *const cfg)
{
	usb_device_endpoint_init_struct_t ep_init;

	LOG_DBG("Enable ep 0x%02x", cfg->addr);

	ep_init.zlt             = 0U;
	ep_init.interval        = cfg->interval;
	ep_init.endpointAddress = cfg->addr;
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

	return usb_mcux_control(dev, kUSB_DeviceControlEndpointInit, &ep_init);
}

static int usb_mcux_ep_disable(const struct device *dev,
			       struct udc_ep_config *const cfg)
{
	LOG_DBG("Disable ep 0x%02x", cfg->addr);

	return usb_mcux_control(dev, kUSB_DeviceControlEndpointDeinit, &cfg->addr);
}

static int usb_mcux_host_wakeup(const struct device *dev)
{
	return -ENOTSUP;
}

static int usb_mcux_set_address(const struct device *dev, const uint8_t addr)
{
	uint8_t temp_addr = addr;

	return usb_mcux_control(dev, kUSB_DeviceControlSetDeviceAddress, &temp_addr);
}

static int usb_mcux_enable(const struct device *dev)
{
	return usb_mcux_control(dev, kUSB_DeviceControlRun, NULL);
}

static int usb_mcux_disable(const struct device *dev)
{
	return usb_mcux_control(dev, kUSB_DeviceControlStop, NULL);
}

static int usb_mcux_init(const struct device *dev)
{
	const struct udc_mcux_config *config = dev->config;
	struct udc_mcux_data *priv = udc_get_private(dev);
	usb_status_t status;
	uint8_t index;

#ifdef CONFIG_USB_UDC_NXP_PHY
	usb_phy_config_struct_t phy_config;
#endif

#ifdef CONFIG_USB_UDC_NXP_EHCI
#if defined(USBHS_STACK_BASE_ADDRS)
	uint32_t usb_base_addrs[] = USBHS_STACK_BASE_ADDRS;
#else
	uint32_t usb_base_addrs[] = USBHS_BASE_ADDRS;
#endif
#endif
#ifdef CONFIG_USB_UDC_NXP_IP3511
#ifdef USB_BASE_ADDRS
	uint32_t ip3511_fs_base[] = USB_BASE_ADDRS;
#endif
#ifdef USBHSD_BASE_ADDRS
	uint32_t ip3511_hs_base[] = USBHSD_BASE_ADDRS;
#endif
#endif

	/* get the right controller id */
	priv->controller_id = 0xFFu; /* invalid value */
#ifdef CONFIG_USB_UDC_NXP_EHCI
	for (index = 0; index < ARRAY_SIZE(usb_base_addrs); index++) {
		if (usb_base_addrs[index] == config->base) {
			priv->controller_id = kUSB_ControllerEhci0 + index;
			break;
		}
	}
#endif
#ifdef CONFIG_USB_UDC_NXP_IP3511
#ifdef USB_BASE_ADDRS
	for (index = 0; index < ARRAY_SIZE(ip3511_fs_base); index++) {
		if (ip3511_fs_base[index] == config->base) {
			priv->controller_id = kUSB_ControllerLpcIp3511Fs0 + index;
			break;
		}
	}
#endif

#ifdef USBHSD_BASE_ADDRS
	if (priv->controller_id == 0xFF) {
		for (index = 0; index < ARRAY_SIZE(ip3511_hs_base); index++) {
			if (ip3511_hs_base[index] == config->base) {
				priv->controller_id = kUSB_ControllerLpcIp3511Hs0 + index;
				break;
			}
		}
	}
#endif
#endif
	if (priv->controller_id == 0xFFu) {
		return -ENOMEM;
	}

#ifdef CONFIG_USB_UDC_NXP_PHY
	if (config->need_to_init_phy) {
		phy_config.D_CAL = config->D_CAL;
		phy_config.TXCAL45DP = config->TXCAL45DP;
		phy_config.TXCAL45DM = config->TXCAL45DM;
		USB_EhciPhyInit(priv->controller_id, 0u, &phy_config);
	}
#endif

	/* Init MCUX USB device driver. */
	status = config->mcux_if->deviceInit(priv->controller_id,
		&priv->device_instance, &(priv->device_instance.controllerHandle));
	if (status != kStatus_USB_Success) {
		return -ENOMEM;
	}

	/* enable USB interrupt */
	config->irq_enable_func(dev);

	LOG_DBG("Initialized USB controller %x", config->base);

	return 0;
}

static int usb_mcux_shutdown(const struct device *dev)
{
	const struct udc_mcux_config *config = dev->config;
	struct udc_mcux_data *priv = udc_get_private(dev);
	usb_status_t status;

	/* Disable interrupt */
	config->irq_disable_func(dev);

	/* De-init MCUX USB device driver. */
	status = config->mcux_if->deviceDeinit(priv->device_instance.controllerHandle);
	if (status != kStatus_USB_Success) {
		return -ENOMEM;
	}

	return 0;
}

static int usb_mcux_lock(const struct device *dev)
{
	return udc_lock_internal(dev, K_FOREVER);
}

static int usb_mcux_unlock(const struct device *dev)
{
	return udc_unlock_internal(dev);
}

static int usb_mcux_driver_preinit(const struct device *dev)
{
	const struct udc_mcux_config *config = dev->config;
	struct udc_data *data = dev->data;
	struct udc_mcux_data *priv = data->priv;
	int err;

	k_mutex_init(&data->mutex);

	for (int i = 0; i < config->num_of_eps; i++) {
		config->ep_cfg_out[i].caps.out = 1;
		if (i == 0) {
			config->ep_cfg_out[i].caps.control = 1;
			config->ep_cfg_out[i].caps.mps = 64;
		} else {
			config->ep_cfg_out[i].caps.bulk = 1;
			config->ep_cfg_out[i].caps.interrupt = 1;
			config->ep_cfg_out[i].caps.iso = 1;
			config->ep_cfg_out[i].caps.mps = 1023;
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
			config->ep_cfg_in[i].caps.mps = 1023;
		}

		config->ep_cfg_in[i].addr = USB_EP_DIR_IN | i;
		err = udc_register_ep(dev, &config->ep_cfg_in[i]);
		if (err != 0) {
			LOG_ERR("Failed to register endpoint");
			return err;
		}
	}

	data->caps.rwup = true;
	data->caps.mps0 = USB_MCUX_MPS0;
	priv->dev = dev;

	pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);

	return 0;
}

static const struct udc_api usb_mcux_api = {
	.ep_enqueue = usb_mcux_ep_enqueue,
	.ep_dequeue = usb_mcux_ep_dequeue,
	.ep_set_halt = usb_mcux_ep_set_halt,
	.ep_clear_halt = usb_mcux_ep_clear_halt,
	.ep_try_config = NULL,
	.ep_enable = usb_mcux_ep_enable,
	.ep_disable = usb_mcux_ep_disable,
	.host_wakeup = usb_mcux_host_wakeup,
	.set_address = usb_mcux_set_address,
	.enable = usb_mcux_enable,
	.disable = usb_mcux_disable,
	.init = usb_mcux_init,
	.shutdown = usb_mcux_shutdown,
	.lock = usb_mcux_lock,
	.unlock = usb_mcux_unlock,
};

#ifdef CONFIG_USB_UDC_NXP_EHCI
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT nxp_ehci


#ifdef CONFIG_USB_UDC_NXP_PHY
#define UDC_MCUX_PHY_CONFIGURE(n)							\
	.need_to_init_phy = DT_NODE_HAS_PROP(DT_DRV_INST(n), phy_handle) ? 1u : 0u,	\
	.D_CAL = DT_PROP_OR(DT_INST_PHANDLE(n, phy_handle), d_cal, 0),			\
	.TXCAL45DP = DT_PROP_OR(DT_INST_PHANDLE(n, phy_handle), txcal45dp, 0),		\
	.TXCAL45DM = DT_PROP_OR(DT_INST_PHANDLE(n, phy_handle), txcal45dm, 0),
#else
#define UDC_MCUX_PHY_CONFIGURE(n)
#endif

#define USB_MCUX_EHCI_DEVICE_DEFINE(n)							\
	static void udc_irq_enable_func##n(const struct device *dev)			\
	{										\
		IRQ_CONNECT(DT_INST_IRQN(n),						\
			    DT_INST_IRQ(n, priority),					\
			    udc_mcux_ehci_isr,						\
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
		UDC_MCUX_PHY_CONFIGURE(n)						\
		.base = (uint32_t)DT_INST_REG_ADDR(n),					\
		.irq_enable_func = udc_irq_enable_func##n,				\
		.irq_disable_func = udc_irq_disable_func##n,				\
		.num_of_eps = DT_INST_PROP(n, num_bidir_endpoints),			\
		.ep_cfg_in = ep_cfg_in##n,						\
		.ep_cfg_out = ep_cfg_out##n,						\
		.mcux_if = &mcux_ehci_usb_iface,					\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),				\
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
	DEVICE_DT_INST_DEFINE(n, usb_mcux_driver_preinit, NULL,				\
			      &udc_data_##n, &priv_config_##n,				\
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
			      &usb_mcux_api);

DT_INST_FOREACH_STATUS_OKAY(USB_MCUX_EHCI_DEVICE_DEFINE)
#endif

#ifdef CONFIG_USB_UDC_NXP_IP3511
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT nxp_lpcip3511


#ifdef CONFIG_USB_UDC_NXP_PHY
#define UDC_MCUX_PHY_CONFIGURE(n)							\
	.need_to_init_phy = DT_NODE_HAS_PROP(DT_DRV_INST(n), phy_handle) ? 1u : 0u,	\
	.D_CAL = DT_PROP_OR(DT_INST_PHANDLE(n, phy_handle), d_cal, 0),			\
	.TXCAL45DP = DT_PROP_OR(DT_INST_PHANDLE(n, phy_handle), txcal45dp, 0),		\
	.TXCAL45DM = DT_PROP_OR(DT_INST_PHANDLE(n, phy_handle), txcal45dm, 0),
#else
#define UDC_MCUX_PHY_CONFIGURE(n)
#endif

#define USB_MCUX_IP3511_DEVICE_DEFINE(n)						\
	static void udc_irq_enable_func##n(const struct device *dev)			\
	{										\
		IRQ_CONNECT(DT_INST_IRQN(n),						\
			    DT_INST_IRQ(n, priority),					\
			    udc_mcux_ip3511_isr,					\
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
		UDC_MCUX_PHY_CONFIGURE(n)						\
		.base = (uint32_t)DT_INST_REG_ADDR(n),					\
		.irq_enable_func = udc_irq_enable_func##n,				\
		.irq_disable_func = udc_irq_disable_func##n,				\
		.num_of_eps = DT_INST_PROP(n, num_bidir_endpoints),			\
		.ep_cfg_in = ep_cfg_in##n,						\
		.ep_cfg_out = ep_cfg_out##n,						\
		.mcux_if = &mcux_ip3511_usb_iface,					\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),				\
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
	DEVICE_DT_INST_DEFINE(n, usb_mcux_driver_preinit, NULL,				\
			      &udc_data_##n, &priv_config_##n,				\
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
			      &usb_mcux_api);

DT_INST_FOREACH_STATUS_OKAY(USB_MCUX_IP3511_DEVICE_DEFINE)
#endif
