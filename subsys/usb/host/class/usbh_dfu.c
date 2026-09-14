/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/usb/class/usbh_dfu.h>

#include "usbh_ch9.h"
#include "usbh_class.h"
#include "usbh_desc.h"
#include "usbh_device.h"

#define DFU_ALTERNATE_MAX_NR          (CONFIG_USBH_DFU_MAX_ALTERNATE_NR)
#define DFU_ALTERNATE_BITMAP_CAPACITY (32)
#define DFU_ALTERNATE_BITMAP_NR DIV_ROUND_UP(DFU_ALTERNATE_MAX_NR, DFU_ALTERNATE_BITMAP_CAPACITY)

/**
 * DFU_1.1.pdf: 6.1.2
 */
struct dfu_getstatus_data {
	/* DFU status */
	uint8_t bStatus;
	/* Device requested timeout in ms */
	uint32_t bwPollTimeout: 24;
	/* DFU state */
	uint8_t bState;
	/* Index to retrieve data over string descriptor  */
	uint8_t iString;
} __packed;

/**
 * DFU_1.1.pdf: Table 3.1 Summary of DFU Class-Specific Requests
 */
#define DFU_REQUEST_TYPE_DFU_DETACH    (0x21)
#define DFU_REQUEST_TYPE_DFU_DNLOAD    (0x21)
#define DFU_REQUEST_TYPE_DFU_UPLOAD    (0xA1)
#define DFU_REQUEST_TYPE_DFU_GETSTATUS (0xA1)
#define DFU_REQUEST_TYPE_DFU_CLRSTATUS (0x21)
#define DFU_REQUEST_TYPE_DFU_GETSTATE  (0xA1)
#define DFU_REQUEST_TYPE_DFU_ABORT     (0x21)

/**
 * Content of these data is bound to the USB device connection lifecycle
 */
struct usbh_dfu_drv_ephemeral {
	/* USB device */
	struct usb_device *udev;
	/* Content of DFU functional descriptor */
	struct usb_dfu_descriptor func_desc;
	/* Preserve last DFU_GETSTATUS report */
	struct dfu_getstatus_data getstatus_data;
	/* Selected interface */
	uint16_t iface;
	/* User specific settings, tweaks and quirks */
	struct usbh_dfu_settings settings;
	/* Bitmap of available alternate_ids, max 32 */
	uint32_t alternate_bitmap[DFU_ALTERNATE_BITMAP_NR];

	/* Download callback context */
	void *dnload_arg;
	/* Download callback return value */
	int dnload_result;
	/* Download callback*/
	usbh_dfu_dnload_block_cb_t dnload_cb;
	/* Upload callback context */
	void *upload_arg;
	/* Upload callback return value */
	int upload_result;
	/* Upload callback */
	usbh_dfu_upload_block_cb_t upload_cb;

	/* Current block index transmitted/received between host and device */
	uint16_t block_nr;
	uint16_t data_alloc_size;

	/* Preallocated buffers */
	struct net_buf *data_netbuf;
	struct net_buf *command_netbuf;
};

/**
 * Driver data
 */
struct usbh_dfu_drv_data {
	/* Mutex */
	struct k_mutex lock;
	/* Device was probed */
	bool probed;
	/* */
	struct usbh_dfu_drv_ephemeral eph;
};

/**
 * Description string for every DFU status
 */
static const char *const dfu_status_msg[] = {
	"No error condition is present",
	"File is not targeted for use by this device",
	"File is for this device but fails some vendor-specific verification test",
	"Device is unable to write memory",
	"Memory erase function failed",
	"Memory erase check failed",
	"Program memory function failed",
	"Programmed memory failed verification",
	"Cannot program memory due to received address that is out of range",
	"Received DFU_DNLOAD with wLength = 0, but device does not think it has all of the data "
	"yet",
	"Device's firmware is corrupt. It cannot return to run-time (non-DFU) operations",
	"iString indicates a vendor-specific error",
	"Device detected unexpected USB reset signaling",
	"Device detected unexpected power on reset",
	"Something went wrong, but the device does not know what it was",
	"Device stalled an unexpected request"};

LOG_MODULE_REGISTER(usbh_dfu, CONFIG_USBH_DFU_LOG_LEVEL);

/**
 * @brief Reset USB bus and enables SOF generation
 *
 * @param drv_data   Pointer driver context
 *
 * @return 0 on success, negative errno on failure
 */
static int usbh_dfu_bus_reset(struct usbh_dfu_drv_data *drv_data)
{
	int result = uhc_bus_reset(((struct usbh_context *)(drv_data->eph.udev->ctx))->dev);

	uhc_sof_enable(((struct usbh_context *)(drv_data->eph.udev->ctx))->dev);

	return result;
}

/**
 * @brief Check whether device reports support for expected DFU version
 *
 * @param drv_data     Pointer driver context
 *
 * @return true if DFU version is invalid/unsupported, false otherwise
 */
static int usbh_dfu_invalid_version(struct usbh_dfu_drv_data *drv_data)
{
	return drv_data->eph.func_desc.bcdDFUVersion != USB_DFU_VERSION;
}

/**
 * @brief Driver is in processing upload/download
 *
 * @param drv_data     Pointer driver context
 *
 * @return true if the upload or download API is in use, false otherwise
 */
static bool usbh_dfu_is_processing(struct usbh_dfu_drv_data *drv_data)
{
	return (drv_data->eph.dnload_cb != NULL) || (drv_data->eph.upload_cb != NULL);
}

/**
 * @brief Perform DFU_DETACH request
 *
 * @param drv_data     Pointer driver context
 * @param timeout_ms   Timeout in millisecond the device is waiting for bus_reset release
 *
 * @return 0 on success, negative errno on failure
 */
static int usbh_req_dfu_detach(struct usbh_dfu_drv_data *drv_data, uint16_t timeout_ms)
{
	/* Comply with the DFU spec, chapter 5.1 */
	if (timeout_ms > drv_data->eph.func_desc.wDetachTimeOut) {
		timeout_ms = drv_data->eph.func_desc.wDetachTimeOut;
	}

	return usbh_req_setup(drv_data->eph.udev, DFU_REQUEST_TYPE_DFU_DETACH, USB_DFU_REQ_DETACH,
			      timeout_ms, drv_data->eph.iface, 0x0000, NULL);
}

