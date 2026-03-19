/*
 * Copyright (c) 2019 Linaro Limited.
 * Copyright 2025 NXP
 * Copyright (c) 2025 STMicroelectronics
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup video_interface
 * @brief Main header file for video driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_VIDEO_H
#define ZEPHYR_INCLUDE_DRIVERS_VIDEO_H

/**
 * @brief Interfaces for video devices.
 * @defgroup video_interface Video
 * @since 2.1
 * @version 1.3.0
 * @ingroup io_interfaces
 * @{
 */

#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/video/types.h>
#include <zephyr/video/controls.h>
#include <zephyr/video/formats.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Function type of @ref video_driver_set_format(), @ref driver_get_format()
 */
typedef int (*video_api_format_t)(const struct device *dev, struct video_format *fmt);

/**
 * @brief Function type of @ref video_driver_set_frmival(), @ref video_driver_get_frmival()
 */
typedef int (*video_api_frmival_t)(const struct device *dev, struct video_frmival *frmival);

/**
 * @brief Function type of @ref video_driver_enum_frmival()
 */
typedef int (*video_api_enum_frmival_t)(const struct device *dev, struct video_frmival_enum *fie);

/**
 * @brief Function type of @ref video_driver_enqueue()
 */
typedef int (*video_api_enqueue_t)(const struct device *dev, struct video_buffer *buf);

/**
 * @brief Function type of @ref video_driver_dequeue()
 */
typedef int (*video_api_dequeue_t)(const struct device *dev, struct video_buffer **buf,
				   k_timeout_t timeout);

/**
 * @brief Function type of @ref video_driver_flush()
 */
typedef int (*video_api_flush_t)(const struct device *dev, bool cancel);

/**
 * @brief Function type of @ref video_driver_get_caps()
 */
typedef int (*video_api_set_stream_t)(const struct device *dev, bool enable,
				      enum video_buf_type type);

/**
 * @brief Function type of @ref video_driver_set_ctrl(), @ref video_driver_get_volatile_ctrl()
 */
typedef int (*video_api_ctrl_t)(const struct device *dev, uint32_t cid);

/**
 * @brief Function type of @ref video_driver_get_caps()
 */
typedef int (*video_api_get_caps_t)(const struct device *dev, struct video_caps *caps);

/**
 * @brief Function type of @ref video_driver_transform_cap()
 */
typedef int (*video_api_transform_cap_t)(const struct device *const dev,
					 const struct video_format_cap *const cap,
					 struct video_format_cap *const res_cap,
					 enum video_buf_type type, uint16_t ind);

/**
 * @brief Function type of @ref video_driver_set_signal()
 */
typedef int (*video_api_set_signal_t)(const struct device *dev, struct k_poll_signal *sig);

/**
 * @brief Function type of @ref video_driver_set_selection(), @ref video_driver_get_selection()
 */
typedef int (*video_api_selection_t)(const struct device *dev, struct video_selection *sel);

/**
 * @brief Video driver API struct type.
 */
__subsystem struct video_driver_api {
	/** Mandatory */
	video_api_format_t set_format;
	/** Mandatory */
	video_api_format_t get_format;
	/** Mandatory */
	video_api_set_stream_t set_stream;
	/** Mandatory */
	video_api_get_caps_t get_caps;
	/** Optional */
	video_api_enqueue_t enqueue;
	/** Optional */
	video_api_dequeue_t dequeue;
	/** Optional */
	video_api_flush_t flush;
	/** Optional */
	video_api_ctrl_t set_ctrl;
	/** Optional */
	video_api_ctrl_t get_volatile_ctrl;
	/** Optional */
	video_api_set_signal_t set_signal;
	/** Optional */
	video_api_frmival_t set_frmival;
	/** Optional */
	video_api_frmival_t get_frmival;
	/** Optional */
	video_api_enum_frmival_t enum_frmival;
	/** Optional */
	video_api_selection_t set_selection;
	/** Optional */
	video_api_selection_t get_selection;
	/** Optional */
	video_api_transform_cap_t transform_cap;
};

/**
 * @brief Set video format of a driver.
 *
 * Configure video device with a specific format.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param fmt Pointer to a video format struct.
 *
 * @retval 0 If successful.
 * @retval -EINVAL If parameters are invalid.
 * @retval -ENOTSUP If format is not supported.
 * @retval -EIO General input / output error.
 */
static inline int video_driver_set_format(const struct device *dev, struct video_format *fmt)
{
	const struct video_driver_api *api = (const struct video_driver_api *)dev->api;

	if (api->set_format == NULL) {
		return -ENOSYS;
	}

	return api->set_format(dev, fmt);
}

/**
 * @brief Get video format of a driver.
 *
 * Get video device current video format.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param fmt Pointer to video format struct.
 *
 * @retval pointer to video format
 */
