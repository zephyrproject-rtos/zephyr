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
	struct uhc_mcux_data *priv = uhc_get_private(dev);

	priv->port_status.wPortStatus |= USB_HUB_PORT_STATUS_POWER;
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

/*
 * Hub Descriptor. The HAL exposes a single port without power switching, TT or
 * over-current reporting.
 * D1:D0 Logical Power Switching Mode, Individual port power switching
 * D4:D3 Over-current Protection Mode, No over-current protection
 */
#define UHC_MCUX_HUB_CHARACTERISTICS (BIT(0) | BIT(4))

struct uhc_mcux_hub_desc {
	struct usb_hub_descriptor hub;
	uint8_t DeviceRemovable[DIV_ROUND_UP(1 + 1, 8)];
	uint8_t PortPwrCtrlMask[DIV_ROUND_UP(1 + 1, 8)];
} __packed;

/* USB 2.0 11.23.2.1: PortPwrCtrlMask should have all bits set to 1B. */
static const struct uhc_mcux_hub_desc uhc_mcux_hub_desc = {
	.hub = {
		.bDescLength = sizeof(struct uhc_mcux_hub_desc),
		.bDescriptorType = USB_HUB_DESCRIPTOR_TYPE,
		.bNbrPorts = 1,
		.wHubCharacteristics = sys_cpu_to_le16(UHC_MCUX_HUB_CHARACTERISTICS),
	},
	.PortPwrCtrlMask = {0xFF},
};

static int hub_sreq_get_desc(const struct device *dev,
			     const struct usb_setup_packet *const setup,
			     struct net_buf *const buf)
{
	size_t len;

	ARG_UNUSED(dev);

	len = MIN(sizeof(uhc_mcux_hub_desc), setup->wLength);
	net_buf_add_mem(buf, &uhc_mcux_hub_desc, len);

	return 0;
}

static int hub_hcreq_get_status(const struct device *dev,
				const struct usb_setup_packet *const setup,
				struct net_buf *const buf)
{
	struct uhc_mcux_data *priv = uhc_get_private(dev);
	const void *data;
	size_t len;

	switch (setup->RequestType.recipient) {
	case USB_REQTYPE_RECIPIENT_DEVICE:
		data = &priv->hub_status;
		len = sizeof(priv->hub_status);
		break;
	case USB_REQTYPE_RECIPIENT_OTHER:
		if (setup->wIndex != 1) {
			return -EPIPE;
		}

		data = &priv->port_status;
		len = sizeof(priv->port_status);
		break;
	default:
		return -EPIPE;
	}

	net_buf_add_mem(buf, data, MIN(len, setup->wLength));

	return 0;
}

static int hub_dev_set_feature(const struct device *dev,
			       const struct usb_setup_packet *const setup)
{
	struct uhc_mcux_data *priv = uhc_get_private(dev);

	/* USB 2.0 Table 11-17: C_HUB_x selector value is its bit number in wHubChange. */
	if (setup->wValue > USB_HCFS_C_HUB_OVER_CURRENT) {
		return -EPIPE;
	}

	priv->hub_status.wHubChange |= BIT(setup->wValue);

	return 0;
}

static int hub_dev_clear_feature(const struct device *dev,
				 const struct usb_setup_packet *const setup)
{
	struct uhc_mcux_data *priv = uhc_get_private(dev);

	if (setup->wValue > USB_HCFS_C_HUB_OVER_CURRENT) {
		return -EPIPE;
	}

	priv->hub_status.wHubChange &= ~BIT(setup->wValue);

	return 0;
}

static int hub_port_reset(const struct device *dev)
{
	struct uhc_mcux_data *priv = uhc_get_private(dev);
	int ret;

	if ((priv->port_status.wPortStatus & USB_HUB_PORT_STATUS_CONNECTION) == 0U) {
		return 0;
	}

	priv->port_status.wPortStatus &= (USB_HUB_PORT_STATUS_POWER |
					  USB_HUB_PORT_STATUS_CONNECTION |
					  USB_HUB_PORT_STATUS_LOW_SPEED |
					  USB_HUB_PORT_STATUS_HIGH_SPEED);
	priv->port_status.wPortStatus |= USB_HUB_PORT_STATUS_RESET;

	ret = uhc_mcux_bus_control(dev, kUSB_HostBusReset);

	priv->port_status.wPortStatus &= ~USB_HUB_PORT_STATUS_RESET;
	if (ret != 0) {
		return ret;
	}

	priv->port_status.wPortStatus |= USB_HUB_PORT_STATUS_ENABLE;
	priv->port_status.wPortChange |= USB_HUB_PORT_CHANGE_RESET;

	return 0;
}