/**
 * @brief Perform DFU_DNLOAD request
 *
 * Reset the pre-allocated buffer and invoke a download callback that
 * should fill the buffer. Upon sucesfull return, it
 * transfer the buffer in DFU_DNLOAD request.
 * A return value 0 from callback indicates that download is complete.
 *
 * @param drv_data     Pointer DFU driver context
 *
 * @return 0 on success, negative errno on failure
 */
static int usbh_req_dfu_dnload(struct usbh_dfu_drv_data *drv_data)
{
	int result = 0, cb_result;

	net_buf_reset(drv_data->eph.data_netbuf);
	cb_result =
		drv_data->eph.dnload_cb(drv_data->eph.dnload_arg, drv_data->eph.data_netbuf->data,
					drv_data->eph.data_alloc_size);
	drv_data->eph.dnload_result = cb_result;

	if ((cb_result >= 0) && (cb_result <= drv_data->eph.data_alloc_size)) {
		net_buf_add(drv_data->eph.data_netbuf, cb_result);
		result = usbh_req_setup(drv_data->eph.udev, DFU_REQUEST_TYPE_DFU_DNLOAD,
					USB_DFU_REQ_DNLOAD, drv_data->eph.block_nr,
					drv_data->eph.iface, cb_result,
					cb_result != 0 ? drv_data->eph.data_netbuf : NULL);
		if (result == 0) {
			drv_data->eph.block_nr++;
		}
	} else if (cb_result > drv_data->eph.data_alloc_size) {
		result = -EINVAL;
	} else {
		result = cb_result; /* Deliver callback errno */
	}

	return result;
}

/**
 * @brief Perform DFU_GETSTATUS request
 *
 * Each DFU_GETSTATUS request causes transition in DFU state machine
 *
 * @param drv_data     Pointer DFU driver context
 *
 * @return 0 on success, negative errno on failure
 */
static int usbh_req_dfu_getstatus(struct usbh_dfu_drv_data *drv_data,
				  struct dfu_getstatus_data *getstatus_data)
{
	const uint16_t wLength = sizeof(*getstatus_data);
	uint8_t *raw_data;
	int result;

	BUILD_ASSERT(sizeof(*getstatus_data) == 6,
		     "Incorrect struct size, might be caused by 24bit integer");

	net_buf_reset(drv_data->eph.command_netbuf);
	result = usbh_req_setup(drv_data->eph.udev, DFU_REQUEST_TYPE_DFU_GETSTATUS,
				USB_DFU_REQ_GETSTATUS, 0, drv_data->eph.iface, wLength,
				drv_data->eph.command_netbuf);
	if (result != 0) {
		return result;
	}

	if (drv_data->eph.command_netbuf->len != sizeof(*getstatus_data)) {
		return -EFAULT;
	}

	raw_data = (uint8_t *)drv_data->eph.command_netbuf->data;
	getstatus_data->bStatus = raw_data[0];
	getstatus_data->bwPollTimeout = sys_get_le24(&raw_data[1]);
	getstatus_data->bState = raw_data[4];
	getstatus_data->iString = raw_data[5];

	return result;
}

/**
 * @brief Perform DFU_CLRSTATUS request
 *
 * @param drv_data     Pointer DFU driver context
 *
 * @return 0 on success, negative errno on failure
 */
static int usbh_req_dfu_clrstatus(struct usbh_dfu_drv_data *drv_data)
{
	return usbh_req_setup(drv_data->eph.udev, DFU_REQUEST_TYPE_DFU_CLRSTATUS,
			      USB_DFU_REQ_CLRSTATUS, 0, drv_data->eph.iface, 0, NULL);
}

/**
 * @brief Perform DFU_ABORT request
 *
 * @param drv_data     Pointer DFU driver context
 *
 * @return 0 on success, negative errno on failure
 */
static int usbh_req_dfu_abort(struct usbh_dfu_drv_data *drv_data)
{
	return usbh_req_setup(drv_data->eph.udev, DFU_REQUEST_TYPE_DFU_ABORT, USB_DFU_REQ_ABORT, 0,
			      drv_data->eph.iface, 0, NULL);
}

/**
 * @brief Perform DFU_GETSTATE request
 *
 * @param drv_data     Pointer DFU driver context
 *
 * @return 0 on success, negative errno on failure
 */
static int usbh_req_dfu_getstate(struct usbh_dfu_drv_data *drv_data, uint8_t *getstate_data)
{
	const uint16_t wLength = 1;
	int result;

	net_buf_reset(drv_data->eph.command_netbuf);
	result = usbh_req_setup(drv_data->eph.udev, DFU_REQUEST_TYPE_DFU_GETSTATE,
				USB_DFU_REQ_GETSTATE, 0, drv_data->eph.iface, wLength,
				drv_data->eph.command_netbuf);

	if (result != 0) {
		return result;
	}

	if (drv_data->eph.command_netbuf->len != sizeof(*getstate_data)) {
		return -EFAULT;
	}

	*getstate_data = *((uint8_t *)drv_data->eph.command_netbuf->data);

	return result;
}

/**
 * @brief Perform DFU_UPLOAD request
 *
 * Allocate buffer of wTransferSize and request DFU_UPLOAD.
 * Invoke a upload callback after sucesfull receive and release the buffer.
 * Length 0 - passed to the callback means that FW upload is complete.
 *
 * @param drv_data     Pointer DFU driver context
 *
 * @return 0 on success, negative errno on failure
 */
