/*******************************************************************************
 *
 * Copyright(c) 2015,2016 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in
 * the documentation and/or other materials provided with the
 * distribution.
 * * Neither the name of Intel Corporation nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/

/**
 * @brief DFU class driver
 *
 * USB DFU device class driver
 *
 */

#include <nanokernel.h>
#include <stdio.h>
#include <errno.h>
#include <flash.h>
#include <usb/usb_device.h>
#include "usb_dfu.h"


#ifndef CONFIG_USB_DFU_DEBUG
#define DBG(...) { ; }
#else
#if defined(CONFIG_STDOUT_CONSOLE)
#include <stdio.h>
#define DBG printf
#else
#define DBG printk
#endif /* CONFIG_STDOUT_CONSOLE */
#endif /* CONFIG_USB_DFU_DEBUG */

/* Alternate settings are used to access additional memory segments.
 * This example uses the alternate settings as an offset into flash.
 */
#define DFU_FLASH_ADDR (dfu_data.flash_base_addr + \
	dfu_data.alt_setting * DFU_ALT_SETTING_OFFSET)

/* Misc. macros */
#define LOW_BYTE(x)  ((x) & 0xFF)
#define HIGH_BYTE(x) ((x) >> 8)

static struct usb_cfg_data dfu_config;

/* Device data structure */
struct dfu_data_t {
	/* Flash device to read/write data from/to */
	struct device *flash_dev;
	/* Flash base address to write/read to/from */
	uint32_t flash_base_addr;
	/* Flash page size */
	uint32_t flash_page_size;
	/* Size of the upload transfer */
	uint32_t flash_upload_size;
	/* Number of bytes sent during upload */
	uint32_t bytes_sent;
	/* Number of bytes received during download */
	uint32_t bytes_rcvd;
	uint32_t alt_setting;              /* DFU alternate setting */
	uint8_t buffer[DFU_MAX_XFER_SIZE]; /* DFU data buffer */
	enum dfu_state state;              /* State of the DFU device */
	enum dfu_status status;            /* Status of the DFU device */
	uint16_t block_nr;                 /* DFU block number */
};

static struct dfu_data_t dfu_data = {
	.state = appIDLE,
	.status = statusOK,
};

