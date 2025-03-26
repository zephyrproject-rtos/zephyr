/*
 * Copyright (c) 2025 Quectel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <soc.h>
#include <string.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>

#include "quec_uhc_driver.h"

LOG_MODULE_REGISTER(quec_uhc_enum, LOG_LEVEL_ERR);

/*===========================================================================
 * 							  variables
 ===========================================================================*/
K_MSGQ_DEFINE(uhc_enum_msgq, sizeof(quec_trans_status_t), 1, 4);


/*===========================================================================
 * 							  functions
 ===========================================================================*/
static void quec_enum_callback(void *ctx)
{
	quec_uhc_xfer_t *xfer = ctx;
	quec_trans_status_t t_event;

	if(xfer == NULL || xfer->status != 0)
	{
		quec_print("transfer err %p", xfer);
		return;		
	}

	t_event.cdc_num = xfer->port_num;
	t_event.status = xfer->status;
	t_event.size = xfer->actual;

	k_msgq_put(&uhc_enum_msgq, &t_event, K_NO_WAIT);
}

static int quec_enum_transfer(quec_uhc_mgr_t *udev, quec_uhc_xfer_t *xfer, uint8_t *buffer, uint32_t size)
{
	int ret = 0, total_size = 0;
	quec_trans_status_t t_event;

	k_msgq_purge(&uhc_enum_msgq);
	udev->api->ep_enable(udev->device, xfer->ep_desc);
	
	do
	{
		xfer->buffer = buffer + total_size;
		xfer->nbytes = UHC_MIN(xfer->ep_desc->wMaxPacketSize, size - total_size);

		if(udev->api->enqueue(udev->device, xfer) != 0)
		{
			quec_print("transfer failed");
			udev->api->ep_disable(udev->device, 0);
			return -1;
		}

		ret = k_msgq_get(&uhc_enum_msgq, &t_event, K_MSEC(xfer->timeouts));
		if(ret != 0 || t_event.status != 0)
		{
			quec_print("transfer error %d %d %d %d", ret, t_event.cdc_num, t_event.size, t_event.status);
			udev->api->ep_disable(udev->device, 0);
			return -1;
		}
		
		total_size += xfer->nbytes;
	}while(total_size < size);

	return total_size;
}

int quec_uhc_set_line_state(quec_uhc_mgr_t *udev, uint16_t intf, uint16_t value)
{
    quec_uhc_req_t setup;
	quec_uhc_dev_t *dev = &udev->dev[QUEC_SYSTEM_PORT];
	quec_uhc_xfer_t *tx_xfer = &dev->tx_port.xfer;
	quec_uhc_xfer_t *rx_xfer = &dev->rx_port.xfer;
	
    setup.request_type = USB_REQ_TYPE_DIR_OUT | USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE;
    setup.bRequest = USB_REQ_SET_LINE_STATE;
    setup.wIndex = intf;
    setup.wLength = 0;
    setup.wValue = value;

	memset(tx_xfer, 0, sizeof(quec_uhc_xfer_t));
	tx_xfer->ep_desc = &dev->tx_port.ep_desc;
	tx_xfer->token = USBH_PID_SETUP;
	tx_xfer->timeouts = 1000;
	tx_xfer->callback = quec_enum_callback;

	if(quec_enum_transfer(udev, tx_xfer, (uint8_t *)&setup, sizeof(setup)) != sizeof(setup))
	{
		quec_print("setup failed");
		return -1;
	}

	memset(rx_xfer, 0, sizeof(quec_uhc_xfer_t));
	rx_xfer->ep_desc = &dev->rx_port.ep_desc;
	rx_xfer->token = USBH_PID_DATA;
	rx_xfer->timeouts = 1000;
	rx_xfer->callback = quec_enum_callback;

	if(quec_enum_transfer(udev, rx_xfer, (uint8_t *)&setup, 0) < 0)
	{
		quec_print("port enable failed");
		return -1;
	} 

	return 0;
}

