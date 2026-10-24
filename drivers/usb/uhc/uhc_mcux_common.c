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
#include <zephyr/init.h>
#include <zephyr/drivers/usb/uhc.h>

#include "uhc_common.h"
#include <usb.h>
#include <usb_host_config.h>
#include <usb_host_mcux_drv_port.h>
#include "uhc_mcux_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uhc_mcux, CONFIG_UHC_DRIVER_LOG_LEVEL);

#define PRV_DATA_HANDLE(_handle) CONTAINER_OF(_handle, struct uhc_mcux_data, mcux_host)

int uhc_mcux_lock(const struct device *dev)
{
	struct uhc_data *data = dev->data;

	return k_mutex_lock(&data->mutex, K_FOREVER);
}

int uhc_mcux_unlock(const struct device *dev)
{
	struct uhc_data *data = dev->data;

	return k_mutex_unlock(&data->mutex);
}

int uhc_mcux_control(const struct device *dev, uint32_t control, void *param)
{
	struct uhc_mcux_data *priv = uhc_get_private(dev);
	usb_status_t status;

	status = priv->mcux_if->controllerIoctl(priv->mcux_host.controllerHandle, control, param);
	if (status != kStatus_USB_Success) {
		return -EIO;
	}

	return 0;
}

int uhc_mcux_bus_control(const struct device *dev, usb_host_bus_control_t type)
{
	return uhc_mcux_control(dev, kUSB_HostBusControl, &type);
}

int uhc_mcux_enable(const struct device *dev)
{
	const struct uhc_mcux_config *config = dev->config;

	config->irq_enable_func(dev);

	return 0;
}

int uhc_mcux_disable(const struct device *dev)
{
	const struct uhc_mcux_config *config = dev->config;

	config->irq_disable_func(dev);

	return 0;
}

int uhc_mcux_shutdown(const struct device *dev)
{
	const struct uhc_mcux_config *config = dev->config;
	struct uhc_mcux_data *priv = uhc_get_private(dev);

	config->irq_disable_func(dev);
	k_thread_abort(&priv->drv_stack_data);
	priv->mcux_if->controllerDestory(priv->mcux_host.controllerHandle);

	return 0;
}

/* Signal bus reset, 50ms SE0 signal */
int uhc_mcux_bus_reset(const struct device *dev)
{
	return uhc_mcux_bus_control(dev, kUSB_HostBusReset);
}

/* Enable SOF generator */
int uhc_mcux_sof_enable(const struct device *dev)
{
	/* MCUX doesn't need it. */
	return 0;
}

/* Disable SOF generator and suspend bus */
int uhc_mcux_bus_suspend(const struct device *dev)
{
	return uhc_mcux_bus_control(dev, kUSB_HostBusSuspend);
}

/* Signal bus resume event, 20ms K-state + low-speed EOP */
int uhc_mcux_bus_resume(const struct device *dev)
{
	return uhc_mcux_bus_control(dev, kUSB_HostBusResume);
}

int uhc_mcux_dequeue(const struct device *dev, struct uhc_transfer *const xfer)
{
	usb_host_pipe_t *mcux_ep;
	usb_host_cancel_param_t cancel_param;

	mcux_ep = xfer->ep->priv;
	if (mcux_ep == NULL) {
		return -ENOMEM;
	}

	cancel_param.pipeHandle = mcux_ep;
	cancel_param.transfer   = NULL; /* cancel all the transfers on the pipe */
	return uhc_mcux_control(dev, kUSB_HostCancelTransfer, &cancel_param);
}

/* Controller driver calls this function when device attach. */
usb_status_t USB_HostAttachDevice(usb_host_handle hostHandle, uint8_t speed, uint8_t hubNumber,
				  uint8_t portNumber, uint8_t level,
				  usb_device_handle *deviceHandle)
{
	enum uhc_event_type type;
	struct uhc_mcux_data *priv;

	if (speed == USB_SPEED_HIGH) {
		type = UHC_EVT_DEV_CONNECTED_HS;
	} else if (speed == USB_SPEED_FULL) {
		type = UHC_EVT_DEV_CONNECTED_FS;
	} else {
		type = UHC_EVT_DEV_CONNECTED_LS;
	}

	priv = (struct uhc_mcux_data *)(PRV_DATA_HANDLE(hostHandle));
	uhc_submit_event(priv->dev, type, 0);

	return kStatus_USB_Success;
}

