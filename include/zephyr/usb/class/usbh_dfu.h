/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB Device Firmware Update (DFU) driver API
 *
 * Header follows Device Class Definition for Device Firmware Update (DFU)
 * Version 1.1 document (DFU_1.1.pdf).
 */

#ifndef ZEPHYR_INCLUDE_USB_CLASS_USBH_DFU_H_
#define ZEPHYR_INCLUDE_USB_CLASS_USBH_DFU_H_

#include <zephyr/device.h>
#include <zephyr/usb/class/usb_dfu_common.h>
#include <zephyr/syscall.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Ignore the device not transitioning from MANIFEST-SYNC to MANIFEST stage on GET_STATUS */
#define USBH_DFU_QUIRK_IGNORE_DNLOAD_COMPLETE_CHECK (1U << 0)

/**
 * Optional structure for non-default settings, tweaks and quirks
 */
struct usbh_dfu_settings {
	/** Bitmap where each bit represents a specific device non-conformance with the DFU
	 * standard. e.g. Not transitioning from MANIFEST-SYNC to MANIFEST stage on a GET_STATUS
	 * request. Use one of 'USBH_DFU_QUIRK_' macros
	 */
	uint32_t quirks;
	/** Exposes multiple storage targets (flash, EEPROM, etc.) via different 'alternate_idx' */
	uint16_t alternate_idx;
	/** Timeout after a detach request, after which the device resumes normal operation if a
	 * bus reset is not issued. A value of zero uses the recommended device 'wDetachTimeOut'
	 * instead.
	 */
	uint16_t detach_timeout_ms;
};

/**
 * @brief App callback for received data from USB Device
 *
 * @param cb_arg    Pointer to app specific context structure
 * @param data      Pointer to received data of USB Device firmware
 * @param len       Length of the data. Zero means firmware upload is complete
 *
 * @return 0 on success, negative errno value on failure.
 */
typedef int (*usbh_dfu_upload_block_cb_t)(void *cb_arg, char *data, const size_t len);

/**
 * @brief App callback for transmitted data to USB Device
 *
 * @param cb_arg    Pointer to app specific context structure
 * @param data      Pointer to data that will be transferred to USB Device
 * @param len       Maximum length of the data
 *
 * @return 0 on firmware complete, positive length of bytes to transfer, negative errno value
 */
typedef int (*usbh_dfu_dnload_block_cb_t)(void *cb_arg, char *data, const size_t len);

/**
 * @brief Optional function, configure non-default driver settings for tweaks and quirks
 *
 * @param dev           Pointer to the device
 * @param settings      Pointer to DFU setting structure to select alternate function, timeouts
 *
 * @return 0 on success, negative errno value on failure.
 */
typedef int (*usbh_dfu_settings_t)(struct device const *dev, struct usbh_dfu_settings *settings);

/**
 * @brief Starts the FW upload from USB Device to USB Host.
 *
 * Performs DFU initialization, DFU upload and invokes upload_cb
 * on every received data block.
 *
 * @param dev           Pointer to the device
 * @param upload_cb     Callback for received data
 * @param upload_arg    Pointer to application/callback context data
 *
 * @return 0 on success, negative errno value on failure.
 */
typedef int (*usbh_dfu_upload_t)(struct device const *dev, usbh_dfu_upload_block_cb_t upload_cb,
				 void *upload_arg);

/**
 * @brief Starts the FW download from USB Host to USB Device.
 *
 * Performs DFU initialization, DFU download and invokes dnload_cb
 * before transmission to USB Device
 *
 * @param dev           Pointer to the device
 * @param dnload_cb     Callback for transferred data
 * @param dnload_arg    Pointer to application/callback context data
 *
 * @return 0 on success, negative errno value on failure.
 */
typedef int (*usbh_dfu_dnload_t)(struct device const *dev, usbh_dfu_dnload_block_cb_t dnload_cb,
				 void *dnload_arg);