static int hub_port_set_feature(const struct device *dev,
				const struct usb_setup_packet *const setup)
{
	struct uhc_mcux_data *priv = uhc_get_private(dev);

	if (setup->wIndex != 1) {
		return -EPIPE;
	}

	switch (setup->wValue) {
	case USB_HCFS_PORT_POWER:
		/* The HAL has no VBUS switch. VBUS is always supplied. */
		priv->port_status.wPortStatus |= USB_HUB_PORT_STATUS_POWER;
		break;
	case USB_HCFS_PORT_RESET:
		return hub_port_reset(dev);
	default:
		return -EPIPE;
	}

	return 0;
}

static int hub_port_clear_feature(const struct device *dev,
				  const struct usb_setup_packet *const setup)
{
	struct uhc_mcux_data *priv = uhc_get_private(dev);
	uint16_t c_bit;

	if (setup->wIndex != 1) {
		return -EPIPE;
	}

	switch (setup->wValue) {
	case USB_HCFS_PORT_POWER:
		/* The HAL has no VBUS switch. VBUS stays supplied. */
		priv->port_status.wPortStatus &= ~USB_HUB_PORT_STATUS_POWER;
		break;
	case USB_HCFS_PORT_ENABLE:
		/* The HAL cannot disable the port. */
		priv->port_status.wPortStatus &= ~USB_HUB_PORT_STATUS_ENABLE;
		break;
	case USB_HCFS_PORT_SUSPEND:
		/* USB 2.0 11.24.2.13: the port is never suspended, treat this as a no-op. */
		break;
	case USB_HCFS_C_PORT_CONNECTION:
	case USB_HCFS_C_PORT_ENABLE:
	case USB_HCFS_C_PORT_SUSPEND:
	case USB_HCFS_C_PORT_OVER_CURRENT:
	case USB_HCFS_C_PORT_RESET:
		/* USB 2.0 Table 11-17: C_PORT_x selector value is 16 plus its bit number. */
		c_bit = BIT(setup->wValue - USB_HCFS_C_PORT_CONNECTION);
		priv->port_status.wPortChange &= ~c_bit;
		break;
	default:
		return -EPIPE;
	}

	return 0;
}

int uhc_mcux_root_hub_control(const struct device *dev,
			      const struct usb_setup_packet *const setup,
			      struct net_buf *const buf)
{
	int ret;

	switch (setup->bRequest) {
	case USB_SREQ_GET_DESCRIPTOR:
		ret = hub_sreq_get_desc(dev, setup, buf);
		break;
	case USB_HCREQ_GET_STATUS:
		ret = hub_hcreq_get_status(dev, setup, buf);
		break;
	case USB_HCREQ_SET_FEATURE:
		if (setup->RequestType.recipient == USB_REQTYPE_RECIPIENT_DEVICE) {
			ret = hub_dev_set_feature(dev, setup);
		} else {
			ret = hub_port_set_feature(dev, setup);
		}
		break;
	case USB_HCREQ_CLEAR_FEATURE:
		if (setup->RequestType.recipient == USB_REQTYPE_RECIPIENT_DEVICE) {
			ret = hub_dev_clear_feature(dev, setup);
		} else {
			ret = hub_port_clear_feature(dev, setup);
		}
		break;
	case USB_HCREQ_CLEAR_TT_BUFFER:
	case USB_HCREQ_RESET_TT:
	case USB_HCREQ_GET_TT_STATE:
	case USB_HCREQ_STOP_TT:
		/* The root port is not connected through a TT. */
		ret = -EPIPE;
		break;
	default:
		LOG_ERR("root hub: unsupported request 0x%02x", setup->bRequest);
		ret = -EPIPE;
		break;
	}

	return ret;
}

/*
 * Hub and Port Status Change Bitmap, USB 2.0 11.12.4: bit 0 is the hub's
 * own change, bit 1 is the single port's change.
 */
int uhc_mcux_root_hub_status(const struct device *dev, struct net_buf *const buf)
{
	struct uhc_mcux_data *priv = uhc_get_private(dev);
	uint8_t bitmap[DIV_ROUND_UP(1 + 1, 8)] = {0};
	bool changed = false;

	if (priv->hub_status.wHubChange != 0U) {
		bitmap[0] |= BIT(0);
		changed = true;
	}

	if (priv->port_status.wPortChange != 0U) {
		bitmap[0] |= BIT(1);
		changed = true;
	}

	if (changed) {
		net_buf_add_mem(buf, bitmap, MIN(sizeof(bitmap), net_buf_tailroom(buf)));
	}

	return 0;
}

/* Controller driver calls this function when device attach. */
usb_status_t USB_HostAttachDevice(usb_host_handle hostHandle, uint8_t speed, uint8_t hubNumber,
				  uint8_t portNumber, uint8_t level,
				  usb_device_handle *deviceHandle)
{
	struct uhc_mcux_data *priv = PRV_DATA_HANDLE(hostHandle);
	uint16_t status = USB_HUB_PORT_STATUS_CONNECTION;

	if (speed == USB_SPEED_HIGH) {
		status |= USB_HUB_PORT_STATUS_HIGH_SPEED;
	} else if (speed != USB_SPEED_FULL) {
		status |= USB_HUB_PORT_STATUS_LOW_SPEED;
	}

	uhc_mcux_lock(priv->dev);
	priv->port_status.wPortStatus &= USB_HUB_PORT_STATUS_POWER;
	priv->port_status.wPortStatus |= status;
	priv->port_status.wPortChange |= USB_HUB_PORT_CHANGE_CONNECTION;
	uhc_mcux_unlock(priv->dev);

	return kStatus_USB_Success;
}

