/*
 * Copyright (c) 2019 Linaro Limited.
 * Copyright 2025 NXP
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup video_interface
 * @brief Main header file for video driver API.
 */

#ifndef ZEPHYR_INCLUDE_VIDEO_H_
#define ZEPHYR_INCLUDE_VIDEO_H_

/**
 * @brief Interfaces for video devices.
 * @defgroup video_interface Video
 * @since 2.1
 * @version 1.1.0
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/device.h>
#include <stddef.h>
#include <zephyr/kernel.h>

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Flag used by @ref video_caps structure to indicate endpoint operates on
 * buffers the size of the video frame
 */
#define LINE_COUNT_HEIGHT (-1)

struct video_control;

/**
 * @brief video_buf_type enum
 *
 * Supported video buffer types of a video device.
 * The direction (input or output) is defined from the device's point of view.
 * Devices like cameras support only output type, encoders support only input
 * types while m2m devices like ISP, PxP support both input and output types.
 */
enum video_buf_type {
	/** input buffer type */
	VIDEO_BUF_TYPE_INPUT,
	/** output buffer type */
	VIDEO_BUF_TYPE_OUTPUT,
};

/**
 * @brief Video format structure
 *
 * Used to configure frame format.
 */
struct video_format {
	/** type of the buffer */
	enum video_buf_type type;
	/** FourCC pixel format value (\ref video_pixel_formats) */
	uint32_t pixelformat;
	/** frame width in pixels. */
	uint32_t width;
	/** frame height in pixels. */
	uint32_t height;
	/**
	 * @brief line stride.
	 *
	 * This is the number of bytes that needs to be added to the address in the
	 * first pixel of a row in order to go to the address of the first pixel of
	 * the next row (>=width).
	 */
	uint32_t pitch;
};

/**
 * @brief Video format capability
 *
 * Used to describe a video endpoint format capability.
 */
struct video_format_cap {
	/** FourCC pixel format value (\ref video_pixel_formats). */
	uint32_t pixelformat;
	/** minimum supported frame width in pixels. */
	uint32_t width_min;
	/** maximum supported frame width in pixels. */
	uint32_t width_max;
	/** minimum supported frame height in pixels. */
	uint32_t height_min;
	/** maximum supported frame height in pixels. */
	uint32_t height_max;
	/** width step size in pixels. */
	uint16_t width_step;
	/** height step size in pixels. */
	uint16_t height_step;
};

/**
 * @brief Video format capabilities
 *
 * Used to describe video endpoint capabilities.
 */
struct video_caps {
	/** type of the buffer */
	enum video_buf_type type;
	/** list of video format capabilities (zero terminated). */
	const struct video_format_cap *format_caps;
	/** minimal count of video buffers to enqueue before being able to start
	 * the stream.
	 */
	uint8_t min_vbuf_count;
	/** Denotes minimum line count of a video buffer that this endpoint
	 * can fill or process. Each line is expected to consume the number
	 * of bytes the selected video format's pitch uses, so the video
	 * buffer must be at least `pitch` * `min_line_count` bytes.
	 * `LINE_COUNT_HEIGHT` is a special value, indicating the endpoint
	 * only supports video buffers with at least enough bytes to store
	 * a full video frame
	 */
	int16_t min_line_count;
	/**
	 * Denotes maximum line count of a video buffer that this endpoint
	 * can fill or process. Similar constraints to `min_line_count`,
	 * but `LINE_COUNT_HEIGHT` indicates that the endpoint will never
	 * fill or process more than a full video frame in one video buffer.
	 */
	int16_t max_line_count;
};

/**
 * @brief Video buffer structure
 *
 * Represents a video frame.
 */
struct video_buffer {
	/** Pointer to driver specific data. */
	/* It must be kept as first field of the struct if used for @ref k_fifo APIs. */
	void *driver_data;
	/** type of the buffer */
	enum video_buf_type type;
	/** pointer to the start of the buffer. */
	uint8_t *buffer;
	/** index of the buffer, optionally set by the application */
	uint8_t index;
	/** size of the buffer in bytes. */
	uint32_t size;
	/** number of bytes occupied by the valid data in the buffer. */
	uint32_t bytesused;
	/** time reference in milliseconds at which the last data byte was
	 * actually received for input endpoints or to be consumed for output
	 * endpoints.
	 */
	uint32_t timestamp;
	/** Line offset within frame this buffer represents, from the
	 * beginning of the frame. This offset is given in pixels,
	 * so `line_offset` * `pitch` provides offset from the start of
	 * the frame in bytes.
	 */
	uint16_t line_offset;
};

/**
 * @brief Supported frame interval type of a video device.
 */
enum video_frmival_type {
	/** discrete frame interval type */
	VIDEO_FRMIVAL_TYPE_DISCRETE = 1,
	/** stepwise frame interval type */
	VIDEO_FRMIVAL_TYPE_STEPWISE = 2,
};

/**
 * @brief Video frame interval structure
 *
 * Used to describe a video frame interval.
 */
struct video_frmival {
	/** numerator of the frame interval */
	uint32_t numerator;
	/** denominator of the frame interval */
	uint32_t denominator;
};

/**
 * @brief Video frame interval stepwise structure
 *
 * Used to describe the video frame interval stepwise type.
 */
struct video_frmival_stepwise {
	/** minimum frame interval in seconds */
	struct video_frmival min;
	/** maximum frame interval in seconds */
	struct video_frmival max;
	/** frame interval step size in seconds */
	struct video_frmival step;
};

/**
 * @brief Video frame interval enumeration structure
 *
 * Used to describe the supported video frame intervals of a given video format.
 */
struct video_frmival_enum {
	/** frame interval index during enumeration */
	uint32_t index;
	/** video format for which the query is made */
	const struct video_format *format;
	/** frame interval type the device supports */
	enum video_frmival_type type;
	/** the actual frame interval */
	union {
		struct video_frmival discrete;
		struct video_frmival_stepwise stepwise;
	};
};

/**
 * @brief Video signal result
 *
 * Identify video event.
 */
enum video_signal_result {
	VIDEO_BUF_DONE,    /**< Buffer is done */
	VIDEO_BUF_ABORTED, /**< Buffer is aborted */
	VIDEO_BUF_ERROR,   /**< Buffer is in error */
};

/**
 * @brief Video selection target
 *
 * Used to indicate which selection to query or set on a video device
 */
enum video_selection_target {
	/** Current crop setting */
	VIDEO_SEL_TGT_CROP,
	/** Crop bound (aka the maximum crop achievable) */
	VIDEO_SEL_TGT_CROP_BOUND,
	/** Native size of the input frame */
	VIDEO_SEL_TGT_NATIVE_SIZE,
	/** Current compose setting */
	VIDEO_SEL_TGT_COMPOSE,
	/** Compose bound (aka the maximum compose achievable) */
	VIDEO_SEL_TGT_COMPOSE_BOUND,
};

/**
 * @brief Description of a rectangle area.
 *
 * Used for crop/compose and possibly within drivers as well
 */
