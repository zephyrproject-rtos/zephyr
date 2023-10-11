/*
 *  LPCUSB, an USB device driver for LPC microcontrollers
 *  Copyright (C) 2006 Bertrik Sikken (bertrik@sikken.nl)
 *  Copyright (c) 2016 Intel Corporation
 *  Copyright (c) 2020 PHYTEC Messtechnik GmbH
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 *  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 *  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 * @brief USB device core layer
 *
 * This module handles control transfer handler, standard request handler and
 * USB Interface for customer application.
 *
 * Control transfers handler is normally installed on the
 * endpoint 0 callback.
 *
 * Control transfers can be of the following type:
 * 0 Standard;
 * 1 Class;
 * 2 Vendor;
 * 3 Reserved.
 *
 * A callback can be installed for each of these control transfers using
 * usb_register_request_handler.
 * When an OUT request arrives, data is collected in the data store provided
 * with the usb_register_request_handler call. When the transfer is done, the
 * callback is called.
 * When an IN request arrives, the callback is called immediately to either
 * put the control transfer data in the data store, or to get a pointer to
 * control transfer data. The data is then packetized and sent to the host.
 *
 * Standard request handler handles the 'chapter 9' processing, specifically
 * the standard device requests in table 9-3 from the universal serial bus
 * specification revision 2.0
 */

#include <errno.h>
#include <stddef.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/usb/usb_device.h>
#include <usb_descriptor.h>
#include <zephyr/usb/class/usb_audio.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usb_device, CONFIG_USB_DEVICE_LOG_LEVEL);

#include <zephyr/usb/bos.h>
#include <os_desc.h>
#include "usb_transfer.h"

#define MAX_DESC_HANDLERS           4 /** Device, interface, endpoint, other */

/* general descriptor field offsets */
#define DESC_bLength                0 /** Length offset */
#define DESC_bDescriptorType        1 /** Descriptor type offset */

/* config descriptor field offsets */
#define CONF_DESC_wTotalLength      2 /** Total length offset */
#define CONF_DESC_bConfigurationValue 5 /** Configuration value offset */
#define CONF_DESC_bmAttributes      7 /** configuration characteristics */

/* interface descriptor field offsets */
#define INTF_DESC_bInterfaceNumber  2 /** Interface number offset */
#define INTF_DESC_bAlternateSetting 3 /** Alternate setting offset */

/* endpoint descriptor field offsets */
#define ENDP_DESC_bEndpointAddress  2U /** Endpoint address offset */
#define ENDP_DESC_bmAttributes      3U /** Bulk or interrupt? */
#define ENDP_DESC_wMaxPacketSize    4U /** Maximum packet size offset */

#define MAX_NUM_REQ_HANDLERS        4U
#define MAX_STD_REQ_MSG_SIZE        8U

K_MUTEX_DEFINE(usb_enable_lock);

static struct usb_dev_priv {
	/** Setup packet */
	struct usb_setup_packet setup;
	/** Pointer to data buffer */
	uint8_t *data_buf;
	/** Remaining bytes in buffer */
	int32_t data_buf_residue;
	/** Total length of control transfer */
	int32_t data_buf_len;
	/** Zero length packet flag of control transfer */
	bool zlp_flag;
	/** Installed custom request handler */
	usb_request_handler custom_req_handler;
	/** USB stack status callback */
	usb_dc_status_callback status_callback;
	/** USB user status callback */
	usb_dc_status_callback user_status_callback;
	/** Pointer to registered descriptors */
	const uint8_t *descriptors;
	/** Array of installed request handler callbacks */
	usb_request_handler req_handlers[MAX_NUM_REQ_HANDLERS];
	/* Buffer used for storing standard, class and vendor request data */
	uint8_t req_data[CONFIG_USB_REQUEST_BUFFER_SIZE];

	/** Variable to check whether the usb has been enabled */
	bool enabled;
	/** Variable to check whether the usb has been configured */
	bool configured;
	/** Currently selected configuration */
	uint8_t configuration;
	/** Currently selected alternate setting */
	uint8_t alt_setting[CONFIG_USB_MAX_ALT_SETTING];
	/** Remote wakeup feature status */
	bool remote_wakeup;
	/** Tracks whether set_endpoint() had been called on an EP */
	uint32_t ep_bm;
	/** Maximum Packet Size (MPS) of control endpoint */
	uint8_t mps0;
} usb_dev;

/* Setup packet definition used to read raw data from USB line */
struct usb_setup_packet_packed {
	uint8_t bmRequestType;
	uint8_t bRequest;
	uint16_t wValue;
	uint16_t wIndex;
	uint16_t wLength;
} __packed;

static bool reset_endpoint(const struct usb_ep_descriptor *ep_desc);

/*
 * @brief print the contents of a setup packet
 *
 * @param [in] setup The setup packet
 *
 */
static void usb_print_setup(struct usb_setup_packet *setup)
{
	/* avoid compiler warning if LOG_DBG is not defined */
	ARG_UNUSED(setup);

	LOG_DBG("Setup: "
		"bmRT 0x%02x, bR 0x%02x, wV 0x%04x, wI 0x%04x, wL 0x%04x",
		setup->bmRequestType,
		setup->bRequest,
		setup->wValue,
		setup->wIndex,
		setup->wLength);
}

static void usb_reset_alt_setting(void)
{
	memset(usb_dev.alt_setting, 0, ARRAY_SIZE(usb_dev.alt_setting));
}

static bool usb_set_alt_setting(uint8_t iface, uint8_t alt_setting)
{
	if (iface < ARRAY_SIZE(usb_dev.alt_setting)) {
		usb_dev.alt_setting[iface] = alt_setting;
		return true;
	}

	return false;
}

static uint8_t usb_get_alt_setting(uint8_t iface)
{
	if (iface < ARRAY_SIZE(usb_dev.alt_setting)) {
		return usb_dev.alt_setting[iface];
	}

	return 0;
}

/*
 * @brief handle a request by calling one of the installed request handlers
 *
 * Local function to handle a request by calling one of the installed request
 * handlers. In case of data going from host to device, the data is at *ppbData.
 * In case of data going from device to host, the handler can either choose to
 * write its data at *ppbData or update the data pointer.
 *
 * @param [in]     setup The setup packet
 * @param [in,out] len   Pointer to data length
 * @param [in,out] data  Data buffer
 *
 * @return true if the request was handles successfully
 */
