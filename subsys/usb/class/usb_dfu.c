/*******************************************************************************
 *
 * Copyright(c) 2015,2016 Intel Corporation.
 * Copyright(c) 2017 PHYTEC Messtechnik GmbH
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
#include <drivers/flash.h>
#include <storage/flash_map.h>
#include <dfu/mcuboot.h>
#include <dfu/flash_img.h>
#include <sys/byteorder.h>
#include <usb/usb_device.h>
#include <usb/usb_common.h>
#include <usb/class/usb_dfu.h>
#include <usb_descriptor.h>
#include <usb_work_q.h>

#define LOG_LEVEL CONFIG_USB_DEVICE_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(usb_dfu);

#define NUMOF_ALTERNATE_SETTINGS	2

#define USB_DFU_MAX_XFER_SIZE		CONFIG_USB_REQUEST_BUFFER_SIZE

#define FIRMWARE_IMAGE_0_LABEL "image-0"
#define FIRMWARE_IMAGE_1_LABEL "image-1"

/* MCUBoot waits for CONFIG_USB_DFU_WAIT_DELAY_MS time in total to let DFU to
 * be commenced. It intermittently checks every INTERMITTENT_CHECK_DELAY
 * milliseconds to see if DFU has started.
 */
#define INTERMITTENT_CHECK_DELAY	50

static struct k_poll_event dfu_event;
static struct k_poll_signal dfu_signal;

static struct k_work dfu_work;

struct dfu_worker_data_t {
	uint8_t buf[USB_DFU_MAX_XFER_SIZE];
	enum dfu_state worker_state;
	uint16_t worker_len;
};

static struct dfu_worker_data_t dfu_data_worker;

struct usb_dfu_config {
	struct usb_if_descriptor if0;
	struct dfu_runtime_descriptor dfu_descr;
} __packed;

