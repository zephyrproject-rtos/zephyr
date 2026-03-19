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
 * @brief Video Driver APIs, for use by the @ref video_api.
 * @defgroup video_interface Video Drivers
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
#include <zephyr/video/video.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Storage type for a wrapper type around a video device
 *
 * These are connected together as a list of devices, used to propagate
 * controls along a chain of subdevices.
 */
struct video_device {
	/** Device held by this video device */
	const struct device *dev;
	/** Source device up in the chain, and providing the data */
	const struct device *src_dev;
	/** List of video controls supported by this device */
	sys_dlist_t ctrls;
};

/**
 * @brief Find the @ref video_device associated with a video @ref device.
 *
 * @param dev Device that will be looked up in the global list of video devices.
 * @return Pointer to the @ref video_device if found.
 * @retval NULL if not found.
 */
struct video_device *video_find_vdev(const struct device *dev);

/**
 * @brief Types used to access drivers video controls
 * @defgroup video_interface_ctrls Video Controls
 * @ingroup video_interface
 * @{
 */

/** Control is read-only */
#define VIDEO_CTRL_FLAG_READ_ONLY  BIT(0)
/** Control is write-only */
#define VIDEO_CTRL_FLAG_WRITE_ONLY BIT(1)
/** Control that needs a freshly read as constantly updated by HW */
#define VIDEO_CTRL_FLAG_VOLATILE   BIT(2)
/** Control is inactive, e.g. manual controls of an autocluster in automatic mode */
#define VIDEO_CTRL_FLAG_INACTIVE   BIT(3)
/** Control that affects other controls, e.g. the primary control of a cluster */
#define VIDEO_CTRL_FLAG_UPDATE     BIT(4)

/**
 * @brief Type of video control
 *
 * This is also affecting the size (32-bit or 64-bit).
 */
enum video_ctrl_type {
	/** Boolean type */
	VIDEO_CTRL_TYPE_BOOLEAN = 1,
	/** Integer type */
	VIDEO_CTRL_TYPE_INTEGER = 2,
	/** 64-bit integer type */
	VIDEO_CTRL_TYPE_INTEGER64 = 3,
	/** Menu string type, standard or driver-defined menu */
	VIDEO_CTRL_TYPE_MENU = 4,
	/** String type */
	VIDEO_CTRL_TYPE_STRING = 5,
	/** Menu integer type, standard or driver-defined menu */
	VIDEO_CTRL_TYPE_INTEGER_MENU = 6,
};

/**
 * @brief Storage type for video controls
 *
 * @see video_control for the struct used in public API
 */
struct video_ctrl {
	/** Internal use for video control grouping */
	struct video_ctrl *cluster;
	/** Internal use for video control grouping */
	uint8_t cluster_sz;
	/** Internal use for auto control handling */
	bool is_auto;
	/** Internal use for volatile control handling */
	bool has_volatiles;

	/** Video device associated with this control */
	const struct video_device *vdev;
	/** Control ID, see @ref video_control_ids */
	uint32_t id;
	/** Type of the control */
	enum video_ctrl_type type;
	/** Flags associated with this control */
	unsigned long flags;
	/** Acceptable range of values for this control */
	struct video_ctrl_range range;
	/** Current value of this control */
	union {
		/** Used for 32-bit control types */
		int32_t val;
		/** Used for 64-bit control types */
		int64_t val64;
	};
	/** Reference to list of possible values for menu controls types */
	union {
		/** Used for menu control types */
		const char *const *menu;
		/** Used for integer menu control types */
		const int64_t *int_menu;
	};
	/** Node place this control in a list */
	sys_dnode_t node;
};

/**
 * @}
 */

/**
 * @def_driverbackendgroup{Video,video_interface}
 * @{
 */

/**
 * @brief Callback API to set or get video format.
 * See @a video_driver_set_format() and @a video_driver_get_format() for argument description.
 */
typedef int (*video_api_format_t)(const struct device *dev, struct video_format *fmt);

/**
 * @brief Callback API to set or get video frame interval.
 * See @a video_driver_set_frmival() and @a video_driver_get_frmival() for argument description.
 */