static bool usb_handle_request(struct usb_setup_packet *setup,
			       int32_t *len, uint8_t **data)
{
	uint32_t type = setup->RequestType.type;
	usb_request_handler handler;

	if (type >= MAX_NUM_REQ_HANDLERS) {
		LOG_DBG("Error Incorrect iType %d", type);
		return false;
	}

	handler = usb_dev.req_handlers[type];
	if (handler == NULL) {
		LOG_DBG("No handler for reqtype %d", type);
		return false;
	}

	if ((*handler)(setup, len, data) < 0) {
		LOG_DBG("Handler Error %d", type);
		usb_print_setup(setup);
		return false;
	}

	return true;
}

/*
 * @brief send next chunk of data (possibly 0 bytes) to host
 */
static void usb_data_to_host(void)
{
	if (usb_dev.zlp_flag == false) {
		uint32_t chunk = usb_dev.data_buf_residue;

		/*Always EP0 for control*/
		usb_write(USB_CONTROL_EP_IN, usb_dev.data_buf,
			  usb_dev.data_buf_residue, &chunk);
		usb_dev.data_buf += chunk;
		usb_dev.data_buf_residue -= chunk;

		/*
		 * Set ZLP flag when host asks for a bigger length and the
		 * last chunk is wMaxPacketSize long, to indicate the last
		 * packet.
		 */
		if (!usb_dev.data_buf_residue && chunk &&
		    usb_dev.setup.wLength > usb_dev.data_buf_len) {
			/* Send less data as requested during the Setup stage */
			if (!(usb_dev.data_buf_len % usb_dev.mps0)) {
				/* Transfers a zero-length packet */
				LOG_DBG("ZLP, requested %u , length %u ",
					usb_dev.setup.wLength,
					usb_dev.data_buf_len);
				usb_dev.zlp_flag = true;
			}
		}

	} else {
		usb_dev.zlp_flag = false;
		usb_dc_ep_write(USB_CONTROL_EP_IN, NULL, 0, NULL);
	}
}

/*
 * @brief handle IN/OUT transfers on EP0
 *
 * @param [in] ep        Endpoint address
 * @param [in] ep_status Endpoint status
 */
static void usb_handle_control_transfer(uint8_t ep,
					enum usb_dc_ep_cb_status_code ep_status)
{
	uint32_t chunk = 0U;
	struct usb_setup_packet *setup = &usb_dev.setup;
	struct usb_setup_packet_packed setup_raw;

	LOG_DBG("ep 0x%02x, status 0x%02x", ep, ep_status);

	if (ep == USB_CONTROL_EP_OUT && ep_status == USB_DC_EP_SETUP) {
		/*
		 * OUT transfer, Setup packet,
		 * reset request message state machine
		 */
		if (usb_dc_ep_read(ep, (uint8_t *)&setup_raw,
				   sizeof(setup_raw), NULL) < 0) {
			LOG_DBG("Read Setup Packet failed");
			usb_dc_ep_set_stall(USB_CONTROL_EP_IN);
			return;
		}

		/* Take care of endianness */
		setup->bmRequestType = setup_raw.bmRequestType;
		setup->bRequest = setup_raw.bRequest;
		setup->wValue = sys_le16_to_cpu(setup_raw.wValue);
		setup->wIndex = sys_le16_to_cpu(setup_raw.wIndex);
		setup->wLength = sys_le16_to_cpu(setup_raw.wLength);

		usb_dev.data_buf = usb_dev.req_data;
		usb_dev.zlp_flag = false;
		/*
		 * Set length to 0 as a precaution so that no trouble
		 * happens if control request handler does not check the
		 * request values sufficiently.
		 */
		usb_dev.data_buf_len = 0;
		usb_dev.data_buf_residue = 0;

		if (usb_reqtype_is_to_device(setup)) {
			if (setup->wLength > CONFIG_USB_REQUEST_BUFFER_SIZE) {
				LOG_ERR("Request buffer too small");
				usb_dc_ep_set_stall(USB_CONTROL_EP_IN);
				usb_dc_ep_set_stall(USB_CONTROL_EP_OUT);
				return;
			}

			if (setup->wLength) {
				/* Continue with data OUT stage */
				usb_dev.data_buf_len = setup->wLength;
				usb_dev.data_buf_residue = setup->wLength;
				return;
			}
		}

		/* Ask installed handler to process request */
		if (!usb_handle_request(setup,
					&usb_dev.data_buf_len,
					&usb_dev.data_buf)) {
			LOG_DBG("usb_handle_request failed");
			usb_dc_ep_set_stall(USB_CONTROL_EP_IN);
			return;
		}

		/* Send smallest of requested and offered length */
		usb_dev.data_buf_residue = MIN(usb_dev.data_buf_len,
					       setup->wLength);
		/* Send first part (possibly a zero-length status message) */
		usb_data_to_host();
	} else if (ep == USB_CONTROL_EP_OUT) {
		/* OUT transfer, data or status packets */
		if (usb_dev.data_buf_residue <= 0) {
			/* absorb zero-length status message */
			if (usb_dc_ep_read(USB_CONTROL_EP_OUT,
					   usb_dev.data_buf, 0, &chunk) < 0) {
				LOG_DBG("Read DATA Packet failed");
				usb_dc_ep_set_stall(USB_CONTROL_EP_IN);
			}
			return;
		}

		if (usb_dc_ep_read(USB_CONTROL_EP_OUT,
				   usb_dev.data_buf,
				   usb_dev.data_buf_residue, &chunk) < 0) {
			LOG_DBG("Read DATA Packet failed");
			usb_dc_ep_set_stall(USB_CONTROL_EP_IN);
			usb_dc_ep_set_stall(USB_CONTROL_EP_OUT);
			return;
		}

		usb_dev.data_buf += chunk;
		usb_dev.data_buf_residue -= chunk;
		if (usb_dev.data_buf_residue == 0) {
			/* Received all, send data to handler */
			usb_dev.data_buf = usb_dev.req_data;
			if (!usb_handle_request(setup,
						&usb_dev.data_buf_len,
						&usb_dev.data_buf)) {
				LOG_DBG("usb_handle_request1 failed");
				usb_dc_ep_set_stall(USB_CONTROL_EP_IN);
				return;
			}

			/*Send status to host*/
			LOG_DBG(">> usb_data_to_host(2)");
			usb_data_to_host();
		}
	} else if (ep == USB_CONTROL_EP_IN) {
		/* Send more data if available */
		if (usb_dev.data_buf_residue != 0 || usb_dev.zlp_flag == true) {
			usb_data_to_host();
		}
	} else {
		__ASSERT_NO_MSG(false);
	}
}

