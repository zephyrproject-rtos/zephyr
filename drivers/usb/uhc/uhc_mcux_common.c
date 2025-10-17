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
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/usb/uhc.h>

#include "uhc_common.h"
#include "usb.h"
#include "usb_host_config.h"
#include "usb_host_mcux_drv_port.h"
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

	mcux_ep = uhc_mcux_init_hal_ep(dev, xfer);
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
	case kUSB_HostGetDevicePortNumber:
	case kUSB_HostGetDeviceHSHubNumber:
	case kUSB_HostGetDeviceHSHubPort:
	case kUSB_HostGetHubThinkTime:
		*infoValue = 0;
		break;
	case kUSB_HostGetDeviceLevel:
		*infoValue = 1;
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

static usb_host_pipe_handle uhc_mcux_check_hal_ep(const struct device *dev,
						  struct uhc_transfer *const xfer)
{
	struct uhc_mcux_data *priv = uhc_get_private(dev);
	usb_host_pipe_t *mcux_ep = NULL;
	uint8_t i;
	usb_status_t status;

	/* if already initialized */
	for (i = 0; i < USB_HOST_CONFIG_MAX_PIPES; i++) {
		uint8_t direction;

		if (USB_EP_GET_IDX(xfer->ep) == 0) {
			direction = 0;
		} else {
			direction = USB_EP_GET_DIR(xfer->ep) ? USB_IN : USB_OUT;
		}

		if (priv->mcux_eps[i] != NULL &&
		    priv->mcux_eps[i]->endpointAddress == USB_EP_GET_IDX(xfer->ep) &&
		    priv->mcux_eps[i]->direction == direction &&
		    priv->mcux_eps[i]->deviceHandle == xfer->udev) {
			mcux_ep = priv->mcux_eps[i];
			break;
		}
	}

	/* TODO: need to check endpoint type too */
	if (mcux_ep != NULL &&
	    (mcux_ep->maxPacketSize != xfer->mps ||
	     mcux_ep->interval != xfer->interval)) {
		/* re-initialize the ep */
		status = priv->mcux_if->controllerClosePipe(priv->mcux_host.controllerHandle,
							    mcux_ep);
		if (status != kStatus_USB_Success) {
			return NULL;
		}

		uhc_mcux_lock(dev);
		priv->mcux_eps[i] = NULL;
		uhc_mcux_unlock(dev);
		mcux_ep = NULL;
	}

	return mcux_ep;
}

usb_host_pipe_t *uhc_mcux_init_hal_ep(const struct device *dev, struct uhc_transfer *const xfer)
{
	usb_status_t status;
	usb_host_pipe_t *mcux_ep;
	struct uhc_mcux_data *priv = uhc_get_private(dev);
	usb_host_pipe_init_t pipe_init;
	uint8_t i;

	/* if already initialized */
	mcux_ep = uhc_mcux_check_hal_ep(dev, xfer);
	if (mcux_ep != NULL) {
		return mcux_ep;
	}

	/* USB_HostHelperGetPeripheralInformation uses this value as first parameter */
	pipe_init.devInstance = xfer->udev;
	pipe_init.nakCount = USB_HOST_CONFIG_MAX_NAK;
	pipe_init.maxPacketSize = xfer->mps;
	pipe_init.endpointAddress = USB_EP_GET_IDX(xfer->ep);
	pipe_init.direction = USB_EP_GET_IDX(xfer->ep) == 0 ? USB_OUT :
			      USB_EP_GET_DIR(xfer->ep) ? USB_IN : USB_OUT;
	/* Current Zephyr Host stack is experimental, the endpoint's interval,
	 * 'number per uframe' and the endpoint type cannot be got yet.
	 */
	pipe_init.numberPerUframe = 0; /* TODO: need right way to implement it. */
	pipe_init.interval = xfer->interval;
	/* TODO: need right way to implement it. */
	if (pipe_init.endpointAddress == 0) {
		pipe_init.pipeType = USB_ENDPOINT_CONTROL;
	} else {
		pipe_init.pipeType = USB_ENDPOINT_BULK;
	}

	status = priv->mcux_if->controllerOpenPipe(priv->mcux_host.controllerHandle,
						   (usb_host_pipe_handle *)&mcux_ep, &pipe_init);

	if (status != kStatus_USB_Success) {
		return NULL;
	}

	/* Initialize mcux hal endpoint pipe
	 * TODO: Need one way to release the pipe.
	 * Otherwise the priv->mcux_eps will be used up after
	 * supporting hub and connecting/disconnecting multiple times.
	 * For example: add endpoint/pipe init and de-init controller
	 * interafce to resolve the issue.
	 */
	uhc_mcux_lock(dev);
	for (i = 0; i < USB_HOST_CONFIG_MAX_PIPES; i++) {
		if (priv->mcux_eps[i] == NULL) {
			priv->mcux_eps[i] = mcux_ep;
			break;
		}
	}
	uhc_mcux_unlock(dev);

	if (i >= USB_HOST_CONFIG_MAX_PIPES) {
		priv->mcux_if->controllerClosePipe(priv->mcux_host.controllerHandle, mcux_ep);
		mcux_ep = NULL;
	}

	return mcux_ep;
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
		mcux_xfer->transferLength = xfer->buf->size;
		mcux_xfer->transferBuffer = xfer->buf->__buf;
	} else {
		mcux_xfer->transferBuffer = NULL;
		mcux_xfer->transferLength = 0;
	}

	if (USB_EP_GET_IDX(xfer->ep) == 0) {
		mcux_xfer->direction = USB_REQTYPE_GET_DIR(mcux_xfer->setupPacket->bmRequestType)
					       ? USB_IN
					       : USB_OUT;
	} else {
		mcux_xfer->direction = USB_EP_DIR_IS_IN(xfer->ep) ? USB_IN : USB_OUT;
	}

	return 0;
}
