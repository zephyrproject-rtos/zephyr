/*
 * Copyright 2018-2023, NXP
 * Copyright (c) 2019 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <string.h>
#include <zephyr/drivers/usb/usb_dc.h>
#include <zephyr/usb/usb_device.h>
#include <soc.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/pinctrl.h>
#include "usb.h"
#include "usb_device.h"
#include "usb_device_config.h"
#include "usb_device_dci.h"

#ifdef CONFIG_USB_DC_NXP_EHCI
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT nxp_ehci
#include "usb_device_ehci.h"
#endif
#ifdef CONFIG_USB_DC_NXP_LPCIP3511
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT nxp_lpcip3511
#include "usb_device_lpcip3511.h"
#endif
#ifdef CONFIG_HAS_MCUX_CACHE
#include <fsl_cache.h>
#endif


#define LOG_LEVEL CONFIG_USB_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(usb_dc_mcux);

static void usb_isr_handler(void);

/* the setup transfer state */
#define SETUP_DATA_STAGE_DONE	(0)
#define SETUP_DATA_STAGE_IN	(1)
#define SETUP_DATA_STAGE_OUT	(2)

/*
 * Endpoint absolute index calculation:
 *
 * MCUX EHCI USB device controller supports a specific
 * number of bidirectional endpoints. Bidirectional means
 * that an endpoint object is represented to the outside
 * as an OUT and an IN Endpoint with its own buffers
 * and control structures.
 *
 * EP_ABS_IDX refers to the corresponding control
 * structure, for example:
 *
 *  EP addr | ep_idx | ep_abs_idx
 * -------------------------------
 *  0x00    | 0x00   | 0x00
 *  0x80    | 0x00   | 0x01
 *  0x01    | 0x01   | 0x02
 *  0x81    | 0x01   | 0x03
 *  ....    | ....   | ....
 *
 * The NUM_OF_EP_MAX (and number of s_ep_ctrl) should be double
 * of num_bidir_endpoints.
 */
#define EP_ABS_IDX(ep)		(USB_EP_GET_IDX(ep) * 2 + \
					(USB_EP_GET_DIR(ep) >> 7))
#define NUM_OF_EP_MAX		(DT_INST_PROP(0, num_bidir_endpoints) * 2)

#define NUM_INSTS DT_NUM_INST_STATUS_OKAY(nxp_ehci) + DT_NUM_INST_STATUS_OKAY(nxp_lpcip3511)
BUILD_ASSERT(NUM_INSTS <= 1, "Only one USB device supported");

/* Controller ID is for HAL usage */
#if defined(CONFIG_SOC_SERIES_IMXRT5XX) || \
	defined(CONFIG_SOC_SERIES_IMXRT6XX) || \
	defined(CONFIG_SOC_LPC55S26) || defined(CONFIG_SOC_LPC55S28) || \
	defined(CONFIG_SOC_LPC55S16)
#define CONTROLLER_ID	kUSB_ControllerLpcIp3511Hs0
#elif defined(CONFIG_SOC_LPC55S36)
#define CONTROLLER_ID	kUSB_ControllerLpcIp3511Fs0
#elif defined(CONFIG_SOC_LPC55S69_CPU0) || defined(CONFIG_SOC_LPC55S69_CPU1)
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(usbhs))
#define CONTROLLER_ID	kUSB_ControllerLpcIp3511Hs0
#elif DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(usbfs))
#define CONTROLLER_ID	kUSB_ControllerLpcIp3511Fs0
#endif /* LPC55s69 */
#elif defined(CONFIG_SOC_SERIES_IMXRT11XX) || \
	defined(CONFIG_SOC_SERIES_IMXRT10XX) || \
	defined(CONFIG_SOC_SERIES_MCXN)
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(usb1))
#define CONTROLLER_ID kUSB_ControllerEhci0
#elif DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(usb2))
#define CONTROLLER_ID kUSB_ControllerEhci1
#endif /* IMX RT */
#elif defined(CONFIG_SOC_SERIES_RW6XX)
#define CONTROLLER_ID kUSB_ControllerEhci0
#else
/* If SOC has EHCI or LPCIP3511 then probably just need to add controller ID to this code */
#error "USB driver does not yet support this SOC"
#endif /* CONTROLLER ID */