/*
 * @brief register a callback for handling requests
 *
 * @param [in] type       Type of request, e.g. USB_REQTYPE_TYPE_STANDARD
 * @param [in] handler    Callback function pointer
 */
static void usb_register_request_handler(int32_t type,
					 usb_request_handler handler)
{
	usb_dev.req_handlers[type] = handler;
}

/*
 * @brief register a pointer to a descriptor block
 *
 * This function registers a pointer to a descriptor block containing all
 * descriptors for the device.
 *
 * @param [in] usb_descriptors The descriptor byte array
 */
static void usb_register_descriptors(const uint8_t *usb_descriptors)
{
	usb_dev.descriptors = usb_descriptors;
}

static bool usb_get_status(struct usb_setup_packet *setup,
			   int32_t *len, uint8_t **data_buf)
{
	uint8_t *data = *data_buf;

	LOG_DBG("Get Status request");
	data[0] = 0U;
	data[1] = 0U;

	if (IS_ENABLED(CONFIG_USB_SELF_POWERED)) {
		data[0] |= USB_GET_STATUS_SELF_POWERED;
	}

	if (IS_ENABLED(CONFIG_USB_DEVICE_REMOTE_WAKEUP)) {
		data[0] |= (usb_dev.remote_wakeup ?
			    USB_GET_STATUS_REMOTE_WAKEUP : 0);
	}

	*len = 2;

	return true;
}

/*
 * @brief get specified USB descriptor
 *
 * This function parses the list of installed USB descriptors and attempts
 * to find the specified USB descriptor.
 *
 * @param [in]  setup      The setup packet
 * @param [out] len        Descriptor length
 * @param [out] data       Descriptor data
 *
 * @return true if the descriptor was found, false otherwise
 */
static bool usb_get_descriptor(struct usb_setup_packet *setup,
			       int32_t *len, uint8_t **data)
{
	uint8_t type = 0U;
	uint8_t index = 0U;
	uint8_t *p = NULL;
	uint32_t cur_index = 0U;
	bool found = false;

	LOG_DBG("Get Descriptor request");
	type = USB_GET_DESCRIPTOR_TYPE(setup->wValue);
	index = USB_GET_DESCRIPTOR_INDEX(setup->wValue);

	/*
	 * Invalid types of descriptors,
	 * see USB Spec. Revision 2.0, 9.4.3 Get Descriptor
	 */
	if ((type == USB_DESC_INTERFACE) || (type == USB_DESC_ENDPOINT) ||
	    (type > USB_DESC_OTHER_SPEED)) {
		return false;
	}

	p = (uint8_t *)usb_dev.descriptors;
	cur_index = 0U;

	while (p[DESC_bLength] != 0U) {
		if (p[DESC_bDescriptorType] == type) {
			if (cur_index == index) {
				found = true;
				break;
			}
			cur_index++;
		}
		/* skip to next descriptor */
		p += p[DESC_bLength];
	}

	if (found) {
		/* set data pointer */
		*data = p;
		/* get length from structure */
		if (type == USB_DESC_CONFIGURATION) {
			/* configuration descriptor is an
			 * exception, length is at offset
			 * 2 and 3
			 */
			*len = (p[CONF_DESC_wTotalLength]) |
			    (p[CONF_DESC_wTotalLength + 1] << 8);
		} else {
			/* normally length is at offset 0 */
			*len = p[DESC_bLength];
		}
	} else {
		/* nothing found */
		LOG_DBG("Desc %x not found!", setup->wValue);
	}
	return found;
}

/*
 * @brief Get 32-bit endpoint bitmask from index
 *
 * In the returned 32-bit word, the bit positions in the lower 16 bits
 * indicate OUT endpoints, while the upper 16 bits indicate IN
 * endpoints
 *
 * @param [in]  ep Endpoint of interest
 *
 * @return 32-bit bitmask
 */
static uint32_t get_ep_bm_from_addr(uint8_t ep)
{
	uint32_t ep_bm = 0;
	uint8_t ep_idx;

	ep_idx = ep & (~USB_EP_DIR_IN);
	if (ep_idx > 15) {
		LOG_ERR("Endpoint 0x%02x is invalid", ep);
		goto done;
	}

	if (ep & USB_EP_DIR_IN) {
		ep_bm = BIT(ep_idx + 16);
	} else {
		ep_bm = BIT(ep_idx);
	}
done:
	return ep_bm;
}

/*
 * @brief configure and enable endpoint
 *
 * This function sets endpoint configuration according to one specified in USB
 * endpoint descriptor and then enables it for data transfers.
 *
 * @param [in]  ep_desc Endpoint descriptor byte array
 *
 * @return true if successfully configured and enabled
 */
static bool set_endpoint(const struct usb_ep_descriptor *ep_desc)
{
	struct usb_dc_ep_cfg_data ep_cfg;
	uint32_t ep_bm;
	int ret;

	ep_cfg.ep_addr = ep_desc->bEndpointAddress;
	ep_cfg.ep_mps = sys_le16_to_cpu(ep_desc->wMaxPacketSize);
	ep_cfg.ep_type = ep_desc->bmAttributes & USB_EP_TRANSFER_TYPE_MASK;

	LOG_DBG("Set endpoint 0x%x type %u MPS %u",
		ep_cfg.ep_addr, ep_cfg.ep_type, ep_cfg.ep_mps);

	/* if endpoint is has been set() previously, reset() it first */
	ep_bm = get_ep_bm_from_addr(ep_desc->bEndpointAddress);
	if (ep_bm & usb_dev.ep_bm) {
		reset_endpoint(ep_desc);
		/* allow any canceled transfers to terminate */
		if (!k_is_in_isr()) {
			k_usleep(150);
		}
	}

	ret = usb_dc_ep_configure(&ep_cfg);
	if (ret == -EALREADY) {
		LOG_WRN("Endpoint 0x%02x already configured", ep_cfg.ep_addr);
	} else if (ret) {
		LOG_ERR("Failed to configure endpoint 0x%02x", ep_cfg.ep_addr);
		return false;
	} else {
		;
	}

	ret = usb_dc_ep_enable(ep_cfg.ep_addr);
	if (ret == -EALREADY) {
		LOG_WRN("Endpoint 0x%02x already enabled", ep_cfg.ep_addr);
	} else if (ret) {
		LOG_ERR("Failed to enable endpoint 0x%02x", ep_cfg.ep_addr);
		return false;
	} else {
		;
	}

	usb_dev.configured = true;
	usb_dev.ep_bm |= ep_bm;

	return true;
}