/* Controller driver calls this function when device detaches. */
usb_status_t USB_HostDetachDevice(usb_host_handle hostHandle, uint8_t hubNumber, uint8_t portNumber)
{
	struct uhc_mcux_data *priv = PRV_DATA_HANDLE(hostHandle);

	uhc_mcux_lock(priv->dev);
	priv->port_status.wPortStatus &= USB_HUB_PORT_STATUS_POWER;
	priv->port_status.wPortChange |= USB_HUB_PORT_CHANGE_CONNECTION;
	uhc_mcux_unlock(priv->dev);

	uhc_mcux_bus_control(priv->dev, kUSB_HostBusEnableAttach);

	return kStatus_USB_Success;
}

/* The HAL does not know about the root hub, a device on the root port has no parent hub. */
static const struct usb_device *uhc_mcux_parent_hub(const struct usb_device *udev)
{
	if (udev->hub == NULL || udev->hub->hub == NULL) {
		return NULL;
	}

	return udev->hub;
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
		if (uhc_mcux_parent_hub(udev) != NULL) {
			*infoValue = udev->hub->addr;
		} else {
			*infoValue = 0;
		}
		break;

	case kUSB_HostGetDevicePortNumber:
		if (uhc_mcux_parent_hub(udev) != NULL) {
			*infoValue = udev->hub_port;
		} else {
			*infoValue = 0;
		}
		break;

	case kUSB_HostGetDeviceHSHubNumber:
		for (; uhc_mcux_parent_hub(udev) != NULL; udev = udev->hub) {
			if (udev->hub->speed == USB_SPEED_SPEED_HS) {
				*infoValue = udev->hub->addr;
				return kStatus_USB_Success;
			}
		}
		*infoValue = 0;
		break;

	case kUSB_HostGetDeviceHSHubPort:
		for (; uhc_mcux_parent_hub(udev) != NULL; udev = udev->hub) {
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
		*infoValue = udev->level - 1U;
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

	if (mcux_ep == NULL) {
		return NULL;
	}

	/* If the ep's attributes have changed, close the ep and return NULL.
	 * The ep will be re-initialized with new attributes.
	 */
	if (mcux_ep->pipeType != xfer->type ||
	    mcux_ep->maxPacketSize != USB_MPS_EP_SIZE(xfer->mps) ||
	    mcux_ep->numberPerUframe != USB_MPS_ADDITIONAL_TRANSACTIONS(xfer->mps) + 1 ||
	    priv->mcux_eps_interval[i] != xfer->interval) {
		status = priv->mcux_if->controllerClosePipe(priv->mcux_host.controllerHandle,
							    mcux_ep);
		if (status != kStatus_USB_Success) {
			return NULL;
		}

		uhc_mcux_lock(dev);
		priv->mcux_eps[i] = NULL;
		priv->mcux_eps_interval[i] = 0;
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
	pipe_init.maxPacketSize = USB_MPS_EP_SIZE(xfer->mps);
	pipe_init.endpointAddress = USB_EP_GET_IDX(xfer->ep);
	pipe_init.direction = USB_EP_GET_IDX(xfer->ep) == 0 ? USB_OUT :
			      USB_EP_GET_DIR(xfer->ep) ? USB_IN : USB_OUT;
	/* Current Zephyr Host stack is experimental, the endpoint's interval,
	 * 'number per uframe' and the endpoint type cannot be got yet.
	 */
	pipe_init.numberPerUframe = USB_MPS_ADDITIONAL_TRANSACTIONS(xfer->mps);
	pipe_init.interval = xfer->interval;
	pipe_init.pipeType = xfer->type;

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
	 * interface to resolve the issue.
	 */
	uhc_mcux_lock(dev);
	for (i = 0; i < USB_HOST_CONFIG_MAX_PIPES; i++) {
		if (priv->mcux_eps[i] == NULL) {
			priv->mcux_eps[i] = mcux_ep;
			priv->mcux_eps_interval[i] = xfer->interval;
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
		if (USB_EP_DIR_IS_OUT(xfer->ep)) {
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

	if (USB_EP_GET_IDX(xfer->ep) == 0) {
		mcux_xfer->direction = USB_REQTYPE_GET_DIR(mcux_xfer->setupPacket->bmRequestType)
					       ? USB_IN
					       : USB_OUT;
	} else {
		mcux_xfer->direction = USB_EP_DIR_IS_IN(xfer->ep) ? USB_IN : USB_OUT;
	}

	return 0;
}