struct video_rect {
	/** left offset of selection rectangle */
	uint32_t left;
	/** top offset of selection rectangle */
	uint32_t top;
	/** width of selection rectangle */
	uint32_t width;
	/** height of selection rectangle */
	uint32_t height;
};

/**
 * @brief Video selection (crop / compose) structure
 *
 * Used to describe the query and set selection target on a video device
 */
struct video_selection {
	/** buffer type, allow to select for device having both input and output */
	enum video_buf_type type;
	/** selection target enum */
	enum video_selection_target target;
	/** selection target rectangle */
	struct video_rect rect;
};

/**
 * @typedef video_api_format_t
 * @brief Function pointer type for video_set/get_format()
 *
 * See video_set/get_format() for argument descriptions.
 */
typedef int (*video_api_format_t)(const struct device *dev, struct video_format *fmt);

/**
 * @typedef video_api_frmival_t
 * @brief Function pointer type for video_set/get_frmival()
 *
 * See video_set/get_frmival() for argument descriptions.
 */
typedef int (*video_api_frmival_t)(const struct device *dev, struct video_frmival *frmival);

/**
 * @typedef video_api_enum_frmival_t
 * @brief List all supported frame intervals of a given format
 *
 * See video_enum_frmival() for argument descriptions.
 */
typedef int (*video_api_enum_frmival_t)(const struct device *dev, struct video_frmival_enum *fie);

/**
 * @typedef video_api_enqueue_t
 * @brief Enqueue a buffer in the driver’s incoming queue.
 *
 * See video_enqueue() for argument descriptions.
 */
typedef int (*video_api_enqueue_t)(const struct device *dev, struct video_buffer *buf);

/**
 * @typedef video_api_dequeue_t
 * @brief Dequeue a buffer from the driver’s outgoing queue.
 *
 * See video_dequeue() for argument descriptions.
 */
typedef int (*video_api_dequeue_t)(const struct device *dev, struct video_buffer **buf,
				   k_timeout_t timeout);

/**
 * @typedef video_api_flush_t
 * @brief Flush endpoint buffers, buffer are moved from incoming queue to
 *        outgoing queue.
 *
 * See video_flush() for argument descriptions.
 */
typedef int (*video_api_flush_t)(const struct device *dev, bool cancel);

/**
 * @typedef video_api_set_stream_t
 * @brief Start or stop streaming on the video device.
 *
 * Start (enable == true) or stop (enable == false) streaming on the video device.
 *
 * @param dev Pointer to the device structure.
 * @param enable If true, start streaming, otherwise stop streaming.
 * @param type The type of the buffers stream to start or stop.
 *
 * @retval 0 on success, otherwise a negative errno code.
 */
typedef int (*video_api_set_stream_t)(const struct device *dev, bool enable,
				      enum video_buf_type type);

/**
 * @typedef video_api_ctrl_t
 * @brief Set/Get a video control value.
 *
 * @param dev Pointer to the device structure.
 * @param cid Id of the control to set/get its value.
 */
typedef int (*video_api_ctrl_t)(const struct device *dev, uint32_t cid);

/**
 * @typedef video_api_get_caps_t
 * @brief Get capabilities of a video endpoint.
 *
 * See video_get_caps() for argument descriptions.
 */
typedef int (*video_api_get_caps_t)(const struct device *dev, struct video_caps *caps);

/**
 * @typedef video_api_set_signal_t
 * @brief Register/Unregister poll signal for buffer events.
 *
 * See video_set_signal() for argument descriptions.
 */
typedef int (*video_api_set_signal_t)(const struct device *dev, struct k_poll_signal *sig);

/**
 * @typedef video_api_selection_t
 * @brief Get/Set video selection (crop / compose)
 *
 * See @ref video_set_selection and @ref video_get_selection for argument descriptions.
 */
typedef int (*video_api_selection_t)(const struct device *dev, struct video_selection *sel);

__subsystem struct video_driver_api {
	/* mandatory callbacks */
	video_api_format_t set_format;
	video_api_format_t get_format;
	video_api_set_stream_t set_stream;
	video_api_get_caps_t get_caps;
	/* optional callbacks */
	video_api_enqueue_t enqueue;
	video_api_dequeue_t dequeue;
	video_api_flush_t flush;
	video_api_ctrl_t set_ctrl;
	video_api_ctrl_t get_volatile_ctrl;
	video_api_set_signal_t set_signal;
	video_api_frmival_t set_frmival;
	video_api_frmival_t get_frmival;
	video_api_enum_frmival_t enum_frmival;
	video_api_selection_t set_selection;
	video_api_selection_t get_selection;
};

/**
 * @brief Set video format.
 *
 * Configure video device with a specific format.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param fmt Pointer to a video format struct.
 *
 * @retval 0 Is successful.
 * @retval -EINVAL If parameters are invalid.
 * @retval -ENOTSUP If format is not supported.
 * @retval -EIO General input / output error.
 */
static inline int video_set_format(const struct device *dev, struct video_format *fmt)
{
	const struct video_driver_api *api;

	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(fmt != NULL);

	api = (const struct video_driver_api *)dev->api;
	if (api->set_format == NULL) {
		return -ENOSYS;
	}

	return api->set_format(dev, fmt);
}

/**
 * @brief Get video format.
 *
 * Get video device current video format.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param fmt Pointer to video format struct.
 *
 * @retval pointer to video format
 */
static inline int video_get_format(const struct device *dev, struct video_format *fmt)
{
	const struct video_driver_api *api;

	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(fmt != NULL);

	api = (const struct video_driver_api *)dev->api;
	if (api->get_format == NULL) {
		return -ENOSYS;
	}

	return api->get_format(dev, fmt);
}

/**
 * @brief Set video frame interval.
 *
 * Configure video device with a specific frame interval.
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
static inline int video_set_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct video_driver_api *api;

	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(frmival != NULL);

	if (frmival->numerator == 0 || frmival->denominator == 0) {
		return -EINVAL;
	}

	api = (const struct video_driver_api *)dev->api;
	if (api->set_frmival == NULL) {
		return -ENOSYS;
	}

	return api->set_frmival(dev, frmival);
}

/**
 * @brief Get video frame interval.
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
static inline int video_get_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct video_driver_api *api;

	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(frmival != NULL);

	api = (const struct video_driver_api *)dev->api;
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
 * Applications should fill the pixelformat, width and height fields of the
 * video_frmival_enum struct first to form a query. Then, the index field is
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
static inline int video_enum_frmival(const struct device *dev, struct video_frmival_enum *fie)
{
	const struct video_driver_api *api;

	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(fie != NULL);
	__ASSERT_NO_MSG(fie->format != NULL);

	api = (const struct video_driver_api *)dev->api;
	if (api->enum_frmival == NULL) {
		return -ENOSYS;
	}

	return api->enum_frmival(dev, fie);
}

/**
 * @brief Enqueue a video buffer.
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
static inline int video_enqueue(const struct device *dev, struct video_buffer *buf)
{
	const struct video_driver_api *api = (const struct video_driver_api *)dev->api;

	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(buf != NULL);
	__ASSERT_NO_MSG(buf->buffer != NULL);

	api = (const struct video_driver_api *)dev->api;
	if (api->enqueue == NULL) {
		return -ENOSYS;
	}

	return api->enqueue(dev, buf);
}

/**
 * @brief Dequeue a video buffer.
 *
 * Dequeue a filled (capturing) or displayed (output) buffer from the driver’s
 * endpoint outgoing queue.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param buf Pointer a video buffer pointer.
 * @param timeout Timeout
 *
 * @retval 0 Is successful.
 * @retval -EINVAL If parameters are invalid.
 * @retval -EIO General input / output error.
 */