/* Structure representing the DFU runtime USB description */
static const uint8_t dfu_runtime_usb_description[] = {
	/* Device descriptor */
	USB_DEVICE_DESC_SIZE,           /* Descriptor size */
	USB_DEVICE_DESC,                /* Descriptor type */
	LOW_BYTE(USB_1_1),
	HIGH_BYTE(USB_1_1),             /* USB version in BCD format */
	0x00,                           /* Class - Interface specific */
	0x00,                           /* SubClass - Interface specific */
	0x00,                           /* Protocol - Interface specific */
	MAX_PACKET_SIZE0,               /* EP0 Max Packet Size */
	LOW_BYTE(DFU_VENDOR_ID),
	HIGH_BYTE(DFU_VENDOR_ID),       /* Vendor Id */
	LOW_BYTE(DFU_PRODUCT_ID),
	HIGH_BYTE(DFU_PRODUCT_ID),      /* Product Id */
	LOW_BYTE(BCDDEVICE_RELNUM),
	HIGH_BYTE(BCDDEVICE_RELNUM),    /* Device Release Number */
	/* Index of Manufacturer String Descriptor */
	0x01,
	/* Index of Product String Descriptor */
	0x02,
	/* Index of Serial Number String Descriptor */
	0x03,
	DFU_NUM_CONF,                   /* Number of Possible Configuration */

	/* Configuration descriptor */
	USB_CONFIGURATION_DESC_SIZE,    /* Descriptor size */
	USB_CONFIGURATION_DESC,         /* Descriptor type */
	/* Total length in bytes of data returned */
	LOW_BYTE(DFU_RUNTIME_CONF_SIZE),
	HIGH_BYTE(DFU_RUNTIME_CONF_SIZE),
	DFU_NUM_ITF,                    /* Number of interfaces */
	0x01,                           /* Configuration value */
	0x00,                           /* Index of the Configuration string */
	USB_CONFIGURATION_ATTRIBUTES,   /* Attributes */
	MAX_LOW_POWER,                  /* Max power consumption */

	/* Interface descriptor */
	USB_INTERFACE_DESC_SIZE,        /* Descriptor size */
	USB_INTERFACE_DESC,             /* Descriptor type */
	0x00,                           /* Interface index */
	0x00,                           /* Alternate setting */
	DFU_NUM_EP,                     /* Number of Endpoints */
	DFU_CLASS,                      /* Class */
	DFU_INTERFACE_SUBCLASS,         /* SubClass */
	DFU_RUNTIME_PROTOCOL,           /* DFU Runtime Protocol */
	/* Index of the Interface String Descriptor */
	0x00,

	/* DFU descriptor */
	USB_DFU_DESC_SIZE,              /* Descriptor size */
	USB_DFU_FUNCTIONAL_DESC,        /* Descriptor type DFU: Functional */
	DFU_ATTR_CAN_DNLOAD |
	DFU_ATTR_CAN_UPLOAD |
	DFU_ATTR_MANIFESTATION_TOLERANT,/* DFU attributes */
	LOW_BYTE(DFU_DETACH_TIMEOUT),
	HIGH_BYTE(DFU_DETACH_TIMEOUT),   /* wDetachTimeOut */
	LOW_BYTE(DFU_MAX_XFER_SIZE),
	HIGH_BYTE(DFU_MAX_XFER_SIZE),    /* wXferSize  - 512bytes */
	0x11, 0,                         /* DFU Version */

	/* String descriptor language, only one, so min size 4 bytes.
	 * 0x0409 English(US) language code used.
	 */
	USB_STRING_DESC_SIZE,            /* Descriptor size */
	USB_STRING_DESC,                 /* Descriptor type */
	0x09,
	0x04,

	/* Manufacturer String Descriptor "Intel" */
	0x0C,
	USB_STRING_DESC,
	'I', 0, 'n', 0, 't', 0, 'e', 0, 'l', 0,

	/* Product String Descriptor "DFU-Dev" */
	0x10,
	USB_STRING_DESC,
	'D', 0, 'F', 0, 'U', 0, '-', 0, 'D', 0, 'e', 0, 'v', 0,

	/* Serial Number String Descriptor "00.01" */
	0x0C,
	USB_STRING_DESC,
	'0', 0, '0', 0, '.', 0, '0', 0, '1', 0
};