static int disable_endpoint(uint8_t ep_addr)
{
	uint32_t ep_bm;
	int ret;

	ret = usb_dc_ep_disable(ep_addr);
	if (ret == -EALREADY) {
		LOG_WRN("Endpoint 0x%02x already disabled", ep_addr);
	} else if (ret) {
		LOG_ERR("Failed to disable endpoint 0x%02x", ep_addr);
		return ret;
	}

	/* clear endpoint mask */
	ep_bm = get_ep_bm_from_addr(ep_addr);
	usb_dev.ep_bm &= ~ep_bm;

	return 0;
}

/*
 * @brief Disable endpoint for transferring data
 *
 * This function cancels transfers that are associated with endpoint and
 * disabled endpoint itself.
 *
 * @param [in]  ep_desc Endpoint descriptor byte array
 *
 * @return true if successfully deconfigured and disabled
 */
static bool reset_endpoint(const struct usb_ep_descriptor *ep_desc)
{
	struct usb_dc_ep_cfg_data ep_cfg;

	ep_cfg.ep_addr = ep_desc->bEndpointAddress;
	ep_cfg.ep_type = ep_desc->bmAttributes & USB_EP_TRANSFER_TYPE_MASK;

	LOG_DBG("Reset endpoint 0x%02x type %u",
		ep_cfg.ep_addr, ep_cfg.ep_type);

	usb_cancel_transfer(ep_cfg.ep_addr);

	return disable_endpoint(ep_cfg.ep_addr) ? false : true;
}

static bool usb_eps_reconfigure(struct usb_ep_descriptor *ep_desc,
				uint8_t cur_alt_setting,
				uint8_t alt_setting)
{
	bool ret;

	if (cur_alt_setting != alt_setting) {
		LOG_DBG("Disable endpoint 0x%02x", ep_desc->bEndpointAddress);
		ret = reset_endpoint(ep_desc);
	} else {
		LOG_DBG("Enable endpoint 0x%02x", ep_desc->bEndpointAddress);
		ret = set_endpoint(ep_desc);
	}

	return ret;
}

/*
 * @brief set USB configuration
 *
 * This function configures the device according to the specified configuration
 * index and alternate setting by parsing the installed USB descriptor list.
 * A configuration index of 0 unconfigures the device.
 *
 * @param [in] setup        The setup packet
 *
 * @return true if successfully configured false if error or unconfigured
 */
static bool usb_set_configuration(struct usb_setup_packet *setup)
{
	uint8_t *p = (uint8_t *)usb_dev.descriptors;
	uint8_t cur_alt_setting = 0xFF;
	uint8_t cur_config = 0xFF;
	bool found = false;

	LOG_DBG("Set Configuration %u request", setup->wValue);

	if (setup->wValue == 0U) {
		usb_reset_alt_setting();
		usb_dev.configuration = setup->wValue;
		if (usb_dev.status_callback) {
			usb_dev.status_callback(USB_DC_CONFIGURED,
						&usb_dev.configuration);
		}

		return true;
	}

	/* configure endpoints for this configuration/altsetting */
	while (p[DESC_bLength] != 0U) {
		switch (p[DESC_bDescriptorType]) {
		case USB_DESC_CONFIGURATION:
			/* remember current configuration index */
			cur_config = p[CONF_DESC_bConfigurationValue];
			if (cur_config == setup->wValue) {
				found = true;
			}

			break;

		case USB_DESC_INTERFACE:
			/* remember current alternate setting */
			cur_alt_setting =
			    p[INTF_DESC_bAlternateSetting];
			break;

		case USB_DESC_ENDPOINT:
			if ((cur_config != setup->wValue) ||
			    (cur_alt_setting != 0)) {
				break;
			}

			found = set_endpoint((struct usb_ep_descriptor *)p);
			break;

		default:
			break;
		}

		/* skip to next descriptor */
		p += p[DESC_bLength];
	}

	if (found) {
		usb_reset_alt_setting();
		usb_dev.configuration = setup->wValue;
		if (usb_dev.status_callback) {
			usb_dev.status_callback(USB_DC_CONFIGURED,
						&usb_dev.configuration);
		}
	} else {
		LOG_DBG("Set Configuration %u failed", setup->wValue);
	}

	return found;
}

/*
 * @brief set USB interface
 *
 * @param [in] setup        The setup packet
 *
 * @return true if successfully configured false if error or unconfigured
 */
static bool usb_set_interface(struct usb_setup_packet *setup)
{
	const uint8_t *p = usb_dev.descriptors;
	const uint8_t *if_desc = NULL;
	struct usb_ep_descriptor *ep;
	uint8_t cur_alt_setting = 0xFF;
	uint8_t cur_iface = 0xFF;
	bool ret = false;

	LOG_DBG("Set Interface %u alternate %u", setup->wIndex, setup->wValue);

	while (p[DESC_bLength] != 0U) {
		switch (p[DESC_bDescriptorType]) {
		case USB_DESC_INTERFACE:
			/* remember current alternate setting */
			cur_alt_setting = p[INTF_DESC_bAlternateSetting];
			cur_iface = p[INTF_DESC_bInterfaceNumber];

			if (cur_iface == setup->wIndex &&
			    cur_alt_setting == setup->wValue) {
				ret = usb_set_alt_setting(setup->wIndex,
							  setup->wValue);
				if_desc = (void *)p;
			}

			LOG_DBG("Current iface %u alt setting %u",
				cur_iface, cur_alt_setting);
			break;
		case USB_DESC_ENDPOINT:
			if (cur_iface == setup->wIndex) {
				ep = (struct usb_ep_descriptor *)p;
				ret = usb_eps_reconfigure(ep, cur_alt_setting,
							  setup->wValue);
			}
			break;
		default:
			break;
		}

		/* skip to next descriptor */
		p += p[DESC_bLength];
	}

	if (usb_dev.status_callback) {
		usb_dev.status_callback(USB_DC_INTERFACE, if_desc);
	}

	return ret;
}

static bool usb_get_interface(struct usb_setup_packet *setup,
			      int32_t *len, uint8_t **data_buf)
{
	const uint8_t *p = usb_dev.descriptors;
	uint8_t *data = *data_buf;
	uint8_t cur_iface;

	while (p[DESC_bLength] != 0U) {
		if (p[DESC_bDescriptorType] == USB_DESC_INTERFACE) {
			cur_iface = p[INTF_DESC_bInterfaceNumber];
			if (cur_iface == setup->wIndex) {
				data[0] = usb_get_alt_setting(cur_iface);
				LOG_DBG("Current iface %u alt setting %u",
					setup->wIndex, data[0]);
				*len = 1;
				return true;
			}
		}

		/* skip to next descriptor */
		p += p[DESC_bLength];
	}

	return false;
}