/* We do not need a buffer for the write side on platforms that have USB RAM.
 * The SDK driver will copy the data buffer to be sent to USB RAM.
 */
#ifdef CONFIG_USB_DC_NXP_LPCIP3511
#define EP_BUF_NUMOF_BLOCKS	(NUM_OF_EP_MAX / 2)
#else
#define EP_BUF_NUMOF_BLOCKS	NUM_OF_EP_MAX
#endif

/* The max MPS is 1023 for FS, 1024 for HS. */
#if defined(CONFIG_NOCACHE_MEMORY)
#define EP_BUF_NONCACHED
K_HEAP_DEFINE_NOCACHE(ep_buf_pool, 1024 * EP_BUF_NUMOF_BLOCKS);
#else
K_HEAP_DEFINE(ep_buf_pool, 1024 * EP_BUF_NUMOF_BLOCKS);
#endif

struct usb_ep_ctrl_data {
	usb_device_callback_message_struct_t transfer_message;
	void *block;
	usb_dc_ep_callback callback;
	uint16_t ep_mps;
	uint8_t ep_enabled : 1;
	uint8_t ep_occupied : 1;
};

struct usb_dc_state {
	usb_device_struct_t dev_struct;
	/* Controller handle */
	usb_dc_status_callback status_cb;
	struct usb_ep_ctrl_data *eps;
	bool attached;
	uint8_t setup_data_stage;
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_USB_MCUX_THREAD_STACK_SIZE);

	struct k_thread thread;
};

static struct usb_ep_ctrl_data s_ep_ctrl[NUM_OF_EP_MAX];
static struct usb_dc_state dev_state;

/* Message queue for the usb thread */
K_MSGQ_DEFINE(usb_dc_msgq, sizeof(usb_device_callback_message_struct_t),
	CONFIG_USB_DC_MSG_QUEUE_LEN, 4);

#if defined(CONFIG_USB_DC_NXP_EHCI)
/* EHCI device driver interface */
static const usb_device_controller_interface_struct_t mcux_usb_iface = {
	USB_DeviceEhciInit, USB_DeviceEhciDeinit, USB_DeviceEhciSend,
	USB_DeviceEhciRecv, USB_DeviceEhciCancel, USB_DeviceEhciControl
};

extern void USB_DeviceEhciIsrFunction(void *deviceHandle);

#elif defined(CONFIG_USB_DC_NXP_LPCIP3511)
/* LPCIP3511 device driver interface */
static const usb_device_controller_interface_struct_t mcux_usb_iface = {
	USB_DeviceLpc3511IpInit, USB_DeviceLpc3511IpDeinit, USB_DeviceLpc3511IpSend,
	USB_DeviceLpc3511IpRecv, USB_DeviceLpc3511IpCancel, USB_DeviceLpc3511IpControl
};

extern void USB_DeviceLpcIp3511IsrFunction(void *deviceHandle);

#endif

int usb_dc_reset(void)
{
	if (dev_state.dev_struct.controllerHandle != NULL) {
		dev_state.dev_struct.controllerInterface->deviceControl(
						dev_state.dev_struct.controllerHandle,
						kUSB_DeviceControlSetDefaultStatus, NULL);
	}

	return 0;
}

int usb_dc_attach(void)
{
	usb_status_t status;

	dev_state.eps = &s_ep_ctrl[0];
	if (dev_state.attached) {
		LOG_WRN("Already attached");
		return 0;
	}

	dev_state.dev_struct.controllerInterface = &mcux_usb_iface;
	status = dev_state.dev_struct.controllerInterface->deviceInit(CONTROLLER_ID,
						&dev_state.dev_struct,
						&dev_state.dev_struct.controllerHandle);
	if (kStatus_USB_Success != status) {
		return -EIO;
	}

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
		    usb_isr_handler, 0, 0);
	irq_enable(DT_INST_IRQN(0));
	dev_state.attached = true;
	status = dev_state.dev_struct.controllerInterface->deviceControl(
						dev_state.dev_struct.controllerHandle,
						kUSB_DeviceControlRun, NULL);

	LOG_DBG("Attached");

	return 0;
}