static inline int video_driver_get_format(const struct device *dev, struct video_format *fmt)
{
	const struct video_driver_api *api = (const struct video_driver_api *)dev->api;

	if (api->get_format == NULL) {
		return -ENOSYS;
	}

	return api->get_format(dev, fmt);
}

/**
 * @brief Apply a video frame interval to a driver.
 *
 * Drivers must not return an error solely because the requested interval doesn’t match the device
 * capabilities. They must instead modify the interval to match what the hardware can provide.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param frmival Pointer to a video frame interval struct.
 *
 * @retval 0 If successful.
 * @retval -ENOSYS If API is not implemented.
 * @retval -EINVAL If parameters are invalid.
 * @retval -EIO General input / output error.
 */
static inline int video_driver_set_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct video_driver_api *api = (const struct video_driver_api *)dev->api;

	if (api->set_frmival == NULL) {
		return -ENOSYS;
	}

	return api->set_frmival(dev, frmival);
}

/**
 * @brief Get video frame interval of a driver.
 *
 * Get current frame interval of the video device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param frmival Pointer to a video frame interval struct.
 *
 * @retval 0 If successful.
 * @retval -ENOSYS If API is not implemented.
 * @retval -EINVAL If parameters are invalid.
 * @retval -EIO General input / output error.
 */
static inline int video_driver_get_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct video_driver_api *api = (const struct video_driver_api *)dev->api;

	if (api->get_frmival == NULL) {
		return -ENOSYS;
	}

	return api->get_frmival(dev, frmival);
}

/**
 * @brief List video frame intervals.
 *
 * List all supported video frame intervals of a given format.
 *
 * Drivers should fill the pixelformat, width and height fields of @ref video_frmival_enum given
 * as query. The index field is used to iterate through the supported frame intervals for this
 * format.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param fie Pointer to a video frame interval enumeration struct.
 *
 * @retval 0 If successful.
 * @retval -ENOSYS If API is not implemented.
 * @retval -EINVAL If parameters are invalid.
 * @retval -EIO General input / output error.
 */
static inline int video_driver_enum_frmival(const struct device *dev,
					    struct video_frmival_enum *fie)
{
	const struct video_driver_api *api = (const struct video_driver_api *)dev->api;

	if (api->enum_frmival == NULL) {
		return -ENOSYS;
	}

	return api->enum_frmival(dev, fie);
}

/**
 * @brief Pass a video buffer to a driver.
 *
 * Enqueue an empty (capturing) or filled (output) video buffer in the driver’s
 * endpoint incoming queue.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param buf Pointer to the video buffer.
 *
 * @retval 0 If successful.
 * @retval -EINVAL If parameters are invalid.
 * @retval -EIO General input / output error.
 */
static inline int video_driver_enqueue(const struct device *dev, struct video_buffer *vbuf)
{
	const struct video_driver_api *api = (const struct video_driver_api *)dev->api;

	if (api->enqueue == NULL) {
		return -ENOSYS;
	}

	return api->enqueue(dev, vbuf);
}

/**
 * @brief Dequeue a video buffer from a driver.
 *
 * Dequeue a filled (capturing) or displayed (output) buffer from the driver’s
 * endpoint outgoing queue.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param buf Pointer a video buffer pointer.
 * @param timeout Timeout
 *
 * @retval 0 If successful.
 * @retval -EINVAL If parameters are invalid.
 * @retval -EIO General input / output error.
 */
static inline int video_driver_dequeue(const struct device *dev, struct video_buffer **vbuf,
				       k_timeout_t timeout)
{
	const struct video_driver_api *api = (const struct video_driver_api *)dev->api;

	if (api->dequeue == NULL) {
		return -ENOSYS;
	}

	return api->dequeue(dev, vbuf, timeout);
}

/**
 * @brief Flush endpoint buffers from a driver.
 *
 * A call to flush finishes when all endpoint buffers have been moved from
 * incoming queue to outgoing queue. Either because canceled or fully processed
 * through the video function.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param cancel If true, cancel buffer processing instead of waiting for
 *        completion.
 *
 * @retval 0 If successful, -ERRNO code otherwise.
 */
static inline int video_driver_flush(const struct device *dev, bool cancel)
{
	const struct video_driver_api *api = (const struct video_driver_api *)dev->api;

	if (api->flush == NULL) {
		return -ENOSYS;
	}

	return api->flush(dev, cancel);
}

/**
 * @brief Start or stop the video driver function.
 *
 * Start (enable == true) or stop (enable == false) streaming on the video device.
 *
 * @param dev Pointer to the device structure.
 * @param enable If true, start streaming, otherwise stop streaming.
 * @param type The type of the buffers stream to start or stop.
 *
 * @retval 0 Successful.
 * @retval -ENOSYS API is not implemented.
 */
