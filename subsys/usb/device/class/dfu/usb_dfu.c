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

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <stdio.h>
#include <errno.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/dfu/flash_img.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/class/usb_dfu.h>
#include <usb_descriptor.h>
#include <usb_work_q.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usb_dfu, CONFIG_USB_DEVICE_LOG_LEVEL);

#define SLOT0_PARTITION			slot0_partition
#define SLOT1_PARTITION			slot1_partition

#define FIRMWARE_IMAGE_0_LABEL "image_0"
#define FIRMWARE_IMAGE_1_LABEL "image_1"

#define USB_DFU_MAX_XFER_SIZE		CONFIG_USB_REQUEST_BUFFER_SIZE

#define INTERMITTENT_CHECK_DELAY	50

#if defined(CONFIG_USB_DFU_REBOOT)
#define DFU_DESC_ATTRIBUTES_MANIF_TOL 0
#else
#define DFU_DESC_ATTRIBUTES_MANIF_TOL DFU_ATTR_MANIFESTATION_TOLERANT
#endif

#if defined(CONFIG_USB_DFU_ENABLE_UPLOAD)
#define DFU_DESC_ATTRIBUTES_CAN_UPLOAD	 DFU_ATTR_CAN_UPLOAD
#else
#define DFU_DESC_ATTRIBUTES_CAN_UPLOAD 0
#endif

#if defined(CONFIG_USB_DFU_WILL_DETACH)
#define DFU_DESC_ATTRIBUTES_WILL_DETACH DFU_ATTR_WILL_DETACH
#else
#define DFU_DESC_ATTRIBUTES_WILL_DETACH 0
#endif

#define DFU_DESC_ATTRIBUTES		(DFU_ATTR_CAN_DNLOAD | \
					 DFU_DESC_ATTRIBUTES_CAN_UPLOAD |\
					 DFU_DESC_ATTRIBUTES_MANIF_TOL |\
					 DFU_DESC_ATTRIBUTES_WILL_DETACH)

static struct k_poll_event dfu_event;
static struct k_poll_signal dfu_signal;
static struct k_work_delayable dfu_timer_work;

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
		.bDescriptorType = USB_DESC_INTERFACE,
		.bInterfaceNumber = 0,
		.bAlternateSetting = 0,
		.bNumEndpoints = 0,
		.bInterfaceClass = USB_BCC_APPLICATION,
		.bInterfaceSubClass = DFU_SUBCLASS,
		.bInterfaceProtocol = DFU_RT_PROTOCOL,
		.iInterface = 0,
	},
	.dfu_descr = {
		.bLength = sizeof(struct dfu_runtime_descriptor),
		.bDescriptorType = DFU_FUNC_DESC,
		.bmAttributes = DFU_DESC_ATTRIBUTES,
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
#if FIXED_PARTITION_EXISTS(SLOT1_PARTITION)
		struct usb_if_descriptor if1;
#endif
		struct dfu_runtime_descriptor dfu_descr;
	} __packed sec_dfu_cfg;
} __packed;