/* Structure representing the DFU mode USB description */
static const uint8_t dfu_mode_usb_description[] = {
	/* Device descriptor */
	USB_DEVICE_DESC_SIZE,           /* Descriptor size */
	USB_DEVICE_DESC,                /* Descriptor type */
	LOW_BYTE(USB_1_1),
	HIGH_BYTE(USB_1_1),             /* USB version in BCD format */
	0x00,                           /* Class - Interface specific */
	0x00,                           /* SubClass - Interface specific */
	0x00,                           /* Protocol - Interface specific */
	MAX_PACKET_SIZE0,               /* EP0 Max Packet Size */
	LOW_BYTE(DFU_VENDOR_ID),
	HIGH_BYTE(DFU_VENDOR_ID),       /* Vendor Id */
	LOW_BYTE(DFU_PRODUCT_ID),
	HIGH_BYTE(DFU_PRODUCT_ID),      /* Product Id */
	LOW_BYTE(BCDDEVICE_RELNUM),
	HIGH_BYTE(BCDDEVICE_RELNUM),    /* Device Release Number */
	/* Index of Manufacturer String Descriptor */
	0x01,
	/* Index of Product String Descriptor */
	0x02,
	/* Index of Serial Number String Descriptor */
	0x03,
	DFU_NUM_CONF,                   /* Number of Possible Configuration */

	/* Configuration descriptor */
	USB_CONFIGURATION_DESC_SIZE,    /* Descriptor size */
	USB_CONFIGURATION_DESC,         /* Descriptor type */
	/* Total length in bytes of data returned */
	LOW_BYTE(DFU_RUNTIME_CONF_SIZE),
	HIGH_BYTE(DFU_RUNTIME_CONF_SIZE),
	DFU_NUM_ITF,                    /* Number of interfaces */
	0x01,                           /* Configuration value */
	0x00,                           /* Index of the Configuration string */
	USB_CONFIGURATION_ATTRIBUTES,   /* Attributes */
	MAX_LOW_POWER,                  /* Max power consumption */

	/* Interface descriptor, alternate setting 0 */
	USB_INTERFACE_DESC_SIZE,        /* Descriptor size */
	USB_INTERFACE_DESC,             /* Descriptor type */
	0x00,                           /* Interface index */
	0x00,                           /* Alternate setting */
	DFU_NUM_EP,                     /* Number of Endpoints */
	DFU_CLASS,                      /* Class */
	DFU_INTERFACE_SUBCLASS,         /* SubClass */
	DFU_MODE_PROTOCOL,              /* DFU Runtime Protocol */
	/* Index of the Interface String Descriptor */
	0x04,

	/* Interface descriptor, alternate setting 1 */
	USB_INTERFACE_DESC_SIZE,        /* Descriptor size */
	USB_INTERFACE_DESC,             /* Descriptor type */
	0x00,                           /* Interface index */
	0x01,                           /* Alternate setting */
	DFU_NUM_EP,                     /* Number of Endpoints */
	DFU_CLASS,                      /* Class */
	DFU_INTERFACE_SUBCLASS,         /* SubClass */
	DFU_MODE_PROTOCOL,              /* DFU Runtime Protocol */
	/* Index of the Interface String Descriptor */
	0x05,

	/* Interface descriptor, alternate setting 2 */
	USB_INTERFACE_DESC_SIZE,        /* Descriptor size */
	USB_INTERFACE_DESC,             /* Descriptor type */
	0x00,                           /* Interface index */
	0x02,                           /* Alternate setting */
	DFU_NUM_EP,                     /* Number of Endpoints */
	DFU_CLASS,                      /* Class */
	DFU_INTERFACE_SUBCLASS,         /* SubClass */
	DFU_MODE_PROTOCOL,              /* DFU Runtime Protocol */
	/* Index of the Interface String Descriptor */
	0x06,

	/* DFU descriptor */
	USB_DFU_DESC_SIZE,              /* Descriptor size */
	USB_DFU_FUNCTIONAL_DESC,        /* Descriptor type DFU: Functional */
	DFU_ATTR_CAN_DNLOAD |
	DFU_ATTR_CAN_UPLOAD |
	DFU_ATTR_MANIFESTATION_TOLERANT,/* DFU attributes */
	LOW_BYTE(DFU_DETACH_TIMEOUT),
	HIGH_BYTE(DFU_DETACH_TIMEOUT),  /* wDetachTimeOut */
	LOW_BYTE(DFU_MAX_XFER_SIZE),
	HIGH_BYTE(DFU_MAX_XFER_SIZE),   /* wXferSize  - 512bytes */
	0x11, 0,                        /* DFU Version */

	/* String descriptor language, only one, so min size 4 bytes.
	 * 0x0409 English(US) language code used.
	 */
	USB_STRING_DESC_SIZE,           /* Descriptor size */
	USB_STRING_DESC,                /* Descriptor type */
	0x09,
	0x04,

	/* Manufacturer String Descriptor "Intel" */
	0x0C,
	USB_STRING_DESC,
	'I', 0, 'n', 0, 't', 0, 'e', 0, 'l', 0,

	/* Product String Descriptor "DFU-Dev" */
	0x10,
	USB_STRING_DESC,
	'D', 0, 'F', 0, 'U', 0, '-', 0, 'D', 0, 'e', 0, 'v', 0,

	/* Serial Number String Descriptor "00.01" */
	0x0C,
	USB_STRING_DESC,
	'0', 0, '0', 0, '.', 0, '0', 0, '1', 0,

	/* Interface alternate setting 0 String Descriptor */
	0x0E,
	USB_STRING_DESC,
	'F', 0, 'L', 0, 'A', 0, 'S', 0, 'H', 0, '0', 0,

	/* Interface alternate setting 0 String Descriptor */
	0x0E,
	USB_STRING_DESC,
	'F', 0, 'L', 0, 'A', 0, 'S', 0, 'H', 0, '1', 0,

	/* Interface alternate setting 0 String Descriptor */
	0x0E,
	USB_STRING_DESC,
	'F', 0, 'L', 0, 'A', 0, 'S', 0, 'H', 0, '2', 0,
};