typedef int (*video_api_frmival_t)(const struct device *dev, struct video_frmival *frmival);

/**
 * @brief Callback API to enumerate supported frame intervals for a format.
 * See @a video_driver_enum_frmival() for argument description.
 */
typedef int (*video_api_enum_frmival_t)(const struct device *dev, struct video_frmival_enum *fie);

/**
 * @brief Callback API to enqueue a buffer in the driver incoming queue.
 * See @a video_driver_enqueue() for argument description.
 */
typedef int (*video_api_enqueue_t)(const struct device *dev, struct video_buffer *buf);

/**
 * @brief Callback API to dequeue a buffer from the driver outgoing queue.
 * See @a video_driver_dequeue() for argument description.
 */
typedef int (*video_api_dequeue_t)(const struct device *dev, struct video_buffer **buf,
				   k_timeout_t timeout);

/**
 * @brief Callback API to flush endpoint buffers.
 * See @a video_driver_flush() for argument description.
 */
typedef int (*video_api_flush_t)(const struct device *dev, bool cancel);

/**
 * @brief Callback API to control stream status.
 * See @a video_driver_set_stream() for argument description.
 */
typedef int (*video_api_set_stream_t)(const struct device *dev, bool enable,
				      enum video_buf_type type);

/**
 * @brief Callback API to set or get a video control value.
 * See @a video_driver_set_ctrl() and @a video_driver_get_volatile_ctrl() for argument description.
 */
typedef int (*video_api_ctrl_t)(const struct device *dev, uint32_t cid);

/**
 * @brief Callback API to get capabilities of a video endpoint.
 * See @a video_get_caps() for argument description.
 */
typedef int (*video_api_get_caps_t)(const struct device *dev, struct video_caps *caps);

/**
 * @brief Callback API to transform a format capability across m2m device endpoints.
 * See @a video_transform_cap() for argument description.
 */
typedef int (*video_api_transform_cap_t)(const struct device *const dev,
					 const struct video_format_cap *const cap,
					 struct video_format_cap *const res_cap,
					 enum video_buf_type type, uint16_t ind);

/**
 * @brief Callback API to register or unregister poll signal for buffer events.
 * See @a video_set_signal() for argument description.
 */
typedef int (*video_api_set_signal_t)(const struct device *dev, struct k_poll_signal *sig);

/**
 * @brief Callback API to set or get video selection (crop/compose).
 * See @a video_driver_set_selection() and @a video_driver_get_selection() for argument description.
 */
typedef int (*video_api_selection_t)(const struct device *dev, struct video_selection *sel);

/**
 * @driver_ops{Video}
 */
__subsystem struct video_driver_api {
	/**
	 * @driver_ops_mandatory @copybrief video_set_format
	 */
	video_api_format_t set_format;
	/**
	 * @driver_ops_mandatory @copybrief video_get_format
	 */
	video_api_format_t get_format;
	/**
	 * @driver_ops_mandatory @copybrief video_stream_start
	 */
	video_api_set_stream_t set_stream;
	/**
	 * @driver_ops_mandatory @copybrief video_get_caps
	 */
	video_api_get_caps_t get_caps;
	/**
	 * @driver_ops_optional @copybrief video_enqueue
	 */
	video_api_enqueue_t enqueue;
	/**
	 * @driver_ops_optional @copybrief video_dequeue
	 */
	video_api_dequeue_t dequeue;
	/**
	 * @driver_ops_optional @copybrief video_flush
	 */
	video_api_flush_t flush;
	/**
	 * @driver_ops_optional @copybrief video_set_ctrl
	 */
	video_api_ctrl_t set_ctrl;
	/**
	 * @driver_ops_optional @copybrief video_get_ctrl
	 */
	video_api_ctrl_t get_volatile_ctrl;
	/**
	 * @driver_ops_optional @copybrief video_set_signal
	 */
	video_api_set_signal_t set_signal;
	/**
	 * @driver_ops_optional @copybrief video_set_frmival
	 */
	video_api_frmival_t set_frmival;
	/**
	 * @driver_ops_optional @copybrief video_get_frmival
	 */
	video_api_frmival_t get_frmival;
	/**
	 * @driver_ops_optional @copybrief video_enum_frmival
	 */
	video_api_enum_frmival_t enum_frmival;
	/**
	 * @driver_ops_optional @copybrief video_set_selection
	 */
	video_api_selection_t set_selection;
	/**
	 * @driver_ops_optional @copybrief video_get_selection
	 */
	video_api_selection_t get_selection;
	/**
	 * @driver_ops_optional @copybrief video_transform_cap
	 */
	video_api_transform_cap_t transform_cap;
};

