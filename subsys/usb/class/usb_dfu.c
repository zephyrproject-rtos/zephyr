/*
 * Copyright (c) 2017 PHYTEC Messtechnik GmbH
 *
 * This file is derived from samples/subsys/usb/dfu/src/usb_dfu.c
 * that is Copyright (c) 2015, 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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

#include <init.h>
#include <kernel.h>
#include <stdio.h>
#include <errno.h>
#include <flash.h>
#include <misc/byteorder.h>
#include <usb/usb_device.h>
#include <usb/usb_common.h>
#include <usb/usb_dfu.h>
#include "../usb_descriptor.h"
#include "../composite.h"

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_USB_LEVEL
#include <logging/sys_log.h>

#define NUMOF_ALTERNATE_SETTINGS	3

#define IMAGE_0_DESC_LENGTH	(sizeof(FLASH_AREA_IMAGE_0_LABEL) * 2)
#define IMAGE_0_UC_IDX_MAX	(IMAGE_0_DESC_LENGTH - 3)
#define IMAGE_0_STRING_IDX_MAX	(sizeof(FLASH_AREA_IMAGE_0_LABEL) - 2)

#define IMAGE_1_DESC_LENGTH	(sizeof(FLASH_AREA_IMAGE_1_LABEL) * 2)
#define IMAGE_1_UC_IDX_MAX	(IMAGE_1_DESC_LENGTH - 3)
#define IMAGE_1_STRING_IDX_MAX	(sizeof(FLASH_AREA_IMAGE_1_LABEL) - 2)

#define IMAGE_2_DESC_LENGTH	(sizeof(FLASH_AREA_IMAGE_SCRATCH_LABEL) * 2)
#define IMAGE_2_UC_IDX_MAX	(IMAGE_2_DESC_LENGTH - 3)
#define IMAGE_2_STRING_IDX_MAX	(sizeof(FLASH_AREA_IMAGE_SCRATCH_LABEL) - 2)

#ifdef CONFIG_USB_COMPOSITE_DEVICE
#define USB_DFU_MAX_XFER_SIZE		CONFIG_USB_COMPOSITE_BUFFER_SIZE
#else
#define USB_DFU_MAX_XFER_SIZE		CONFIG_USB_DFU_MAX_XFER_SIZE
#endif

struct dev_dfu_mode_descriptor {
	struct usb_device_descriptor device_descriptor;
	struct usb_cfg_descriptor cfg_descr;
	struct usb_dfu_config {
		struct usb_if_descriptor if0;
		struct usb_if_descriptor if1;
		struct usb_if_descriptor if2;
		struct dfu_runtime_descriptor dfu_descr;
	} __packed dfu_cfg;
	struct usb_string_desription {
		struct usb_string_descriptor lang_descr;
		struct usb_mfr_descriptor {
			u8_t bLength;
			u8_t bDescriptorType;
			u8_t bString[MFR_DESC_LENGTH - 2];
		} __packed unicode_mfr;

		struct usb_product_descriptor {
			u8_t bLength;
			u8_t bDescriptorType;
			u8_t bString[PRODUCT_DESC_LENGTH - 2];
		} __packed unicode_product;

		struct usb_sn_descriptor {
			u8_t bLength;
			u8_t bDescriptorType;
			u8_t bString[SN_DESC_LENGTH - 2];
		} __packed unicode_sn;

		struct image_0_descriptor {
			u8_t bLength;
			u8_t bDescriptorType;
			u8_t bString[IMAGE_0_DESC_LENGTH - 2];
		} __packed unicode_image0;

		struct image_1_descriptor {
			u8_t bLength;
			u8_t bDescriptorType;
			u8_t bString[IMAGE_1_DESC_LENGTH - 2];
		} __packed unicode_image1;

		struct image_scratch_descriptor {
			u8_t bLength;
			u8_t bDescriptorType;
			u8_t bString[IMAGE_2_DESC_LENGTH - 2];
		} __packed unicode_image2;
	} __packed string_descr;

	struct usb_desc_header term_descr;
} __packed;

static struct dev_dfu_mode_descriptor dfu_mode_desc = {
	/* Device descriptor */
	.device_descriptor = {
		.bLength = sizeof(struct usb_device_descriptor),
		.bDescriptorType = USB_DEVICE_DESC,
		.bcdUSB = sys_cpu_to_le16(USB_1_1),
		.bDeviceClass = 0,
		.bDeviceSubClass = 0,
		.bDeviceProtocol = 0,
		.bMaxPacketSize0 = MAX_PACKET_SIZE0,
		.idVendor = sys_cpu_to_le16((u16_t)CONFIG_USB_DEVICE_VID),
		.idProduct = sys_cpu_to_le16((u16_t)CONFIG_USB_DEVICE_PID),
		.bcdDevice = sys_cpu_to_le16(BCDDEVICE_RELNUM),
		.iManufacturer = 1,
		.iProduct = 2,
		.iSerialNumber = 3,
		.bNumConfigurations = 1,
	},
	/* Configuration descriptor */
	.cfg_descr = {
		.bLength = sizeof(struct usb_cfg_descriptor),
		.bDescriptorType = USB_CONFIGURATION_DESC,
		.wTotalLength = sizeof(struct dev_dfu_mode_descriptor)
			      - sizeof(struct usb_device_descriptor)
			      - sizeof(struct usb_string_desription)
			      - sizeof(struct usb_desc_header),
		.bNumInterfaces = 1,
		.bConfigurationValue = 1,
		.iConfiguration = 0,
		.bmAttributes = USB_CONFIGURATION_ATTRIBUTES,
		.bMaxPower = MAX_LOW_POWER,
	},
	.dfu_cfg = {
		/* Interface descriptor */
		.if0 = {
			.bLength = sizeof(struct usb_if_descriptor),
			.bDescriptorType = USB_INTERFACE_DESC,
			.bInterfaceNumber = 0,
			.bAlternateSetting = 0,
			.bNumEndpoints = 0,
			.bInterfaceClass = DFU_DEVICE_CLASS,
			.bInterfaceSubClass = DFU_SUBCLASS,
			.bInterfaceProtocol = DFU_MODE_PROTOCOL,
			.iInterface = 4,
		},
		.if1 = {
			.bLength = sizeof(struct usb_if_descriptor),
			.bDescriptorType = USB_INTERFACE_DESC,
			.bInterfaceNumber = 0,
			.bAlternateSetting = 1,
			.bNumEndpoints = 0,
			.bInterfaceClass = DFU_DEVICE_CLASS,
			.bInterfaceSubClass = DFU_SUBCLASS,
			.bInterfaceProtocol = DFU_MODE_PROTOCOL,
			.iInterface = 5,
		},
		.if2 = {
			.bLength = sizeof(struct usb_if_descriptor),
			.bDescriptorType = USB_INTERFACE_DESC,
			.bInterfaceNumber = 0,
			.bAlternateSetting = 2,
			.bNumEndpoints = 0,
			.bInterfaceClass = DFU_DEVICE_CLASS,
			.bInterfaceSubClass = DFU_SUBCLASS,
			.bInterfaceProtocol = DFU_MODE_PROTOCOL,
			.iInterface = 6,
		},
		.dfu_descr = {
			.bLength = sizeof(struct dfu_runtime_descriptor),
			.bDescriptorType = DFU_FUNC_DESC,
			.bmAttributes = DFU_ATTR_CAN_DNLOAD |
					DFU_ATTR_CAN_UPLOAD |
					DFU_ATTR_MANIFESTATION_TOLERANT,
			.wDetachTimeOut =
				sys_cpu_to_le16(CONFIG_USB_DFU_DETACH_TIMEOUT),
			.wTransferSize =
				sys_cpu_to_le16(USB_DFU_MAX_XFER_SIZE),
			.bcdDFUVersion =
				sys_cpu_to_le16(DFU_VERSION),
		},
	},
	.string_descr = {
		.lang_descr = {
			.bLength = sizeof(struct usb_string_descriptor),
			.bDescriptorType = USB_STRING_DESC,
			.bString = sys_cpu_to_le16(0x0409),
		},
		/* Manufacturer String Descriptor */
		.unicode_mfr = {
			.bLength = MFR_DESC_LENGTH,
			.bDescriptorType = USB_STRING_DESC,
			.bString = CONFIG_USB_DEVICE_MANUFACTURER,
		},
		/* Product String Descriptor */
		.unicode_product = {
			.bLength = PRODUCT_DESC_LENGTH,
			.bDescriptorType = USB_STRING_DESC,
			.bString = CONFIG_USB_DEVICE_PRODUCT,
		},
		/* Serial Number String Descriptor */
		.unicode_sn = {
			.bLength = SN_DESC_LENGTH,
			.bDescriptorType = USB_STRING_DESC,
			.bString = CONFIG_USB_DEVICE_SN,
		},
		/* Image 0 String Descriptor */
		.unicode_image0 = {
			.bLength = IMAGE_0_DESC_LENGTH,
			.bDescriptorType = USB_STRING_DESC,
			.bString = FLASH_AREA_IMAGE_0_LABEL,
		},
		/* Image 1 String Descriptor */
		.unicode_image1 = {
			.bLength = IMAGE_1_DESC_LENGTH,
			.bDescriptorType = USB_STRING_DESC,
			.bString = FLASH_AREA_IMAGE_1_LABEL,
		},
		/* Image Scratch String Descriptor */
		.unicode_image2 = {
			.bLength = IMAGE_2_DESC_LENGTH,
			.bDescriptorType = USB_STRING_DESC,
			.bString = FLASH_AREA_IMAGE_SCRATCH_LABEL,
		},
	},
	.term_descr = {
		.bLength = 0,
		.bDescriptorType = 0,
	},
};
static struct usb_cfg_data dfu_config;