/**
 * @brief Helper function to check if in DFU app state.
 *
 * @return  true if app state, false otherwise.
 */
static bool dfu_check_app_state(void)
{
	if (dfu_data.state == appIDLE ||
	    dfu_data.state == appDETACH) {
		dfu_data.state = appIDLE;
		return true;
	}

	return false;
}

/**
 * @brief Helper function to reset DFU internal counters.
 */
static void dfu_reset_counters(void)
{
	dfu_data.bytes_sent = 0;
	dfu_data.bytes_rcvd = 0;
	dfu_data.block_nr = 0;
}

/**
 * @brief Handler called for DFU Class requests not handled by the USB stack.
 *
 * @param pSetup    Information about the request to execute.
 * @param len       Size of the buffer.
 * @param data      Buffer containing the request result.
 *
 * @return  0 on success, negative errno code on fail.
 */
static int dfu_class_handle_req(struct usb_setup_packet *pSetup,
		int32_t *data_len, uint8_t **data)
{
	int ret;
	uint32_t len, bytes_left;

	switch (pSetup->bRequest) {
	case DFU_GETSTATUS:
		DBG("DFU_GETSTATUS: status %d, state %d\n",
		    dfu_data.status, dfu_data.state);

		if (dfu_data.state == dfuMANIFEST_SYNC)
			dfu_data.state = dfuIDLE;

		(*data)[0] = dfu_data.status;
		(*data)[1] = 0;
		(*data)[2] = 1;
		(*data)[3] = 0;
		(*data)[4] = dfu_data.state;
		(*data)[5] = 0;
		*data_len = 6;
		break;

	case DFU_GETSTATE:
		DBG("DFU_GETSTATE\n");
		(*data)[0] = dfu_data.state;
		*data_len = 1;
		break;

	case DFU_ABORT:
		DBG("DFU_ABORT\n");

		if (dfu_check_app_state())
			return -EINVAL;

		dfu_reset_counters();
		dfu_data.state = dfuIDLE;
		dfu_data.status = statusOK;
		break;

	case DFU_CLRSTATUS:
		DBG("DFU_CLRSTATUS\n");

		if (dfu_check_app_state())
			return -EINVAL;

		dfu_data.state = dfuIDLE;
		dfu_data.status = statusOK;
		break;

	case DFU_DNLOAD:
		DBG("DFU_DNLOAD block %d, len %d, state %d\n",
		    pSetup->wValue, pSetup->wLength, dfu_data.state);

		if (dfu_check_app_state())
			return -EINVAL;

		switch (dfu_data.state) {
		case dfuIDLE:
			dfu_reset_counters();
			DBG("DFU_DNLOAD start\n");
		case dfuDNLOAD_IDLE:
			if (pSetup->wLength != 0) {
				/* Download has started */
				dfu_data.state = dfuDNLOAD_IDLE;

				if (!(dfu_data.bytes_rcvd %
				      dfu_data.flash_page_size)) {
					ret = flash_erase(dfu_data.flash_dev,
					    DFU_FLASH_ADDR +
						    dfu_data.bytes_rcvd,
					    dfu_data.flash_page_size);
					DBG("Flash erase\n");
					if (ret) {
						dfu_data.state = dfuERROR;
						dfu_data.status = errERASE;
						DBG("DFU flash erase error, "
						    "ret %d\n", ret);
					}
				}

				/* Flash write len should be multiple of 4 */
				len = (pSetup->wLength + 3) & (~3);
				ret = flash_write(dfu_data.flash_dev,
				    DFU_FLASH_ADDR +
					    dfu_data.bytes_rcvd,
				    *data, len);
				if (ret) {
					dfu_data.state = dfuERROR;
					dfu_data.status = errWRITE;
					DBG("DFU flash write error, ret %d\n",
					    ret);
				} else
					dfu_data.bytes_rcvd += pSetup->wLength;
			} else {
				/* Download completed */
				dfu_data.state = dfuMANIFEST_SYNC;
				dfu_reset_counters();
			}
			break;
		default:
			DBG("DFU_DNLOAD wrong state %d\n", dfu_data.state);
			dfu_data.state = dfuERROR;
			dfu_data.status = errUNKNOWN;
			dfu_reset_counters();
			return -EINVAL;
		}
		break;
	case DFU_UPLOAD:
		DBG("DFU_UPLOAD block %d, len %d, state %d\n",
		    pSetup->wValue, pSetup->wLength, dfu_data.state);

		if (dfu_check_app_state())
			return -EINVAL;

		switch (dfu_data.state) {
		case dfuIDLE:
			dfu_reset_counters();
			DBG("DFU_UPLOAD start\n");
		case dfuUPLOAD_IDLE:
			if (!pSetup->wLength ||
			    dfu_data.block_nr != pSetup->wValue) {
				DBG("DFU_UPLOAD block %d, expected %d, "
				    "len %d\n", pSetup->wValue,
				    dfu_data.block_nr, pSetup->wLength);
				dfu_data.state = dfuERROR;
				dfu_data.status = errUNKNOWN;
				break;
			}
			/* Upload in progress */
			bytes_left = dfu_data.flash_upload_size -
			    dfu_data.bytes_sent;
			if (bytes_left < pSetup->wLength)
				len = bytes_left;
			else
				len = pSetup->wLength;

			if (len) {
				ret = flash_read(dfu_data.flash_dev,
				    DFU_FLASH_ADDR +
					    dfu_data.bytes_sent,
				    dfu_data.buffer, len);
				if (ret) {
					dfu_data.state = dfuERROR;
					dfu_data.status = errFILE;
					break;
				}
			}
			*data_len = len;
			*data = dfu_data.buffer;

			dfu_data.bytes_sent += len;
			dfu_data.block_nr++;

			if (dfu_data.bytes_sent == dfu_data.flash_upload_size &&
			    len < pSetup->wLength) {
				/* Upload completed when a
				 * short packet is received
				 */
				*data_len = 0;
				dfu_data.state = dfuIDLE;
			} else
				dfu_data.state = dfuUPLOAD_IDLE;

			break;
		default:
			DBG("DFU_UPLOAD wrong state %d\n", dfu_data.state);
			dfu_data.state = dfuERROR;
			dfu_data.status = errUNKNOWN;
			dfu_reset_counters();
			return -EINVAL;
		}
		break;
	case DFU_DETACH:
		DBG("DFU_DETACH timeout %d, state %d\n",
			pSetup->wValue, dfu_data.state);

		if (dfu_data.state != appIDLE) {
			dfu_data.state = appIDLE;
			return -EINVAL;
		}
		/* Move to appDETACH state */
		dfu_data.state = appDETACH;

		/* We should start a timer here but in order to
		 * keep things simple and do not increase the size
		 * we rely on the host to get us out of the appATTACHED
		 * state if needed.
		 */

		/* Set the DFU mode descriptors to be used after reset */
		dfu_config.usb_device_description = dfu_mode_usb_description;
		usb_set_config(&dfu_config);
		break;
	default:
		DBG("DFU UNKNOWN STATE: %d\n", pSetup->bRequest);
		return -EINVAL;
	}

	return 0;
}