/**
 * @brief Check if the device is in Configured state
 *
 * @return true if Configured, false otherwise.
 */
static bool is_device_configured(void)
{
	return (usb_dev.configuration != 0);
}

/*
 * @brief handle a standard device request
 *
 * @param [in]     setup    The setup packet
 * @param [in,out] len      Pointer to data length
 * @param [in,out] data_buf Data buffer
 *
 * @return true if the request was handled successfully
 */
static bool usb_handle_std_device_req(struct usb_setup_packet *setup,
				      int32_t *len, uint8_t **data_buf)
{
	uint8_t *data = *data_buf;

	if (usb_reqtype_is_to_host(setup)) {
		switch (setup->bRequest) {
		case USB_SREQ_GET_STATUS:
			return usb_get_status(setup, len, data_buf);

		case USB_SREQ_GET_DESCRIPTOR:
			return usb_get_descriptor(setup, len, data_buf);

		case USB_SREQ_GET_CONFIGURATION:
			LOG_DBG("Get Configuration request");
			/* indicate if we are configured */
			data[0] = usb_dev.configuration;
			*len = 1;
			return true;
		default:
			break;
		}
	} else {
		switch (setup->bRequest) {
		case USB_SREQ_SET_ADDRESS:
			LOG_DBG("Set Address %u request", setup->wValue);
			return !usb_dc_set_address(setup->wValue);

		case USB_SREQ_SET_CONFIGURATION:
			return usb_set_configuration(setup);

		case USB_SREQ_CLEAR_FEATURE:
			LOG_DBG("Clear Feature request");

			if (IS_ENABLED(CONFIG_USB_DEVICE_REMOTE_WAKEUP)) {
				if (setup->wValue == USB_SFS_REMOTE_WAKEUP) {
					usb_dev.remote_wakeup = false;
					return true;
				}
			}
			break;

		case USB_SREQ_SET_FEATURE:
			LOG_DBG("Set Feature request");

			if (IS_ENABLED(CONFIG_USB_DEVICE_REMOTE_WAKEUP)) {
				if (setup->wValue == USB_SFS_REMOTE_WAKEUP) {
					usb_dev.remote_wakeup = true;
					return true;
				}
			}
			break;

		default:
			break;
		}
	}

	LOG_DBG("Unsupported bmRequestType 0x%02x bRequest 0x%02x",
		setup->bmRequestType, setup->bRequest);
	return false;
}

/**
 * @brief Check if the interface of given number is valid
 *
 * @param [in] interface Number of the addressed interface
 *
 * This function searches through descriptor and checks
 * is the Host has addressed valid interface.
 *
 * @return true if interface exists - valid
 */
static bool is_interface_valid(uint8_t interface)
{
	const uint8_t *p = (uint8_t *)usb_dev.descriptors;
	const struct usb_cfg_descriptor *cfg_descr;

	/* Search through descriptor for matching interface */
	while (p[DESC_bLength] != 0U) {
		if (p[DESC_bDescriptorType] == USB_DESC_CONFIGURATION) {
			cfg_descr = (const struct usb_cfg_descriptor *)p;
			if (interface < cfg_descr->bNumInterfaces) {
				return true;
			}
		}
		p += p[DESC_bLength];
	}

	return false;
}

/*
 * @brief handle a standard interface request
 *
 * @param [in]     setup    The setup packet
 * @param [in,out] len      Pointer to data length
 * @param [in]     data_buf Data buffer
 *
 * @return true if the request was handled successfully
 */
static bool usb_handle_std_interface_req(struct usb_setup_packet *setup,
					 int32_t *len, uint8_t **data_buf)
{
	uint8_t *data = *data_buf;

	/** The device must be configured to accept standard interface
	 * requests and the addressed Interface must be valid.
	 */
	if (!is_device_configured() ||
	   (!is_interface_valid((uint8_t)setup->wIndex))) {
		return false;
	}

	if (usb_reqtype_is_to_host(setup)) {
		switch (setup->bRequest) {
		case USB_SREQ_GET_STATUS:
			/* no bits specified */
			data[0] = 0U;
			data[1] = 0U;
			*len = 2;
			return true;

		case USB_SREQ_GET_INTERFACE:
			return usb_get_interface(setup, len, data_buf);

		default:
			break;
		}
	} else {
		if (setup->bRequest == USB_SREQ_SET_INTERFACE) {
			return usb_set_interface(setup);
		}

	}

	LOG_DBG("Unsupported bmRequestType 0x%02x bRequest 0x%02x",
		setup->bmRequestType, setup->bRequest);
	return false;
}

/**
 * @brief Check if the endpoint of given address is valid
 *
 * @param [in] ep Address of the Endpoint
 *
 * This function checks if the Endpoint of given address
 * is valid for the configured device. Valid Endpoint is
 * either Control Endpoint or one used by the device.
 *
 * @return true if endpoint exists - valid
 */
static bool is_ep_valid(uint8_t ep)
{
	const struct usb_ep_cfg_data *ep_data;

	/* Check if its Endpoint 0 */
	if (USB_EP_GET_IDX(ep) == 0) {
		return true;
	}

	STRUCT_SECTION_FOREACH(usb_cfg_data, cfg_data) {
		ep_data = cfg_data->endpoint;

		for (uint8_t n = 0; n < cfg_data->num_endpoints; n++) {
			if (ep_data[n].ep_addr == ep) {
				return true;
			}
		}
	}

	return false;
}

static bool usb_get_status_endpoint(struct usb_setup_packet *setup,
				    int32_t *len, uint8_t **data_buf)
{
	uint8_t ep = setup->wIndex;
	uint8_t *data = *data_buf;

	/* Check if request addresses valid Endpoint */
	if (!is_ep_valid(ep)) {
		return false;
	}

	/* This request is valid for Control Endpoints when
	 * the device is not yet configured. For other
	 * Endpoints the device must be configured.
	 * Firstly check if addressed ep is Control Endpoint.
	 * If no then the device must be in Configured state
	 * to accept the request.
	 */
	if ((USB_EP_GET_IDX(ep) == 0) || is_device_configured()) {
		/* bit 0 - Endpoint halted or not */
		usb_dc_ep_is_stalled(ep, &data[0]);
		data[1] = 0U;
		*len = 2;
		return true;
	}

	return false;
}