/* Device data structure */
struct dfu_data_t {
	/* Flash device to read/write data from/to */
	struct device *flash_dev;
	/* Flash layout data (image-0, image-1, image-scratch) */
	u32_t flash_addr;
	u32_t flash_page_size;
	u32_t flash_upload_size;
	/* Number of bytes sent during upload */
	u32_t bytes_sent;
	/* Number of bytes received during download */
	u32_t bytes_rcvd;
	u32_t alt_setting;              /* DFU alternate setting */
#ifdef CONFIG_USB_COMPOSITE_DEVICE
	u8_t *buffer;
#else
	u8_t buffer[USB_DFU_MAX_XFER_SIZE]; /* DFU data buffer */
#endif
	enum dfu_state state;              /* State of the DFU device */
	enum dfu_status status;            /* Status of the DFU device */
	u16_t block_nr;                 /* DFU block number */
};

static struct dfu_data_t dfu_data = {
	.state = appIDLE,
	.status = statusOK,
	.flash_addr = CONFIG_FLASH_BASE_ADDRESS + FLASH_AREA_IMAGE_0_OFFSET,
	.flash_upload_size = FLASH_AREA_IMAGE_0_SIZE,
	.flash_page_size = CONFIG_FLASH_SIZE,
	.alt_setting = 0,
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
		s32_t *data_len, u8_t **data)
{
	int ret;
	u32_t len, bytes_left;

	switch (pSetup->bRequest) {
	case DFU_GETSTATUS:
		SYS_LOG_DBG("DFU_GETSTATUS: status %d, state %d",
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
		SYS_LOG_DBG("DFU_GETSTATE");
		(*data)[0] = dfu_data.state;
		*data_len = 1;
		break;

	case DFU_ABORT:
		SYS_LOG_DBG("DFU_ABORT");

		if (dfu_check_app_state())
			return -EINVAL;

		dfu_reset_counters();
		dfu_data.state = dfuIDLE;
		dfu_data.status = statusOK;
		break;

	case DFU_CLRSTATUS:
		SYS_LOG_DBG("DFU_CLRSTATUS");

		if (dfu_check_app_state())
			return -EINVAL;

		dfu_data.state = dfuIDLE;
		dfu_data.status = statusOK;
		break;

	case DFU_DNLOAD:
		SYS_LOG_DBG("DFU_DNLOAD block %d, len %d, state %d",
		    pSetup->wValue, pSetup->wLength, dfu_data.state);

		if (dfu_check_app_state())
			return -EINVAL;

		switch (dfu_data.state) {
		case dfuIDLE:
			dfu_reset_counters();
			SYS_LOG_DBG("DFU_DNLOAD start");
		case dfuDNLOAD_IDLE:
			if (!pSetup->wLength) {
				/* Download completed */
				dfu_data.state = dfuMANIFEST_SYNC;
				dfu_reset_counters();
				break;
			}

			/* Download has started */
			dfu_data.state = dfuDNLOAD_IDLE;

			if (!(dfu_data.bytes_rcvd % dfu_data.flash_page_size)) {
				ret = flash_erase(dfu_data.flash_dev,
				      dfu_data.flash_addr + dfu_data.bytes_rcvd,
				      dfu_data.flash_page_size);
				SYS_LOG_DBG("Flash erase");
				if (ret) {
					dfu_data.state = dfuERROR;
					dfu_data.status = errERASE;
					SYS_LOG_DBG("DFU flash erase error, "
						    "ret %d", ret);
				}
			}

			/* Flash write len should be multiple of 4 */
			len = (pSetup->wLength + 3) & (~3);
			ret = flash_write(dfu_data.flash_dev,
				dfu_data.flash_addr + dfu_data.bytes_rcvd,
				*data, len);
			if (ret) {
				dfu_data.state = dfuERROR;
				dfu_data.status = errWRITE;
				SYS_LOG_DBG("DFU flash write error, ret %d",
					    ret);
			} else
				dfu_data.bytes_rcvd += pSetup->wLength;
			break;
		default:
			SYS_LOG_DBG("DFU_DNLOAD wrong state %d",
				    dfu_data.state);
			dfu_data.state = dfuERROR;
			dfu_data.status = errUNKNOWN;
			dfu_reset_counters();
			return -EINVAL;
		}
		break;
	case DFU_UPLOAD:
		SYS_LOG_DBG("DFU_UPLOAD block %d, len %d, state %d",
		    pSetup->wValue, pSetup->wLength, dfu_data.state);

		if (dfu_check_app_state())
			return -EINVAL;

		switch (dfu_data.state) {
		case dfuIDLE:
			dfu_reset_counters();
			SYS_LOG_DBG("DFU_UPLOAD start");
		case dfuUPLOAD_IDLE:
			if (!pSetup->wLength ||
			    dfu_data.block_nr != pSetup->wValue) {
				SYS_LOG_DBG("DFU_UPLOAD block %d, expected %d, "
				    "len %d", pSetup->wValue,
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
						 dfu_data.flash_addr +
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
			SYS_LOG_DBG("DFU_UPLOAD wrong state %d",
				    dfu_data.state);
			dfu_data.state = dfuERROR;
			dfu_data.status = errUNKNOWN;
			dfu_reset_counters();
			return -EINVAL;
		}
		break;
	case DFU_DETACH:
		SYS_LOG_DBG("DFU_DETACH timeout %d, state %d",
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
		dfu_config.usb_device_description = (u8_t *) &dfu_mode_desc;
		if (usb_set_config(&dfu_config) != 0) {
			SYS_LOG_ERR("usb_set_config failed in DFU_DETACH");
			return -EIO;
		}
		break;
	default:
		SYS_LOG_DBG("DFU UNKNOWN STATE: %d", pSetup->bRequest);
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
		SYS_LOG_DBG("USB device error");
		break;
	case USB_DC_RESET:
		SYS_LOG_DBG("USB device reset detected, state %d",
			    dfu_data.state);
		if (dfu_data.state == appDETACH) {
			dfu_data.state = dfuIDLE;
		}
		break;
	case USB_DC_CONNECTED:
		SYS_LOG_DBG("USB device connected");
		break;
	case USB_DC_CONFIGURED:
		SYS_LOG_DBG("USB device configured");
		break;
	case USB_DC_DISCONNECTED:
		SYS_LOG_DBG("USB device disconnected");
		break;
	case USB_DC_SUSPEND:
		SYS_LOG_DBG("USB device supended");
		break;
	case USB_DC_RESUME:
		SYS_LOG_DBG("USB device resumed");
		break;
	case USB_DC_UNKNOWN:
	default:
		SYS_LOG_DBG("USB unknown state");
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
		s32_t *data_len, u8_t **data)
{
	ARG_UNUSED(data);

	if (REQTYPE_GET_RECIP(pSetup->bmRequestType) ==
	    REQTYPE_RECIP_INTERFACE) {
		if (pSetup->bRequest == REQ_SET_INTERFACE) {
			SYS_LOG_DBG("DFU alternate setting %d", pSetup->wValue);

			switch (pSetup->wValue) {
			case 0:
				dfu_data.flash_addr =
					CONFIG_FLASH_BASE_ADDRESS +
					FLASH_AREA_IMAGE_0_OFFSET;
				dfu_data.flash_upload_size =
					FLASH_AREA_IMAGE_0_SIZE;
				break;
			case 1:
				dfu_data.flash_addr =
					CONFIG_FLASH_BASE_ADDRESS +
					FLASH_AREA_IMAGE_1_OFFSET;
				dfu_data.flash_upload_size =
					FLASH_AREA_IMAGE_1_SIZE;
				break;
			case 2:
				dfu_data.flash_addr =
					CONFIG_FLASH_BASE_ADDRESS +
					FLASH_AREA_IMAGE_SCRATCH_OFFSET;
				dfu_data.flash_upload_size =
					FLASH_AREA_IMAGE_SCRATCH_SIZE;
				break;
			default:
				SYS_LOG_DBG("Invalid DFU alternate setting");
				return -ENOTSUP;
			}
			dfu_data.alt_setting = pSetup->wValue;
			*data_len = 0;
			return 0;
		}
	}

	/* Not handled by us */
	return -ENOTSUP;
}

/* Configuration of the DFU Device send to the USB Driver */
static struct usb_cfg_data dfu_config = {
	.usb_device_description = NULL,
	.cb_usb_status = dfu_status_cb,
	.interface = {
		.class_handler = dfu_class_handle_req,
		.custom_handler = dfu_custom_handle_req,
		.payload_data = NULL,
	},
	.num_endpoints = 0,
};

static int usb_dfu_init(struct device *dev)
{
	int ret;

	ARG_UNUSED(dev);

	usb_fix_unicode_string(MFR_UC_IDX_MAX, MFR_STRING_IDX_MAX,
		(u8_t *)dfu_mode_desc.string_descr.unicode_mfr.bString);

	usb_fix_unicode_string(PRODUCT_UC_IDX_MAX, PRODUCT_STRING_IDX_MAX,
		(u8_t *)dfu_mode_desc.string_descr.unicode_product.bString);

	usb_fix_unicode_string(SN_UC_IDX_MAX, SN_STRING_IDX_MAX,
		(u8_t *)dfu_mode_desc.string_descr.unicode_sn.bString);

	usb_fix_unicode_string(IMAGE_0_UC_IDX_MAX, IMAGE_0_STRING_IDX_MAX,
		(u8_t *)dfu_mode_desc.string_descr.unicode_image0.bString);

	usb_fix_unicode_string(IMAGE_1_UC_IDX_MAX, IMAGE_1_STRING_IDX_MAX,
		(u8_t *)dfu_mode_desc.string_descr.unicode_image1.bString);

	usb_fix_unicode_string(IMAGE_2_UC_IDX_MAX, IMAGE_2_STRING_IDX_MAX,
		(u8_t *)dfu_mode_desc.string_descr.unicode_image2.bString);

	dfu_data.flash_dev = device_get_binding(CONFIG_USB_DFU_FLASH_DEVICE);
	if (!dfu_data.flash_dev) {
		SYS_LOG_ERR("Flash device not found\n");
		return -ENODEV;
	}

#ifdef CONFIG_USB_COMPOSITE_DEVICE
	ret = composite_add_function(&dfu_config, FIRST_IFACE_DFU);
	if (ret < 0) {
		SYS_LOG_ERR("Failed to add a function");
		return ret;
	}
	dfu_data.buffer = dfu_config.interface.payload_data;
#else
	dfu_config.interface.payload_data = dfu_data.buffer;
	dfu_config.usb_device_description =
		usb_get_device_descriptor(0);

	/* Initialize the USB driver with the right configuration */
	ret = usb_set_config(&dfu_config);
	if (ret < 0) {
		SYS_LOG_DBG("Failed to config USB");
		return ret;
	}

	/* Enable USB driver */
	ret = usb_enable(&dfu_config);
	if (ret < 0) {
		SYS_LOG_DBG("Failed to enable USB");
		return ret;
	}
#endif

	return 0;
}

SYS_INIT(usb_dfu_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