/**
 * @brief Callback used to know the USB connection status
 *
 * @param status USB device status code.
 *
 * @return  N/A.
 */
static void dfu_status_cb(enum usb_dc_status_code status)
{
	/* Check the USB status and do needed action if required */
	switch (status) {
	case USB_DC_ERROR:
		DBG("USB device error\n");
		break;
	case USB_DC_RESET:
		DBG("USB device reset detected, state %d\n", dfu_data.state);
		if (dfu_data.state == appDETACH) {
			dfu_data.state = dfuIDLE;
		}
		break;
	case USB_DC_CONNECTED:
		DBG("USB device connected\n");
		break;
	case USB_DC_CONFIGURED:
		DBG("USB device configured\n");
		break;
	case USB_DC_DISCONNECTED:
		DBG("USB device disconnected\n");
		break;
	case USB_DC_SUSPEND:
		DBG("USB device supended\n");
		break;
	case USB_DC_RESUME:
		DBG("USB device resumed\n");
		break;
	case USB_DC_UNKNOWN:
	default:
		DBG("USB unknown state\n");
		break;
	}
}

/**
 * @brief Custom handler for standard ('chapter 9') requests
 *        in order to catch the SET_INTERFACE request and
 *        extract the interface alternate setting
 *
 * @param pSetup    Information about the request to execute.
 * @param len       Size of the buffer.
 * @param data      Buffer containing the request result.
 *
 * @return  0 if SET_INTERFACE request, -ENOTSUP otherwise.
 */