int usb_dc_detach(void)
{
	usb_status_t status;

	if (dev_state.dev_struct.controllerHandle == NULL) {
		LOG_WRN("Device not attached");
		return 0;
	}

	status = dev_state.dev_struct.controllerInterface->deviceControl(
						   dev_state.dev_struct.controllerHandle,
						   kUSB_DeviceControlStop,
						   NULL);
	if (kStatus_USB_Success != status) {
		return -EIO;
	}

	status = dev_state.dev_struct.controllerInterface->deviceDeinit(
						   dev_state.dev_struct.controllerHandle);
	if (kStatus_USB_Success != status) {
		return -EIO;
	}

	dev_state.dev_struct.controllerHandle = NULL;
	dev_state.attached = false;
	LOG_DBG("Detached");

	return 0;
}

int usb_dc_set_address(const uint8_t addr)
{
	usb_status_t status;

	dev_state.dev_struct.deviceAddress = addr;
	status = dev_state.dev_struct.controllerInterface->deviceControl(
		dev_state.dev_struct.controllerHandle,
		kUSB_DeviceControlPreSetDeviceAddress,
		&dev_state.dev_struct.deviceAddress);
	if (kStatus_USB_Success != status) {
		LOG_ERR("Failed to set device address");
		return -EINVAL;
	}
	return 0;
}

int usb_dc_ep_check_cap(const struct usb_dc_ep_cfg_data *const cfg)
{
	uint8_t ep_abs_idx =  EP_ABS_IDX(cfg->ep_addr);
	uint8_t ep_idx = USB_EP_GET_IDX(cfg->ep_addr);

	if ((cfg->ep_type == USB_DC_EP_CONTROL) && ep_idx) {
		LOG_ERR("invalid endpoint configuration");
		return -1;
	}

	if (ep_abs_idx >= NUM_OF_EP_MAX) {
		LOG_ERR("endpoint index/address out of range");
		return -1;
	}

	return 0;
}

int usb_dc_ep_configure(const struct usb_dc_ep_cfg_data *const cfg)
{
	uint8_t ep_abs_idx =  EP_ABS_IDX(cfg->ep_addr);
	usb_device_endpoint_init_struct_t ep_init;
	struct usb_ep_ctrl_data *eps = &dev_state.eps[ep_abs_idx];
	usb_status_t status;
	uint8_t ep;

	ep_init.zlt = 0U;
	ep_init.endpointAddress = cfg->ep_addr;
	ep_init.maxPacketSize = cfg->ep_mps;
	ep_init.transferType = cfg->ep_type;

	if (ep_abs_idx >= NUM_OF_EP_MAX) {
		LOG_ERR("Wrong endpoint index/address");
		return -EINVAL;
	}

	if (dev_state.eps[ep_abs_idx].ep_enabled) {
		LOG_WRN("Endpoint already configured");
		return 0;
	}

	ep = cfg->ep_addr;
	status = dev_state.dev_struct.controllerInterface->deviceControl(
			dev_state.dev_struct.controllerHandle,
			kUSB_DeviceControlEndpointDeinit, &ep);
	if (kStatus_USB_Success != status) {
		LOG_WRN("Failed to un-initialize endpoint (status=%d)", (int)status);
	}

#ifdef CONFIG_USB_DC_NXP_LPCIP3511
	/* Allocate buffers used during read operation */
	if (USB_EP_DIR_IS_OUT(cfg->ep_addr)) {
#endif
		void **block;

		block = &(eps->block);
		if (*block) {
			k_heap_free(&ep_buf_pool, *block);
			*block = NULL;
		}

		*block = k_heap_alloc(&ep_buf_pool, cfg->ep_mps, K_NO_WAIT);
		if (*block == NULL) {
			LOG_ERR("Failed to allocate memory");
			return -ENOMEM;
		}

		memset(*block, 0, cfg->ep_mps);
#ifdef CONFIG_USB_DC_NXP_LPCIP3511
	}
#endif

	dev_state.eps[ep_abs_idx].ep_mps = cfg->ep_mps;
	status = dev_state.dev_struct.controllerInterface->deviceControl(
			dev_state.dev_struct.controllerHandle,
			kUSB_DeviceControlEndpointInit, &ep_init);
	if (kStatus_USB_Success != status) {
		LOG_ERR("Failed to initialize endpoint");
		return -EIO;
	}

	/*
	 * If it is control endpoint, controller will prime setup
	 * here set the occupied flag.
	 */
	if ((USB_EP_GET_IDX(cfg->ep_addr) == USB_CONTROL_ENDPOINT) &&
	    (USB_EP_DIR_IS_OUT(cfg->ep_addr))) {
		dev_state.eps[ep_abs_idx].ep_occupied = true;
	}
	dev_state.eps[ep_abs_idx].ep_enabled = true;

	return 0;
}