static int usbh_req_dfu_upload(struct usbh_dfu_drv_data *drv_data)
{
	int result = 0;

	net_buf_reset(drv_data->eph.data_netbuf);
	result = usbh_req_setup(drv_data->eph.udev, DFU_REQUEST_TYPE_DFU_UPLOAD, USB_DFU_REQ_UPLOAD,
				drv_data->eph.block_nr, drv_data->eph.iface,
				drv_data->eph.data_alloc_size, drv_data->eph.data_netbuf);
	if (result == 0) {
		drv_data->eph.block_nr++;
		drv_data->eph.upload_result = drv_data->eph.upload_cb(
			drv_data->eph.upload_arg, drv_data->eph.data_netbuf->data,
			drv_data->eph.data_netbuf->len);
		if (drv_data->eph.upload_result < 0) {
			/* callback errno */
			result = drv_data->eph.upload_result;
		} else if (drv_data->eph.data_netbuf->len < drv_data->eph.data_alloc_size) {
			/* Received short frame; invoke callback with 0 length to confirm receive
			 * complete
			 */
			drv_data->eph.upload_result =
				drv_data->eph.upload_cb(drv_data->eph.upload_arg, NULL, 0);
			result = drv_data->eph.upload_result < 0 ? drv_data->eph.upload_result : 0;
		} else {
			drv_data->eph.upload_result = drv_data->eph.data_netbuf->len;
		}
	}

	return result;
}

/**
 * @brief FW upload function
 *
 * Executes DFU_DNLOAD transitions over DFU state machine,
 * calls user callback before transfer data block to the device
 *
 * @param dev           Pointer device
 * @param settings      Various parameters for transactions
 *
 * @return 0 on success, negative errno on failure
 */
static int usbh_dfu_settings_api(struct device const *dev, struct usbh_dfu_settings *settings)
{
	struct usbh_dfu_drv_data *drv_data;
	uint32_t bitmap_pos, bitmap_idx;
	int result;

	if (dev == NULL || settings == NULL) {
		return -EINVAL;
	}
	drv_data = dev->data;

	result = k_mutex_lock(&drv_data->lock, K_FOREVER);
	if (result) {
		return result;
	}

	do {
		if (!drv_data->probed) {
			LOG_DBG("Driver was not probed yet");
			result = -EAGAIN;
			break;
		}

		if (usbh_dfu_is_processing(drv_data)) {
			LOG_ERR("Forbidden call from callback");
			result = -EALREADY;
			break;
		}

		if (settings->alternate_idx >= DFU_ALTERNATE_MAX_NR) {
			LOG_ERR("Requested alternate interface %d is greater than 31",
				settings->alternate_idx);
			result = -EINVAL;
			break;
		}

		/* Requested alternate setting was not enumerated */
		bitmap_pos = settings->alternate_idx % DFU_ALTERNATE_BITMAP_CAPACITY;
		bitmap_idx = settings->alternate_idx / DFU_ALTERNATE_BITMAP_CAPACITY;
		if (!(drv_data->eph.alternate_bitmap[bitmap_idx] & BIT(bitmap_pos))) {
			LOG_ERR("Requested alternate interface %d does not support required "
				"protocol",
				settings->alternate_idx);
			result = -EINVAL;
			break;
		}

		memcpy(&drv_data->eph.settings, settings, sizeof(drv_data->eph.settings));
	} while (0);

	k_mutex_unlock(&drv_data->lock);

	return result;
}

/**
 * @brief Returns whether 'state' supports abort transition to DFU_IDLE
 *
 * @param state     DFU state
 *
 * @return whether 'abort' transition can be used in 'state'
 */
static inline bool usbh_dfu_state_support_abort(uint8_t state)
{
	return ((state == DFU_DNLOAD_SYNC) || (state == DFU_DNLOAD_IDLE) ||
		(state == DFU_MANIFEST_SYNC) || (state == DFU_UPLOAD_IDLE) || (state == DFU_IDLE));
}

/**
 * @brief Get state and returns whether 'state' supports abort transition to DFU_IDLE
 *
 * @param drv_data     Driver context
 *
 * @return whether 'abort' transition can be used in current 'state'
 */
static inline bool usbh_dfu_support_abort(struct usbh_dfu_drv_data *const drv_data)
{
	uint8_t state;
	int result;

	result = usbh_req_dfu_getstate(drv_data, &state);
	if (result) {
		return false;
	}

	return usbh_dfu_state_support_abort(state);
}

/**
 * @brief Check the initialization state of DFU state machine
 *
 * @param drv_data     Pointer DFU driver context
 *
 * @return 0 on success, negative errno on failure
 */
static int usbh_dfu_init(struct usbh_dfu_drv_data *const drv_data)
{
	uint8_t state;
	int result;

	/* Use alternate settings */
	result = usbh_req_set_alt(drv_data->eph.udev, drv_data->eph.iface,
				  drv_data->eph.settings.alternate_idx);
	if (result) {
		return result;
	}

	result = usbh_req_dfu_getstate(drv_data, &state);
	if (result) {
		return result;
	}

	/* Attempting to bring device from ERROR state to IDLE and continue */
	if (unlikely(state == DFU_ERROR)) {
		result = usbh_req_dfu_clrstatus(drv_data);
		if (result) {
			return result;
		}

		result = usbh_req_dfu_getstate(drv_data, &state);
		if (result) {
			return result;
		}
	}
	/* Attempting to bring device from non-IDLE state to IDLE and continue  */
	else if (unlikely((state != DFU_IDLE) && usbh_dfu_state_support_abort(state))) {
		result = usbh_req_dfu_abort(drv_data);
		if (result) {
			return result;
		}

		result = usbh_req_dfu_getstate(drv_data, &state);
		if (result) {
			return result;
		}
	}

	if (state != DFU_IDLE) {
		LOG_ERR("Device is not in expected IDLE state");
		return -EIO;
	}

	return 0;
}

/**
 * @brief Perform device state/status check, perform clrstatus on error
 *
 * @param drv_data     Pointer DFU driver context
 *
 * @return 0 on success, negative errno on failure
 */
static int usbh_dfu_check_and_clear_device_error(struct usbh_dfu_drv_data *const drv_data)
{
	if (drv_data->eph.getstatus_data.bState == DFU_ERROR) {
		LOG_ERR("Device entered DFU_ERROR state. Clearing status and aborting operation");
		if (drv_data->eph.getstatus_data.bStatus < ARRAY_SIZE(dfu_status_msg)) {
			LOG_ERR("%s", dfu_status_msg[drv_data->eph.getstatus_data.bStatus]);
		}
		usbh_req_dfu_clrstatus(drv_data);
		return -EFAULT;
	}

	return 0;
}