/**
 * @brief Starts the FW download from USB Host to USB Device.
 *
 * Return status of DFU state machine
 *
 * @param dev           Pointer to the device
 *
 * @return positive status on success, negative errno value on failure.
 */
typedef int (*usbh_dfu_get_status_t)(struct device const *dev);

/**
 * @brief Check whether the device matches requested VID/PID
 *
 * Return whether device matches requested VID/PID, 0xFFFF value means match any.
 *
 * @param dev           Pointer to the device
 * @param vid           Requested VendorID
 * @param pid           Requested ProductID
 *
 * @return negative errno on failure, 1 for accepted, 0 for rejected
 */
typedef int (*usbh_dfu_match_vid_pid_t)(struct device const *dev, uint16_t vid, uint16_t pid);

/**
 * @driver_ops{USBH DFU}
 */
__subsystem struct usbh_dfu_driver_api {
	/**
	 * @driver_ops_optional @copybrief usbh_dfu_settings
	 */
	usbh_dfu_settings_t settings;
	/**
	 * @driver_ops_mandatory @copybrief usbh_dfu_upload
	 */
	usbh_dfu_upload_t upload;
	/**
	 * @driver_ops_mandatory @copybrief usbh_dfu_dnload
	 */
	usbh_dfu_dnload_t dnload;
	/**
	 * @driver_ops_optional @copybrief usbh_dfu_get_status
	 */
	usbh_dfu_get_status_t get_status;
	/**
	 * @driver_ops_optional @copybrief usbh_dfu_match_vid_pid
	 */
	usbh_dfu_match_vid_pid_t match_vid_pid;
};

/**
 * @brief Optional function, configure non-default driver settings for tweaks and quirks
 *
 * @param dev           Pointer to the device
 * @param settings      Pointer to DFU setting structure to select alternate function, timeouts, ...
 *
 * @return 0 on success, negative errno value on failure.
 */
typedef int (*usbh_dfurt_settings_t)(struct device const *dev, struct usbh_dfu_settings *settings);

/**
 * @brief Switch DFU runtime device to DFU mode
 *
 * @param dev           Pointer to the device
 *
 * @return 0 on success, negative errno value on failure.
 */
typedef int (*usbh_dfurt_enter_dfu_t)(struct device const *dev);

/**
 * @driver_ops{USBH DFURT}
 */
__subsystem struct usbh_dfurt_driver_api {
	/**
	 * @driver_ops_optional @copybrief usbh_dfurt_settings
	 */
	usbh_dfurt_settings_t settings;
	/**
	 * @driver_ops_optional @copybrief usbh_dfurt_enter_dfu
	 */
	usbh_dfurt_enter_dfu_t enter_dfu;
};

/**
 * @brief Configure DFU optional settings for tweaks and quirks
 *
 * @param dev           Pointer to the device
 * @param settings      Optional, pointer to DFU setting structure to select alternate function,
 * timeouts, ...
 *
 * @return 0 on success, negative errno value on failure.
 */
__syscall int usbh_dfu_settings(struct device const *dev, struct usbh_dfu_settings *settings);

static inline int z_impl_usbh_dfu_settings(struct device const *dev,
					   struct usbh_dfu_settings *settings)
{
	struct usbh_dfu_driver_api const *api = DEVICE_API_GET(usbh_dfu, dev);

	if (api->settings == NULL) {
		return -ENOSYS;
	}
	return api->settings(dev, settings);
}

/**
 * @brief Starts the FW upload from USB Device to USB Host.
 *
 * Performs DFU initialization, DFU upload and invokes upload_cb
 * on every received data block.
 *
 * @param dev           Pointer to the device
 * @param upload_cb     Callback for received data
 * @param upload_arg    Pointer to application/callback context data
 *
 * @return 0 on success, negative errno value on failure.
 */
__syscall int usbh_dfu_upload(struct device const *dev, usbh_dfu_upload_block_cb_t upload_cb,
			      void *upload_arg);