int usb_dc_ep_set_stall(const uint8_t ep)
{
	uint8_t endpoint = ep;
	uint8_t ep_abs_idx = EP_ABS_IDX(ep);
	usb_status_t status;

	if (ep_abs_idx >= NUM_OF_EP_MAX) {
		LOG_ERR("Wrong endpoint index/address");
		return -EINVAL;
	}

	status = dev_state.dev_struct.controllerInterface->deviceControl(
			dev_state.dev_struct.controllerHandle,
			kUSB_DeviceControlEndpointStall, &endpoint);
	if (kStatus_USB_Success != status) {
		LOG_ERR("Failed to stall endpoint");
		return -EIO;
	}

	return 0;
}

int usb_dc_ep_clear_stall(const uint8_t ep)
{
	uint8_t endpoint = ep;
	uint8_t ep_abs_idx = EP_ABS_IDX(ep);
	usb_status_t status;

	if (ep_abs_idx >= NUM_OF_EP_MAX) {
		LOG_ERR("Wrong endpoint index/address");
		return -EINVAL;
	}

	status = dev_state.dev_struct.controllerInterface->deviceControl(
			dev_state.dev_struct.controllerHandle,
			kUSB_DeviceControlEndpointUnstall, &endpoint);
	if (kStatus_USB_Success != status) {
		LOG_ERR("Failed to clear stall");
		return -EIO;
	}

	if ((USB_EP_GET_IDX(ep) != USB_CONTROL_ENDPOINT) &&
	    (USB_EP_DIR_IS_OUT(ep))) {
		status = dev_state.dev_struct.controllerInterface->deviceRecv(
				dev_state.dev_struct.controllerHandle, ep,
				(uint8_t *)dev_state.eps[ep_abs_idx].block,
				(uint32_t)dev_state.eps[ep_abs_idx].ep_mps);
		if (kStatus_USB_Success != status) {
			LOG_ERR("Failed to enable reception on 0x%02x", ep);
			return -EIO;
		}

		dev_state.eps[ep_abs_idx].ep_occupied = true;
	}

	return 0;
}

int usb_dc_ep_is_stalled(const uint8_t ep, uint8_t *const stalled)
{
	uint8_t ep_abs_idx = EP_ABS_IDX(ep);
	usb_device_endpoint_status_struct_t ep_status;
	usb_status_t status;

	if (ep_abs_idx >= NUM_OF_EP_MAX) {
		LOG_ERR("Wrong endpoint index/address");
		return -EINVAL;
	}

	if (!stalled) {
		return -EINVAL;
	}

	*stalled = 0;
	ep_status.endpointAddress = ep;
	ep_status.endpointStatus = kUSB_DeviceEndpointStateIdle;
	status = dev_state.dev_struct.controllerInterface->deviceControl(
			dev_state.dev_struct.controllerHandle,
			kUSB_DeviceControlGetEndpointStatus, &ep_status);
	if (kStatus_USB_Success != status) {
		LOG_ERR("Failed to get endpoint status");
		return -EIO;
	}

	*stalled = (uint8_t)ep_status.endpointStatus;

	return 0;
}

int usb_dc_ep_halt(const uint8_t ep)
{
	return usb_dc_ep_set_stall(ep);
}

int usb_dc_ep_enable(const uint8_t ep)
{
	uint8_t ep_abs_idx = EP_ABS_IDX(ep);
	usb_status_t status;

	/*
	 * endpoint 0 OUT is primed by controller driver when configure this
	 * endpoint.
	 */
	if (!ep_abs_idx) {
		return 0;
	}

	if (ep_abs_idx >= NUM_OF_EP_MAX) {
		LOG_ERR("Wrong endpoint index/address");
		return -EINVAL;
	}

	if (dev_state.eps[ep_abs_idx].ep_occupied) {
		LOG_WRN("endpoint 0x%x already enabled", ep);
		return -EALREADY;
	}

	if ((USB_EP_GET_IDX(ep) != USB_CONTROL_ENDPOINT) &&
	    (USB_EP_DIR_IS_OUT(ep))) {
		status = dev_state.dev_struct.controllerInterface->deviceRecv(
				dev_state.dev_struct.controllerHandle, ep,
				(uint8_t *)dev_state.eps[ep_abs_idx].block,
				(uint32_t)dev_state.eps[ep_abs_idx].ep_mps);
		if (kStatus_USB_Success != status) {
			LOG_ERR("Failed to enable reception on 0x%02x", ep);
			return -EIO;
		}

		dev_state.eps[ep_abs_idx].ep_occupied = true;
	} else {
		/*
		 * control endpoint just be enabled before enumeration,
		 * when running here, setup has been primed.
		 */
		dev_state.eps[ep_abs_idx].ep_occupied = true;
	}

	return 0;
}