/**
 * @brief Performs abort transition if abort transition is valid
 *
 * @param drv_data     Pointer DFU driver context
 */
static void usbh_dfu_abort_if_possible(struct usbh_dfu_drv_data *const drv_data)
{
	if (usbh_dfu_support_abort(drv_data)) {
		LOG_ERR("Performing abort");
		usbh_req_dfu_abort(drv_data);
	}
}

/**
 * @brief Implement upload transitions
 *
 * @param drv_data     Pointer DFU driver context
 */
static int usbh_dfu_upload_processing(struct usbh_dfu_drv_data *const drv_data)
{
	int result;

	result = usbh_req_dfu_upload(drv_data);
	if (result) {
		return result;
	}

	result = usbh_req_dfu_getstatus(drv_data, &drv_data->eph.getstatus_data);
	if (result) {
		return result;
	}

	return 0;
}

/**
 * @brief FW upload function
 *
 * Executes DFU_UPLOAD transitions over DFU state machine,
 * calls user callback afer receiving each data block from the device
 *
 * @param dev           Pointer device
 * @param upload_cb     Application callback
 * @param upload_arg    Callback specific context
 *
 * @return 0 on success, negative errno on failure
 */
static int usbh_dfu_upload_api(struct device const *dev, usbh_dfu_upload_block_cb_t upload_cb,
			       void *upload_arg)
{
	struct usbh_dfu_drv_data *drv_data;
	int result, err_result;

	if (dev == NULL || upload_cb == NULL) {
		return -EINVAL;
	}
	drv_data = dev->data;

	result = k_mutex_lock(&drv_data->lock, K_FOREVER);
	if (result) {
		return result;
	}

	do {
		if (!drv_data->probed) {
			LOG_DBG("Driver was not probed yet");
			result = -EAGAIN;
			break;
		}

		if (usbh_dfu_invalid_version(drv_data)) {
			LOG_ERR("Device does not support DFU 1.1");
			result = -ENOTSUP;
			break;
		}

		if ((drv_data->eph.func_desc.bmAttributes & USB_DFU_ATTR_CAN_UPLOAD) == 0) {
			LOG_ERR("Device does not support upload");
			result = -ENOTSUP;
			break;
		}

		if (usbh_dfu_is_processing(drv_data)) {
			LOG_ERR("Forbidden call from callback");
			result = -EALREADY;
			break;
		}

		result = usbh_dfu_init(drv_data);
		if (result) {
			break;
		}

		/* Reset block counter */
		drv_data->eph.block_nr = 0;
		drv_data->eph.upload_cb = upload_cb;
		drv_data->eph.upload_arg = upload_arg;

		do {
			result = usbh_dfu_upload_processing(drv_data);
		} while ((result == 0) && (drv_data->eph.upload_result != 0) &&
			 (drv_data->eph.getstatus_data.bState != DFU_ERROR));

		err_result = usbh_dfu_check_and_clear_device_error(drv_data);
		if (err_result) {
			result = err_result;
			break;
		}

		if (result != 0) {
			LOG_ERR("Upload failed");
			usbh_dfu_abort_if_possible(drv_data);
		}
	} while (0);

	drv_data->eph.upload_cb = NULL;
	drv_data->eph.upload_arg = NULL;

	k_mutex_unlock(&drv_data->lock);

	return result;
}

/**
 * @brief Implement download transitions
 *
 * @param drv_data     Pointer DFU driver context
 *
 * @return 0 on success, negative errno on failure
 */
static int usbh_dfu_dnload_processing_start(struct usbh_dfu_drv_data *const drv_data)
{
	int result = 0;

	result = usbh_req_dfu_dnload(drv_data);
	if (result) {
		return result;
	}

	/* All transfers are complete */
	if (drv_data->eph.dnload_result == 0) {
		return result;
	}

	/* Wait for transfer to complete */
	result = usbh_req_dfu_getstatus(drv_data, &drv_data->eph.getstatus_data);
	if (result) {
		return result;
	}
	if (drv_data->eph.getstatus_data.bState == DFU_DNBUSY) {
		k_sleep(K_MSEC(drv_data->eph.getstatus_data.bwPollTimeout));
	}

	return result;
}

/**
 * @brief Implement download transitions
 *
 * @param drv_data     Pointer DFU driver context
 *
 * @return 0 on success, negative errno on failure
 */
static int usbh_dfu_dnload_processing_repeat(struct usbh_dfu_drv_data *const drv_data)
{
	int result = 0;

	result = usbh_req_dfu_getstatus(drv_data, &drv_data->eph.getstatus_data);
	if (result) {
		return result;
	}

	if (drv_data->eph.getstatus_data.bState != DFU_DNLOAD_IDLE) {
		return -EFAULT;
	}

	result = usbh_req_dfu_dnload(drv_data);
	if (result) {
		return result;
	}

	/* All transfers are complete */
	if (drv_data->eph.dnload_result == 0) {
		return result;
	}

	/* Wait for transfer to complete */
	result = usbh_req_dfu_getstatus(drv_data, &drv_data->eph.getstatus_data);
	if (result) {
		return result;
	}

	if (drv_data->eph.getstatus_data.bState == DFU_DNBUSY) {
		k_sleep(K_MSEC(drv_data->eph.getstatus_data.bwPollTimeout));
	}

	return result;
}

/**
 * @brief Implement download complete transitions
 *
 * @param drv_data     Pointer DFU driver context
 *
 * @return 0 on success, negative errno on failure
 */