static inline int z_impl_usbh_dfu_upload(struct device const *dev,
					 usbh_dfu_upload_block_cb_t upload_cb, void *upload_arg)
{
	struct usbh_dfu_driver_api const *api = DEVICE_API_GET(usbh_dfu, dev);

	if (api->upload == NULL) {
		return -ENOSYS;
	}
	return api->upload(dev, upload_cb, upload_arg);
}

/**
 * @brief Starts the FW download from USB Host to USB Device.
 *
 * Performs DFU initialization, DFU download and invokes dnload_cb
 * before transmission to USB Device
 *
 * @param dev           Pointer to the device
 * @param dnload_cb     Callback for transferred data
 * @param dnload_arg    Pointer to application/callback context data
 *
 * @return 0 on success, negative errno value on failure.
 */
__syscall int usbh_dfu_dnload(struct device const *dev, usbh_dfu_dnload_block_cb_t dnload_cb,
			      void *dnload_arg);

static inline int z_impl_usbh_dfu_dnload(struct device const *dev,
					 usbh_dfu_dnload_block_cb_t dnload_cb, void *dnload_arg)
{
	struct usbh_dfu_driver_api const *api = DEVICE_API_GET(usbh_dfu, dev);

	if (api->dnload == NULL) {
		return -ENOSYS;
	}

	return api->dnload(dev, dnload_cb, dnload_arg);
}

/**
 * @brief Return DFU status
 *
 * @param dev           Pointer to the device
 *
 * @return positive DFU status, negative errno value on failure.
 */
__syscall int usbh_dfu_get_status(struct device const *dev);

static inline int z_impl_usbh_dfu_get_status(struct device const *dev)
{
	struct usbh_dfu_driver_api const *api = DEVICE_API_GET(usbh_dfu, dev);

	if (api->get_status == NULL) {
		return -ENOSYS;
	}

	return api->get_status(dev);
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
__syscall int usbh_dfu_match_vid_pid(struct device const *dev, uint16_t vid, uint16_t pid);

static inline int z_impl_usbh_dfu_match_vid_pid(struct device const *dev, uint16_t vid,
						uint16_t pid)
{
	struct usbh_dfu_driver_api const *api = DEVICE_API_GET(usbh_dfu, dev);

	if (api->match_vid_pid == NULL) {
		return -ENOSYS;
	}

	return api->match_vid_pid(dev, vid, pid);
}

/**
 * @brief Switch DFU runtime device to DFU mode
 *
 * Switch DFU runtime device to DFU mode. Depending on device flags
 * it may reset USB bus.
 *
 * @param dev           Pointer to the device
 *
 * @return 0 on success, negative errno value on failure.
 */
__syscall int usbh_dfurt_enter_dfu(struct device const *dev);

static inline int z_impl_usbh_dfurt_enter_dfu(struct device const *dev)
{
	struct usbh_dfurt_driver_api const *api = DEVICE_API_GET(usbh_dfurt, dev);

	if (api->enter_dfu == NULL) {
		return -ENOSYS;
	}

	return api->enter_dfu(dev);
}

/**
 * @brief Configure DFU runtime optional settings for tweaks and quirks
 *
 * @param dev           Pointer to the device
 * @param settings      Optional, pointer to DFU setting structure to select alternate function,
 * timeouts, ...
 *
 * @return 0 on success, negative errno value on failure.
 */
__syscall int usbh_dfurt_settings(struct device const *dev, struct usbh_dfu_settings *settings);

static inline int z_impl_usbh_dfurt_settings(struct device const *dev,
					     struct usbh_dfu_settings *settings)
{
	struct usbh_dfurt_driver_api const *api = DEVICE_API_GET(usbh_dfurt, dev);

	if (api->settings == NULL) {
		return -ENOSYS;
	}
	return api->settings(dev, settings);
}

#include <zephyr/syscalls/usbh_dfu.h>

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_USB_CLASS_USBH_DFU_H_ */