static inline int video_dequeue(const struct device *dev, struct video_buffer **buf,
				k_timeout_t timeout)
{
	const struct video_driver_api *api;

	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(buf != NULL);

	api = (const struct video_driver_api *)dev->api;
	if (api->dequeue == NULL) {
		return -ENOSYS;
	}

	return api->dequeue(dev, buf, timeout);
}

/**
 * @brief Flush endpoint buffers.
 *
 * A call to flush finishes when all endpoint buffers have been moved from
 * incoming queue to outgoing queue. Either because canceled or fully processed
 * through the video function.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param cancel If true, cancel buffer processing instead of waiting for
 *        completion.
 *
 * @retval 0 Is successful, -ERRNO code otherwise.
 */
static inline int video_flush(const struct device *dev, bool cancel)
{
	const struct video_driver_api *api;

	__ASSERT_NO_MSG(dev != NULL);

	api = (const struct video_driver_api *)dev->api;
	if (api->flush == NULL) {
		return -ENOSYS;
	}

	return api->flush(dev, cancel);
}

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
 * @retval 0 Is successful.
 * @retval -EIO General input / output error.
 */
static inline int video_stream_start(const struct device *dev, enum video_buf_type type)
{
	const struct video_driver_api *api;

	__ASSERT_NO_MSG(dev != NULL);

	api = (const struct video_driver_api *)dev->api;
	if (api->set_stream == NULL) {
		return -ENOSYS;
	}

	return api->set_stream(dev, true, type);
}

/**
 * @brief Stop the video device function.
 *
 * On video_stream_stop, driver must stop any transactions or wait until they
 * finish.
 *
 * @param dev Pointer to the device structure.
 * @param type The type of the buffers stream to stop.
 *
 * @retval 0 Is successful.
 * @retval -EIO General input / output error.
 */
static inline int video_stream_stop(const struct device *dev, enum video_buf_type type)
{
	const struct video_driver_api *api;
	int ret;

	__ASSERT_NO_MSG(dev != NULL);

	api = (const struct video_driver_api *)dev->api;
	if (api->set_stream == NULL) {
		return -ENOSYS;
	}

	ret = api->set_stream(dev, false, type);
	video_flush(dev, true);

	return ret;
}

/**
 * @brief Get the capabilities of a video endpoint.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param caps Pointer to the video_caps struct to fill.
 *
 * @retval 0 Is successful, -ERRNO code otherwise.
 */
static inline int video_get_caps(const struct device *dev, struct video_caps *caps)
{
	const struct video_driver_api *api;

	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(caps != NULL);

	api = (const struct video_driver_api *)dev->api;
	if (api->get_caps == NULL) {
		return -ENOSYS;
	}

	return api->get_caps(dev, caps);
}

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

struct video_ctrl_query;

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
 * @brief Register/Unregister k_poll signal for a video endpoint.
 *
 * Register a poll signal to the endpoint, which will be signaled on frame
 * completion (done, aborted, error). Registering a NULL poll signal
 * unregisters any previously registered signal.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param sig Pointer to k_poll_signal
 *
 * @retval 0 Is successful, -ERRNO code otherwise.
 */