static int usbh_dfu_dnload_complete(struct usbh_dfu_drv_data *const drv_data)
{
	uint8_t state;
	int result;

	/* NOTE: Last chance to invoke abort at this point.
	 * Following 'getstatus' causes transition to DFU_MANIFEST, FW is written to device
	 */

	LOG_DBG("Device is updating the FW");
	result = usbh_req_dfu_getstatus(drv_data, &drv_data->eph.getstatus_data);
	if (result) {
		return result;
	}

	/* Wait for device to update FW */
	if (drv_data->eph.getstatus_data.bState == DFU_MANIFEST) {
		k_sleep(K_MSEC(drv_data->eph.getstatus_data.bwPollTimeout));
	} else if (drv_data->eph.settings.quirks & USBH_DFU_QUIRK_IGNORE_DNLOAD_COMPLETE_CHECK) {
		k_sleep(K_MSEC(drv_data->eph.getstatus_data.bwPollTimeout));
	} else {
		LOG_ERR("Device is not in expected DFU_MANIFEST state");
		return -EIO;
	}

	result = usbh_req_dfu_getstate(drv_data, &state);
	if (result) {
		return -EIO;
	}

	if (state == DFU_MANIFEST_WAIT_RST) {
		if (0 == (drv_data->eph.func_desc.bmAttributes & USB_DFU_ATTR_WILL_DETACH)) {
			result = usbh_dfu_bus_reset(drv_data);
		}
	} else if (state == DFU_MANIFEST_SYNC) {
		/* Request getstatus to transition to DFU_IDLE */
		result = usbh_req_dfu_getstatus(drv_data, &drv_data->eph.getstatus_data);
	} else if (!(drv_data->eph.settings.quirks & USBH_DFU_QUIRK_IGNORE_DNLOAD_COMPLETE_CHECK)) {
		LOG_ERR("Device is not in expected state");
		return -EIO;
	}

	return result;
}

/**
 * @brief FW upload function
 *
 * Executes DFU_DNLOAD transitions over DFU state machine,
 * calls user callback before transfer data block to the device
 *
 * @param dev           Pointer device
 * @param dnload_cb     Application callback
 * @param dnload_arg    Callback specific context
 *
 * @return 0 on success, negative errno on failure
 */
static int usbh_dfu_dnload_api(struct device const *dev, usbh_dfu_dnload_block_cb_t dnload_cb,
			       void *dnload_arg)
{
	struct usbh_dfu_drv_data *drv_data;
	int result, err_result;

	if (dev == NULL || dnload_cb == NULL) {
		return -EINVAL;
	}
	drv_data = dev->data;

	result = k_mutex_lock(&drv_data->lock, K_FOREVER);
	if (result) {
		return result;
	}

	do {
		if (!drv_data->probed) {
			LOG_DBG("Driver was not probed yet");
			result = -EAGAIN;
			break;
		}

		if (usbh_dfu_invalid_version(drv_data)) {
			LOG_ERR("Device does not support DFU 1.1");
			result = -ENOTSUP;
			break;
		}

		if ((drv_data->eph.func_desc.bmAttributes & USB_DFU_ATTR_CAN_DNLOAD) == 0) {
			LOG_ERR("Device does not support download");
			result = -ENOTSUP;
			break;
		}

		if (usbh_dfu_is_processing(drv_data)) {
			LOG_ERR("Device upload/download is already in progress");
			result = -EALREADY;
			break;
		}

		result = usbh_dfu_init(drv_data);
		if (result) {
			break;
		}

		/* Reset block counter */
		drv_data->eph.block_nr = 0;
		drv_data->eph.dnload_cb = dnload_cb;
		drv_data->eph.dnload_arg = dnload_arg;

		/* Download by chunks */
		result = usbh_dfu_dnload_processing_start(drv_data);
		while ((drv_data->eph.dnload_result > 0) && (result == 0) &&
		       (drv_data->eph.getstatus_data.bState != DFU_ERROR)) {
			result = usbh_dfu_dnload_processing_repeat(drv_data);
		}

		err_result = usbh_dfu_check_and_clear_device_error(drv_data);
		if (err_result) {
			result = err_result;
			break;
		}

		if (result != 0) {
			LOG_ERR("Download failed");
			usbh_dfu_abort_if_possible(drv_data);
			break;
		}

		result = usbh_dfu_dnload_complete(drv_data);

		err_result = usbh_dfu_check_and_clear_device_error(drv_data);
		if (err_result) {
			result = err_result;
			break;
		}

		/* NOTE: At this point, transitioning to ABORT on error is not possible. */

	} while (0);

	drv_data->eph.dnload_cb = NULL;
	drv_data->eph.dnload_arg = NULL;

	k_mutex_unlock(&drv_data->lock);

	return result;
}

/**
 * @brief Get last known status of DFU state machine
 *
 * @param dev           Pointer to the device
 *
 * @return 0 on success, negative errno on failure
 */
static int usbh_dfu_get_status_api(struct device const *dev)
{
	struct usbh_dfu_drv_data *drv_data;
	int result;

	if (dev == NULL) {
		return -EINVAL;
	}
	drv_data = dev->data;

	result = k_mutex_lock(&drv_data->lock, K_FOREVER);
	if (result) {
		return result;
	}

	do {
		if (!drv_data->probed) {
			LOG_DBG("Driver was not probed yet");
			result = -EAGAIN;
			break;
		}

		if (usbh_dfu_invalid_version(drv_data)) {
			LOG_ERR("Device does not support DFU 1.1");
			result = -ENOTSUP;
			break;
		}

		result = drv_data->eph.getstatus_data.bStatus;
	} while (0);

	k_mutex_unlock(&drv_data->lock);

	return result;
}

/**
 * @brief Check whether the device matches requested VID/PID
 *
 * @param dev           Pointer to the device
 * @param vid           Requested VendorID
 * @param pid           Requested ProductID
 *
 * @return negative errno on failure, 1 for accepted, 0 for rejected
 */
static int usbh_match_vid_pid_api(struct device const *dev, uint16_t vid, uint16_t pid)
{
	struct usbh_dfu_drv_data *drv_data;
	int result;

	if (dev == NULL) {
		return -EINVAL;
	}
	drv_data = dev->data;

	result = k_mutex_lock(&drv_data->lock, K_FOREVER);
	if (result) {
		return result;
	}

	do {
		if (!drv_data->probed) {
			LOG_DBG("Driver was not probed yet");
			result = -EAGAIN;
			break;
		}

		result = (((vid == 0xFFFF) || (vid == drv_data->eph.udev->dev_desc.idVendor)) &&
			  ((pid == 0xFFFF) || (pid == drv_data->eph.udev->dev_desc.idProduct)))
				 ? 1
				 : 0;
	} while (0);

	k_mutex_unlock(&drv_data->lock);

	return result;
}