/* Controller driver calls this function when device detaches. */
usb_status_t USB_HostDetachDevice(usb_host_handle hostHandle, uint8_t hubNumber, uint8_t portNumber)
{
	struct uhc_mcux_data *priv;

	priv = (struct uhc_mcux_data *)(PRV_DATA_HANDLE(hostHandle));
	uhc_submit_event(priv->dev, UHC_EVT_DEV_REMOVED, 0);
	uhc_mcux_bus_control(priv->dev, kUSB_HostBusEnableAttach);

	return kStatus_USB_Success;
}

/* MCUX Controller driver calls this function to get the device information. */
usb_status_t USB_HostHelperGetPeripheralInformation(usb_device_handle deviceHandle,
						    uint32_t infoCode, uint32_t *infoValue)
{
	/* The deviceHandle is struct usb_device pointer */
	struct usb_device *udev = (struct usb_device *)deviceHandle;
	usb_host_dev_info_t info_code;

	if ((deviceHandle == NULL) || (infoValue == NULL)) {
		return kStatus_USB_InvalidParameter;
	}
	info_code = (usb_host_dev_info_t)infoCode;
	switch (info_code) {
	case kUSB_HostGetDeviceAddress:
		*infoValue = udev->addr;
		break;

	case kUSB_HostGetDeviceHubNumber:
		if (udev->hub != NULL) {
			*infoValue = udev->hub->addr;
		} else {
			*infoValue = 0;
		}
		break;

	case kUSB_HostGetDevicePortNumber:
		*infoValue = udev->hub_port;
		break;

	case kUSB_HostGetDeviceHSHubNumber:
		for (; udev->hub != NULL; udev = udev->hub) {
			if (udev->hub->speed == USB_SPEED_SPEED_HS) {
				*infoValue = udev->hub->addr;
				return kStatus_USB_Success;
			}
		}
		*infoValue = 0;
		break;

	case kUSB_HostGetDeviceHSHubPort:
		for (; udev->hub != NULL; udev = udev->hub) {
			if (udev->hub->speed == USB_SPEED_SPEED_HS) {
				*infoValue = udev->hub_port;
				return kStatus_USB_Success;
			}
		}
		*infoValue = 0;
		break;

	case kUSB_HostGetHubThinkTime: {
		uint32_t total_think_time = 0;

		if (udev->speed != USB_SPEED_SPEED_HS) {
			for (; udev->hub != NULL; udev = udev->hub) {
				total_think_time += udev->hub->tt;
			}
		}
		*infoValue = total_think_time;
		break;
	}

	case kUSB_HostGetDeviceLevel:
		*infoValue = udev->level;
		break;

	case kUSB_HostGetDeviceSpeed:
		switch (udev->speed) {
		case USB_SPEED_SPEED_LS:
			*infoValue = USB_SPEED_LOW;
			break;
		case USB_SPEED_SPEED_FS:
			*infoValue = USB_SPEED_FULL;
			break;
		case USB_SPEED_SPEED_HS:
			*infoValue = USB_SPEED_HIGH;
			break;
		case USB_SPEED_UNKNOWN:
		case USB_SPEED_SPEED_SS:
		default:
			*infoValue = USB_SPEED_HIGH;
			/* TODO: report error */
			break;
		}
		break;

	default:
		break;
	}

	return kStatus_USB_Success;
}