int usb_dc_ep_disable(const uint8_t ep)
{
	uint8_t ep_abs_idx = EP_ABS_IDX(ep);
	usb_status_t status;

	if (ep_abs_idx >= NUM_OF_EP_MAX) {
		LOG_ERR("Wrong endpoint index/address");
		return -EINVAL;
	}

	if (dev_state.dev_struct.controllerHandle != NULL) {
		status = dev_state.dev_struct.controllerInterface->deviceCancel(
							dev_state.dev_struct.controllerHandle,
							ep);
		if (kStatus_USB_Success != status) {
			LOG_ERR("Failed to disable ep 0x%02x", ep);
			return -EIO;
		}
	}

	dev_state.eps[ep_abs_idx].ep_enabled = false;
	dev_state.eps[ep_abs_idx].ep_occupied = false;

	return 0;
}

int usb_dc_ep_flush(const uint8_t ep)
{
	uint8_t ep_abs_idx = EP_ABS_IDX(ep);

	if (ep_abs_idx >= NUM_OF_EP_MAX) {
		LOG_ERR("Wrong endpoint index/address");
		return -EINVAL;
	}

	LOG_DBG("Not implemented, idx 0x%02x, ep %u", ep_abs_idx, ep);

	return 0;
}

int usb_dc_ep_write(const uint8_t ep, const uint8_t *const data,
		    const uint32_t data_len, uint32_t *const ret_bytes)
{
	uint8_t ep_abs_idx = EP_ABS_IDX(ep);
	uint8_t *buffer;
	uint32_t len_to_send = data_len;
	usb_status_t status;

	if (ep_abs_idx >= NUM_OF_EP_MAX) {
		LOG_ERR("Wrong endpoint index/address");
		return -EINVAL;
	}

	if (USB_EP_GET_DIR(ep) != USB_EP_DIR_IN) {
		LOG_ERR("Wrong endpoint direction");
		return -EINVAL;
	}

	/* Copy the data for SoC's that do not have a USB RAM
	 * as the SDK driver will copy the data into USB RAM,
	 * if available.
	 */
#ifndef CONFIG_USB_DC_NXP_LPCIP3511
	buffer = (uint8_t *)dev_state.eps[ep_abs_idx].block;

	if (data_len > dev_state.eps[ep_abs_idx].ep_mps) {
		len_to_send = dev_state.eps[ep_abs_idx].ep_mps;
	}

	for (uint32_t n = 0; n < len_to_send; n++) {
		buffer[n] = data[n];
	}
#else
	buffer = (uint8_t *)data;
#endif

#if defined(CONFIG_HAS_MCUX_CACHE) && !defined(EP_BUF_NONCACHED)
	DCACHE_CleanByRange((uint32_t)buffer, len_to_send);
#endif
	status = dev_state.dev_struct.controllerInterface->deviceSend(
						dev_state.dev_struct.controllerHandle,
						ep, buffer, len_to_send);
	if (kStatus_USB_Success != status) {
		LOG_ERR("Failed to fill ep 0x%02x buffer", ep);
		return -EIO;
	}

	if (ret_bytes) {
		*ret_bytes = len_to_send;
	}

	return 0;
}

