/*
 * Copyright (c) 2018-2019, NXP
 * Copyright (c) 2019 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_kinetis_usbd

#include <soc.h>
#include <string.h>
#include <drivers/usb/usb_dc.h>
#include <usb/usb_device.h>
#include <soc.h>
#include <device.h>
#include "usb_dc_mcux.h"
#include "usb_device_ehci.h"

#ifdef CONFIG_HAS_MCUX_CACHE
#include <fsl_cache.h>
#endif

#define LOG_LEVEL CONFIG_USB_DRIVER_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(usb_dc_mcux_ehci);

#define CONTROLLER_ID kUSB_ControllerEhci0

static void usb_isr_handler(void);
extern void USB_DeviceEhciIsrFunction(void *deviceHandle);

/* the setup transfer state */
#define SETUP_DATA_STAGE_DONE	(0)
#define SETUP_DATA_STAGE_IN	(1)
#define SETUP_DATA_STAGE_OUT	(2)

/*
 * Endpoint absolut index calculation:
 *
 * MCUX EHCI USB device controller supports a specific
 * number of bidirectional endpoints. Bidirectional means
 * that an endpoint object is represented to the outside
 * as an OUT and an IN Eindpoint with its own buffers
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

/* The minimum value is 1 */
#define EP_BUF_NUMOF_BLOCKS	((NUM_OF_EP_MAX + 3) / 4)

/* The max MPS is 1023 for FS, 1024 for HS. */
#if defined(CONFIG_NOCACHE_MEMORY)
#define EP_BUF_NONCACHED
__nocache K_MEM_POOL_DEFINE(ep_buf_pool, 16, 1024, EP_BUF_NUMOF_BLOCKS, 4);
#else
K_MEM_POOL_DEFINE(ep_buf_pool, 16, 1024, EP_BUF_NUMOF_BLOCKS, 4);
#endif

static usb_ep_ctrl_data_t s_ep_ctrl[NUM_OF_EP_MAX];
static usb_device_struct_t dev_data;

#if ((defined(USB_DEVICE_CONFIG_EHCI)) && (USB_DEVICE_CONFIG_EHCI > 0U))
/* EHCI device driver interface */
static const usb_device_controller_interface_struct_t ehci_iface = {
	USB_DeviceEhciInit, USB_DeviceEhciDeinit, USB_DeviceEhciSend,
	USB_DeviceEhciRecv, USB_DeviceEhciCancel, USB_DeviceEhciControl
};
#endif

int usb_dc_reset(void)
{
	if (dev_data.controllerHandle != NULL) {
		dev_data.interface->deviceControl(dev_data.controllerHandle,
						  kUSB_DeviceControlStop, NULL);
		dev_data.interface->deviceDeinit(dev_data.controllerHandle);
		dev_data.controllerHandle = NULL;
	}

	return 0;
}

int usb_dc_attach(void)
{
	usb_status_t status;

	dev_data.eps = &s_ep_ctrl[0];
	if (dev_data.attached) {
		LOG_WRN("Already attached");
		return 0;
	}

	dev_data.interface = &ehci_iface;
	status = dev_data.interface->deviceInit(CONTROLLER_ID, &dev_data,
						&dev_data.controllerHandle);
	if (kStatus_USB_Success != status) {
		return -EIO;
	}

	/* Connect and enable USB interrupt */
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
		    usb_isr_handler, 0, 0);
	irq_enable(DT_INST_IRQN(0));
	dev_data.attached = true;
	status = dev_data.interface->deviceControl(dev_data.controllerHandle,
						   kUSB_DeviceControlRun, NULL);
	if (kStatus_USB_Success != status) {
		return -EIO;
	}

	LOG_DBG("Attached");

	return 0;
}

int usb_dc_detach(void)
{
	usb_status_t status;

	if (dev_data.controllerHandle == NULL) {
		LOG_WRN("Device not attached");
		return 0;
	}

	status = dev_data.interface->deviceControl(dev_data.controllerHandle,
						   kUSB_DeviceControlStop,
						   NULL);
	if (kStatus_USB_Success != status) {
		return -EIO;
	}

	status = dev_data.interface->deviceDeinit(dev_data.controllerHandle);
	if (kStatus_USB_Success != status) {
		return -EIO;
	}

	dev_data.controllerHandle = NULL;
	dev_data.attached = false;
	LOG_DBG("Detached");

	return 0;
}