static inline int video_set_signal(const struct device *dev, struct k_poll_signal *sig)
{
	const struct video_driver_api *api;

	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(sig != NULL);

	api = (const struct video_driver_api *)dev->api;
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
 * @retval 0 Is successful.
 * @retval -EINVAL If parameters are invalid.
 * @retval -ENOTSUP If format is not supported.
 * @retval -EIO General input / output error.
 */
static inline int video_set_selection(const struct device *dev, struct video_selection *sel)
{
	const struct video_driver_api *api;

	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(sel != NULL);

	api = (const struct video_driver_api *)dev->api;
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
 * @retval 0 Is successful.
 * @retval -EINVAL If parameters are invalid.
 * @retval -ENOTSUP If format is not supported.
 * @retval -EIO General input / output error.
 */
static inline int video_get_selection(const struct device *dev, struct video_selection *sel)
{
	const struct video_driver_api *api;

	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(sel != NULL);

	api = (const struct video_driver_api *)dev->api;
	if (api->get_selection == NULL) {
		return -ENOSYS;
	}

	return api->get_selection(dev, sel);
}

/**
 * @brief Allocate aligned video buffer.
 *
 * @param size Size of the video buffer (in bytes).
 * @param align Alignment of the requested memory, must be a power of two.
 * @param timeout Timeout duration or K_NO_WAIT
 *
 * @retval pointer to allocated video buffer
 */
struct video_buffer *video_buffer_aligned_alloc(size_t size, size_t align, k_timeout_t timeout);

/**
 * @brief Allocate video buffer.
 *
 * @param size Size of the video buffer (in bytes).
 * @param timeout Timeout duration or K_NO_WAIT
 *
 * @retval pointer to allocated video buffer
 */
struct video_buffer *video_buffer_alloc(size_t size, k_timeout_t timeout);

/**
 * @brief Release a video buffer.
 *
 * @param buf Pointer to the video buffer to release.
 */
void video_buffer_release(struct video_buffer *buf);

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
 * @brief Compute the difference between two frame intervals
 *
 * @param frmival Frame interval to turn into microseconds.
 *
 * @return The frame interval value in microseconds.
 */
static inline uint64_t video_frmival_nsec(const struct video_frmival *frmival)
{
	__ASSERT_NO_MSG(frmival != NULL);
	__ASSERT_NO_MSG(frmival->denominator != 0);

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
 * @defgroup video_pixel_formats Video pixel formats
 * The '|' characters separate the pixels or logical blocks, and spaces separate the bytes.
 * The uppercase letter represents the most significant bit.
 * The lowercase letters represent the rest of the bits.
 * @{
 */

/**
 * @brief Four-character-code uniquely identifying the pixel format
 */
#define VIDEO_FOURCC(a, b, c, d)                                                                   \
	((uint32_t)(a) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))

/**
 * @brief Convert a four-character-string to a four-character-code
 *
 * Convert a string literal or variable into a four-character-code
 * as defined by @ref VIDEO_FOURCC.
 *
 * @param str String to be converted
 * @return Four-character-code.
 */
#define VIDEO_FOURCC_FROM_STR(str) VIDEO_FOURCC((str)[0], (str)[1], (str)[2], (str)[3])

/**
 * @brief Convert a four-character-code to a four-character-string
 *
 * Convert a four-character code as defined by @ref VIDEO_FOURCC into a string that can be used
 * anywhere, such as in debug logs with the %s print formatter.
 *
 * @param fourcc The 32-bit four-character-code integer to be converted, in CPU-native endinaness.
 * @return Four-character-string built out of it.
 */
#define VIDEO_FOURCC_TO_STR(fourcc)                                                                \
	((char[]){                                                                                 \
		(char)((fourcc) & 0xFF),                                                           \
		(char)(((fourcc) >> 8) & 0xFF),                                                    \
		(char)(((fourcc) >> 16) & 0xFF),                                                   \
		(char)(((fourcc) >> 24) & 0xFF),                                                   \
		'\0'                                                                               \
	})

/**
 * @name Bayer formats (R, G, B channels).
 *
 * The full color information is spread over multiple pixels.
 *
 * When the format includes more than 8-bit per pixel, a strategy becomes needed to pack
 * the bits over multiple bytes, as illustrated for each format.
 *
 * The number above the 'R', 'r', 'G', 'g', 'B', 'b' are hints about which pixel number the
 * following bits belong to.
 *
 * @{
 */

/**
 * @code{.unparsed}
 *   0          1          2          3
 * | Bbbbbbbb | Gggggggg | Bbbbbbbb | Gggggggg | ...
 * | Gggggggg | Rrrrrrrr | Gggggggg | Rrrrrrrr | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SBGGR8 VIDEO_FOURCC('B', 'A', '8', '1')

/**
 * @code{.unparsed}
 *   0          1          2          3
 * | Gggggggg | Bbbbbbbb | Gggggggg | Bbbbbbbb | ...
 * | Rrrrrrrr | Gggggggg | Rrrrrrrr | Gggggggg | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SGBRG8 VIDEO_FOURCC('G', 'B', 'R', 'G')

/**
 * @code{.unparsed}
 *   0          1          2          3
 * | Gggggggg | Rrrrrrrr | Gggggggg | Rrrrrrrr | ...
 * | Bbbbbbbb | Gggggggg | Bbbbbbbb | Gggggggg | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SGRBG8 VIDEO_FOURCC('G', 'R', 'B', 'G')

/**
 * @code{.unparsed}
 *   0          1          2          3
 * | Rrrrrrrr | Gggggggg | Rrrrrrrr | Gggggggg | ...
 * | Gggggggg | Bbbbbbbb | Gggggggg | Bbbbbbbb | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SRGGB8 VIDEO_FOURCC('R', 'G', 'G', 'B')

/**
 * @code{.unparsed}
 *   0          1          2          3          3 2 1 0
 * | Bbbbbbbb | Gggggggg | Bbbbbbbb | Gggggggg | ggbbggbb | ...
 * | Gggggggg | Rrrrrrrr | Gggggggg | Rrrrrrrr | rrggrrgg | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SBGGR10P VIDEO_FOURCC('p', 'B', 'A', 'A')

/**
 * @code{.unparsed}
 *   0          1          2          3          3 2 1 0
 * | Gggggggg | Bbbbbbbb | Gggggggg | Bbbbbbbb | bbggbbgg | ...
 * | Rrrrrrrr | Gggggggg | Rrrrrrrr | Gggggggg | ggrrggrr | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SGBRG10P VIDEO_FOURCC('p', 'G', 'A', 'A')

/**
 * @code{.unparsed}
 *   0          1          2          3          3 2 1 0
 * | Gggggggg | Rrrrrrrr | Gggggggg | Rrrrrrrr | rrggrrgg | ...
 * | Bbbbbbbb | Gggggggg | Bbbbbbbb | Gggggggg | ggbbggbb | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SGRBG10P VIDEO_FOURCC('p', 'g', 'A', 'A')

/**
 * @code{.unparsed}
 *   0          1          2          3          3 2 1 0
 * | Rrrrrrrr | Gggggggg | Rrrrrrrr | Gggggggg | ggrrggrr | ...
 * | Gggggggg | Bbbbbbbb | Gggggggg | Bbbbbbbb | bbggbbgg | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SRGGB10P VIDEO_FOURCC('p', 'R', 'A', 'A')

/**
 * @code{.unparsed}
 *   0          1          1   0      2          3          3   2
 * | Bbbbbbbb | Gggggggg | ggggbbbb | Bbbbbbbb | Gggggggg | ggggbbbb | ...
 * | Gggggggg | Rrrrrrrr | rrrrgggg | Gggggggg | Rrrrrrrr | rrrrgggg | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SBGGR12P VIDEO_FOURCC('p', 'B', 'C', 'C')

/**
 * @code{.unparsed}
 *   0          1          1   0      2          3          3   2
 * | Gggggggg | Bbbbbbbb | bbbbgggg | Gggggggg | Bbbbbbbb | bbbbgggg | ...
 * | Rrrrrrrr | Gggggggg | ggggrrrr | Rrrrrrrr | Gggggggg | ggggrrrr | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SGBRG12P VIDEO_FOURCC('p', 'G', 'C', 'C')

/**
 * @code{.unparsed}
 *   0          1          1   0      2          3          3   2
 * | Gggggggg | Rrrrrrrr | rrrrgggg | Gggggggg | Rrrrrrrr | rrrrgggg | ...
 * | Bbbbbbbb | Gggggggg | ggggbbbb | Bbbbbbbb | Gggggggg | ggggbbbb | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SGRBG12P VIDEO_FOURCC('p', 'g', 'C', 'C')

/**
 * @code{.unparsed}
 *   0          1          1   0      2          3          3   2
 * | Rrrrrrrr | Gggggggg | ggggrrrr | Rrrrrrrr | Gggggggg | ggggrrrr | ...
 * | Gggggggg | Bbbbbbbb | bbbbgggg | Gggggggg | Bbbbbbbb | bbbbgggg | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SRGGB12P VIDEO_FOURCC('p', 'R', 'C', 'C')

/**
 * @code{.unparsed}
 *   0          1          2          3          1 0      2   1    3     2
 * | Bbbbbbbb | Gggggggg | Bbbbbbbb | Gggggggg | ggbbbbbb bbbbgggg ggggggbb | ...
 * | Gggggggg | Rrrrrrrr | Gggggggg | Rrrrrrrr | rrgggggg ggggrrrr rrrrrrgg | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SBGGR14P VIDEO_FOURCC('p', 'B', 'E', 'E')

/**
 * @code{.unparsed}
 *   0          1          2          3          1 0      2   1    3     2
 * | Gggggggg | Bbbbbbbb | Gggggggg | Bbbbbbbb | bbgggggg ggggbbbb bbbbbbgg | ...
 * | Rrrrrrrr | Gggggggg | Rrrrrrrr | Gggggggg | ggrrrrrr rrrrgggg ggggggrr | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SGBRG14P VIDEO_FOURCC('p', 'G', 'E', 'E')

/**
 * @code{.unparsed}
 *   0          1          2          3          1 0      2   1    3     2
 * | Gggggggg | Rrrrrrrr | Gggggggg | Rrrrrrrr | rrgggggg ggggrrrr rrrrrrgg | ...
 * | Bbbbbbbb | Gggggggg | Bbbbbbbb | Gggggggg | ggbbbbbb bbbbgggg ggggggbb | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SGRBG14P VIDEO_FOURCC('p', 'g', 'E', 'E')

/**
 * @code{.unparsed}
 *   0          1          2          3          1 0      2   1    3     2
 * | Rrrrrrrr | Gggggggg | Rrrrrrrr | Gggggggg | ggrrrrrr rrrrgggg ggggggrr | ...
 * | Gggggggg | Bbbbbbbb | Gggggggg | Bbbbbbbb | bbgggggg ggggbbbb bbbbbbgg | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SRGGB14P VIDEO_FOURCC('p', 'R', 'E', 'E')

/**
 * @code{.unparsed}
 * | bbbbbbbb 000000Bb | gggggggg 000000Gg | bbbbbbbb 000000Bb | gggggggg 000000Gg | ...
 * | gggggggg 000000Gg | rrrrrrrr 000000Rr | gggggggg 000000Gg | rrrrrrrr 000000Rr | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SBGGR10 VIDEO_FOURCC('B', 'G', '1', '0')

/**
 * @code{.unparsed}
 * | gggggggg 000000Gg | bbbbbbbb 000000Bb | gggggggg 000000Gg | bbbbbbbb 000000Bb | ...
 * | rrrrrrrr 000000Rr | gggggggg 000000Gg | rrrrrrrr 000000Rr | gggggggg 000000Gg | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SGBRG10 VIDEO_FOURCC('G', 'B', '1', '0')

/**
 * @code{.unparsed}
 * | gggggggg 000000Gg | rrrrrrrr 000000Rr | gggggggg 000000Gg | rrrrrrrr 000000Rr | ...
 * | bbbbbbbb 000000Bb | gggggggg 000000Gg | bbbbbbbb 000000Bb | gggggggg 000000Gg | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SGRBG10 VIDEO_FOURCC('B', 'A', '1', '0')

/**
 * @code{.unparsed}
 * | rrrrrrrr 000000Rr | gggggggg 000000Gg | rrrrrrrr 000000Rr | gggggggg 000000Gg | ...
 * | gggggggg 000000Gg | bbbbbbbb 000000Bb | gggggggg 000000Gg | bbbbbbbb 000000Bb | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SRGGB10 VIDEO_FOURCC('R', 'G', '1', '0')

/**
 * @code{.unparsed}
 * | bbbbbbbb 0000Bbbb | gggggggg 0000Gggg | bbbbbbbb 0000Bbbb | gggggggg 0000Gggg | ...
 * | gggggggg 0000Gggg | rrrrrrrr 0000Rrrr | gggggggg 0000Gggg | rrrrrrrr 0000Rrrr | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SBGGR12 VIDEO_FOURCC('B', 'G', '1', '2')

/**
 * @code{.unparsed}
 * | gggggggg 0000Gggg | bbbbbbbb 0000Bbbb | gggggggg 0000Gggg | bbbbbbbb 0000Bbbb | ...
 * | rrrrrrrr 0000Rrrr | gggggggg 0000Gggg | rrrrrrrr 0000Rrrr | gggggggg 0000Gggg | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SGBRG12 VIDEO_FOURCC('G', 'B', '1', '2')

/**
 * @code{.unparsed}
 * | gggggggg 0000Gggg | rrrrrrrr 0000Rrrr | gggggggg 0000Gggg | rrrrrrrr 0000Rrrr | ...
 * | bbbbbbbb 0000Bbbb | gggggggg 0000Gggg | bbbbbbbb 0000Bbbb | gggggggg 0000Gggg | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SGRBG12 VIDEO_FOURCC('B', 'A', '1', '2')

/**
 * @code{.unparsed}
 * | rrrrrrrr 0000Rrrr | gggggggg 0000Gggg | rrrrrrrr 0000Rrrr | gggggggg 0000Gggg | ...
 * | gggggggg 0000Gggg | bbbbbbbb 0000Bbbb | gggggggg 0000Gggg | bbbbbbbb 0000Bbbb | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SRGGB12 VIDEO_FOURCC('R', 'G', '1', '2')

/**
 * @code{.unparsed}
 * | bbbbbbbb 00Bbbbbb | gggggggg 00Gggggg | bbbbbbbb 00Bbbbbb | gggggggg 00Gggggg | ...
 * | gggggggg 00Gggggg | rrrrrrrr 00Rrrrrr | gggggggg 00Gggggg | rrrrrrrr 00Rrrrrr | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SBGGR14 VIDEO_FOURCC('B', 'G', '1', '4')

/**
 * @code{.unparsed}
 * | gggggggg 00Gggggg | bbbbbbbb 00Bbbbbb | gggggggg 00Gggggg | bbbbbbbb 00Bbbbbb | ...
 * | rrrrrrrr 00Rrrrrr | gggggggg 00Gggggg | rrrrrrrr 00Rrrrrr | gggggggg 00Gggggg | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SGBRG14 VIDEO_FOURCC('G', 'B', '1', '4')

/**
 * @code{.unparsed}
 * | gggggggg 00Gggggg | rrrrrrrr 00Rrrrrr | gggggggg 00Gggggg | rrrrrrrr 00Rrrrrr | ...
 * | bbbbbbbb 00Bbbbbb | gggggggg 00Gggggg | bbbbbbbb 00Bbbbbb | gggggggg 00Gggggg | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SGRBG14 VIDEO_FOURCC('G', 'R', '1', '4')

/**
 * @code{.unparsed}
 * | rrrrrrrr 00Rrrrrr | gggggggg 00Gggggg | rrrrrrrr 00Rrrrrr | gggggggg 00Gggggg | ...
 * | gggggggg 00Gggggg | bbbbbbbb 00Bbbbbb | gggggggg 00Gggggg | bbbbbbbb 00Bbbbbb | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SRGGB14 VIDEO_FOURCC('R', 'G', '1', '4')

/**
 * @code{.unparsed}
 * | bbbbbbbb Bbbbbbbb | gggggggg Gggggggg | bbbbbbbb Bbbbbbbb | gggggggg Gggggggg | ...
 * | gggggggg Gggggggg | rrrrrrrr Rrrrrrrr | gggggggg Gggggggg | rrrrrrrr Rrrrrrrr | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SBGGR16 VIDEO_FOURCC('B', 'Y', 'R', '2')

/**
 * @code{.unparsed}
 * | gggggggg Gggggggg | bbbbbbbb Bbbbbbbb | gggggggg Gggggggg | bbbbbbbb Bbbbbbbb | ...
 * | rrrrrrrr Rrrrrrrr | gggggggg Gggggggg | rrrrrrrr Rrrrrrrr | gggggggg Gggggggg | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SGBRG16 VIDEO_FOURCC('G', 'B', '1', '6')

/**
 * @code{.unparsed}
 * | gggggggg Gggggggg | rrrrrrrr Rrrrrrrr | gggggggg Gggggggg | rrrrrrrr Rrrrrrrr | ...
 * | bbbbbbbb Bbbbbbbb | gggggggg Gggggggg | bbbbbbbb Bbbbbbbb | gggggggg Gggggggg | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SGRBG16 VIDEO_FOURCC('G', 'R', '1', '6')

/**
 * @code{.unparsed}
 * | rrrrrrrr Rrrrrrrr | gggggggg Gggggggg | rrrrrrrr Rrrrrrrr | gggggggg Gggggggg | ...
 * | gggggggg Gggggggg | bbbbbbbb Bbbbbbbb | gggggggg Gggggggg | bbbbbbbb Bbbbbbbb | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_SRGGB16 VIDEO_FOURCC('R', 'G', '1', '6')

/**
 * @}
 */

/**
 * @name Grayscale formats
 * Luminance (Y) channel only, in various bit depth and packing.
 *
 * When the format includes more than 8-bit per pixel, a strategy becomes needed to pack
 * the bits over multiple bytes, as illustrated for each format.
 *
 * The number above the 'Y', 'y' are hints about which pixel number the following bits belong to.
 *
 * @{
 */

/**
 * Same as Y8 (8-bit luma-only) following the standard FOURCC naming,
 * or L8 in some graphics libraries.
 *
 * @code{.unparsed}
 *   0          1          2          3
 * | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_GREY VIDEO_FOURCC('G', 'R', 'E', 'Y')

/**
 * @code{.unparsed}
 *   0          1          2          3          3 2 1 0
 * | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | yyyyyyyy | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_Y10P VIDEO_FOURCC('Y', '1', '0', 'P')

/**
 * @code{.unparsed}
 *   0          1          1   0      2          3          3   2
 * | Yyyyyyyy | Yyyyyyyy | yyyyyyyy | Yyyyyyyy | Yyyyyyyy | yyyyyyyy | ...
 * | Yyyyyyyy | Yyyyyyyy | yyyyyyyy | Yyyyyyyy | Yyyyyyyy | yyyyyyyy | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_Y12P VIDEO_FOURCC('Y', '1', '2', 'P')

/**
 * @code{.unparsed}
 *   0          1          2          3          1 0      2   1    3     2
 * | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | yyyyyyyy yyyyyyyy yyyyyyyy | ...
 * | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | yyyyyyyy yyyyyyyy yyyyyyyy | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_Y14P VIDEO_FOURCC('Y', '1', '4', 'P')

/**
 * Little endian, with the 6 most significant bits set to Zero.
 * @code{.unparsed}
 *   0                   1                   2                   3
 * | yyyyyyyy 000000Yy | yyyyyyyy 000000Yy | yyyyyyyy 000000Yy | yyyyyyyy 000000Yy | ...
 * | yyyyyyyy 000000Yy | yyyyyyyy 000000Yy | yyyyyyyy 000000Yy | yyyyyyyy 000000Yy | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_Y10 VIDEO_FOURCC('Y', '1', '0', ' ')

/**
 * Little endian, with the 4 most significant bits set to Zero.
 * @code{.unparsed}
 *   0                   1                   2                   3
 * | yyyyyyyy 0000Yyyy | yyyyyyyy 0000Yyyy | yyyyyyyy 0000Yyyy | yyyyyyyy 0000Yyyy | ...
 * | yyyyyyyy 0000Yyyy | yyyyyyyy 0000Yyyy | yyyyyyyy 0000Yyyy | yyyyyyyy 0000Yyyy | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_Y12 VIDEO_FOURCC('Y', '1', '2', ' ')

/**
 * Little endian, with the 2 most significant bits set to Zero.
 * @code{.unparsed}
 *   0                   1                   2                   3
 * | yyyyyyyy 00Yyyyyy | yyyyyyyy 00Yyyyyy | yyyyyyyy 00Yyyyyy | yyyyyyyy 00Yyyyyy | ...
 * | yyyyyyyy 00Yyyyyy | yyyyyyyy 00Yyyyyy | yyyyyyyy 00Yyyyyy | yyyyyyyy 00Yyyyyy | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_Y14 VIDEO_FOURCC('Y', '1', '4', ' ')

/**
 * Little endian.
 * @code{.unparsed}
 *   0                   1                   2                   3
 * | yyyyyyyy Yyyyyyyy | yyyyyyyy Yyyyyyyy | yyyyyyyy Yyyyyyyy | yyyyyyyy Yyyyyyyy | ...
 * | yyyyyyyy Yyyyyyyy | yyyyyyyy Yyyyyyyy | yyyyyyyy Yyyyyyyy | yyyyyyyy Yyyyyyyy | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_Y16 VIDEO_FOURCC('Y', '1', '6', ' ')

/**
 * @}
 */

/**
 * @name RGB formats
 * Per-color (R, G, B) channels.
 * @{
 */

/**
 * 5 red bits [15:11], 6 green bits [10:5], 5 blue bits [4:0].
 * This 16-bit integer is then packed in big endian format over two bytes:
 *
 * @code{.unparsed}
 *   15.....8 7......0
 * | RrrrrGgg gggBbbbb | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_RGB565X VIDEO_FOURCC('R', 'G', 'B', 'R')

/**
 * 5 red bits [15:11], 6 green bits [10:5], 5 blue bits [4:0].
 * This 16-bit integer is then packed in little endian format over two bytes:
 *
 * @code{.unparsed}
 *   7......0 15.....8
 * | gggBbbbb RrrrrGgg | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_RGB565 VIDEO_FOURCC('R', 'G', 'B', 'P')

/**
 * 24 bit RGB format with 8 bit per component
 *
 * @code{.unparsed}
 * | Bbbbbbbb Gggggggg Rggggggg | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_BGR24 VIDEO_FOURCC('B', 'G', 'R', '3')

/**
 * 24 bit RGB format with 8 bit per component
 *
 * @code{.unparsed}
 * | Rggggggg Gggggggg Bbbbbbbb | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_RGB24 VIDEO_FOURCC('R', 'G', 'B', '3')

/**
 * @code{.unparsed}
 * | Aaaaaaaa Rrrrrrrr Gggggggg Bbbbbbbb | ...
 * @endcode
 */

#define VIDEO_PIX_FMT_ARGB32 VIDEO_FOURCC('B', 'A', '2', '4')

/**
 * @code{.unparsed}
 * | Bbbbbbbb Gggggggg Rrrrrrrr Aaaaaaaa | ...
 * @endcode
 */

#define VIDEO_PIX_FMT_ABGR32 VIDEO_FOURCC('A', 'R', '2', '4')

/**
 * @code{.unparsed}
 * | Rrrrrrrr Gggggggg Bbbbbbbb Aaaaaaaa | ...
 * @endcode
 */

#define VIDEO_PIX_FMT_RGBA32 VIDEO_FOURCC('A', 'B', '2', '4')

/**
 * @code{.unparsed}
 * | Aaaaaaaa Bbbbbbbb Gggggggg Rrrrrrrr | ...
 * @endcode
 */

#define VIDEO_PIX_FMT_BGRA32 VIDEO_FOURCC('R', 'A', '2', '4')

/**
 * The first byte is empty (X) for each pixel.
 *
 * @code{.unparsed}
 * | Xxxxxxxx Rrrrrrrr Gggggggg Bbbbbbbb | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_XRGB32 VIDEO_FOURCC('B', 'X', '2', '4')

/**
 * @}
 */

/**
 * @name YUV formats
 * Luminance (Y) and chrominance (U, V) channels.
 * @{
 */

/**
 * There is either a missing channel per pixel, U or V.
 * The value is to be averaged over 2 pixels to get the value of individual pixel.
 *
 * @code{.unparsed}
 * | Yyyyyyyy Uuuuuuuu | Yyyyyyyy Vvvvvvvv | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_YUYV VIDEO_FOURCC('Y', 'U', 'Y', 'V')

/**
 * @code{.unparsed}
 * | Yyyyyyyy Vvvvvvvv | Yyyyyyyy Uuuuuuuu | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_YVYU VIDEO_FOURCC('Y', 'V', 'Y', 'U')

/**
 * @code{.unparsed}
 * | Vvvvvvvv Yyyyyyyy | Uuuuuuuu Yyyyyyyy | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_VYUY VIDEO_FOURCC('V', 'Y', 'U', 'Y')

/**
 * @code{.unparsed}
 * | Uuuuuuuu Yyyyyyyy | Vvvvvvvv Yyyyyyyy | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_UYVY VIDEO_FOURCC('U', 'Y', 'V', 'Y')

/**
 * The first byte is empty (X) for each pixel.
 *
 * @code{.unparsed}
 * | Xxxxxxxx Yyyyyyyy Uuuuuuuu Vvvvvvvv | ...
 * @endcode
 */
#define VIDEO_PIX_FMT_XYUV32 VIDEO_FOURCC('X', 'Y', 'U', 'V')

/**
 * Planar formats
 */
/**
 * Chroma (U/V) are subsampled horizontaly and vertically
 *
 * @code{.unparsed}
 * | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | ...
 * | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | ...
 * | ... |
 * | Uuuuuuuu   Vvvvvvvv | Uuuuuuuu   Vvvvvvvv | ...
 * | ... |
 * @endcode
 *
 * Below diagram show how luma and chroma relate to each others
 *
 * @code{.unparsed}
 *  Y0        Y1        Y2        Y3        ...
 *  Y6        Y7        Y8        Y9        ...
 *  ...
 *
 *  U0/1/6/7  V0/1/6/7  U2/3/8/9  V2/3/8/9  ...
 *  ...
 * @endcode
 */
#define VIDEO_PIX_FMT_NV12 VIDEO_FOURCC('N', 'V', '1', '2')

/**
 * Chroma (U/V) are subsampled horizontaly and vertically
 *
 * @code{.unparsed}
 * | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | ...
 * | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | ...
 * | ... |
 * | Vvvvvvvv   Uuuuuuuu | Vvvvvvvv   Uuuuuuuu | ...
 * | ... |
 * @endcode
 *
 * Below diagram show how luma and chroma relate to each others
 *
 * @code{.unparsed}
 *  Y0        Y1        Y2        Y3        ...
 *  Y6        Y7        Y8        Y9        ...
 *  ...
 *
 *  V0/1/6/7  U0/1/6/7  V2/3/8/9  U2/3/8/9  ...
 *  ...
 * @endcode
 */
#define VIDEO_PIX_FMT_NV21 VIDEO_FOURCC('N', 'V', '2', '1')

/**
 * Chroma (U/V) are subsampled horizontaly
 *
 * @code{.unparsed}
 * | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | ...
 * | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | ...
 * | ... |
 * | Uuuuuuuu   Vvvvvvvv | Uuuuuuuu   Vvvvvvvv | ...
 * | Uuuuuuuu   Vvvvvvvv | Uuuuuuuu   Vvvvvvvv | ...
 * | ... |
 * @endcode
 *
 * Below diagram show how luma and chroma relate to each others
 *
 * @code{.unparsed}
 *  Y0        Y1        Y2        Y3        ...
 *  Y6        Y7        Y8        Y9        ...
 *  ...
 *
 *  U0/1      V0/1      U2/3      V2/3      ...
 *  U6/7      V6/7      U8/9      V8/9      ...
 *  ...
 * @endcode
 */
#define VIDEO_PIX_FMT_NV16 VIDEO_FOURCC('N', 'V', '1', '6')

/**
 * Chroma (U/V) are subsampled horizontaly
 *
 * @code{.unparsed}
 * | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | ...
 * | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | ...
 * | ... |
 * | Vvvvvvvv   Uuuuuuuu | Vvvvvvvv   Uuuuuuuu | ...
 * | Vvvvvvvv   Uuuuuuuu | Vvvvvvvv   Uuuuuuuu | ...
 * | ... |
 * @endcode
 *
 * Below diagram show how luma and chroma relate to each others
 *
 * @code{.unparsed}
 *  Y0        Y1        Y2        Y3        ...
 *  Y6        Y7        Y8        Y9        ...
 *  ...
 *
 *  V0/1      U0/1      V2/3      U2/3      ...
 *  V6/7      U6/7      V8/9      U8/9      ...
 *  ...
 * @endcode
 */

#define VIDEO_PIX_FMT_NV61 VIDEO_FOURCC('N', 'V', '6', '1')

/**
 * Chroma (U/V) are not subsampled
 *
 * @code{.unparsed}
 * | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy |
 * | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy |
 * | ... |
 * | Uuuuuuuu   Vvvvvvvv | Uuuuuuuu   Vvvvvvvv | Uuuuuuuu   Vvvvvvvv | Uuuuuuuu   Vvvvvvvv |
 * | Uuuuuuuu   Vvvvvvvv | Uuuuuuuu   Vvvvvvvv | Uuuuuuuu   Vvvvvvvv | Uuuuuuuu   Vvvvvvvv |
 * | ... |
 * @endcode
 *
 * Below diagram show how luma and chroma relate to each others
 *
 * @code{.unparsed}
 *  Y0        Y1        Y2        Y3        ...
 *  Y6        Y7        Y8        Y9        ...
 *  ...
 *
 *  U0        V0        U1        V1        U2        V2        U3        V3        ...
 *  U6        V6        U7        V7        U8        V8        U9        V9        ...
 *  ...
 * @endcode
 */
#define VIDEO_PIX_FMT_NV24 VIDEO_FOURCC('N', 'V', '2', '4')

/**
 * Chroma (U/V) are not subsampled
 *
 * @code{.unparsed}
 * | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy |
 * | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy |
 * | ... |
 * | Vvvvvvvv   Uuuuuuuu | Vvvvvvvv   Uuuuuuuu | Vvvvvvvv   Uuuuuuuu | Vvvvvvvv   Uuuuuuuu |
 * | Vvvvvvvv   Uuuuuuuu | Vvvvvvvv   Uuuuuuuu | Vvvvvvvv   Uuuuuuuu | Vvvvvvvv   Uuuuuuuu |
 * | ... |
 * @endcode
 *
 * Below diagram show how luma and chroma relate to each others
 *
 * @code{.unparsed}
 *  Y0        Y1        Y2        Y3        ...
 *  Y6        Y7        Y8        Y9        ...
 *  ...
 *
 *  V0        U0        V1        U1        V2        U2        V3        U3        ...
 *  V6        U6        V7        U7        V8        U8        V9        U9        ...
 *  ...
 * @endcode
 */
#define VIDEO_PIX_FMT_NV42 VIDEO_FOURCC('N', 'V', '4', '2')

/**
 * Chroma (U/V) are subsampled horizontaly and vertically
 *
 * @code{.unparsed}
 * | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy |
 * | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy |
 * | ... |
 * | Uuuuuuuu | Uuuuuuuu |
 * | ... |
 * | Vvvvvvvv | Vvvvvvvv |
 * | ... |
 * @endcode
 *
 * Below diagram show how luma and chroma relate to each others
 *
 * @code{.unparsed}
 *  Y0        Y1        Y2        Y3        ...
 *  Y6        Y7        Y8        Y9        ...
 *  ...
 *
 *  U0/1/6/7      U2/3/8/9      ...
 *  ...
 *
 *  V0/1/6/7      V2/3/8/9      ...
 *  ...
 * @endcode
 */
#define VIDEO_PIX_FMT_YUV420 VIDEO_FOURCC('Y', 'U', '1', '2')

/**
 * Chroma (U/V) are subsampled horizontaly and vertically
 *
 * @code{.unparsed}
 * | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy |
 * | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy | Yyyyyyyy |
 * | ... |
 * | Vvvvvvvv | Vvvvvvvv |
 * | ... |
 * | Uuuuuuuu | Uuuuuuuu |
 * | ... |
 * @endcode
 *
 * Below diagram show how luma and chroma relate to each others
 *
 * @code{.unparsed}
 *  Y0        Y1        Y2        Y3        ...
 *  Y6        Y7        Y8        Y9        ...
 *  ...
 *
 *  V0/1/6/7      V2/3/8/9      ...
 *  ...
 *
 *  U0/1/6/7      U2/3/8/9      ...
 *  ...
 * @endcode
 */
#define VIDEO_PIX_FMT_YVU420 VIDEO_FOURCC('Y', 'V', '1', '2')

/**
 * @}
 */

/**
 * @name Compressed formats
 * @{
 */

/**
 * Both JPEG (single frame) and Motion-JPEG (MJPEG, multiple JPEG frames concatenated)
 */
#define VIDEO_PIX_FMT_JPEG VIDEO_FOURCC('J', 'P', 'E', 'G')

/**
 * @}
 */

/**
 * @brief Get number of bits per pixel of a pixel format
 *
 * @param pixfmt FourCC pixel format value (@ref video_pixel_formats).
 *
 * @retval 0 if the format is unhandled or if it is variable number of bits
 * @retval >0 bit size of one pixel for this format
 */
static inline unsigned int video_bits_per_pixel(uint32_t pixfmt)
{
	switch (pixfmt) {
	case VIDEO_PIX_FMT_SBGGR8:
	case VIDEO_PIX_FMT_SGBRG8:
	case VIDEO_PIX_FMT_SGRBG8:
	case VIDEO_PIX_FMT_SRGGB8:
	case VIDEO_PIX_FMT_GREY:
		return 8;
	case VIDEO_PIX_FMT_SBGGR10P:
	case VIDEO_PIX_FMT_SGBRG10P:
	case VIDEO_PIX_FMT_SGRBG10P:
	case VIDEO_PIX_FMT_SRGGB10P:
	case VIDEO_PIX_FMT_Y10P:
		return 10;
	case VIDEO_PIX_FMT_SBGGR12P:
	case VIDEO_PIX_FMT_SGBRG12P:
	case VIDEO_PIX_FMT_SGRBG12P:
	case VIDEO_PIX_FMT_SRGGB12P:
	case VIDEO_PIX_FMT_Y12P:
	case VIDEO_PIX_FMT_NV12:
	case VIDEO_PIX_FMT_NV21:
	case VIDEO_PIX_FMT_YUV420:
	case VIDEO_PIX_FMT_YVU420:
		return 12;
	case VIDEO_PIX_FMT_SBGGR14P:
	case VIDEO_PIX_FMT_SGBRG14P:
	case VIDEO_PIX_FMT_SGRBG14P:
	case VIDEO_PIX_FMT_SRGGB14P:
	case VIDEO_PIX_FMT_Y14P:
		return 14;
	case VIDEO_PIX_FMT_RGB565:
	case VIDEO_PIX_FMT_YUYV:
	case VIDEO_PIX_FMT_YVYU:
	case VIDEO_PIX_FMT_UYVY:
	case VIDEO_PIX_FMT_VYUY:
	case VIDEO_PIX_FMT_SBGGR10:
	case VIDEO_PIX_FMT_SGBRG10:
	case VIDEO_PIX_FMT_SGRBG10:
	case VIDEO_PIX_FMT_SRGGB10:
	case VIDEO_PIX_FMT_SBGGR12:
	case VIDEO_PIX_FMT_SGBRG12:
	case VIDEO_PIX_FMT_SGRBG12:
	case VIDEO_PIX_FMT_SRGGB12:
	case VIDEO_PIX_FMT_SBGGR14:
	case VIDEO_PIX_FMT_SGBRG14:
	case VIDEO_PIX_FMT_SGRBG14:
	case VIDEO_PIX_FMT_SRGGB14:
	case VIDEO_PIX_FMT_SBGGR16:
	case VIDEO_PIX_FMT_SGBRG16:
	case VIDEO_PIX_FMT_SGRBG16:
	case VIDEO_PIX_FMT_SRGGB16:
	case VIDEO_PIX_FMT_Y10:
	case VIDEO_PIX_FMT_Y12:
	case VIDEO_PIX_FMT_Y14:
	case VIDEO_PIX_FMT_Y16:
	case VIDEO_PIX_FMT_NV16:
	case VIDEO_PIX_FMT_NV61:
		return 16;
	case VIDEO_PIX_FMT_BGR24:
	case VIDEO_PIX_FMT_RGB24:
	case VIDEO_PIX_FMT_NV24:
	case VIDEO_PIX_FMT_NV42:
		return 24;
	case VIDEO_PIX_FMT_XRGB32:
	case VIDEO_PIX_FMT_XYUV32:
	case VIDEO_PIX_FMT_ARGB32:
	case VIDEO_PIX_FMT_ABGR32:
	case VIDEO_PIX_FMT_RGBA32:
	case VIDEO_PIX_FMT_BGRA32:
		return 32;
	default:
		/* Variable number of bits per pixel or unknown format */
		return 0;
	}
}

/**
 * @}
 */

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

#endif /* ZEPHYR_INCLUDE_VIDEO_H_ */