/**
 * @brief DFU driver initialization
 *
 * @param c_data  Pointer to USB class
 *
 * @return 0 on success, negative errno value on failure.
 */
static int usbh_dfu_class_init(struct usbh_class_data *const c_data)
{
	const struct device *dev = c_data->priv;
	struct usbh_dfu_drv_data *const drv_data = dev->data;

	k_mutex_init(&drv_data->lock);
	drv_data->probed = false;

	return 0;
}

/**
 * @brief Parse configuration descriptors for DFU device
 *
 * @param c_data        Pointer to USB class
 * @param udev          Pointer to USB device
 * @param iface         Interface number
 * @param protocol      Filter iface descriptor of requested protocol
 *
 * @return 0 on success, negative errno value on failure.
 */
static int usbh_dfu_parse_desc(struct usbh_class_data *const c_data, struct usb_device *const udev,
			       uint8_t iface, const uint8_t protocol)
{
	struct usb_if_descriptor *if_desc;
	const struct device *dev = c_data->priv;
	struct usbh_dfu_drv_data *const drv_data = dev->data;
	const struct usb_desc_header *desc;
	uint32_t bitmap_pos, bitmap_idx, alternate_idx = 0;
	bool in_interface = false;

	desc = udev->cfg_desc;
	if ((desc == NULL) || (desc->bDescriptorType != USB_DESC_CONFIGURATION)) {
		return -EFAULT;
	}

	desc = usbh_desc_get_next(desc);
	while (desc != NULL) {
		if (usbh_desc_is_valid(desc, sizeof(struct usb_if_descriptor),
				       USB_DESC_INTERFACE)) {
			if_desc = ((struct usb_if_descriptor *)desc);
			if ((if_desc->bInterfaceClass == USB_BCC_APPLICATION) &&
			    (if_desc->bInterfaceSubClass == USB_DFU_SUBCLASS) &&
			    (if_desc->bInterfaceProtocol == protocol) &&
			    (if_desc->bInterfaceNumber == iface)) {
				if (if_desc->bAlternateSetting < DFU_ALTERNATE_MAX_NR) {
					bitmap_idx = if_desc->bAlternateSetting /
						     DFU_ALTERNATE_BITMAP_CAPACITY;
					bitmap_pos = if_desc->bAlternateSetting %
						     DFU_ALTERNATE_BITMAP_CAPACITY;
					drv_data->eph.alternate_bitmap[bitmap_idx] |=
						BIT(bitmap_pos);
				}
				in_interface = true;
			} else {
				in_interface = false;
			}
		} else if (desc->bDescriptorType == USB_DESC_DFU_FUNCTIONAL && in_interface) {
			if (desc->bLength != sizeof(drv_data->eph.func_desc)) {
				return -EFAULT;
			}
			memcpy(&drv_data->eph.func_desc, desc, sizeof(drv_data->eph.func_desc));
			drv_data->eph.func_desc.wDetachTimeOut =
				sys_le16_to_cpu(drv_data->eph.func_desc.wDetachTimeOut);
			drv_data->eph.func_desc.wTransferSize =
				sys_le16_to_cpu(drv_data->eph.func_desc.wTransferSize);
			drv_data->eph.func_desc.bcdDFUVersion =
				sys_le16_to_cpu(drv_data->eph.func_desc.bcdDFUVersion);
		}
		desc = usbh_desc_get_next(desc);
	}

	if (drv_data->eph.func_desc.wTransferSize == 0) {
		LOG_ERR("Device request 0 transfer size");
		return -EFAULT;
	}

	/* Use first alternate settings as the default one */
	for (bitmap_idx = 0; bitmap_idx < DFU_ALTERNATE_BITMAP_NR; bitmap_idx++) {
		alternate_idx = find_lsb_set(drv_data->eph.alternate_bitmap[bitmap_idx]);
		if (alternate_idx) {
			break;
		}
	}

	if (alternate_idx == 0) {
		LOG_ERR("No alternate setting was found");
		return -ENODEV;
	}

	drv_data->eph.settings.alternate_idx =
		(DFU_ALTERNATE_BITMAP_CAPACITY * bitmap_idx) + (alternate_idx - 1);
	drv_data->eph.data_alloc_size =
		MIN(drv_data->eph.func_desc.wTransferSize, CONFIG_USBH_DFU_LIMIT_DATA_ALLOC_BYTES);

	return 0;
}

/**
 * @brief DFU driver probe function, invoked on USB device attach
 *
 * @param c_data        Pointer to USB class
 * @param udev          Pointer to USB device
 * @param iface         Interface number
 *
 * @return 0 on success, negative errno value on failure.
 */
static int usbh_dfu_class_probe(struct usbh_class_data *const c_data, struct usb_device *const udev,
				uint8_t iface)
{
	const struct device *dev = c_data->priv;
	struct usbh_dfu_drv_data *drv_data = (void *)dev->data;
	int result;

	LOG_DBG("DFU device was attached\n");

	if ((udev == NULL) || (udev->state != USB_STATE_CONFIGURED)) {
		LOG_ERR("USB device not properly configured");
		return -ENODEV;
	}

	if (drv_data == NULL) {
		LOG_ERR("No DFU device instance is available");
		return -ENODEV;
	}

	result = k_mutex_lock(&drv_data->lock, K_FOREVER);
	if (result) {
		return result;
	}

	memset(&drv_data->eph, 0x00, sizeof(drv_data->eph));
	drv_data->eph.udev = udev;
	drv_data->eph.iface = iface;

	do {
		result = usbh_dfu_parse_desc(c_data, udev, iface, USB_DFU_PROTOCOL_DFU);
		if (result) {
			break;
		}

		drv_data->eph.data_netbuf =
			usbh_xfer_buf_alloc(drv_data->eph.udev, drv_data->eph.data_alloc_size);
		if (drv_data->eph.data_netbuf == NULL) {
			result = -ENOMEM;
			break;
		}

		/* Allocated memory sized for the largest structure, used for GET_STATE and
		 * GET_STATUS requests.
		 */
		drv_data->eph.command_netbuf =
			usbh_xfer_buf_alloc(drv_data->eph.udev, sizeof(struct dfu_getstatus_data));
		if (drv_data->eph.command_netbuf == NULL) {
			result = -ENOMEM;
			break;
		}
	} while (0);

	if (result && (drv_data->eph.data_netbuf != NULL)) {
		usbh_xfer_buf_free(drv_data->eph.udev, drv_data->eph.data_netbuf);
		drv_data->eph.data_netbuf = NULL;
	}

	if (result && (drv_data->eph.command_netbuf != NULL)) {
		usbh_xfer_buf_free(drv_data->eph.udev, drv_data->eph.command_netbuf);
		drv_data->eph.command_netbuf = NULL;
	}

	if (result == 0) {
		drv_data->probed = true;
	}

	k_mutex_unlock(&drv_data->lock);

	return result;
}