int quec_uhc_get_desc(quec_uhc_mgr_t *udev, uint8_t type, uint8_t *buffer, int nbytes)
{
    quec_uhc_req_t setup;
	quec_uhc_dev_t *dev = &udev->dev[QUEC_SYSTEM_PORT];
	quec_uhc_xfer_t *tx_xfer = &dev->tx_port.xfer;
	quec_uhc_xfer_t *rx_xfer = &dev->rx_port.xfer;
	
    setup.request_type = USB_REQ_TYPE_DIR_IN | USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_DEVICE;
    setup.bRequest = USB_REQ_GET_DESCRIPTOR;
    setup.wIndex = 0;
    setup.wLength = nbytes;
    setup.wValue = type << 8;

	memset(tx_xfer, 0, sizeof(quec_uhc_xfer_t));
	tx_xfer->ep_desc = &dev->tx_port.ep_desc;
	tx_xfer->token = USBH_PID_SETUP;
	tx_xfer->timeouts = 1000;
	tx_xfer->callback = quec_enum_callback;

	if(quec_enum_transfer(udev, tx_xfer, (uint8_t *)&setup, sizeof(setup)) != sizeof(setup))
	{
		quec_print("setup failed");
		return -1;
	}

	memset(rx_xfer, 0, sizeof(quec_uhc_xfer_t));
	rx_xfer->ep_desc = &dev->rx_port.ep_desc;
	rx_xfer->token = USBH_PID_DATA;
	rx_xfer->timeouts = 1000;
	rx_xfer->callback = quec_enum_callback;

	if(quec_enum_transfer(udev, rx_xfer, buffer, nbytes) != nbytes)
	{
		quec_print("get data failed");
		return -1;
	}

	memset(tx_xfer, 0, sizeof(quec_uhc_xfer_t));
	tx_xfer->ep_desc = &dev->tx_port.ep_desc;
	tx_xfer->token = USBH_PID_DATA;
	tx_xfer->timeouts = 1000;
	tx_xfer->callback = quec_enum_callback;

	if(quec_enum_transfer(udev, tx_xfer, (uint8_t *)&setup, 0) < 0)
	{
		quec_print("get desc failed");
		return -1;
	}
	
    return 0;
}

int quec_uhc_set_interface(quec_uhc_mgr_t *udev, uint16_t intf)
{
    quec_uhc_req_t setup;
	quec_uhc_dev_t *dev = &udev->dev[QUEC_SYSTEM_PORT];
	quec_uhc_xfer_t *tx_xfer = &dev->tx_port.xfer;
	quec_uhc_xfer_t *rx_xfer = &dev->rx_port.xfer;

    setup.request_type = USB_REQ_TYPE_DIR_OUT | USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_INTERFACE;
    setup.bRequest = USB_REQ_SET_INTERFACE;
    setup.wIndex = 0;
    setup.wLength = 0;
    setup.wValue = intf;

	memset(tx_xfer, 0, sizeof(quec_uhc_xfer_t));
	tx_xfer->ep_desc = &dev->tx_port.ep_desc;
	tx_xfer->token = USBH_PID_SETUP;
	tx_xfer->timeouts = 1000;
	tx_xfer->callback = quec_enum_callback;
	
	if(quec_enum_transfer(udev, tx_xfer, (uint8_t *)&setup, sizeof(setup)) != sizeof(setup))
	{
		quec_print("setup failed");
		return -1;
	}

	memset(rx_xfer, 0, sizeof(quec_uhc_xfer_t));
	rx_xfer->ep_desc = &dev->rx_port.ep_desc;
	rx_xfer->token = USBH_PID_DATA;
	rx_xfer->timeouts = 1000;
	rx_xfer->callback = quec_enum_callback;
	
	if(quec_enum_transfer(udev, rx_xfer, (uint8_t *)&setup, 0) < 0)
	{
		quec_print("set intf failed");
		return -1;
	}

    return 0;
}

int quec_uhc_set_address(quec_uhc_mgr_t *udev, uint8_t address)
{
    quec_uhc_req_t setup;
	quec_uhc_dev_t *dev = &udev->dev[QUEC_SYSTEM_PORT];
	quec_uhc_xfer_t *tx_xfer = &dev->tx_port.xfer;
	quec_uhc_xfer_t *rx_xfer = &dev->rx_port.xfer;

    setup.request_type = USB_REQ_TYPE_DIR_OUT | USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_DEVICE;
    setup.bRequest = USB_REQ_SET_ADDRESS;
    setup.wIndex = 0;
    setup.wLength = 0;
    setup.wValue = address;

	memset(tx_xfer, 0, sizeof(quec_uhc_xfer_t));
	tx_xfer->ep_desc = &dev->tx_port.ep_desc;
	tx_xfer->token = USBH_PID_SETUP;
	tx_xfer->timeouts = 1000;
	tx_xfer->callback = quec_enum_callback;

	if(quec_enum_transfer(udev, tx_xfer, (uint8_t *)&setup, sizeof(setup)) != sizeof(setup))
	{
		quec_print("setup failed");
		return -1;
	}

	udev->api->set_address(udev->device, address);

	memset(rx_xfer, 0, sizeof(quec_uhc_xfer_t));
	rx_xfer->ep_desc = &dev->rx_port.ep_desc;
	rx_xfer->token = USBH_PID_DATA;
	rx_xfer->timeouts = 1000;
	rx_xfer->callback = quec_enum_callback;
	
	if(quec_enum_transfer(udev, rx_xfer, (uint8_t *)&setup, 0) < 0)
	{
		quec_print("set addr failed");
		return -1;		
	}

	udev->dev_address = address;

    return 0;
}