static void update_control_stage(usb_device_callback_message_struct_t *cb_msg,
				 uint32_t data_len, uint32_t max_data_len)
{
	struct usb_setup_packet *usbd_setup;

	usbd_setup = (struct usb_setup_packet *)cb_msg->buffer;

	if (cb_msg->isSetup) {
		if (usbd_setup->wLength == 0) {
			dev_state.setup_data_stage = SETUP_DATA_STAGE_DONE;
		} else if (usb_reqtype_is_to_host(usbd_setup)) {
			dev_state.setup_data_stage = SETUP_DATA_STAGE_IN;
		} else {
			dev_state.setup_data_stage = SETUP_DATA_STAGE_OUT;
		}
	} else {
		if (dev_state.setup_data_stage != SETUP_DATA_STAGE_DONE) {
			if ((data_len >= max_data_len) ||
			    (data_len < dev_state.eps[0].ep_mps)) {
				dev_state.setup_data_stage = SETUP_DATA_STAGE_DONE;
			}
		}
	}
}

int usb_dc_ep_read_wait(uint8_t ep, uint8_t *data, uint32_t max_data_len,
			uint32_t *read_bytes)
{
	uint8_t ep_abs_idx = EP_ABS_IDX(ep);
	uint32_t data_len;
	uint8_t *bufp = NULL;

	if (dev_state.eps[ep_abs_idx].ep_occupied) {
		LOG_ERR("Endpoint is occupied by the controller");
		return -EBUSY;
	}

	if ((ep_abs_idx >= NUM_OF_EP_MAX) ||
	    (USB_EP_GET_DIR(ep) != USB_EP_DIR_OUT)) {
		LOG_ERR("Wrong endpoint index/address/direction");
		return -EINVAL;
	}

	/* Allow to read 0 bytes */
	if (!data && max_data_len) {
		LOG_ERR("Wrong arguments");
		return -EINVAL;
	}

	/*
	 * It is control setup, we should use message.buffer,
	 * this buffer is from internal setup array.
	 */
	bufp = dev_state.eps[ep_abs_idx].transfer_message.buffer;
	data_len = dev_state.eps[ep_abs_idx].transfer_message.length;
	if (data_len == USB_UNINITIALIZED_VAL_32) {
		if (read_bytes) {
			*read_bytes = 0;
		}
		return -EINVAL;
	}

	if (!data && !max_data_len) {
		/* When both buffer and max data to read are zero return the
		 * available data in buffer.
		 */
		if (read_bytes) {
			*read_bytes = data_len;
		}
		return 0;
	}

	if (data_len > max_data_len) {
		LOG_WRN("Not enough room to copy all the data!");
		data_len = max_data_len;
	}

	if (data != NULL) {
		for (uint32_t i = 0; i < data_len; i++) {
			data[i] = bufp[i];
		}
	}

	if (read_bytes) {
		*read_bytes = data_len;
	}

	if (USB_EP_GET_IDX(ep) == USB_ENDPOINT_CONTROL) {
		update_control_stage(&dev_state.eps[0].transfer_message,
				     data_len, max_data_len);
	}

	return 0;
}

int usb_dc_ep_read_continue(uint8_t ep)
{
	uint8_t ep_abs_idx = EP_ABS_IDX(ep);
	usb_status_t status;

	if (ep_abs_idx >= NUM_OF_EP_MAX ||
	    USB_EP_GET_DIR(ep) != USB_EP_DIR_OUT) {
		LOG_ERR("Wrong endpoint index/address/direction");
		return -EINVAL;
	}

	if (dev_state.eps[ep_abs_idx].ep_occupied) {
		LOG_WRN("endpoint 0x%x already occupied", ep);
		return -EBUSY;
	}

	if (USB_EP_GET_IDX(ep) == USB_ENDPOINT_CONTROL) {
		if (dev_state.setup_data_stage == SETUP_DATA_STAGE_DONE) {
			return 0;
		}

		if (dev_state.setup_data_stage == SETUP_DATA_STAGE_IN) {
			dev_state.setup_data_stage = SETUP_DATA_STAGE_DONE;
		}
	}

	status = dev_state.dev_struct.controllerInterface->deviceRecv(
			    dev_state.dev_struct.controllerHandle, ep,
			    (uint8_t *)dev_state.eps[ep_abs_idx].block,
			    dev_state.eps[ep_abs_idx].ep_mps);
	if (kStatus_USB_Success != status) {
		LOG_ERR("Failed to enable reception on ep 0x%02x", ep);
		return -EIO;
	}

	dev_state.eps[ep_abs_idx].ep_occupied = true;

	return 0;
}