static bool usb_halt_endpoint_req(struct usb_setup_packet *setup, bool halt)
{
	uint8_t ep = setup->wIndex;

	/* Check if request addresses valid Endpoint */
	if (!is_ep_valid(ep)) {
		return false;
	}

	/* This request is valid for Control Endpoints when
	 * the device is not yet configured. For other
	 * Endpoints the device must be configured.
	 * Firstly check if addressed ep is Control Endpoint.
	 * If no then the device must be in Configured state
	 * to accept the request.
	 */
	if ((USB_EP_GET_IDX(ep) == 0) || is_device_configured()) {
		if (halt) {
			LOG_INF("Set halt ep 0x%02x", ep);
			usb_dc_ep_set_stall(ep);
			if (usb_dev.status_callback) {
				usb_dev.status_callback(USB_DC_SET_HALT, &ep);
			}
		} else {
			LOG_INF("Clear halt ep 0x%02x", ep);
			usb_dc_ep_clear_stall(ep);
			if (usb_dev.status_callback) {
				usb_dev.status_callback(USB_DC_CLEAR_HALT, &ep);
			}
		}

		return true;
	}

	return false;
}

/*
 * @brief handle a standard endpoint request
 *
 * @param [in]     setup    The setup packet
 * @param [in,out] len      Pointer to data length
 * @param [in]     data_buf Data buffer
 *
 * @return true if the request was handled successfully
 */
static bool usb_handle_std_endpoint_req(struct usb_setup_packet *setup,
					int32_t *len, uint8_t **data_buf)
{

	if (usb_reqtype_is_to_host(setup)) {
		if (setup->bRequest == USB_SREQ_GET_STATUS) {
			return usb_get_status_endpoint(setup, len, data_buf);
		}
	} else {
		switch (setup->bRequest) {
		case USB_SREQ_CLEAR_FEATURE:
			if (setup->wValue == USB_SFS_ENDPOINT_HALT) {
				return usb_halt_endpoint_req(setup, false);
			}
			break;
		case USB_SREQ_SET_FEATURE:
			if (setup->wValue == USB_SFS_ENDPOINT_HALT) {
				return usb_halt_endpoint_req(setup, true);
			}
			break;
		default:
			break;
		}
	}

	LOG_DBG("Unsupported bmRequestType 0x%02x bRequest 0x%02x",
		setup->bmRequestType, setup->bRequest);
	return false;
}

/*
 * @brief default handler for standard ('chapter 9') requests
 *
 * If a custom request handler was installed, this handler is called first.
 *
 * @param [in]     setup    The setup packet
 * @param [in,out] len      Pointer to data length
 * @param [in]     data_buf Data buffer
 *
 * @return true if the request was handled successfully
 */
static int usb_handle_standard_request(struct usb_setup_packet *setup,
				       int32_t *len, uint8_t **data_buf)
{
	int rc = 0;

	if (!usb_handle_bos(setup, len, data_buf)) {
		return 0;
	}

	if (!usb_handle_os_desc(setup, len, data_buf)) {
		return 0;
	}

	/* try the custom request handler first */
	if (usb_dev.custom_req_handler &&
	    !usb_dev.custom_req_handler(setup, len, data_buf)) {
		return 0;
	}

	switch (setup->RequestType.recipient) {
	case USB_REQTYPE_RECIPIENT_DEVICE:
		if (usb_handle_std_device_req(setup, len, data_buf) == false) {
			rc = -EINVAL;
		}
		break;
	case USB_REQTYPE_RECIPIENT_INTERFACE:
		if (usb_handle_std_interface_req(setup, len, data_buf) == false) {
			rc = -EINVAL;
		}
		break;
	case USB_REQTYPE_RECIPIENT_ENDPOINT:
		if (usb_handle_std_endpoint_req(setup, len, data_buf) == false) {
			rc = -EINVAL;
		}
		break;
	default:
		rc = -EINVAL;
	}
	return rc;
}

/*
 * @brief Registers a callback for custom device requests
 *
 * In usb_register_custom_req_handler, the custom request handler gets a first
 * chance at handling the request before it is handed over to the 'chapter 9'
 * request handler.
 *
 * This can be used for example in HID devices, where a REQ_GET_DESCRIPTOR
 * request is sent to an interface, which is not covered by the 'chapter 9'
 * specification.
 *
 * @param [in] handler Callback function pointer
 */
static void usb_register_custom_req_handler(usb_request_handler handler)
{
	usb_dev.custom_req_handler = handler;
}

/*
 * @brief register a callback for device status
 *
 * This function registers a callback for device status. The registered callback
 * is used to report changes in the status of the device controller.
 *
 * @param [in] cb Callback function pointer
 */
static void usb_register_status_callback(usb_dc_status_callback cb)
{
	usb_dev.status_callback = cb;
}

static int foreach_ep(int (* endpoint_callback)(const struct usb_ep_cfg_data *))
{
	struct usb_ep_cfg_data *ep_data;

	STRUCT_SECTION_FOREACH(usb_cfg_data, cfg_data) {
		ep_data = cfg_data->endpoint;

		for (uint8_t n = 0; n < cfg_data->num_endpoints; n++) {
			int ret;

			ret = endpoint_callback(&ep_data[n]);
			if (ret < 0) {
				return ret;
			}
		}
	}

	return 0;
}

static int disable_interface_ep(const struct usb_ep_cfg_data *ep_data)
{
	uint32_t ep_bm;
	int ret;

	ret = usb_dc_ep_disable(ep_data->ep_addr);

	/* clear endpoint mask */
	ep_bm = get_ep_bm_from_addr(ep_data->ep_addr);
	usb_dev.ep_bm &= ~ep_bm;

	return ret;
}

static void forward_status_cb(enum usb_dc_status_code status, const uint8_t *param)
{
	if (status == USB_DC_DISCONNECTED) {
		usb_reset_alt_setting();
	}

	if (status == USB_DC_DISCONNECTED || status == USB_DC_RESET) {
		if (usb_dev.configured) {
			usb_cancel_transfers();
			foreach_ep(disable_interface_ep);
			usb_dev.configured = false;
		}
	}

	STRUCT_SECTION_FOREACH(usb_cfg_data, cfg_data) {
		if (cfg_data->cb_usb_status) {
			cfg_data->cb_usb_status(cfg_data, status, param);
		}
	}

	if (usb_dev.user_status_callback) {
		usb_dev.user_status_callback(status, param);
	}
}

/**
 * @brief turn on/off USB VBUS voltage
 *
 * To utilize this in the devicetree the chosen node should have a
 * zephyr,usb-device property that points to the usb device controller node.
 * Additionally the usb device controller node should have a vbus-gpios
 * property that has the GPIO details.
 *
 * Something like:
 *
 * chosen {
 *      zephyr,usb-device = &usbd;
 * };
 *
 * usbd: usbd {
 *      vbus-gpios = <&gpio1 5 GPIO_ACTIVE_HIGH>;
 * };
 *
 * @param on Set to false to turn off and to true to turn on VBUS
 *
 * @return 0 on success, negative errno code on fail
 */