int quec_uhc_set_configure(quec_uhc_mgr_t *udev, uint16_t value)
{
    quec_uhc_req_t setup;
	quec_uhc_dev_t *dev = &udev->dev[QUEC_SYSTEM_PORT];
	quec_uhc_xfer_t *tx_xfer = &dev->tx_port.xfer;
	quec_uhc_xfer_t *rx_xfer = &dev->rx_port.xfer;

    setup.request_type = USB_REQ_TYPE_DIR_OUT | USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_DEVICE;
    setup.bRequest = USB_REQ_SET_CONFIGURATION;
    setup.wIndex = 0;
    setup.wLength = 0;
    setup.wValue = value;

	memset(tx_xfer, 0, sizeof(quec_uhc_xfer_t));
	tx_xfer->ep_desc = &dev->tx_port.ep_desc;
	tx_xfer->token = USBH_PID_SETUP;
	tx_xfer->timeouts = 1000;
	tx_xfer->callback = quec_enum_callback;

	if(quec_enum_transfer(udev, tx_xfer, (uint8_t *)&setup, sizeof(setup)) != sizeof(setup))
	{
		quec_print("setup failed");
		return -1;
	}

	memset(rx_xfer, 0, sizeof(quec_uhc_xfer_t));
	rx_xfer->ep_desc = &dev->rx_port.ep_desc;
	rx_xfer->token = USBH_PID_DATA;
	rx_xfer->timeouts = 1000;
	rx_xfer->callback = quec_enum_callback;

	if(quec_enum_transfer(udev, rx_xfer, (uint8_t *)&setup, 0) < 0)
	{
		quec_print("set cfg failed");
		return -1;		
	}
	
    return 0;
}

int quec_uhc_parse_config_desc(uhc_cfg_descriptor_t *cfg_desc, int intf_num, usb_intf_ep_desc_t *desc)
{
	int ptr, ep_cnt = 0;
	usb_desc_head_t *header;
	usb_intf_desc_t *intf_desc;
	usb_endp_desc_t *ep_desc;

	ptr = (uint32_t)cfg_desc + cfg_desc->bLength;
	while(ptr < (uint32_t)cfg_desc + cfg_desc->wTotalLength)
	{
		header = (struct usb_desc_header *)ptr;
		if(header->bDescriptorType == USB_DESC_TYPE_INTERFACE)
		{
			intf_desc = (usb_intf_desc_t *)ptr;
			if(intf_desc->bInterfaceNumber == intf_num)
			{
				memcpy((void *)&desc->intf_desc, (void *)intf_desc, intf_desc->bLength);
				ptr = ptr + intf_desc->bLength;				
				while(ptr < (uint32_t)cfg_desc + cfg_desc->wTotalLength && ep_cnt < intf_desc->bNumEndpoints)
				{						
					header = (struct usb_desc_header *)ptr;					
					if(header->bDescriptorType == USB_DESC_TYPE_ENDPOINT)
					{
						ep_desc = (usb_endp_desc_t *)ptr;
						if(ep_desc->bmAttributes == USB_EP_ATTR_INT)
						{
							memcpy((void *)&desc->ctrl_ep_desc, (void *)ep_desc, ep_desc->bLength);
							ep_cnt++;
						}
						else if(ep_desc->bmAttributes == USB_EP_ATTR_BULK)
						{
							if(ep_desc->bEndpointAddress & 0x80)
							{
								memcpy((void *)&desc->in_ep_desc, (void *)ep_desc, ep_desc->bLength);
							}
							else
							{
								memcpy((void *)&desc->out_ep_desc, (void *)ep_desc, ep_desc->bLength);
							}
							ep_cnt++;
						}
						else
						{
							quec_print("invalid ep addr 0x%x attr %d", ep_desc->bEndpointAddress, ep_desc->bmAttributes);
							return -1;
						}
					}
					else if(header->bDescriptorType == USB_DESC_TYPE_INTERFACE)
					{
						quec_print("invalid interface %d", intf_desc->bInterfaceNumber);
						return -1;
					}

					if(ep_cnt == intf_desc->bNumEndpoints)
					{
						return 0;
					}
					ptr += header->bLength;
				}
			}
		}
		ptr += header->bLength;
	}

	return -1;
}

