/*
 * Copyright (c) 2019 Linaro Limited.
 * Copyright 2025 NXP
 * Copyright (c) 2025 STMicroelectronics
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public APIs for video
 * @ingroup video_api
 */

#ifndef ZEPHYR_INCLUDE_VIDEO_VIDEO_H
#define ZEPHYR_INCLUDE_VIDEO_VIDEO_H

/**
 * @brief Video Public APIs.
 * @defgroup video_api Video APIs
 * @since 4.4
 * @version 0.1.0
 * @ingroup video
 * @{
 */

#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/video/controls.h>
#include <zephyr/video/formats.h>
#include <zephyr/video/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Video buffering APIs.
 * @defgroup video_api_buffers Video Buffers
 * @{
 */

/**
 * @brief Pass a video buffer to a video device.
 *
 * @note users are recommended to use @ref video_enqueue_buffer instead
 *
 * Enqueue an empty (capturing) or filled (output) video buffer in the driver’s
 * endpoint incoming queue.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param buf Pointer to the video buffer.
 *
 * @retval 0 Is successful.
 * @retval -EINVAL If parameters are invalid.
 * @retval -EIO General input / output error.
 */
int video_enqueue(const struct device *dev, struct video_buffer *buf);

/**
 * @brief Import an external memory to the video buffer pool
 *
 * Import an externally allocated memory as a @ref video_buffer in the video buffer pool
 *
 * @param mem Pointer to the external memory
 * @param sz Size of the external memory
 * @param idx Returned index of the imported video buffer in the video buffer pool
 *
 * @retval 0 on success or a negative errno code on failure.
 */
int video_import_buffer(uint8_t *mem, size_t sz, uint16_t *idx);

/**
 * @brief Pass a video buffer to a video device.
 *
 * Enqueue an empty (capturing) or filled (output) video buffer in the driver’s
 * endpoint incoming queue.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param vbuf Buffer returned from the video device.
 * @param timeout Timeout duration or K_NO_WAIT
 *
 * @retval 0 Is successful.
 * @retval -EINVAL If parameters are invalid.
 * @retval -EIO General input / output error.
 */
int video_dequeue(const struct device *dev, struct video_buffer **vbuf, k_timeout_t timeout);

/**
 * @brief Allocate aligned video buffer.
 *
 * @param size Size of the video buffer (in bytes).
 * @param align Alignment of the requested memory, must be a power of two.
 * @param timeout Timeout duration or K_NO_WAIT
 *
 * @return pointer to allocated video buffer
 */
struct video_buffer *video_buffer_aligned_alloc(size_t size, size_t align, k_timeout_t timeout);

/**
 * @brief Allocate video buffer.
 *
 * @param size Size of the video buffer (in bytes).
 * @param timeout Timeout duration or K_NO_WAIT
 *
 * @return pointer to allocated video buffer
 */
struct video_buffer *video_buffer_alloc(size_t size, k_timeout_t timeout);

/**
 * @brief Release a video buffer.
 *
 * @param buf Pointer to the video buffer to release.
 *
 * @retval 0 on success or a negative errno on failure
 */
int video_buffer_release(struct video_buffer *buf);

/**
 * @brief Transfer a buffer between 2 video device
 *
 * Helper function which dequeues a buffer from a source device and enqueues it into a
 * sink device, changing its buffer type between the two.
 *
 * @param src		Video device from where buffer is dequeued (source)
 * @param sink		Video device into which the buffer is queued (sink)
 * @param src_type	Video buffer type on the source device
 * @param sink_type	Video buffer type on the sink device
 * @param timeout	Timeout to be applied on dequeue
 *
 * @return 0 on success, otherwise a negative errno code
 */
int video_transfer_buffer(const struct device *src, const struct device *sink,
			  enum video_buf_type src_type, enum video_buf_type sink_type,
			  k_timeout_t timeout);

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
int video_set_signal(const struct device *dev, struct k_poll_signal *sig);

/**
 * @}
 */

/**
 * @brief Video control APIs.
 * @defgroup video_api_control Video Control
 * @{
 */

/**
 * @struct video_control
 * @brief Video control structure
 *
 * Used to get/set a video control.
 * @see video_ctrl for the struct used in the driver implementation
 */
struct video_control {
	/** control id */
	uint32_t id;
	/** control value */
	union {
		int32_t val;
		int64_t val64;
	};
};

/**
 * @struct video_control_query
 * @brief Video control query structure
 *
 * Used to query information about a control.
 */
struct video_ctrl_query {
	/** device being queried, application needs to set this field */
	const struct device *dev;
	/** control id, application needs to set this field */
	uint32_t id;
	/** control type */
	uint32_t type;
	/** control name */
	const char *name;
	/** control flags */
	uint32_t flags;
	/** control range */
	struct video_ctrl_range range;
	/** menu if control is of menu type */
	union {
		const char *const *menu;
		const int64_t *int_menu;
	};
};

/**
 * @brief Set the value of a control.
 *
 * This set the value of a video control, value type depends on control ID, and
 * must be interpreted accordingly.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param control Pointer to the video control struct.
 *
 * @retval 0 Is successful.
 * @retval -EINVAL If parameters are invalid.
 * @retval -ENOTSUP If format is not supported.
 * @retval -EIO General input / output error.
 */
int video_set_ctrl(const struct device *dev, struct video_control *control);

/**
 * @brief Get the current value of a control.
 *
 * This retrieve the value of a video control, value type depends on control ID,
 * and must be interpreted accordingly.
 *
 * @param dev Pointer to the device structure.
 * @param control Pointer to the video control struct.
 *
 * @retval 0 Is successful.
 * @retval -EINVAL If parameters are invalid.
 * @retval -ENOTSUP If format is not supported.
 * @retval -EIO General input / output error.
 */
int video_get_ctrl(const struct device *dev, struct video_control *control);

/**
 * @brief Query information about a control.
 *
 * Applications set the id field of the query structure, the function fills the rest of this
 * structure. It is possible to enumerate base class controls (i.e., VIDEO_CID_BASE + x) by calling
 * this function with successive id values starting from VIDEO_CID_BASE up to and exclusive
 * VIDEO_CID_LASTP1. The function may return -ENOTSUP if a control in this range is not supported.
 * Applications can also enumerate private controls by starting at VIDEO_CID_PRIVATE_BASE and
 * incrementing the id until the driver returns -ENOTSUP. For other control classes, it's a bit more
 * difficult. Hence, the best way to enumerate all kinds of device's supported controls is to
 * iterate with VIDEO_CTRL_FLAG_NEXT_CTRL.
 *
 * @param cq Pointer to the control query struct.
 *
 * @retval 0 If successful.
 * @retval -EINVAL If the control id is invalid.
 * @retval -ENOTSUP If the control id is not supported.
 */
int video_query_ctrl(struct video_ctrl_query *cq);

/**
 * @brief Print all the information of a control.
 *
 * Print all the information of a control including its name, type, flag, range,
 * menu (if any) and current value, i.e. by invoking the video_get_ctrl(), in a
 * human readble format.
 *
 * @param cq Pointer to the control query struct.
 */
void video_print_ctrl(const struct video_ctrl_query *const cq);

/**
 * @brief Return the link-frequency advertised by a device
 *
 * Device exposing a CSI link should advertise at least one of the following two controls:
 *   - @ref VIDEO_CID_LINK_FREQ
 *   - @ref VIDEO_CID_PIXEL_RATE
 *
 * At first the helper will try read the @ref VIDEO_CID_LINK_FREQ and if not available will
 * approximate the link-frequency from the @ref VIDEO_CID_PIXEL_RATE value, taking into
 * consideration the bits per pixel of the format and the number of lanes.
 *
 * @param dev Video device to query.
 * @param bpp Amount of bits per pixel of the pixel format produced by the device
 * @param lane_nb Number of CSI-2 lanes used
 */
int64_t video_get_csi_link_freq(const struct device *dev, uint8_t bpp, uint8_t lane_nb);

/**
 * @}
 */

/**
 * @brief Video streaming state control.
 * @defgroup video_api_stream Video Stream
 * @{
 */

/**
 * @brief Start the video device function.
 *
 * video_stream_start is called to enter ‘streaming’ state (capture, output...).
 * The driver may receive buffers with video_enqueue() before video_stream_start
 * is called. If driver/device needs a minimum number of buffers before being
 * able to start streaming, then driver set the min_vbuf_count to the related
 * endpoint capabilities.
 *
 * @param dev Pointer to the device structure.
 * @param type The type of the buffers stream to start.
 *
 * @retval 0 Successful.
 * @retval -EINVAL Parameters are invalid.
 * @retval -EIO General input / output error.
 */
int video_stream_start(const struct device *dev, enum video_buf_type type);

/**
 * @brief Stop the video device function.
 *
 * On video_stream_stop, driver must stop any transactions or wait until they
 * finish.
 *
 * @param dev Pointer to the device structure.
 * @param type The type of the buffers stream to stop.
 *
 * @retval 0 Successful.
 * @retval -EINVAL Parameters are invalid.
 * @retval -EIO General input / output error.
 */
int video_stream_stop(const struct device *dev, enum video_buf_type type);

/**
 * @}
 */

/**
 * @brief Video frame interval APIs.
 * @defgroup video_api_frmival Video Frame Interval
 * @{
 */

/**
 * @brief List video frame intervals.
 *
 * List all supported video frame intervals of a given format.
 *
 * Applications should fill the pixelformat, width and height fields of the
 * @ref video_frmival_enum struct first to form a query. Then, the index field is
 * used to iterate through the supported frame intervals list.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param fie Pointer to a video frame interval enumeration struct.
 *
 * @retval 0 If successful.
 * @retval -ENOSYS If API is not implemented.
 * @retval -EINVAL If parameters are invalid.
 * @retval -EIO General input / output error.
 */
int video_enum_frmival(const struct device *dev, struct video_frmival_enum *fie);

/**
 * @brief Set video frame interval.
 *
 * Configure video device with a specific frame interval, using the closest matchiing frame
 * interval that  the driver can provide, updating @p frmival with the value effectively
 * applied to the driver.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param frmival Pointer to a video frame interval struct.
 *
 * @retval 0 If successful.
 * @retval -ENOSYS If API is not implemented.
 * @retval -EINVAL If parameters are invalid.
 * @retval -EIO General input / output error.
 */
int video_set_frmival(const struct device *dev, struct video_frmival *frmival);

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
int video_get_frmival(const struct device *dev, struct video_frmival *frmival);

/**
 * @brief Compute the difference between two frame intervals
 *
 * @param frmival Frame interval to turn into microseconds.
 *
 * @return The frame interval value in microseconds.
 */
static inline uint64_t video_frmival_nsec(const struct video_frmival *frmival)
{
	if (frmival == NULL || frmival->denominator == 0) {
		return -EINVAL;
	}

	return (uint64_t)NSEC_PER_SEC * frmival->numerator / frmival->denominator;
}

/**
 * @brief Find the closest match to a frame interval value within a stepwise frame interval.
 *
 * @param stepwise The stepwise frame interval range to search
 * @param desired The frame interval for which find the closest match
 * @param match The resulting frame interval closest to @p desired
 */
void video_closest_frmival_stepwise(const struct video_frmival_stepwise *stepwise,
				    const struct video_frmival *desired,
				    struct video_frmival *match);

/**
 * @brief Find the closest match to a frame interval value within a video device.
 *
 * To compute the closest match, fill @p match with the following fields:
 *
 * - @c match->format to the @ref video_format of interest.
 * - @c match->type to @ref VIDEO_FRMIVAL_TYPE_DISCRETE.
 * - @c match->discrete to the desired frame interval.
 *
 * The result will be loaded into @p match, with the following fields set:
 *
 * - @c match->discrete to the value of the closest frame interval.
 * - @c match->index to the index of the closest frame interval.
 *
 * @param dev Video device to query.
 * @param match Frame interval enumerator with the query, and loaded with the result.
 */
void video_closest_frmival(const struct device *dev, struct video_frmival_enum *match);

/**
 * @}
 */

/**
 * @brief Video format APIs.
 * @defgroup video_api_format Video Format
 * @{
 */

/**
 * @brief Estimate the size and pitch in bytes of a @ref video_format
 *
 * This helper should only be used by drivers that support the whole image frame.
 *
 * For uncompressed formats, it gives the actual size and pitch of the
 * whole raw image without any padding.
 *
 * For compressed formats, it gives a rough estimate size of a complete
 * compressed frame.
 *
 * @param fmt Pointer to the video format structure
 * @return 0 on success, otherwise a negative errno code
 */
int video_estimate_fmt_size(struct video_format *fmt);

/**
 * @brief Set compose rectangle (if applicable) prior to setting format
 *
 * Some devices expose compose capabilities, allowing them to apply a transformation
 * (downscale / upscale) to the frame. For those devices, it is necessary to set the
 * compose rectangle before being able to apply the frame format (which must have the
 * same width / height as the compose rectangle width / height).
 * In order to allow non-compose aware application to be able to control such devices,
 * introduce a helper which, if available, will apply the compose rectangle prior to
 * setting the format.
 *
 * @param dev Pointer to the video device struct to set format
 * @param fmt Pointer to a video format struct.
 *
 * @retval 0 Is successful.
 * @retval -EINVAL If parameters are invalid.
 * @retval -ENOTSUP If format is not supported.
 * @retval -EIO General input / output error.
 */
int video_set_compose_format(const struct device *dev, struct video_format *fmt);

/**
 * @brief Set video format.
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
int video_set_format(const struct device *dev, struct video_format *fmt);

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
int video_get_format(const struct device *dev, struct video_format *fmt);

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
int video_set_selection(const struct device *dev, struct video_selection *sel);

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
int video_get_selection(const struct device *dev, struct video_selection *sel);

/**
 * @}
 */

/**
 * @brief Video capabilities APIs.
 * @defgroup video_api_caps Video Capabilities
 * @{
 */

/**
 * @brief Get the capabilities of a video endpoint.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param caps Pointer to the video_caps struct to fill.
 *
 * @retval 0 If successful, -ERRNO code otherwise.
 * @retval -ENOSYS API is not implemented.
 */
int video_get_caps(const struct device *dev, struct video_caps *caps);

/**
 * @brief Search for a format that matches in a list of capabilities
 *
 * @param fmts The format capability list to search.
 * @param fmt The format to find in the list.
 * @param idx The pointer to a number of the first format that matches.
 *
 * @return 0 when a format is found.
 * @return -ENOENT when no matching format is found.
 */
int video_format_caps_index(const struct video_format_cap *fmts, const struct video_format *fmt,
			    size_t *idx);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_VIDEO_VIDEO_H */