static inline int video_driver_set_stream(const struct device *dev, bool enable,
					  enum video_buf_type type)
{
	const struct video_driver_api *api = (const struct video_driver_api *)dev->api;

	if (api->set_stream == NULL) {
		return -ENOSYS;
	}

	return api->set_stream(dev, true, type);
}

/**
 * @brief Get a volatile video control value of a driver.
 *
 * Update the control value of @p cid by requesting it from the hardware.
 * After this, the matching @c ctrl->val contains the updated value.
 *
 * @param dev Pointer to the device structure.
 * @param cid Id of the control to set/get its value.
 *
 * @retval 0 If successful
 * @retval -ENOSYS API is not implemented.
 */
static inline int video_driver_get_volatile_ctrl(const struct device *dev, uint32_t cid)
{
	const struct video_driver_api *api = (const struct video_driver_api *)dev->api;

	if (api->get_volatile_ctrl == NULL) {
		return -ENOSYS;
	}

	return api->get_volatile_ctrl(dev, cid);
}

/**
 * @brief Set a video control value of a driver.
 *
 * Apply a video control value of @p cid to a device.
 * The value is taken from the matching @c ctrl->val.
 *
 * @param dev Pointer to the device structure.
 * @param cid Id of the control to set/get its value.
 *
 * @retval 0 If successful
 * @retval -ENOSYS API is not implemented.
 */
static inline int video_driver_set_ctrl(const struct device *dev, uint32_t cid)
{
	const struct video_driver_api *api = (const struct video_driver_api *)dev->api;

	if (api->set_ctrl == NULL) {
		return -ENOSYS;
	}

	return api->set_ctrl(dev, cid);
}

/**
 * @brief Get the capabilities of a video driver endpoint.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param caps Pointer to the video_caps struct to fill.
 *
 * @retval 0 If successful, -ERRNO code otherwise.
 * @retval -ENOSYS API is not implemented.
 */
static inline int video_driver_get_caps(const struct device *dev, struct video_caps *caps)
{
	const struct video_driver_api *api = (const struct video_driver_api *)dev->api;

	if (api->get_caps == NULL) {
		return -ENOSYS;
	}

	return api->get_caps(dev, caps);
}

/**
 * @brief Transform a video format capability from one end to the other end of a m2m video device.
 *
 * See @ref video_transform_cap for detailed description and usage.
 *
 * @param dev Pointer to the device structure.
 * @param cap Pointer to the source video format capability structure.
 * @param res_cap Pointer to the resulting video format capability structure, filled by the driver.
 * @param type The @ref video_buf_type of the resulting transformed cap.
 * @param ind Index of the resulting transformed cap.
 *
 * @retval 0 Success.
 * @retval -ENOSYS API is not implemented.
 * @retval -EINVAL Parameters are invalid.
 * @retval -ENOTSUP The transformation is not supported.
 * @retval -EIO General input / output error.
 */
static inline int video_driver_transform_cap(const struct device *const dev,
					     const struct video_format_cap *const cap,
					     struct video_format_cap *const res_cap,
					     enum video_buf_type type, uint16_t ind)
{
	const struct video_driver_api *api = (const struct video_driver_api *)dev->api;

	if (api->transform_cap == NULL) {
		return -ENOSYS;
	}

	return api->transform_cap(dev, cap, res_cap, type, ind);
}

/**
 * @brief Register/Unregister k_poll signal for a video endpoint.
 *
 * Register a poll signal to the endpoint, which will be signaled on frame
 * completion (done, aborted, error). Registering a NULL poll signal
 * unregisters any previously registered signal.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param sig Pointer to k_poll_signal
 *
 * @retval 0 If successful, -ERRNO code otherwise.
 */
static inline int video_driver_set_signal(const struct device *dev, struct k_poll_signal *sig)
{
	const struct video_driver_api *api = (const struct video_driver_api *)dev->api;

	if (api->set_signal == NULL) {
		return -ENOSYS;
	}

	return api->set_signal(dev, sig);
}

/**
 * @brief Set video selection (crop/compose).
 *
 * Configure the optional crop and compose feature of a video device.
 * Crop is first applied on the input frame, and the result of that crop is applied
 * to the compose. The result of the compose (width/height) is equal to the format
 * width/height given to the @ref video_set_format function.
 *
 * Some targets are inter-dependents. For instance, setting a @ref VIDEO_SEL_TGT_CROP will
 * reset @ref VIDEO_SEL_TGT_COMPOSE to the same size.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param sel Pointer to a video selection structure
 *
 * @retval 0 If successful.
 * @retval -EINVAL If parameters are invalid.
 * @retval -ENOTSUP If format is not supported.
 * @retval -EIO General input / output error.
 */