int uhc_mcux_ep_enable(const struct device *dev, struct usb_host_ep *const host_ep)
{
	struct uhc_mcux_data *priv = uhc_get_private(dev);
	usb_host_pipe_init_t pipe_init;
	usb_host_pipe_t *mcux_ep;
	usb_status_t status;

	/*
	 * Both directions of the control endpoint use one pipe. It is opened
	 * for the OUT direction and attached to the IN direction below, so
	 * there is nothing to do for the IN direction.
	 */
	if (host_ep->addr == USB_CONTROL_EP_IN) {
		return 0;
	}

	/* USB_HostHelperGetPeripheralInformation uses this value as first parameter */
	pipe_init.devInstance = host_ep->udev;
	pipe_init.nakCount = USB_HOST_CONFIG_MAX_NAK;
	pipe_init.maxPacketSize = USB_MPS_EP_SIZE(host_ep->mps);
	pipe_init.endpointAddress = USB_EP_GET_IDX(host_ep->addr);
	pipe_init.direction = USB_EP_GET_IDX(host_ep->addr) == 0 ? USB_OUT :
			      USB_EP_DIR_IS_IN(host_ep->addr) ? USB_IN : USB_OUT;
	pipe_init.numberPerUframe = USB_MPS_ADDITIONAL_TRANSACTIONS(host_ep->mps);
	pipe_init.interval = host_ep->interval;
	pipe_init.pipeType = uhc_ep_type(host_ep);

	status = priv->mcux_if->controllerOpenPipe(priv->mcux_host.controllerHandle,
						   (usb_host_pipe_handle *)&mcux_ep, &pipe_init);
	if (status != kStatus_USB_Success) {
		return -ENOMEM;
	}

	host_ep->priv = mcux_ep;

	if (host_ep->addr == USB_CONTROL_EP_OUT) {
		struct usb_host_ep *ep_in;

		/* The IN direction uses the pipe opened for the OUT direction */
		ep_in = uhc_get_udev_ep(host_ep->udev, USB_CONTROL_EP_IN);
		ep_in->priv = mcux_ep;
	}

	return 0;
}

int uhc_mcux_ep_disable(const struct device *dev, struct usb_host_ep *const host_ep)
{
	struct uhc_mcux_data *priv = uhc_get_private(dev);
	usb_status_t status;

	/*
	 * The pipe of the control endpoint belongs to the OUT direction, so
	 * there is nothing to do for the IN direction.
	 */
	if (host_ep->addr == USB_CONTROL_EP_IN) {
		return 0;
	}

	if (host_ep->priv == NULL) {
		return 0;
	}

	status = priv->mcux_if->controllerClosePipe(priv->mcux_host.controllerHandle,
						    host_ep->priv);
	if (status != kStatus_USB_Success) {
		return -EIO;
	}

	return 0;
}

int uhc_mcux_hal_init_transfer_common(const struct device *dev, usb_host_transfer_t *mcux_xfer,
				      usb_host_pipe_handle mcux_ep_handle,
				      struct uhc_transfer *const xfer,
				      host_inner_transfer_callback_t cb)
{
	mcux_xfer->uhc_xfer = xfer;
	mcux_xfer->transferPipe = mcux_ep_handle;
	mcux_xfer->transferSofar = 0;
	mcux_xfer->next = NULL;
	mcux_xfer->setupStatus = 0;
	mcux_xfer->callbackFn = cb;
	mcux_xfer->callbackParam = (void *)dev;
	mcux_xfer->setupPacket = (usb_setup_struct_t *)&xfer->setup_pkt[0];
	if (xfer->buf != NULL) {
		if (USB_EP_DIR_IS_OUT(xfer->ep->addr)) {
			mcux_xfer->transferLength = xfer->buf->len;
			mcux_xfer->transferBuffer = xfer->buf->data;
		} else {
			mcux_xfer->transferLength = net_buf_tailroom(xfer->buf);
			mcux_xfer->transferBuffer = net_buf_tail(xfer->buf);
		}
	} else {
		mcux_xfer->transferBuffer = NULL;
		mcux_xfer->transferLength = 0;
	}

	if (USB_EP_GET_IDX(xfer->ep->addr) == 0) {
		mcux_xfer->direction = USB_REQTYPE_GET_DIR(mcux_xfer->setupPacket->bmRequestType)
					       ? USB_IN
					       : USB_OUT;
	} else {
		mcux_xfer->direction = USB_EP_DIR_IS_IN(xfer->ep->addr) ? USB_IN : USB_OUT;
	}

	return 0;
}