USBD_DEVICE_DESCR_DEFINE(secondary)
struct dev_dfu_mode_descriptor dfu_mode_desc = {
	/* Device descriptor */
	.device_descriptor = {
		.bLength = sizeof(struct usb_device_descriptor),
		.bDescriptorType = USB_DESC_DEVICE,
		.bcdUSB = sys_cpu_to_le16(USB_SRN_2_0),
		.bDeviceClass = 0,
		.bDeviceSubClass = 0,
		.bDeviceProtocol = 0,
		.bMaxPacketSize0 = USB_MAX_CTRL_MPS,
		.idVendor = sys_cpu_to_le16((uint16_t)CONFIG_USB_DEVICE_VID),
		.idProduct =
			sys_cpu_to_le16((uint16_t)CONFIG_USB_DEVICE_DFU_PID),
		.bcdDevice = sys_cpu_to_le16(USB_BCD_DRN),
		.iManufacturer = 1,
		.iProduct = 2,
		.iSerialNumber = 3,
		.bNumConfigurations = 1,
	},
	/* Configuration descriptor */
	.cfg_descr = {
		.bLength = sizeof(struct usb_cfg_descriptor),
		.bDescriptorType = USB_DESC_CONFIGURATION,
		.wTotalLength = 0,
		.bNumInterfaces = 1,
		.bConfigurationValue = 1,
		.iConfiguration = 0,
		.bmAttributes = USB_SCD_RESERVED |
				COND_CODE_1(CONFIG_USB_SELF_POWERED,
					    (USB_SCD_SELF_POWERED), (0)) |
				COND_CODE_1(CONFIG_USB_DEVICE_REMOTE_WAKEUP,
					    (USB_SCD_REMOTE_WAKEUP), (0)),
		.bMaxPower = CONFIG_USB_MAX_POWER,
	},
	.sec_dfu_cfg = {
		/* Interface descriptor */
		.if0 = {
			.bLength = sizeof(struct usb_if_descriptor),
			.bDescriptorType = USB_DESC_INTERFACE,
			.bInterfaceNumber = 0,
			.bAlternateSetting = 0,
			.bNumEndpoints = 0,
			.bInterfaceClass = USB_BCC_APPLICATION,
			.bInterfaceSubClass = DFU_SUBCLASS,
			.bInterfaceProtocol = DFU_MODE_PROTOCOL,
			.iInterface = 4,
		},
#if FIXED_PARTITION_EXISTS(SLOT1_PARTITION)
		.if1 = {
			.bLength = sizeof(struct usb_if_descriptor),
			.bDescriptorType = USB_DESC_INTERFACE,
			.bInterfaceNumber = 0,
			.bAlternateSetting = 1,
			.bNumEndpoints = 0,
			.bInterfaceClass = USB_BCC_APPLICATION,
			.bInterfaceSubClass = DFU_SUBCLASS,
			.bInterfaceProtocol = DFU_MODE_PROTOCOL,
			.iInterface = 5,
		},
#endif
		.dfu_descr = {
			.bLength = sizeof(struct dfu_runtime_descriptor),
			.bDescriptorType = DFU_FUNC_DESC,
			.bmAttributes = DFU_DESC_ATTRIBUTES,
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

#if FIXED_PARTITION_EXISTS(SLOT1_PARTITION)
	struct image_1_descriptor {
		uint8_t bLength;
		uint8_t bDescriptorType;
		uint8_t bString[USB_BSTRING_LENGTH(FIRMWARE_IMAGE_1_LABEL)];
	} __packed utf16le_image1;
#endif
} __packed;

USBD_STRING_DESCR_DEFINE(secondary)
struct usb_string_desription string_descr = {
	.lang_descr = {
		.bLength = sizeof(struct usb_string_descriptor),
		.bDescriptorType = USB_DESC_STRING,
		.bString = sys_cpu_to_le16(0x0409),
	},
	/* Manufacturer String Descriptor */
	.utf16le_mfr = {
		.bLength = USB_STRING_DESCRIPTOR_LENGTH(
				CONFIG_USB_DEVICE_MANUFACTURER),
		.bDescriptorType = USB_DESC_STRING,
		.bString = CONFIG_USB_DEVICE_MANUFACTURER,
	},
	/* Product String Descriptor */
	.utf16le_product = {
		.bLength = USB_STRING_DESCRIPTOR_LENGTH(
				CONFIG_USB_DEVICE_PRODUCT),
		.bDescriptorType = USB_DESC_STRING,
		.bString = CONFIG_USB_DEVICE_PRODUCT,
	},
	/* Serial Number String Descriptor */
	.utf16le_sn = {
		.bLength = USB_STRING_DESCRIPTOR_LENGTH(CONFIG_USB_DEVICE_SN),
		.bDescriptorType = USB_DESC_STRING,
		.bString = CONFIG_USB_DEVICE_SN,
	},
	/* Image 0 String Descriptor */
	.utf16le_image0 = {
		.bLength = USB_STRING_DESCRIPTOR_LENGTH(
				FIRMWARE_IMAGE_0_LABEL),
		.bDescriptorType = USB_DESC_STRING,
		.bString = FIRMWARE_IMAGE_0_LABEL,
	},
#if FIXED_PARTITION_EXISTS(SLOT1_PARTITION)
	/* Image 1 String Descriptor */
	.utf16le_image1 = {
		.bLength = USB_STRING_DESCRIPTOR_LENGTH(
				FIRMWARE_IMAGE_1_LABEL),
		.bDescriptorType = USB_DESC_STRING,
		.bString = FIRMWARE_IMAGE_1_LABEL,
	},
#endif
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

#if FIXED_PARTITION_EXISTS(SLOT1_PARTITION)
	#define DOWNLOAD_FLASH_AREA_ID FIXED_PARTITION_ID(SLOT1_PARTITION)
#else
	#define DOWNLOAD_FLASH_AREA_ID FIXED_PARTITION_ID(SLOT0_PARTITION)
#endif


static struct dfu_data_t dfu_data = {
	.state = appIDLE,
	.status = statusOK,
	.flash_area_id = DOWNLOAD_FLASH_AREA_ID,
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
		const bool should_confirm = IS_ENABLED(CONFIG_USB_DFU_PERMANENT_DOWNLOAD);

		LOG_DBG("flash write done");
		dfu_data.state = dfuMANIFEST_SYNC;
		dfu_reset_counters();

		LOG_DBG("Should confirm: %d", should_confirm);
		if (boot_request_upgrade(should_confirm)) {
			dfu_data.state = dfuERROR;
			dfu_data.status = errWRITE;
		}

		k_poll_signal_raise(&dfu_signal, 0);
	} else {
		dfu_data.state = dfuDNLOAD_IDLE;
	}

	LOG_DBG("bytes written 0x%x", flash_img_bytes_written(&dfu_data.ctx));
}

static void dfu_enter_idle(void)
{
	dfu_data.state = dfuIDLE;

	/* Set the DFU mode descriptors to be used after reset */
	dfu_config.usb_device_description = (uint8_t *) &dfu_mode_desc;
	if (usb_set_config(dfu_config.usb_device_description)) {
		LOG_ERR("usb_set_config failed during DFU idle entry");
	}
}

static void dfu_timer_work_handler(struct k_work *item)
{
	ARG_UNUSED(item);

	if (dfu_data.state == appDETACH) {
		if (IS_ENABLED(CONFIG_USB_DFU_WILL_DETACH)) {
			if (usb_dc_detach()) {
				LOG_ERR("usb_dc_detach failed");
			}
			dfu_enter_idle();

			/* Wait 1 SOF period to ensure the host notices the deconnection. */
			k_sleep(K_MSEC(1));

			if (usb_dc_attach()) {
				LOG_ERR("usb_dc_attach failed");
			}
		} else {
			dfu_data.state = appIDLE;
		}
	}
}

#ifdef CONFIG_USB_DFU_REBOOT
static struct k_work_delayable reboot_work;

static void reboot_work_handler(struct k_work *item)
{
	ARG_UNUSED(item);

	sys_reboot(SYS_REBOOT_WARM);
}

static void reboot_schedule(void)
{
	LOG_DBG("Scheduling reboot in 500ms");

	/*
	 * Reboot with a delay,
	 * so there is some time to send the status to the host
	 */
	k_work_schedule_for_queue(&USB_WORK_Q, &reboot_work, K_MSEC(500));
}
#endif

static int dfu_class_handle_to_host(struct usb_setup_packet *setup,
				    int32_t *data_len, uint8_t **data)
{
	uint32_t bytes_left;
	uint32_t len;
	int ret;

	switch (setup->bRequest) {
	case DFU_GETSTATUS:
		LOG_DBG("DFU_GETSTATUS: status %d, state %d",
			dfu_data.status, dfu_data.state);

		if (dfu_data.state == dfuMANIFEST_SYNC) {

#if defined(CONFIG_USB_DFU_REBOOT)
			dfu_data.state = dfuMANIFEST_WAIT_RST;
			reboot_schedule();
#else
			dfu_data.state = dfuIDLE;
#endif
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

	case DFU_UPLOAD:
		LOG_DBG("DFU_UPLOAD block %d, len %d, state %d",
			setup->wValue, setup->wLength, dfu_data.state);

		if (!IS_ENABLED(CONFIG_USB_DFU_ENABLE_UPLOAD)) {
			LOG_WRN("Firmware uploading is not enabled");
			dfu_data.status = errSTALLEDPKT;
			dfu_data.state = dfuERROR;
			return -ENOTSUP;
		}

		if (dfu_check_app_state()) {
			return -EINVAL;
		}

		switch (dfu_data.state) {
		case dfuIDLE:
			dfu_reset_counters();
			LOG_DBG("DFU_UPLOAD start");
			__fallthrough;
		case dfuUPLOAD_IDLE:
			if (!setup->wLength ||
			    dfu_data.block_nr != setup->wValue) {
				LOG_ERR("DFU_UPLOAD block %d, expected %d, "
					"len %d", setup->wValue,
					dfu_data.block_nr, setup->wLength);
				dfu_data.state = dfuERROR;
				dfu_data.status = errUNKNOWN;
				return -EINVAL;
			}

			/* Upload in progress */
			bytes_left = dfu_data.flash_upload_size -
				     dfu_data.bytes_sent;
			if (bytes_left < setup->wLength) {
				len = bytes_left;
			} else {
				len = setup->wLength;
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
					return -EINVAL;
				}
				ret = flash_area_read(fa, dfu_data.bytes_sent,
						      *data, len);
				flash_area_close(fa);
				if (ret) {
					dfu_data.state = dfuERROR;
					dfu_data.status = errFILE;
					return -EINVAL;
				}
			}
			*data_len = len;

			dfu_data.bytes_sent += len;
			dfu_data.block_nr++;

			if (dfu_data.bytes_sent == dfu_data.flash_upload_size &&
			    len < setup->wLength) {
				/* Upload completed when a
				 * short packet is received
				 */
				*data_len = 0;
				dfu_data.state = dfuIDLE;
			} else {
				dfu_data.state = dfuUPLOAD_IDLE;
			}

			break;
		default:
			LOG_ERR("DFU_UPLOAD wrong state %d", dfu_data.state);
			dfu_data.state = dfuERROR;
			dfu_data.status = errUNKNOWN;
			dfu_reset_counters();
			return -EINVAL;
		}
		break;

	default:
		LOG_DBG("Unsupported bmRequestType 0x%02x bRequest 0x%02x",
			setup->bmRequestType, setup->bRequest);
		return -EINVAL;
	}

	return 0;
}

static int dfu_class_handle_to_device(struct usb_setup_packet *setup,
				      int32_t *data_len, uint8_t **data)
{
	uint16_t timeout;

	switch (setup->bRequest) {
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
			setup->wValue, setup->wLength, dfu_data.state);

		if (dfu_check_app_state()) {
			return -EINVAL;
		}

		switch (dfu_data.state) {
		case dfuIDLE:
			LOG_DBG("DFU_DNLOAD start");
			dfu_reset_counters();
			k_poll_signal_reset(&dfu_signal);

			if (dfu_data.flash_area_id != DOWNLOAD_FLASH_AREA_ID) {
				dfu_data.status = errWRITE;
				dfu_data.state = dfuERROR;
				LOG_ERR("This area can not be overwritten");
				break;
			}

			dfu_data.state = dfuDNBUSY;
			dfu_data_worker.worker_state = dfuIDLE;
			dfu_data_worker.worker_len  = setup->wLength;
			memcpy(dfu_data_worker.buf, *data, setup->wLength);
			k_work_submit_to_queue(&USB_WORK_Q, &dfu_work);
			break;
		case dfuDNLOAD_IDLE:
			dfu_data.state = dfuDNBUSY;
			dfu_data_worker.worker_state = dfuDNLOAD_IDLE;
			dfu_data_worker.worker_len  = setup->wLength;

			memcpy(dfu_data_worker.buf, *data, setup->wLength);
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
	case DFU_DETACH:
		LOG_DBG("DFU_DETACH timeout %d, state %d",
			setup->wValue, dfu_data.state);

		if (dfu_data.state != appIDLE) {
			dfu_data.state = appIDLE;
			return -EINVAL;
		}

		/* Move to appDETACH state */
		dfu_data.state = appDETACH;
		if (IS_ENABLED(CONFIG_USB_DFU_WILL_DETACH)) {
			/* Note: Detach should happen once the status stage
			 * finishes but the USB device stack does not expose
			 * such callback. Wait fixed time (ignore wValue) to
			 * let device finish control transfer status stage.
			 */
			timeout = CONFIG_USB_DFU_DETACH_TIMEOUT;
		} else {
			/* Begin detach timeout timer */
			timeout = MIN(setup->wValue, CONFIG_USB_DFU_DETACH_TIMEOUT);
		}
		k_work_reschedule_for_queue(&USB_WORK_Q, &dfu_timer_work, K_MSEC(timeout));
		break;
	default:
		LOG_DBG("Unsupported bmRequestType 0x%02x bRequest 0x%02x",
			setup->bmRequestType, setup->bRequest);
		return -EINVAL;
	}

	return 0;
}

/**
 * @brief Handler called for DFU Class requests not handled by the USB stack.
 *
 * @param setup     Information about the request to execute.
 * @param len       Size of the buffer.
 * @param data      Buffer containing the request result.
 *
 * @return  0 on success, negative errno code on fail.
 */
static int dfu_class_handle_req(struct usb_setup_packet *setup,
				int32_t *data_len, uint8_t **data)
{
	if (usb_reqtype_is_to_host(setup)) {
		return dfu_class_handle_to_host(setup, data_len, data);
	} else {
		return dfu_class_handle_to_device(setup, data_len, data);
	}
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
		if (!IS_ENABLED(CONFIG_USB_DFU_WILL_DETACH)) {
			/* Stop the appDETACH timeout timer */
			k_work_cancel_delayable(&dfu_timer_work);
			if (dfu_data.state == appDETACH) {
				dfu_enter_idle();
			}
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
		LOG_DBG("USB device suspended");
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
 * @param setup    Information about the request to execute.
 * @param len       Size of the buffer.
 * @param data      Buffer containing the request result.
 *
 * @return          -ENOTSUP so that the stack can process control request.
 */

static int dfu_custom_handle_req(struct usb_setup_packet *setup,
				 int32_t *data_len, uint8_t **data)
{
	ARG_UNUSED(data);

	if (usb_reqtype_is_to_host(setup) ||
	    setup->RequestType.recipient != USB_REQTYPE_RECIPIENT_INTERFACE) {
		return -ENOTSUP;
	}

	if (setup->bRequest == USB_SREQ_SET_INTERFACE) {
		LOG_DBG("DFU alternate setting %d", setup->wValue);

		const struct flash_area *fa;

		switch (setup->wValue) {
		case 0:
			dfu_data.flash_area_id =
			    FIXED_PARTITION_ID(SLOT0_PARTITION);
			break;
#if FIXED_PARTITION_EXISTS(SLOT1_PARTITION)
		case 1:
			dfu_data.flash_area_id = DOWNLOAD_FLASH_AREA_ID;
			break;
#endif
		default:
			LOG_WRN("Invalid DFU alternate setting");
			return -ENOTSUP;
		}

		if (flash_area_open(dfu_data.flash_area_id, &fa)) {
			return -EIO;
		}

		dfu_data.flash_upload_size = fa->fa_size;
		flash_area_close(fa);
		dfu_data.alt_setting = setup->wValue;
	}

	/* Never handled by us */
	return -EINVAL;
}

static void dfu_interface_config(struct usb_desc_header *head,
				 uint8_t bInterfaceNumber)
{
	ARG_UNUSED(head);

	dfu_cfg.if0.bInterfaceNumber = bInterfaceNumber;
}

/* Configuration of the DFU Device send to the USB Driver */
USBD_DEFINE_CFG_DATA(dfu_config) = {
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
USBD_DEFINE_CFG_DATA(dfu_mode_config) = {
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
		if (boot_erase_img_bank(DOWNLOAD_FLASH_AREA_ID)) {
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

static int usb_dfu_init(void)
{
	const struct flash_area *fa;


	k_work_init(&dfu_work, dfu_work_handler);
	k_poll_signal_init(&dfu_signal);
	k_work_init_delayable(&dfu_timer_work, dfu_timer_work_handler);

#ifdef CONFIG_USB_DFU_REBOOT
	k_work_init_delayable(&reboot_work, reboot_work_handler);
#endif

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
void wait_for_usb_dfu(k_timeout_t delay)
{
	k_timepoint_t end = sys_timepoint_calc(delay);

	/* Wait for a prescribed duration of time. If DFU hasn't started within
	 * that time, stop waiting and proceed further.
	 */
	while (!sys_timepoint_expired(end)) {
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