static int usb_vbus_set(bool on)
{
#define USB_DEV_NODE DT_CHOSEN(zephyr_usb_device)
#if DT_NODE_HAS_STATUS(USB_DEV_NODE, okay) && \
    DT_NODE_HAS_PROP(USB_DEV_NODE, vbus_gpios)
	int ret = 0;
	struct gpio_dt_spec gpio_dev = GPIO_DT_SPEC_GET(USB_DEV_NODE, vbus_gpios);

	if (!gpio_is_ready_dt(&gpio_dev)) {
		LOG_DBG("USB requires GPIO. Device %s is not ready!", gpio_dev.port->name);
		return -ENODEV;
	}

	/* Enable USB IO */
	ret = gpio_pin_configure_dt(&gpio_dev, GPIO_OUTPUT);
	if (ret) {
		return ret;
	}

	ret = gpio_pin_set_dt(&gpio_dev, on == true ? 1 : 0);
	if (ret) {
		return ret;
	}
#endif

	return 0;
}

int usb_deconfig(void)
{
	/* unregister descriptors */
	usb_register_descriptors(NULL);

	/* unregister standard request handler */
	usb_register_request_handler(USB_REQTYPE_TYPE_STANDARD, NULL);

	/* unregister class request handlers for each interface*/
	usb_register_request_handler(USB_REQTYPE_TYPE_CLASS, NULL);

	/* unregister class request handlers for each interface*/
	usb_register_custom_req_handler(NULL);

	/* unregister status callback */
	usb_register_status_callback(NULL);

	/* unregister user status callback */
	usb_dev.user_status_callback = NULL;

	/* Reset USB controller */
	usb_dc_reset();

	return 0;
}

int usb_disable(void)
{
	int ret;

	if (usb_dev.enabled != true) {
		/*Already disabled*/
		return 0;
	}

	ret = usb_dc_detach();
	if (ret < 0) {
		return ret;
	}

	usb_cancel_transfers();
	for (uint8_t i = 0; i <= 15; i++) {
		if (usb_dev.ep_bm & BIT(i)) {
			ret = disable_endpoint(i);
			if (ret < 0) {
				return ret;
			}
		}
		if (usb_dev.ep_bm & BIT(i + 16)) {
			ret = disable_endpoint(USB_EP_DIR_IN | i);
			if (ret < 0) {
				return ret;
			}
		}
	}

	/* Disable VBUS if needed */
	usb_vbus_set(false);

	usb_dev.enabled = false;

	return 0;
}

int usb_write(uint8_t ep, const uint8_t *data, uint32_t data_len, uint32_t *bytes_ret)
{
	int tries = CONFIG_USB_NUMOF_EP_WRITE_RETRIES;
	int ret;

	do {
		ret = usb_dc_ep_write(ep, data, data_len, bytes_ret);
		if (ret == -EAGAIN) {
			LOG_WRN("Failed to write endpoint buffer 0x%02x", ep);
			k_yield();
		}

	} while (ret == -EAGAIN && tries--);

	return ret;
}

int usb_read(uint8_t ep, uint8_t *data, uint32_t max_data_len, uint32_t *ret_bytes)
{
	return usb_dc_ep_read(ep, data, max_data_len, ret_bytes);
}

int usb_ep_set_stall(uint8_t ep)
{
	return usb_dc_ep_set_stall(ep);
}

int usb_ep_clear_stall(uint8_t ep)
{
	return usb_dc_ep_clear_stall(ep);
}

int usb_ep_read_wait(uint8_t ep, uint8_t *data, uint32_t max_data_len, uint32_t *ret_bytes)
{
	return usb_dc_ep_read_wait(ep, data, max_data_len, ret_bytes);
}

int usb_ep_read_continue(uint8_t ep)
{
	return usb_dc_ep_read_continue(ep);
}

bool usb_get_remote_wakeup_status(void)
{
	return usb_dev.remote_wakeup;
}

int usb_wakeup_request(void)
{
	if (IS_ENABLED(CONFIG_USB_DEVICE_REMOTE_WAKEUP)) {
		if (usb_get_remote_wakeup_status()) {
			return usb_dc_wakeup_request();
		}
		return -EACCES;
	} else {
		return -ENOTSUP;
	}
}

/*
 * The functions class_handler(), custom_handler() and vendor_handler()
 * go through the interfaces one after the other and compare the
 * bInterfaceNumber with the wIndex and and then call the appropriate
 * callback of the USB function.
 * Note, a USB function can have more than one interface and the
 * request does not have to be directed to the first interface (unlikely).
 * These functions can be simplified and moved to usb_handle_request()
 * when legacy initialization through the usb_set_config() and
 * usb_enable() is no longer needed.
 */

static int class_handler(struct usb_setup_packet *pSetup,
			 int32_t *len, uint8_t **data)
{
	const struct usb_if_descriptor *if_descr;
	struct usb_interface_cfg_data *iface;

	LOG_DBG("bRequest 0x%02x, wIndex 0x%04x",
		pSetup->bRequest, pSetup->wIndex);

	STRUCT_SECTION_FOREACH(usb_cfg_data, cfg_data) {
		iface = &cfg_data->interface;
		if_descr = cfg_data->interface_descriptor;
		/*
		 * Wind forward until it is within the range
		 * of the current descriptor.
		 */
		if ((uint8_t *)if_descr < usb_dev.descriptors) {
			continue;
		}

		if (iface->class_handler &&
		    if_descr->bInterfaceNumber == (pSetup->wIndex & 0xFF)) {
			return iface->class_handler(pSetup, len, data);
		}
	}

	return -ENOTSUP;
}