/**
 * @brief DFU driver remove function, invoked on USB device removal
 *
 * @param c_data        Pointer to USB class
 *
 * @return 0 on success, negative errno value on failure.
 */
static int usbh_dfu_class_removed(struct usbh_class_data *const c_data)
{
	const struct device *dev = c_data->priv;
	struct usbh_dfu_drv_data *drv_data = (void *)dev->data;
	int result;

	result = k_mutex_lock(&drv_data->lock, K_FOREVER);
	if (result) {
		return result;
	}

	if (drv_data->eph.data_netbuf != NULL) {
		usbh_xfer_buf_free(drv_data->eph.udev, drv_data->eph.data_netbuf);
		drv_data->eph.data_netbuf = NULL;
	}
	if (drv_data->eph.command_netbuf != NULL) {
		usbh_xfer_buf_free(drv_data->eph.udev, drv_data->eph.command_netbuf);
		drv_data->eph.command_netbuf = NULL;
	}

	drv_data->eph.udev = NULL;
	drv_data->probed = false;
	k_mutex_unlock(&drv_data->lock);

	LOG_DBG("DFU device was removed\n");

	return result;
}

static __maybe_unused struct usbh_class_api usbh_dfu_class_api = {
	.init = usbh_dfu_class_init,
	.probe = usbh_dfu_class_probe,
	.removed = usbh_dfu_class_removed,
};

/**
 * @brief Check the initialization state of DFU state machine
 *
 * @param drv_data     Pointer DFU driver context
 * @param idle_state   Expected idle state
 *
 * @return 0 on success, negative errno on failure
 */
static int usbh_dfurt_init(struct usbh_dfu_drv_data *const drv_data, const uint8_t idle_state)
{
	int result;

	ARG_UNUSED(idle_state);

	/* Use alternate settings */
	result = usbh_req_set_alt(drv_data->eph.udev, drv_data->eph.iface,
				  drv_data->eph.settings.alternate_idx);
	if (result) {
		return result;
	}

	return 0;
}

/**
 * @brief Request USB device mode change, from DFU-runtime mode to DFU mode
 *
 * @param dev   Pointer to device
 *
 * @return 0 on success, negative errno value on failure.
 */
static int usbh_dfurt_enter_dfu_api(struct device const *dev)
{
	struct usbh_dfu_drv_data *drv_data;
	uint16_t timeout_ms;
	int result;

	if (dev == NULL) {
		return -EINVAL;
	}
	drv_data = dev->data;

	result = k_mutex_lock(&drv_data->lock, K_FOREVER);
	if (result) {
		return result;
	}

	do {
		if (!drv_data->probed) {
			LOG_DBG("Driver was not probed yet");
			result = -EAGAIN;
			break;
		}

		if (usbh_dfu_invalid_version(drv_data)) {
			LOG_ERR("Device does not support DFU 1.1");
			result = -ENOTSUP;
			break;
		}

		result = usbh_dfurt_init(drv_data, APP_IDLE);
		if (result) {
			break;
		}

		/* Use device recommended time to wait for bus reset release */
		timeout_ms = drv_data->eph.func_desc.wDetachTimeOut;

		/* Adjusts the user-requested time to meet device constraint */
		if (drv_data->eph.settings.detach_timeout_ms &&
		    (drv_data->eph.settings.detach_timeout_ms <
		     drv_data->eph.func_desc.wDetachTimeOut)) {
			timeout_ms = drv_data->eph.settings.detach_timeout_ms;
		}
		LOG_DBG("detach timeout %d", (unsigned int)timeout_ms);

		result = usbh_req_dfu_detach(drv_data, timeout_ms);
		if (result) {
			break;
		}

		/* Host must reset the bus if the device cannot detach on its own */
		if (!(drv_data->eph.func_desc.bmAttributes & USB_DFU_ATTR_WILL_DETACH)) {
			result = usbh_dfu_bus_reset(drv_data);
		}
	} while (0);

	k_mutex_unlock(&drv_data->lock);

	return result;
}

/**
 * @brief DFU-runtime  driver initialization
 *
 * @param c_data  Pointer to USB class
 *
 * @return 0 on success, negative errno value on failure.
 */
static int usbh_dfurt_class_init(struct usbh_class_data *const c_data)
{
	const struct device *dev = c_data->priv;
	struct usbh_dfu_drv_data *const drv_data = dev->data;

	k_mutex_init(&drv_data->lock);
	drv_data->probed = false;

	return 0;
}

/**
 * @brief DFU-runtime driver probe function, invoked on USB device attach
 *
 * @param c_data        Pointer to USB class
 * @param udev          Pointer to USB device
 * @param iface         Interface number
 *
 * @return 0 on success, negative errno value on failure.
 */
static int usbh_dfurt_class_probe(struct usbh_class_data *const c_data,
				  struct usb_device *const udev, uint8_t iface)
{
	const struct device *dev = c_data->priv;
	struct usbh_dfu_drv_data *drv_data = (void *)dev->data;
	int result;

	LOG_DBG("DFU_RT device was attached\n");

	result = k_mutex_lock(&drv_data->lock, K_FOREVER);
	if (result) {
		return result;
	}

	memset(&drv_data->eph, 0x00, sizeof(drv_data->eph));
	drv_data->eph.udev = udev;
	drv_data->eph.iface = iface;