static inline int video_driver_set_selection(const struct device *dev, struct video_selection *sel)
{
	const struct video_driver_api *api = (const struct video_driver_api *)dev->api;

	if (api->set_selection == NULL) {
		return -ENOSYS;
	}

	return api->set_selection(dev, sel);
}

/**
 * @brief Get video selection (crop/compose).
 *
 * Retrieve the current settings related to the crop and compose of the video device.
 * This can also be used to read the native size of the input stream of the video
 * device.
 * This function can be used to read crop / compose capabilities of the device prior
 * to performing configuration via the @ref video_set_selection api.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param sel Pointer to a video selection structure, @c type and @c target set by the caller
 *
 * @retval 0 If successful.
 * @retval -EINVAL If parameters are invalid.
 * @retval -ENOTSUP If format is not supported.
 * @retval -EIO General input / output error.
 */
static inline int video_driver_get_selection(const struct device *dev, struct video_selection *sel)
{
	const struct video_driver_api *api = (const struct video_driver_api *)dev->api;

	if (api->get_selection == NULL) {
		return -ENOSYS;
	}

	return api->get_selection(dev, sel);
}

/**
 * @name MIPI CSI-2 Data Types
 * @brief Standard MIPI CSI-2 data type identifiers for camera sensor interfaces
 *
 * These constants define the data type field values used in MIPI CSI-2 packet headers to identify
 * the format and encoding of transmitted image data. The data type field is 6 bits wide, allowing
 * values from 0x00 to 0x3F.
 *
 * @{
 */

/** NULL data type - used for padding or synchronization */
#define VIDEO_MIPI_CSI2_DT_NULL                 0x10
/** Blanking data - horizontal/vertical blanking information */
#define VIDEO_MIPI_CSI2_DT_BLANKING             0x11
/** Embedded 8-bit data - sensor metadata or configuration data */
#define VIDEO_MIPI_CSI2_DT_EMBEDDED_8           0x12
/** YUV 4:2:0 format with 8 bits per component */
#define VIDEO_MIPI_CSI2_DT_YUV420_8             0x18
/** YUV 4:2:0 format with 10 bits per component */
#define VIDEO_MIPI_CSI2_DT_YUV420_10            0x19
/** YUV 4:2:0 CSPS (Chroma Shifted Pixel Sampling) 8-bit format */
#define VIDEO_MIPI_CSI2_DT_YUV420_CSPS_8	0x1c
/** YUV 4:2:0 CSPS (Chroma Shifted Pixel Sampling) 10-bit format */
#define VIDEO_MIPI_CSI2_DT_YUV420_CSPS_10	0x1d
/** YUV 4:2:2 format with 8 bits per component */
#define VIDEO_MIPI_CSI2_DT_YUV422_8             0x1e
/** YUV 4:2:2 format with 10 bits per component */
#define VIDEO_MIPI_CSI2_DT_YUV422_10            0x1f
/** RGB format with 4 bits per color component */
#define VIDEO_MIPI_CSI2_DT_RGB444               0x20
/** RGB format with 5 bits per color component */
#define VIDEO_MIPI_CSI2_DT_RGB555               0x21
/** RGB format with 5-6-5 bits per R-G-B components */
#define VIDEO_MIPI_CSI2_DT_RGB565               0x22
/** RGB format with 6 bits per color component */
#define VIDEO_MIPI_CSI2_DT_RGB666               0x23
/** RGB format with 8 bits per color component */
#define VIDEO_MIPI_CSI2_DT_RGB888               0x24
/** Raw sensor data with 6 bits per pixel */
#define VIDEO_MIPI_CSI2_DT_RAW6                 0x28
/** Raw sensor data with 7 bits per pixel */
#define VIDEO_MIPI_CSI2_DT_RAW7                 0x29
/** Raw sensor data with 8 bits per pixel */
#define VIDEO_MIPI_CSI2_DT_RAW8                 0x2a
/** Raw sensor data with 10 bits per pixel */
#define VIDEO_MIPI_CSI2_DT_RAW10                0x2b
/** Raw sensor data with 12 bits per pixel */
#define VIDEO_MIPI_CSI2_DT_RAW12                0x2c
/** Raw sensor data with 14 bits per pixel */
#define VIDEO_MIPI_CSI2_DT_RAW14                0x2d

/**
 * @brief User-defined data type generator macro
 *
 * Generates user-defined data type identifier for custom or proprietary formats.
 * The MIPI CSI-2 specification reserves data types 0x30 to 0x37 for user-specific implementations.
 *
 * @note Parameter n must be in range 0-7 to generate valid user-defined data types
 *
 * @param n User-defined type index (0-7)
 * @return Data type value in user-defined range (0x30-0x37)
 */
#define VIDEO_MIPI_CSI2_DT_USER(n)	(0x30 + (n))

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_VIDEO_H */