int usb_dc_ep_read(const uint8_t ep, uint8_t *const data,
		   const uint32_t max_data_len, uint32_t *const read_bytes)
{
	int retval = usb_dc_ep_read_wait(ep, data, max_data_len, read_bytes);

	if (retval) {
		return retval;
	}

	if (!data && !max_data_len) {
		/*
		 * When both buffer and max data to read are zero the above
		 * call would fetch the data len and we simply return.
		 */
		return 0;
	}

	return usb_dc_ep_read_continue(ep);
}

int usb_dc_ep_set_callback(const uint8_t ep, const usb_dc_ep_callback cb)
{
	uint8_t ep_abs_idx = EP_ABS_IDX(ep);

	if (ep_abs_idx >= NUM_OF_EP_MAX) {
		LOG_ERR("Wrong endpoint index/address");
		return -EINVAL;
	}

	if (!dev_state.attached) {
		return -EINVAL;
	}
	dev_state.eps[ep_abs_idx].callback = cb;

	return 0;
}

void usb_dc_set_status_callback(const usb_dc_status_callback cb)
{
	dev_state.status_cb = cb;
}

int usb_dc_ep_mps(const uint8_t ep)
{
	uint8_t ep_abs_idx = EP_ABS_IDX(ep);

	if (ep_abs_idx >= NUM_OF_EP_MAX) {
		LOG_ERR("Wrong endpoint index/address");
		return -EINVAL;
	}

	return dev_state.eps[ep_abs_idx].ep_mps;
}

static void handle_bus_reset(void)
{
	usb_device_endpoint_init_struct_t ep_init;
	uint8_t ep_abs_idx = 0;
	usb_status_t status;

	dev_state.dev_struct.deviceAddress = 0;
	status = dev_state.dev_struct.controllerInterface->deviceControl(
			dev_state.dev_struct.controllerHandle,
			kUSB_DeviceControlSetDefaultStatus, NULL);
	if (kStatus_USB_Success != status) {
		LOG_ERR("Failed to set default status");
	}

	for (int i = 0; i < NUM_OF_EP_MAX; i++) {
		dev_state.eps[i].ep_occupied = false;
		dev_state.eps[i].ep_enabled = false;
	}

	ep_init.zlt = 0U;
	ep_init.transferType = USB_ENDPOINT_CONTROL;
	ep_init.maxPacketSize = USB_CONTROL_EP_MPS;
	ep_init.endpointAddress = USB_CONTROL_EP_OUT;

	ep_abs_idx =  EP_ABS_IDX(ep_init.endpointAddress);
	dev_state.eps[ep_abs_idx].ep_mps = USB_CONTROL_EP_MPS;

	status = dev_state.dev_struct.controllerInterface->deviceControl(
			dev_state.dev_struct.controllerHandle,
			kUSB_DeviceControlEndpointInit, &ep_init);
	if (kStatus_USB_Success != status) {
		LOG_ERR("Failed to initialize control OUT endpoint");
	}

	dev_state.eps[ep_abs_idx].ep_occupied = false;
	dev_state.eps[ep_abs_idx].ep_enabled = true;

	ep_init.endpointAddress = USB_CONTROL_EP_IN;
	ep_abs_idx = EP_ABS_IDX(ep_init.endpointAddress);
	dev_state.eps[ep_abs_idx].ep_mps = USB_CONTROL_EP_MPS;
	status = dev_state.dev_struct.controllerInterface->deviceControl(
			dev_state.dev_struct.controllerHandle,
			kUSB_DeviceControlEndpointInit, &ep_init);
	if (kStatus_USB_Success != status) {
		LOG_ERR("Failed to initialize control IN endpoint");
	}

	dev_state.eps[ep_abs_idx].ep_occupied = false;
	dev_state.eps[ep_abs_idx].ep_enabled = true;
}