	do {
		result = usbh_dfu_parse_desc(c_data, udev, iface, USB_DFU_PROTOCOL_RUNTIME);
		if (result) {
			break;
		}

		drv_data->eph.command_netbuf =
			usbh_xfer_buf_alloc(drv_data->eph.udev, sizeof(struct dfu_getstatus_data));
		if (drv_data->eph.command_netbuf == NULL) {
			result = -ENOMEM;
			break;
		}
	} while (0);

	if (result == 0) {
		drv_data->probed = true;
	}
	k_mutex_unlock(&drv_data->lock);

	return result;
}

/**
 * @brief DFU-runtime driver remove function, invoked on USB device removal
 *
 * @param c_data        Pointer to USB class
 *
 * @return 0 on success, negative errno value on failure.
 */
static int usbh_dfurt_class_removed(struct usbh_class_data *const c_data)
{
	const struct device *dev = c_data->priv;
	struct usbh_dfu_drv_data *drv_data = (void *)dev->data;
	int result;

	result = k_mutex_lock(&drv_data->lock, K_FOREVER);
	if (result) {
		return result;
	}

	if (drv_data->eph.command_netbuf != NULL) {
		usbh_xfer_buf_free(drv_data->eph.udev, drv_data->eph.command_netbuf);
		drv_data->eph.command_netbuf = NULL;
	}

	drv_data->eph.udev = NULL;
	drv_data->probed = false;
	k_mutex_unlock(&drv_data->lock);

	LOG_DBG("DFU_RT device was removed\n");

	return result;
}

static __maybe_unused struct usbh_class_api usbh_dfurt_class_api = {
	.init = usbh_dfurt_class_init,
	.probe = usbh_dfurt_class_probe,
	.removed = usbh_dfurt_class_removed,
};

static __maybe_unused struct usbh_class_filter usbh_dfu_filters[] = {
	{
		.flags = USBH_CLASS_MATCH_CODE_TRIPLE,
		.class = USB_BCC_APPLICATION,
		.sub = USB_DFU_SUBCLASS,
		.proto = USB_DFU_PROTOCOL_DFU,
	},
	{0},
};

static __maybe_unused struct usbh_class_filter usbh_dfurt_filters[] = {
	{
		.flags = USBH_CLASS_MATCH_CODE_TRIPLE,
		.class = USB_BCC_APPLICATION,
		.sub = USB_DFU_SUBCLASS,
		.proto = USB_DFU_PROTOCOL_RUNTIME,
	},
	{0},
};

/**
 * @brief USB Host DFU driver table
 */
static __maybe_unused DEVICE_API(usbh_dfu,
				 usbh_dfu_api) = {.settings = usbh_dfu_settings_api,
						  .upload = usbh_dfu_upload_api,
						  .dnload = usbh_dfu_dnload_api,
						  .get_status = usbh_dfu_get_status_api,
						  .match_vid_pid = usbh_match_vid_pid_api};

/**
 * @brief USB Host DFU-runtime driver table
 */
static __maybe_unused DEVICE_API(usbh_dfurt, usbh_dfurt_api) = {
	.settings = usbh_dfu_settings_api, .enter_dfu = usbh_dfurt_enter_dfu_api};

#define DT_DRV_COMPAT zephyr_usbh_dfu_device
#define USBH_DFU_DEVICE_DEFINE(n)                                                                  \
	COND_CODE_0(DT_INST_PROP(n, match_class),                                           \
	(static struct usbh_class_filter const usbh_dfu_vid_pid_filters_##n[] = {           \
		{                                                                           \
			.flags = USBH_CLASS_MATCH_VID_PID,                                  \
			.vid = (DT_INST_REG_ADDR(n) >> 16u) & 0xFFFFu,                      \
			.pid = DT_INST_REG_ADDR(n) & 0xFFFFu,                               \
		},                                                                          \
		{0u}                                                                        \
	};), ())      \
	static struct usbh_dfu_drv_data usbh_dfu_drv_data##n;                                      \
	DEVICE_DT_INST_DEFINE(n, NULL, NULL, &usbh_dfu_drv_data##n, NULL, POST_KERNEL,             \
			      CONFIG_USBH_DFU_INIT_PRIORITY, &usbh_dfu_api);                       \
	USBH_DEFINE_CLASS(                                                                         \
		dfu_host_c_data_##n, &usbh_dfu_class_api, (void *)DEVICE_DT_INST_GET(n),           \
		COND_CODE_1(DT_INST_PROP(n, match_class),                                   \
				(usbh_dfu_filters), (usbh_dfu_vid_pid_filters_##n)));
DT_INST_FOREACH_STATUS_OKAY(USBH_DFU_DEVICE_DEFINE)

#undef DT_DRV_COMPAT
#undef USBH_DFU_DEVICE_DEFINE
#define DT_DRV_COMPAT zephyr_usbh_dfurt_device
#define USBH_DFU_DEVICE_DEFINE(n)                                                                  \
	COND_CODE_0(DT_INST_PROP(n, match_class),                                           \
	(static struct usbh_class_filter const usbh_dfurt_vid_pid_filters_##n[] = {         \
		{                                                                           \
			.flags = USBH_CLASS_MATCH_VID_PID,                                  \
			.vid = (DT_INST_REG_ADDR(n) >> 16u) & 0xFFFFu,                      \
			.pid = DT_INST_REG_ADDR(n) & 0xFFFFu,                               \
		},                                                                          \
		{0u}                                                                        \
	};), ())      \
	static struct usbh_dfu_drv_data usbh_dfurt_data##n;                                        \
	DEVICE_DT_INST_DEFINE(n, NULL, NULL, &usbh_dfurt_data##n, NULL, POST_KERNEL,               \
			      CONFIG_USBH_DFU_INIT_PRIORITY, &usbh_dfurt_api);                     \
	USBH_DEFINE_CLASS(                                                                         \
		dfurt_host_c_data_##n, &usbh_dfurt_class_api, (void *)DEVICE_DT_INST_GET(n),       \
		COND_CODE_1(DT_INST_PROP(n, match_class),                                \
				(usbh_dfurt_filters), (usbh_dfurt_vid_pid_filters_##n)));
DT_INST_FOREACH_STATUS_OKAY(USBH_DFU_DEVICE_DEFINE)