static int custom_handler(struct usb_setup_packet *pSetup,
			  int32_t *len, uint8_t **data)
{
	const struct usb_if_descriptor *if_descr;
	struct usb_interface_cfg_data *iface;

	LOG_DBG("bRequest 0x%02x, wIndex 0x%04x",
		pSetup->bRequest, pSetup->wIndex);

	STRUCT_SECTION_FOREACH(usb_cfg_data, cfg_data) {
		iface = &cfg_data->interface;
		if_descr = cfg_data->interface_descriptor;
		/*
		 * Wind forward until it is within the range
		 * of the current descriptor.
		 */
		if ((uint8_t *)if_descr < usb_dev.descriptors) {
			continue;
		}

		if (iface->custom_handler == NULL) {
			continue;
		}

		if (if_descr->bInterfaceNumber == (pSetup->wIndex & 0xFF)) {
			return iface->custom_handler(pSetup, len, data);
		} else {
			/*
			 * Audio has several interfaces.  if_descr points to
			 * the first interface, but the request may be for
			 * subsequent ones, so forward each request to audio.
			 * The class does not actively engage in request
			 * handling and therefore we can ignore return value.
			 */
			if (if_descr->bInterfaceClass == USB_BCC_AUDIO) {
				(void)iface->custom_handler(pSetup, len, data);
			}
		}
	}

	return -ENOTSUP;
}

static int vendor_handler(struct usb_setup_packet *pSetup,
			  int32_t *len, uint8_t **data)
{
	struct usb_interface_cfg_data *iface;

	LOG_DBG("bRequest 0x%02x, wIndex 0x%04x",
		pSetup->bRequest, pSetup->wIndex);

	if (usb_os_desc_enabled()) {
		if (!usb_handle_os_desc_feature(pSetup, len, data)) {
			return 0;
		}
	}

	STRUCT_SECTION_FOREACH(usb_cfg_data, cfg_data) {
		iface = &cfg_data->interface;
		if (iface->vendor_handler) {
			if (!iface->vendor_handler(pSetup, len, data)) {
				return 0;
			}
		}
	}

	return -ENOTSUP;
}

static int composite_setup_ep_cb(void)
{
	struct usb_ep_cfg_data *ep_data;

	STRUCT_SECTION_FOREACH(usb_cfg_data, cfg_data) {
		ep_data = cfg_data->endpoint;
		for (uint8_t n = 0; n < cfg_data->num_endpoints; n++) {
			LOG_DBG("set cb, ep: 0x%x", ep_data[n].ep_addr);
			if (usb_dc_ep_set_callback(ep_data[n].ep_addr,
						   ep_data[n].ep_cb)) {
				return -1;
			}
		}
	}

	return 0;
}

int usb_set_config(const uint8_t *device_descriptor)
{
	/* register descriptors */
	usb_register_descriptors(device_descriptor);

	/* register standard request handler */
	usb_register_request_handler(USB_REQTYPE_TYPE_STANDARD,
				     usb_handle_standard_request);

	/* register class request handlers for each interface*/
	usb_register_request_handler(USB_REQTYPE_TYPE_CLASS, class_handler);

	/* register vendor request handler */
	usb_register_request_handler(USB_REQTYPE_TYPE_VENDOR, vendor_handler);

	/* register class request handlers for each interface*/
	usb_register_custom_req_handler(custom_handler);

	return 0;
}

int usb_enable(usb_dc_status_callback status_cb)
{
	int ret;
	struct usb_dc_ep_cfg_data ep0_cfg;
	struct usb_device_descriptor *dev_desc = (void *)usb_dev.descriptors;

	/* Prevent from calling usb_enable form different context.
	 * This should only be called once.
	 */
	LOG_DBG("lock usb_enable_lock mutex");
	k_mutex_lock(&usb_enable_lock, K_FOREVER);

	if (usb_dev.enabled == true) {
		LOG_WRN("USB device support already enabled");
		ret = -EALREADY;
		goto out;
	}

	/* Enable VBUS if needed */
	ret = usb_vbus_set(true);
	if (ret < 0) {
		goto out;
	}

	usb_dev.user_status_callback = status_cb;
	usb_register_status_callback(forward_status_cb);
	usb_dc_set_status_callback(forward_status_cb);

	ret = usb_dc_attach();
	if (ret < 0) {
		goto out;
	}

	ret = usb_transfer_init();
	if (ret < 0) {
		goto out;
	}

	if (dev_desc->bDescriptorType != USB_DESC_DEVICE ||
	    dev_desc->bMaxPacketSize0 == 0) {
		LOG_ERR("Erroneous device descriptor or bMaxPacketSize0");
		ret = -EINVAL;
		goto out;
	}

	/* Configure control EP */
	usb_dev.mps0 = dev_desc->bMaxPacketSize0;
	ep0_cfg.ep_mps = usb_dev.mps0;
	ep0_cfg.ep_type = USB_DC_EP_CONTROL;

	ep0_cfg.ep_addr = USB_CONTROL_EP_OUT;
	ret = usb_dc_ep_configure(&ep0_cfg);
	if (ret < 0) {
		goto out;
	}

	ep0_cfg.ep_addr = USB_CONTROL_EP_IN;
	ret = usb_dc_ep_configure(&ep0_cfg);
	if (ret < 0) {
		goto out;
	}

	/* Register endpoint 0 handlers*/
	ret = usb_dc_ep_set_callback(USB_CONTROL_EP_OUT,
				     usb_handle_control_transfer);
	if (ret < 0) {
		goto out;
	}

	ret = usb_dc_ep_set_callback(USB_CONTROL_EP_IN,
				     usb_handle_control_transfer);
	if (ret < 0) {
		goto out;
	}

	/* Register endpoint handlers*/
	ret = composite_setup_ep_cb();
	if (ret < 0) {
		goto out;
	}

	/* Enable control EP */
	ret = usb_dc_ep_enable(USB_CONTROL_EP_OUT);
	if (ret < 0) {
		goto out;
	}
	usb_dev.ep_bm |= get_ep_bm_from_addr(USB_CONTROL_EP_OUT);

	ret = usb_dc_ep_enable(USB_CONTROL_EP_IN);
	if (ret < 0) {
		goto out;
	}
	usb_dev.ep_bm |= get_ep_bm_from_addr(USB_CONTROL_EP_IN);

	usb_dev.enabled = true;
	ret = 0;
out:
	LOG_DBG("unlock usb_enable_lock mutex");
	k_mutex_unlock(&usb_enable_lock);
	return ret;
}

/*
 * This function configures the USB device stack based on USB descriptor and
 * usb_cfg_data.
 */
static int usb_device_init(void)
{
	uint8_t *device_descriptor;

	if (usb_dev.enabled == true) {
		return -EALREADY;
	}

	/* register device descriptor */
	device_descriptor = usb_get_device_descriptor();
	if (!device_descriptor) {
		LOG_ERR("Failed to configure USB device stack");
		return -1;
	}

	usb_set_config(device_descriptor);

	if (IS_ENABLED(CONFIG_USB_DEVICE_INITIALIZE_AT_BOOT)) {
		return usb_enable(NULL);
	}

	return 0;
}

SYS_INIT(usb_device_init, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY);