/**
 * @}
 */

/**
 * @brief Set video format of a driver.
 *
 * Configure video device with a specific format.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param fmt Pointer to a video format struct.
 *
 * @retval 0 on success.
 * @retval -ENOTSUP Format is not supported.
 * @retval -ENOSYS API is not implemented.
 */
static inline int video_driver_set_format(const struct device *dev, struct video_format *fmt)
{
	const struct video_driver_api *api = DEVICE_API_GET(video, dev);

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
 * @retval 0 on success.
 * @retval -ENOSYS API is not implemented.
 */
static inline int video_driver_get_format(const struct device *dev, struct video_format *fmt)
{
	const struct video_driver_api *api = DEVICE_API_GET(video, dev);

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
 * @retval 0 on success.
 * @retval -ENOSYS API is not implemented.
 */
static inline int video_driver_set_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct video_driver_api *api = DEVICE_API_GET(video, dev);

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
 * @retval 0 on success.
 * @retval -ENOSYS API is not implemented.
 */
static inline int video_driver_get_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct video_driver_api *api = DEVICE_API_GET(video, dev);

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
 * Callers should fill the pixelformat, width and height fields of @ref video_frmival_enum first
 * to form a query. Then, the index field is used to iterate through the supported frame intervals
 * list.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param fie Pointer to a video frame interval enumeration struct.
 *
 * @retval 0 on success.
 * @retval -ENOSYS API is not implemented.
 */
static inline int video_driver_enum_frmival(const struct device *dev,
					    struct video_frmival_enum *fie)
{
	const struct video_driver_api *api = DEVICE_API_GET(video, dev);

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
 * @param vbuf Pointer to the video buffer.
 *
 * @retval 0 on success.
 * @retval -ENOSYS API is not implemented.
 */
static inline int video_driver_enqueue(const struct device *dev, struct video_buffer *vbuf)
{
	const struct video_driver_api *api = DEVICE_API_GET(video, dev);

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
 * @param vbuf Pointer a video buffer pointer.
 * @param timeout Timeout
 *
 * @retval 0 on success.
 * @retval -ENOSYS API is not implemented.
 */
static inline int video_driver_dequeue(const struct device *dev, struct video_buffer **vbuf,
				       k_timeout_t timeout)
{
	const struct video_driver_api *api = DEVICE_API_GET(video, dev);

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
 * @retval 0 on success.
 * @retval -ENOSYS API is not implemented.
 */
static inline int video_driver_flush(const struct device *dev, bool cancel)
{
	const struct video_driver_api *api = DEVICE_API_GET(video, dev);

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
 * @retval 0 on success.
 * @retval -ENOSYS API is not implemented.
 */
static inline int video_driver_set_stream(const struct device *dev, bool enable,
					  enum video_buf_type type)
{
	const struct video_driver_api *api = DEVICE_API_GET(video, dev);

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
 * @retval 0 on success.
 * @retval -ENOSYS API is not implemented.
 */
static inline int video_driver_get_volatile_ctrl(const struct device *dev, uint32_t cid)
{
	const struct video_driver_api *api = DEVICE_API_GET(video, dev);

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
	const struct video_driver_api *api = DEVICE_API_GET(video, dev);

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
 * @retval 0 on success
 * @retval -ENOSYS API is not implemented.
 */
static inline int video_driver_get_caps(const struct device *dev, struct video_caps *caps)
{
	const struct video_driver_api *api = DEVICE_API_GET(video, dev);

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
 * @retval 0 on success.
 * @retval -ENOSYS API is not implemented.
 * @retval -ENOTSUP The transformation is not supported.
 */
static inline int video_driver_transform_cap(const struct device *const dev,
					     const struct video_format_cap *const cap,
					     struct video_format_cap *const res_cap,
					     enum video_buf_type type, uint16_t ind)
{
	const struct video_driver_api *api = DEVICE_API_GET(video, dev);

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
 * @retval 0 on success
 * @retval -ENOSYS API is not implemented.
 */
static inline int video_driver_set_signal(const struct device *dev, struct k_poll_signal *sig)
{
	const struct video_driver_api *api = DEVICE_API_GET(video, dev);

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
 * @retval 0 on success.
 * @retval -ENOSYS API is not implemented.
 */
static inline int video_driver_set_selection(const struct device *dev, struct video_selection *sel)
{
	const struct video_driver_api *api = DEVICE_API_GET(video, dev);

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
 * @retval 0 on success.
 * @retval -ENOSYS API is not implemented.
 * @retval -ENOTSUP Format is not supported.
 */
static inline int video_driver_get_selection(const struct device *dev, struct video_selection *sel)
{
	const struct video_driver_api *api = DEVICE_API_GET(video, dev);

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
 * @brief Map pixel formats to their MIPI data type equivalent
 *
 * Only the formats that were an exact match are mapped to equivalent MIPI data types.
 * A driver might want to handle the non-standard types before calling this function.
 *
 * Mind that while most receivers store @ref VIDEO_MIPI_CSI2_DT_YUV422_8 as YUYV, it is effectively
 * the UYVY format being sent over MIPI lanes.
 *
 * @param pixfmt Pixel format to convert
 * @retval the matching MIPI data type if found
 * @retval VIDEO_MIPI_CSI2_DT_NULL when the format has no known MIPI data type equivalent
 */
static inline uint8_t video_mipi_data_type(uint32_t pixfmt)
{
	switch (pixfmt) {
	case VIDEO_PIX_FMT_GREY:
	case VIDEO_PIX_FMT_SBGGR8:
	case VIDEO_PIX_FMT_SGBRG8:
	case VIDEO_PIX_FMT_SGRBG8:
	case VIDEO_PIX_FMT_SRGGB8:
		return VIDEO_MIPI_CSI2_DT_RAW8;
	case VIDEO_PIX_FMT_Y10P:
	case VIDEO_PIX_FMT_SBGGR10P:
	case VIDEO_PIX_FMT_SGBRG10P:
	case VIDEO_PIX_FMT_SGRBG10P:
	case VIDEO_PIX_FMT_SRGGB10P:
		return VIDEO_MIPI_CSI2_DT_RAW10;
	case VIDEO_PIX_FMT_Y12P:
	case VIDEO_PIX_FMT_SBGGR12P:
	case VIDEO_PIX_FMT_SGBRG12P:
	case VIDEO_PIX_FMT_SGRBG12P:
	case VIDEO_PIX_FMT_SRGGB12P:
		return VIDEO_MIPI_CSI2_DT_RAW12;
	case VIDEO_PIX_FMT_Y14P:
	case VIDEO_PIX_FMT_SBGGR14P:
	case VIDEO_PIX_FMT_SGBRG14P:
	case VIDEO_PIX_FMT_SGRBG14P:
	case VIDEO_PIX_FMT_SRGGB14P:
		return VIDEO_MIPI_CSI2_DT_RAW14;
	case VIDEO_PIX_FMT_UYVY:
		return VIDEO_MIPI_CSI2_DT_YUV422_8;
	case VIDEO_PIX_FMT_RGB565:
		return VIDEO_MIPI_CSI2_DT_RGB565;
	case VIDEO_PIX_FMT_RGB24:
		return VIDEO_MIPI_CSI2_DT_RGB888;
	default:
		return VIDEO_MIPI_CSI2_DT_NULL;
	}
}

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