int usb_dc_set_address(const uint8_t addr)
{
	LOG_DBG("");
	dev_data.address = addr;
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
	struct k_mem_block *block;
	struct usb_ep_ctrl_data *eps = &dev_data.eps[ep_abs_idx];
	usb_status_t status;

	ep_init.zlt = 0U;
	ep_init.endpointAddress = cfg->ep_addr;
	ep_init.maxPacketSize = cfg->ep_mps;
	ep_init.transferType = cfg->ep_type;
	dev_data.eps[ep_abs_idx].ep_type = cfg->ep_type;

	if (ep_abs_idx >= NUM_OF_EP_MAX) {
		LOG_ERR("Wrong endpoint index/address");
		return -EINVAL;
	}

	if (dev_data.eps[ep_abs_idx].ep_enabled) {
		LOG_WRN("Endpoint already configured");
		return 0;
	}

	block = &(eps->block);
	if (block->data) {
		k_mem_pool_free(block);
		block->data = NULL;
	}

	if (k_mem_pool_alloc(&ep_buf_pool, block, cfg->ep_mps, K_MSEC(10))) {
		LOG_ERR("Memory allocation time-out");
		return -ENOMEM;
	}

	memset(block->data, 0, cfg->ep_mps);
	dev_data.eps[ep_abs_idx].ep_mps = cfg->ep_mps;
	status = dev_data.interface->deviceControl(dev_data.controllerHandle,
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
		dev_data.eps[ep_abs_idx].ep_occupied = true;
	}
	dev_data.eps[ep_abs_idx].ep_enabled = true;

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

	status = dev_data.interface->deviceControl(dev_data.controllerHandle,
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

	status = dev_data.interface->deviceControl(dev_data.controllerHandle,
			kUSB_DeviceControlEndpointUnstall, &endpoint);
	if (kStatus_USB_Success != status) {
		LOG_ERR("Failed to clear stall");
		return -EIO;
	}

	if ((USB_EP_GET_IDX(ep) != USB_CONTROL_ENDPOINT) &&
	    (USB_EP_DIR_IS_OUT(ep))) {
		status = dev_data.interface->deviceRecv(
				dev_data.controllerHandle, ep,
				(uint8_t *)dev_data.eps[ep_abs_idx].block.data,
				(uint32_t)dev_data.eps[ep_abs_idx].ep_mps);
		if (kStatus_USB_Success != status) {
			LOG_ERR("Failed to enable reception on 0x%02x", ep);
			return -EIO;
		}

		dev_data.eps[ep_abs_idx].ep_occupied = true;
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
	status = dev_data.interface->deviceControl(dev_data.controllerHandle,
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

	if (dev_data.eps[ep_abs_idx].ep_occupied) {
		LOG_WRN("endpoint 0x%x already enabled", ep);
		return -EALREADY;
	}

	if ((USB_EP_GET_IDX(ep) != USB_CONTROL_ENDPOINT) &&
	    (USB_EP_DIR_IS_OUT(ep))) {
		status = dev_data.interface->deviceRecv(
				dev_data.controllerHandle, ep,
				(uint8_t *)dev_data.eps[ep_abs_idx].block.data,
				(uint32_t)dev_data.eps[ep_abs_idx].ep_mps);
		if (kStatus_USB_Success != status) {
			LOG_ERR("Failed to enable reception on 0x%02x", ep);
			return -EIO;
		}

		dev_data.eps[ep_abs_idx].ep_occupied = true;
	} else {
		/*
		 * control enpoint just be enabled before enumeration,
		 * when running here, setup has been primed.
		 */
		dev_data.eps[ep_abs_idx].ep_occupied = true;
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

	status = dev_data.interface->deviceCancel(dev_data.controllerHandle,
						  ep);
	if (kStatus_USB_Success != status) {
		LOG_ERR("Failed to disable ep 0x%02x", ep);
		return -EIO;
	}

	dev_data.eps[ep_abs_idx].ep_enabled = false;

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
	uint8_t *buffer = (uint8_t *)dev_data.eps[ep_abs_idx].block.data;
	uint32_t len_to_send;
	usb_status_t status;

	if (ep_abs_idx >= NUM_OF_EP_MAX) {
		LOG_ERR("Wrong endpoint index/address");
		return -EINVAL;
	}

	if (data_len > dev_data.eps[ep_abs_idx].ep_mps) {
		len_to_send = dev_data.eps[ep_abs_idx].ep_mps;
	} else {
		len_to_send = data_len;
	}

	for (uint32_t n = 0; n < len_to_send; n++) {
		buffer[n] = data[n];
	}

#if defined(CONFIG_HAS_MCUX_CACHE) && !defined(EP_BUF_NONCACHED)
	DCACHE_CleanByRange((uint32_t)buffer, len_to_send);
#endif
	status = dev_data.interface->deviceSend(dev_data.controllerHandle,
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
			dev_data.setupDataStage = SETUP_DATA_STAGE_DONE;
		} else if (REQTYPE_GET_DIR(usbd_setup->bmRequestType)
			   == REQTYPE_DIR_TO_HOST) {
			dev_data.setupDataStage = SETUP_DATA_STAGE_IN;
		} else {
			dev_data.setupDataStage = SETUP_DATA_STAGE_OUT;
		}
	} else {
		if (dev_data.setupDataStage != SETUP_DATA_STAGE_DONE) {
			if ((data_len >= max_data_len) ||
			    (data_len < dev_data.eps[0].ep_mps)) {
				dev_data.setupDataStage = SETUP_DATA_STAGE_DONE;
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

	while (dev_data.eps[ep_abs_idx].ep_occupied) {
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
	bufp = dev_data.eps[ep_abs_idx].transfer_message.buffer;
	data_len = dev_data.eps[ep_abs_idx].transfer_message.length;
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
		update_control_stage(&dev_data.eps[0].transfer_message,
				     data_len, max_data_len);
	}

	return 0;
}

int usb_dc_ep_read_continue(uint8_t ep)
{
	uint8_t ep_abs_idx = EP_ABS_IDX(ep);
	usb_status_t status;

	if (ep_abs_idx >= NUM_OF_EP_MAX) {
		LOG_ERR("Wrong endpoint index/address");
		return -EINVAL;
	}

	if (dev_data.eps[ep_abs_idx].ep_occupied) {
		LOG_WRN("endpoint 0x%x already occupied", ep);
		return -EBUSY;
	}

	if (USB_EP_GET_IDX(ep) == USB_ENDPOINT_CONTROL) {
		if (dev_data.setupDataStage == SETUP_DATA_STAGE_DONE) {
			return 0;
		}

		if (dev_data.setupDataStage == SETUP_DATA_STAGE_IN) {
			dev_data.setupDataStage = SETUP_DATA_STAGE_DONE;
		}
	}

	status = dev_data.interface->deviceRecv(dev_data.controllerHandle, ep,
			       (uint8_t *)dev_data.eps[ep_abs_idx].block.data,
			       dev_data.eps[ep_abs_idx].ep_mps);
	if (kStatus_USB_Success != status) {
		LOG_ERR("Failed to enable reception on ep 0x%02x", ep);
		return -EIO;
	}

	dev_data.eps[ep_abs_idx].ep_occupied = true;

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

	if (!dev_data.attached) {
		return -EINVAL;
	}
	dev_data.eps[ep_abs_idx].callback = cb;

	return 0;
}

void usb_dc_set_status_callback(const usb_dc_status_callback cb)
{
	dev_data.status_callback = cb;
}

int usb_dc_ep_mps(const uint8_t ep)
{
	uint8_t ep_abs_idx = EP_ABS_IDX(ep);

	if (ep_abs_idx >= NUM_OF_EP_MAX) {
		LOG_ERR("Wrong endpoint index/address");
		return -EINVAL;
	}

	return dev_data.eps[ep_abs_idx].ep_mps;
}

static void handle_bus_reset(void)
{
	usb_device_endpoint_init_struct_t ep_init;
	uint8_t ep_abs_idx = 0;
	usb_status_t status;

	dev_data.address = 0;
	status = dev_data.interface->deviceControl(dev_data.controllerHandle,
			kUSB_DeviceControlSetDefaultStatus, NULL);
	if (kStatus_USB_Success != status) {
		LOG_ERR("Failed to set default status");
	}

	for (int i = 0; i < NUM_OF_EP_MAX; i++) {
		dev_data.eps[i].ep_occupied = false;
		dev_data.eps[i].ep_enabled = false;
	}

	ep_init.zlt = 0U;
	ep_init.transferType = USB_ENDPOINT_CONTROL;
	ep_init.maxPacketSize = EP0_MAX_PACKET_SIZE;
	ep_init.endpointAddress = EP0_OUT;

	ep_abs_idx =  EP_ABS_IDX(ep_init.endpointAddress);
	dev_data.eps[ep_abs_idx].ep_mps = EP0_MAX_PACKET_SIZE;

	status = dev_data.interface->deviceControl(dev_data.controllerHandle,
			kUSB_DeviceControlEndpointInit, &ep_init);
	if (kStatus_USB_Success != status) {
		LOG_ERR("Failed to initialize control OUT endpoint");
	}

	dev_data.eps[ep_abs_idx].ep_occupied = false;
	dev_data.eps[ep_abs_idx].ep_enabled = true;

	ep_init.endpointAddress = EP0_IN;
	ep_abs_idx = EP_ABS_IDX(ep_init.endpointAddress);
	dev_data.eps[ep_abs_idx].ep_mps = EP0_MAX_PACKET_SIZE;
	status = dev_data.interface->deviceControl(dev_data.controllerHandle,
			kUSB_DeviceControlEndpointInit, &ep_init);
	if (kStatus_USB_Success != status) {
		LOG_ERR("Failed to initialize control IN endpoint");
	}

	dev_data.eps[ep_abs_idx].ep_occupied = false;
	dev_data.eps[ep_abs_idx].ep_enabled = true;
}

static void handle_transfer_msg(usb_device_callback_message_struct_t *cb_msg)
{
	uint8_t ep_status_code = 0;
	uint8_t ep = cb_msg->code;
	uint8_t ep_abs_idx = EP_ABS_IDX(ep);
	usb_status_t status;

	dev_data.eps[ep_abs_idx].ep_occupied = false;

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
			if ((dev_data.address != 0) && (ep_abs_idx == 1)) {
				/*
				 * Set Address in the status stage in
				 * the IN transfer.
				 */
				status = dev_data.interface->deviceControl(
					dev_data.controllerHandle,
					kUSB_DeviceControlSetDeviceAddress,
					&dev_data.address);
				if (kStatus_USB_Success != status) {
					LOG_ERR("Failed to set device address");
					return;
				}
				dev_data.address = 0;
			}
			ep_status_code = USB_DC_EP_DATA_IN;
		}
		/* OUT TOKEN */
		else {
			ep_status_code = USB_DC_EP_DATA_OUT;
		}
	}

	if (dev_data.eps[ep_abs_idx].callback) {
#if defined(CONFIG_HAS_MCUX_CACHE) && !defined(EP_BUF_NONCACHED)
		if (cb_msg->length) {
			DCACHE_InvalidateByRange((uint32_t)cb_msg->buffer,
						 cb_msg->length);
		}
#endif
		dev_data.eps[ep_abs_idx].callback(ep, ep_status_code);
	} else {
		LOG_ERR("No cb pointer for endpoint 0x%02x", ep);
	}
}

/* Notify the up layer the KHCI status changed. */
void USB_DeviceNotificationTrigger(void *handle, void *msg)
{
	uint8_t ep_abs_idx;
	usb_device_callback_message_struct_t *cb_msg =
		(usb_device_callback_message_struct_t *)msg;

	switch (cb_msg->code) {
	case kUSB_DeviceNotifyBusReset:
		handle_bus_reset();
		dev_data.status_callback(USB_DC_RESET, NULL);
		break;
	case kUSB_DeviceNotifyError:
		dev_data.status_callback(USB_DC_ERROR, NULL);
		break;
	case kUSB_DeviceNotifySuspend:
		dev_data.status_callback(USB_DC_SUSPEND, NULL);
		break;
	case kUSB_DeviceNotifyResume:
		dev_data.status_callback(USB_DC_RESUME, NULL);
		break;
	default:
		ep_abs_idx = EP_ABS_IDX(cb_msg->code);

		if (ep_abs_idx >= NUM_OF_EP_MAX) {
			LOG_ERR("Wrong endpoint index/address");
			return;
		}

		memcpy(&dev_data.eps[ep_abs_idx].transfer_message, cb_msg,
		       sizeof(usb_device_callback_message_struct_t));
		handle_transfer_msg(&dev_data.eps[ep_abs_idx].transfer_message);
	}
}

static void usb_isr_handler(void)
{
	USB_DeviceEhciIsrFunction(&dev_data);
}