USBD_CLASS_DESCR_DEFINE(primary, 0) struct usb_dfu_config dfu_cfg = {
	/* Interface descriptor */
	.if0 = {
		.bLength = sizeof(struct usb_if_descriptor),
		.bDescriptorType = USB_INTERFACE_DESC,
		.bInterfaceNumber = 0,
		.bAlternateSetting = 0,
		.bNumEndpoints = 0,
		.bInterfaceClass = DFU_DEVICE_CLASS,
		.bInterfaceSubClass = DFU_SUBCLASS,
		.bInterfaceProtocol = DFU_RT_PROTOCOL,
		.iInterface = 0,
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
};

/* dfu mode device descriptor */

struct dev_dfu_mode_descriptor {
	struct usb_device_descriptor device_descriptor;
	struct usb_cfg_descriptor cfg_descr;
	struct usb_sec_dfu_config {
		struct usb_if_descriptor if0;
		struct usb_if_descriptor if1;
		struct dfu_runtime_descriptor dfu_descr;
	} __packed sec_dfu_cfg;
} __packed;


USBD_DEVICE_DESCR_DEFINE(secondary)
struct dev_dfu_mode_descriptor dfu_mode_desc = {
	/* Device descriptor */
	.device_descriptor = {
		.bLength = sizeof(struct usb_device_descriptor),
		.bDescriptorType = USB_DEVICE_DESC,
		.bcdUSB = sys_cpu_to_le16(USB_2_0),
		.bDeviceClass = 0,
		.bDeviceSubClass = 0,
		.bDeviceProtocol = 0,
		.bMaxPacketSize0 = USB_MAX_CTRL_MPS,
		.idVendor = sys_cpu_to_le16((uint16_t)CONFIG_USB_DEVICE_VID),
		.idProduct = sys_cpu_to_le16((uint16_t)CONFIG_USB_DEVICE_PID),
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
		.wTotalLength = 0,
		.bNumInterfaces = 1,
		.bConfigurationValue = 1,
		.iConfiguration = 0,
		.bmAttributes = USB_CONFIGURATION_ATTRIBUTES,
		.bMaxPower = CONFIG_USB_MAX_POWER,
	},
	.sec_dfu_cfg = {
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
};

struct usb_string_desription {
	struct usb_string_descriptor lang_descr;
	struct usb_mfr_descriptor {
		uint8_t bLength;
		uint8_t bDescriptorType;
		uint8_t bString[USB_BSTRING_LENGTH(
				CONFIG_USB_DEVICE_MANUFACTURER)];
	} __packed utf16le_mfr;

	struct usb_product_descriptor {
		uint8_t bLength;
		uint8_t bDescriptorType;
		uint8_t bString[USB_BSTRING_LENGTH(CONFIG_USB_DEVICE_PRODUCT)];
	} __packed utf16le_product;

	struct usb_sn_descriptor {
		uint8_t bLength;
		uint8_t bDescriptorType;
		uint8_t bString[USB_BSTRING_LENGTH(CONFIG_USB_DEVICE_SN)];
	} __packed utf16le_sn;

	struct image_0_descriptor {
		uint8_t bLength;
		uint8_t bDescriptorType;
		uint8_t bString[USB_BSTRING_LENGTH(FIRMWARE_IMAGE_0_LABEL)];
	} __packed utf16le_image0;

	struct image_1_descriptor {
		uint8_t bLength;
		uint8_t bDescriptorType;
		uint8_t bString[USB_BSTRING_LENGTH(FIRMWARE_IMAGE_1_LABEL)];
	} __packed utf16le_image1;
} __packed;

USBD_STRING_DESCR_DEFINE(secondary)
struct usb_string_desription string_descr = {
	.lang_descr = {
		.bLength = sizeof(struct usb_string_descriptor),
		.bDescriptorType = USB_STRING_DESC,
		.bString = sys_cpu_to_le16(0x0409),
	},
	/* Manufacturer String Descriptor */
	.utf16le_mfr = {
		.bLength = USB_STRING_DESCRIPTOR_LENGTH(
				CONFIG_USB_DEVICE_MANUFACTURER),
		.bDescriptorType = USB_STRING_DESC,
		.bString = CONFIG_USB_DEVICE_MANUFACTURER,
	},
	/* Product String Descriptor */
	.utf16le_product = {
		.bLength = USB_STRING_DESCRIPTOR_LENGTH(
				CONFIG_USB_DEVICE_PRODUCT),
		.bDescriptorType = USB_STRING_DESC,
		.bString = CONFIG_USB_DEVICE_PRODUCT,
	},
	/* Serial Number String Descriptor */
	.utf16le_sn = {
		.bLength = USB_STRING_DESCRIPTOR_LENGTH(CONFIG_USB_DEVICE_SN),
		.bDescriptorType = USB_STRING_DESC,
		.bString = CONFIG_USB_DEVICE_SN,
	},
	/* Image 0 String Descriptor */
	.utf16le_image0 = {
		.bLength = USB_STRING_DESCRIPTOR_LENGTH(
				FIRMWARE_IMAGE_0_LABEL),
		.bDescriptorType = USB_STRING_DESC,
		.bString = FIRMWARE_IMAGE_0_LABEL,
	},
	/* Image 1 String Descriptor */
	.utf16le_image1 = {
		.bLength = USB_STRING_DESCRIPTOR_LENGTH(
				FIRMWARE_IMAGE_1_LABEL),
		.bDescriptorType = USB_STRING_DESC,
		.bString = FIRMWARE_IMAGE_1_LABEL,
	},
};

/* This element marks the end of the entire descriptor. */
USBD_TERM_DESCR_DEFINE(secondary) struct usb_desc_header term_descr = {
	.bLength = 0,
	.bDescriptorType = 0,
};

static struct usb_cfg_data dfu_config;

/* Device data structure */
struct dfu_data_t {
	uint8_t flash_area_id;
	uint32_t flash_upload_size;
	/* Number of bytes sent during upload */
	uint32_t bytes_sent;
	uint32_t alt_setting;              /* DFU alternate setting */
	struct flash_img_context ctx;
	enum dfu_state state;              /* State of the DFU device */
	enum dfu_status status;            /* Status of the DFU device */
	uint16_t block_nr;                 /* DFU block number */
	uint16_t bwPollTimeout;
};

static struct dfu_data_t dfu_data = {
	.state = appIDLE,
	.status = statusOK,
	.flash_area_id = FLASH_AREA_ID(image_1),
	.alt_setting = 0,
	.bwPollTimeout = CONFIG_USB_DFU_DEFAULT_POLLTIMEOUT,
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
	dfu_data.bytes_sent = 0U;
	dfu_data.block_nr = 0U;
	if (flash_img_init(&dfu_data.ctx)) {
		LOG_ERR("flash img init error");
		dfu_data.state = dfuERROR;
		dfu_data.status = errUNKNOWN;
	}
}

static void dfu_flash_write(uint8_t *data, size_t len)
{
	bool flush = false;

	if (!len) {
		/* Download completed */
		flush = true;
	}

	if (flash_img_buffered_write(&dfu_data.ctx, data, len, flush)) {
		LOG_ERR("flash write error");
		dfu_data.state = dfuERROR;
		dfu_data.status = errWRITE;
	} else if (!len) {
		LOG_DBG("flash write done");
		dfu_data.state = dfuMANIFEST_SYNC;
		dfu_reset_counters();
		if (boot_request_upgrade(false)) {
			dfu_data.state = dfuERROR;
			dfu_data.status = errWRITE;
		}
	} else {
		dfu_data.state = dfuDNLOAD_IDLE;
	}

	LOG_DBG("bytes written 0x%x", flash_img_bytes_written(&dfu_data.ctx));
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
		LOG_DBG("DFU_GETSTATUS: status %d, state %d",
			dfu_data.status, dfu_data.state);

		if (dfu_data.state == dfuMANIFEST_SYNC) {
			dfu_data.state = dfuIDLE;
		}

		/* bStatus */
		(*data)[0] = dfu_data.status;
		/* bwPollTimeout */
		sys_put_le16(dfu_data.bwPollTimeout, &(*data)[1]);
		(*data)[3] = 0U;
		/* bState */
		(*data)[4] = dfu_data.state;
		/* iString */
		(*data)[5] = 0U;
		*data_len = 6;
		break;

	case DFU_GETSTATE:
		LOG_DBG("DFU_GETSTATE");
		(*data)[0] = dfu_data.state;
		*data_len = 1;
		break;

	case DFU_ABORT:
		LOG_DBG("DFU_ABORT");

		if (dfu_check_app_state()) {
			return -EINVAL;
		}

		dfu_reset_counters();
		dfu_data.state = dfuIDLE;
		dfu_data.status = statusOK;
		break;

	case DFU_CLRSTATUS:
		LOG_DBG("DFU_CLRSTATUS");

		if (dfu_check_app_state()) {
			return -EINVAL;
		}

		dfu_data.state = dfuIDLE;
		dfu_data.status = statusOK;
		break;

	case DFU_DNLOAD:
		LOG_DBG("DFU_DNLOAD block %d, len %d, state %d",
			pSetup->wValue, pSetup->wLength, dfu_data.state);

		if (dfu_check_app_state()) {
			return -EINVAL;
		}

		switch (dfu_data.state) {
		case dfuIDLE:
			LOG_DBG("DFU_DNLOAD start");
			dfu_reset_counters();
			k_poll_signal_reset(&dfu_signal);

			if (dfu_data.flash_area_id !=
			    FLASH_AREA_ID(image_1)) {
				dfu_data.status = errWRITE;
				dfu_data.state = dfuERROR;
				LOG_ERR("This area can not be overwritten");
				break;
			}

			dfu_data.state = dfuDNBUSY;
			dfu_data_worker.worker_state = dfuIDLE;
			dfu_data_worker.worker_len  = pSetup->wLength;
			memcpy(dfu_data_worker.buf, *data, pSetup->wLength);
			k_work_submit_to_queue(&USB_WORK_Q, &dfu_work);
			break;
		case dfuDNLOAD_IDLE:
			dfu_data.state = dfuDNBUSY;
			dfu_data_worker.worker_state = dfuDNLOAD_IDLE;
			dfu_data_worker.worker_len  = pSetup->wLength;
			if (dfu_data_worker.worker_len == 0U) {
				dfu_data.state = dfuMANIFEST_SYNC;
				k_poll_signal_raise(&dfu_signal, 0);
			}

			memcpy(dfu_data_worker.buf, *data, pSetup->wLength);
			k_work_submit_to_queue(&USB_WORK_Q, &dfu_work);
			break;
		default:
			LOG_ERR("DFU_DNLOAD wrong state %d", dfu_data.state);
			dfu_data.state = dfuERROR;
			dfu_data.status = errUNKNOWN;
			dfu_reset_counters();
			return -EINVAL;
		}
		break;
	case DFU_UPLOAD:
		LOG_DBG("DFU_UPLOAD block %d, len %d, state %d",
			pSetup->wValue, pSetup->wLength, dfu_data.state);

		if (dfu_check_app_state()) {
			return -EINVAL;
		}

		switch (dfu_data.state) {
		case dfuIDLE:
			dfu_reset_counters();
			LOG_DBG("DFU_UPLOAD start");
		case dfuUPLOAD_IDLE:
			if (!pSetup->wLength ||
			    dfu_data.block_nr != pSetup->wValue) {
				LOG_DBG("DFU_UPLOAD block %d, expected %d, "
					"len %d", pSetup->wValue,
					dfu_data.block_nr, pSetup->wLength);
				dfu_data.state = dfuERROR;
				dfu_data.status = errUNKNOWN;
				break;
			}

			/* Upload in progress */
			bytes_left = dfu_data.flash_upload_size -
				     dfu_data.bytes_sent;
			if (bytes_left < pSetup->wLength) {
				len = bytes_left;
			} else {
				len = pSetup->wLength;
			}

			if (len > USB_DFU_MAX_XFER_SIZE) {
				/*
				 * The host could requests more data as stated
				 * in wTransferSize. Limit upload length to the
				 * size of the request-buffer.
				 */
				len = USB_DFU_MAX_XFER_SIZE;
			}

			if (len) {
				const struct flash_area *fa;

				ret = flash_area_open(dfu_data.flash_area_id,
						      &fa);
				if (ret) {
					dfu_data.state = dfuERROR;
					dfu_data.status = errFILE;
					break;
				}
				ret = flash_area_read(fa, dfu_data.bytes_sent,
						      *data, len);
				flash_area_close(fa);
				if (ret) {
					dfu_data.state = dfuERROR;
					dfu_data.status = errFILE;
					break;
				}
			}
			*data_len = len;

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
			LOG_ERR("DFU_UPLOAD wrong state %d", dfu_data.state);
			dfu_data.state = dfuERROR;
			dfu_data.status = errUNKNOWN;
			dfu_reset_counters();
			return -EINVAL;
		}
		break;
	case DFU_DETACH:
		LOG_DBG("DFU_DETACH timeout %d, state %d",
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
		dfu_config.usb_device_description = (uint8_t *) &dfu_mode_desc;
		if (usb_set_config(dfu_config.usb_device_description) != 0) {
			LOG_ERR("usb_set_config failed in DFU_DETACH");
			return -EIO;
		}
		break;
	default:
		LOG_WRN("DFU UNKNOWN STATE: %d", pSetup->bRequest);
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
static void dfu_status_cb(struct usb_cfg_data *cfg,
			  enum usb_dc_status_code status,
			  const uint8_t *param)
{
	ARG_UNUSED(param);
	ARG_UNUSED(cfg);

	/* Check the USB status and do needed action if required */
	switch (status) {
	case USB_DC_ERROR:
		LOG_DBG("USB device error");
		break;
	case USB_DC_RESET:
		LOG_DBG("USB device reset detected, state %d", dfu_data.state);
		if (dfu_data.state == appDETACH) {
			dfu_data.state = dfuIDLE;
		}
		break;
	case USB_DC_CONNECTED:
		LOG_DBG("USB device connected");
		break;
	case USB_DC_CONFIGURED:
		LOG_DBG("USB device configured");
		break;
	case USB_DC_DISCONNECTED:
		LOG_DBG("USB device disconnected");
		break;
	case USB_DC_SUSPEND:
		LOG_DBG("USB device supended");
		break;
	case USB_DC_RESUME:
		LOG_DBG("USB device resumed");
		break;
	case USB_DC_SOF:
		break;
	case USB_DC_UNKNOWN:
	default:
		LOG_DBG("USB unknown state");
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
	ARG_UNUSED(data);

	if (REQTYPE_GET_RECIP(pSetup->bmRequestType) ==
	    REQTYPE_RECIP_INTERFACE) {
		if (pSetup->bRequest == REQ_SET_INTERFACE) {
			LOG_DBG("DFU alternate setting %d", pSetup->wValue);

			const struct flash_area *fa;

			switch (pSetup->wValue) {
			case 0:
				dfu_data.flash_area_id =
				    FLASH_AREA_ID(image_0);
				break;
			case 1:
				dfu_data.flash_area_id =
				    FLASH_AREA_ID(image_1);
				break;
			default:
				LOG_WRN("Invalid DFU alternate setting");
				return -ENOTSUP;
			}

			if (flash_area_open(dfu_data.flash_area_id, &fa)) {
				return -EIO;
			}

			dfu_data.flash_upload_size = fa->fa_size;
			flash_area_close(fa);

			dfu_data.alt_setting = pSetup->wValue;
			*data_len = 0;
			return 0;
		}
	}

	/* Not handled by us */
	return -EINVAL;
}

static void dfu_interface_config(struct usb_desc_header *head,
				 uint8_t bInterfaceNumber)
{
	ARG_UNUSED(head);

	dfu_cfg.if0.bInterfaceNumber = bInterfaceNumber;
}

/* Configuration of the DFU Device send to the USB Driver */
USBD_CFG_DATA_DEFINE(primary, dfu) struct usb_cfg_data dfu_config = {
	.usb_device_description = NULL,
	.interface_config = dfu_interface_config,
	.interface_descriptor = &dfu_cfg.if0,
	.cb_usb_status = dfu_status_cb,
	.interface = {
		.class_handler = dfu_class_handle_req,
		.custom_handler = dfu_custom_handle_req,
	},
	.num_endpoints = 0,
};

/*
 * Dummy configuration, this is necessary to configure DFU mode descriptor
 * which is an alternative (secondary) device descriptor.
 */
USBD_CFG_DATA_DEFINE(secondary, dfu) struct usb_cfg_data dfu_mode_config = {
	.usb_device_description = NULL,
	.interface_config = NULL,
	.interface_descriptor = &dfu_mode_desc.sec_dfu_cfg.if0,
	.cb_usb_status = dfu_status_cb,
	.interface = {
		.class_handler = dfu_class_handle_req,
		.custom_handler = dfu_custom_handle_req,
	},
	.num_endpoints = 0,
};

static void dfu_work_handler(struct k_work *item)
{
	ARG_UNUSED(item);

	switch (dfu_data_worker.worker_state) {
	case dfuIDLE:
/*
 * If progressive erase is enabled, then erase take place while
 * image collection, so not erase whole bank at DFU beginning
 */
#ifndef CONFIG_IMG_ERASE_PROGRESSIVELY
		if (boot_erase_img_bank(FLASH_AREA_ID(image_1))) {
			dfu_data.state = dfuERROR;
			dfu_data.status = errERASE;
			break;
		}
#endif
	case dfuDNLOAD_IDLE:
		dfu_flash_write(dfu_data_worker.buf,
				dfu_data_worker.worker_len);
		break;
	default:
		LOG_ERR("OUT of state machine");
		break;
	}
}

static int usb_dfu_init(const struct device *dev)
{
	const struct flash_area *fa;

	ARG_UNUSED(dev);

	k_work_init(&dfu_work, dfu_work_handler);
	k_poll_signal_init(&dfu_signal);

	if (flash_area_open(dfu_data.flash_area_id, &fa)) {
		return -EIO;
	}

	dfu_data.flash_upload_size = fa->fa_size;
	flash_area_close(fa);

	return 0;
}

/**
 * @brief Function to check if DFU is started.
 *
 * @return  true if DNBUSY/DNLOAD_IDLE, false otherwise.
 */
static bool is_dfu_started(void)
{
	if ((dfu_data.state == dfuDNBUSY) ||
	    (dfu_data.state == dfuDNLOAD_IDLE)) {
		return true;
	}

	return false;
}

/**
 * @brief Function to check and wait while the USB DFU is in progress.
 *
 * @return  N/A
 */
void wait_for_usb_dfu(void)
{
	/* Wait for a prescribed duration of time. If DFU hasn't started within
	 * that time, stop waiting and proceed further.
	 */
	for (int time = 0;
		time < (CONFIG_USB_DFU_WAIT_DELAY_MS/INTERMITTENT_CHECK_DELAY);
		time++) {
		if (is_dfu_started()) {
			k_poll_event_init(&dfu_event, K_POLL_TYPE_SIGNAL,
				K_POLL_MODE_NOTIFY_ONLY, &dfu_signal);

			/* Wait till DFU is complete */
			if (k_poll(&dfu_event, 1, K_FOREVER) != 0) {
				LOG_DBG("USB DFU Error");
			}

			LOG_INF("USB DFU Completed");
			break;
		}

		k_msleep(INTERMITTENT_CHECK_DELAY);
	}
}

SYS_INIT(usb_dfu_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