int quec_uhc_port_desc_init(quec_uhc_mgr_t *udev, usb_endp_desc_t *in, usb_endp_desc_t *out)
{
	quec_uhc_dev_t *dev = &udev->dev[QUEC_SYSTEM_PORT];

	memcpy(&dev->rx_port.ep_desc, in, sizeof(struct usb_ep_descriptor));
	memcpy(&dev->tx_port.ep_desc, out, sizeof(struct usb_ep_descriptor));

	dev->rx_port.port_num = 0;
	dev->tx_port.port_num = 0;

	return 0;	
}

int quec_uhc_reset(quec_uhc_mgr_t *udev)
{
	udev->dev_address = 0;
	udev->api->set_address(udev->device, 0);
	return udev->api->reset(udev->device);
}

int quec_uhc_enum_process(quec_uhc_mgr_t *udev, usb_device_desc_t *dev_desc, uhc_cfg_descriptor_t *cfg_desc)
{
	int ret = 0;
	struct usb_ep_descriptor ctl_in_ep;
	struct usb_ep_descriptor ctl_out_ep;

	quec_print("start enumeration...");
	if(quec_uhc_reset(udev) < 0)
	{
		quec_print("reset device fail");
		return -1;
	}

	ctl_in_ep.bEndpointAddress = 0x00 | USB_DIR_IN;
	ctl_in_ep.bmAttributes = USB_EP_ATTR_CONTROL;
	ctl_in_ep.wMaxPacketSize = DEVICE_DESC_PRE_SIZE;
	ctl_out_ep.bEndpointAddress = 0x00 | USB_DIR_OUT;
	ctl_out_ep.bmAttributes = USB_EP_ATTR_CONTROL;
	ctl_out_ep.wMaxPacketSize = DEVICE_DESC_PRE_SIZE;
	
	quec_uhc_port_desc_init(udev, &ctl_in_ep, &ctl_out_ep);
	
	ret = quec_uhc_get_desc(udev, USB_DESC_TYPE_DEVICE, (void *)dev_desc, DEVICE_DESC_PRE_SIZE);
	if(ret != 0)
	{
		quec_print("get device desc failed");
		return -1;
	}

	quec_print("device desc size %d", dev_desc->bLength);

	if(quec_uhc_reset(udev) < 0)
	{
		quec_print("reset device fail");
		return -1;	
	}

	ret = quec_uhc_set_address(udev, 0xE);
	if(ret != 0)
	{
		quec_print("get device desc failed");
		return -1;
	}
	
	ctl_in_ep.wMaxPacketSize = dev_desc->bMaxPacketSize0;
	ctl_out_ep.wMaxPacketSize = dev_desc->bMaxPacketSize0;
	
	ret = quec_uhc_port_desc_init(udev, &ctl_in_ep, &ctl_out_ep);
	if(ret != 0)
	{
		quec_print("control re-init failed");
		return -1;
	}

	ret = quec_uhc_get_desc(udev, USB_DESC_TYPE_DEVICE, (uint8_t *)dev_desc, dev_desc->bLength);
	if(ret != 0)
	{
		quec_print("get device desc failed");
		return -1;
	}
	
	ret = quec_uhc_get_desc(udev, USB_DESC_TYPE_CONFIGURATION, (uint8_t *)cfg_desc, 9);
	if(ret != 0 || cfg_desc->wTotalLength > CFG_DESC_MAX_SIZE)
	{
		quec_print("get device desc failed");
		return -1;
	}
	
	ret = quec_uhc_get_desc(udev, USB_DESC_TYPE_CONFIGURATION, (uint8_t *)cfg_desc, cfg_desc->wTotalLength);
	if(ret != 0 || cfg_desc->wTotalLength > CFG_DESC_MAX_SIZE)
	{
		quec_print("get config desc failed");
		return -1;
	}

	ret = quec_uhc_set_configure(udev, 1);
	if(ret < 0)
	{
		quec_print("set config err");
		return -1;
	}

	quec_print("enum done vid 0x%x pid 0x%x intf num %d", dev_desc->idVendor, dev_desc->idProduct, cfg_desc->bNumInterfaces);
	
	return 0;
}