static int dfu_custom_handle_req(struct usb_setup_packet *pSetup,
		int32_t *data_len, uint8_t **data)
{
	if (REQTYPE_GET_RECIP(pSetup->bmRequestType) ==
	    REQTYPE_RECIP_INTERFACE) {
		if (pSetup->bRequest == REQ_SET_INTERFACE) {
			DBG("DFU alternate setting %d\n", pSetup->wValue);

			if (pSetup->wValue >= DFU_MODE_ALTERNATE_SETTINGS) {
				DBG("Invalid DFU alternate setting (%d)\n",
				    pSetup->wValue);
			} else {
				dfu_data.alt_setting = pSetup->wValue;
			}
			*data_len = 0;
			return 0;
		}
	}

	/* Not handled by us */
	return -ENOTSUP;
}

/* Configuration of the DFU Device send to the USB Driver */
static struct usb_cfg_data dfu_config = {
	.usb_device_description = dfu_runtime_usb_description,
	.cb_usb_status = dfu_status_cb,
	.interface = {
		.class_handler = dfu_class_handle_req,
		.custom_handler = dfu_custom_handle_req,
		.payload_data = dfu_data.buffer,
	},
	.num_endpoints = DFU_NUM_EP,
};

/**
 * @brief DFU class driver start routine
 *
 * @param flash_dev         Flash device driver to be used.
 * @param flash_base_addr   Flash address to write/read to/from.
 * @param flash_page_size   Flash page size.
 * @param flash_upload_size Flash upload data size.
 *
 * @return  N/A.
 */
int dfu_start(struct device *flash_dev, uint32_t flash_base_addr,
		uint32_t flash_page_size, uint32_t flash_upload_size)
{
	int ret;

	if (!flash_dev)
		return -EINVAL;

	dfu_data.flash_dev = flash_dev;
	dfu_data.flash_base_addr = flash_base_addr;
	dfu_data.flash_page_size = flash_page_size;
	dfu_data.flash_upload_size = flash_upload_size;

	/* Initialize the USB driver with the right configuration */
	ret = usb_set_config(&dfu_config);
	if (ret < 0) {
		DBG("Failed to config USB\n");
		return ret;
	}

	/* Enable USB driver */
	ret = usb_enable(&dfu_config);
	if (ret < 0) {
		DBG("Failed to enable USB\n");
		return ret;
	}

	return 0;
}