static void handle_transfer_msg(usb_device_callback_message_struct_t *cb_msg)
{
	uint8_t ep_status_code = 0;
	uint8_t ep = cb_msg->code;
	uint8_t ep_abs_idx = EP_ABS_IDX(ep);
	usb_status_t status;

	dev_state.eps[ep_abs_idx].ep_occupied = false;

	if (cb_msg->length == UINT32_MAX) {
		/*
		 * Probably called from USB_DeviceEhciCancel()
		 * LOG_WRN("Drop message for ep 0x%02x", ep);
		 */
		return;
	}

	if (cb_msg->isSetup) {
		ep_status_code = USB_DC_EP_SETUP;
	} else {
		/* IN TOKEN */
		if (USB_EP_DIR_IS_IN(ep)) {
			if ((dev_state.dev_struct.deviceAddress != 0) && (ep_abs_idx == 1)) {
				/*
				 * Set Address in the status stage in
				 * the IN transfer.
				 */
				status = dev_state.dev_struct.controllerInterface->deviceControl(
					dev_state.dev_struct.controllerHandle,
					kUSB_DeviceControlSetDeviceAddress,
					&dev_state.dev_struct.deviceAddress);
				if (kStatus_USB_Success != status) {
					LOG_ERR("Failed to set device address");
					return;
				}
				dev_state.dev_struct.deviceAddress = 0;
			}
			ep_status_code = USB_DC_EP_DATA_IN;
		}
		/* OUT TOKEN */
		else {
			ep_status_code = USB_DC_EP_DATA_OUT;
		}
	}

	if (dev_state.eps[ep_abs_idx].callback) {
#if defined(CONFIG_HAS_MCUX_CACHE) && !defined(EP_BUF_NONCACHED)
		if (cb_msg->length) {
			DCACHE_InvalidateByRange((uint32_t)cb_msg->buffer,
						 cb_msg->length);
		}
#endif
		dev_state.eps[ep_abs_idx].callback(ep, ep_status_code);
	} else {
		LOG_ERR("No cb pointer for endpoint 0x%02x", ep);
	}
}

/**
 * Similar to the kinetis driver, this thread is used to not run the USB device
 * stack/endpoint callbacks in the ISR context. This is because callbacks from
 * the USB stack may use mutexes, or other kernel functions not supported from
 * an interrupt context.
 */
static void usb_mcux_thread_main(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	uint8_t ep_abs_idx;
	usb_device_callback_message_struct_t msg;

	while (1) {
		k_msgq_get(&usb_dc_msgq, &msg, K_FOREVER);
		switch (msg.code) {
		case kUSB_DeviceNotifyBusReset:
			handle_bus_reset();
			dev_state.status_cb(USB_DC_RESET, NULL);
			break;
		case kUSB_DeviceNotifyError:
			dev_state.status_cb(USB_DC_ERROR, NULL);
			break;
		case kUSB_DeviceNotifySuspend:
			dev_state.status_cb(USB_DC_SUSPEND, NULL);
			break;
		case kUSB_DeviceNotifyResume:
			dev_state.status_cb(USB_DC_RESUME, NULL);
			break;
		default:
			ep_abs_idx = EP_ABS_IDX(msg.code);

			if (ep_abs_idx >= NUM_OF_EP_MAX) {
				LOG_ERR("Wrong endpoint index/address");
				return;
			}

			memcpy(&dev_state.eps[ep_abs_idx].transfer_message, &msg,
			       sizeof(usb_device_callback_message_struct_t));
			handle_transfer_msg(&dev_state.eps[ep_abs_idx].transfer_message);
		}
	}
}

/* Notify the up layer the KHCI status changed. */
usb_status_t USB_DeviceNotificationTrigger(void *handle, void *msg)
{
	/* Submit to message queue */
	k_msgq_put(&usb_dc_msgq,
		(usb_device_callback_message_struct_t *)msg, K_NO_WAIT);
	return kStatus_USB_Success;
}

static void usb_isr_handler(void)
{
#if defined(CONFIG_USB_DC_NXP_EHCI)
	USB_DeviceEhciIsrFunction(&dev_state);
#elif defined(CONFIG_USB_DC_NXP_LPCIP3511)
	USB_DeviceLpcIp3511IsrFunction(&dev_state);
#endif
}

static int usb_mcux_init(void)
{
	int err;

	k_thread_create(&dev_state.thread, dev_state.thread_stack,
			CONFIG_USB_MCUX_THREAD_STACK_SIZE,
			usb_mcux_thread_main, NULL, NULL, NULL,
			K_PRIO_COOP(2), 0, K_NO_WAIT);
	k_thread_name_set(&dev_state.thread, "usb_mcux");

	PINCTRL_DT_INST_DEFINE(0);

	/* Apply pinctrl state */
	err = pinctrl_apply_state(PINCTRL_DT_INST_DEV_CONFIG_GET(0), PINCTRL_STATE_DEFAULT);
	if (err) {
		return err;
	}

	return 0;
}

SYS_INIT(usb_mcux_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
