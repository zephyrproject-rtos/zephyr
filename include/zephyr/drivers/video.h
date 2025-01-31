/**
 * @file
 *
 * @brief Public APIs for Video.
 */

/*
 * Copyright (c) 2019 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_VIDEO_H_
#define ZEPHYR_INCLUDE_VIDEO_H_

/**
 * @brief Video Interface
 * @defgroup video_interface Video Interface
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

/**
 * @struct video_format
 * @brief Video format structure
 *
 * Used to configure frame format.
 */
struct video_format {
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
 * @struct video_format_cap
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
 * @struct video_caps
 * @brief Video format capabilities
 *
 * Used to describe video endpoint capabilities.
 */
struct video_caps {
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
 * @struct video_buffer
 * @brief Video buffer structure
 *
 * Represent a video frame.
 */
struct video_buffer {
	/** pointer to driver specific data. */
	void *driver_data;
	/** pointer to the start of the buffer. */
	uint8_t *buffer;
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
 * @brief video_frmival_type enum
 *
 * Supported frame interval type of a video device.
 */
enum video_frmival_type {
	/** discrete frame interval type */
	VIDEO_FRMIVAL_TYPE_DISCRETE = 1,
	/** stepwise frame interval type */
	VIDEO_FRMIVAL_TYPE_STEPWISE = 2,
};

/**
 * @struct video_frmival
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
 * @struct video_frmival_stepwise
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
 * @struct video_frmival_enum
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
 * @brief video_endpoint_id enum
 *
 * Identify the video device endpoint.
 */
enum video_endpoint_id {
	/** Targets some part of the video device not bound to an endpoint */
	VIDEO_EP_NONE = -1,
	/** Targets all input or output endpoints of the device */
	VIDEO_EP_ALL = -2,
	/** Targets all input endpoints of the device: those consuming data */
	VIDEO_EP_IN = -3,
	/** Targets all output endpoints of the device: those producing data */
	VIDEO_EP_OUT = -4,
};

/**
 * @brief video_event enum
 *
 * Identify video event.
 */
enum video_signal_result {
	VIDEO_BUF_DONE,
	VIDEO_BUF_ABORTED,
	VIDEO_BUF_ERROR,
};

/**
 * @typedef video_api_set_format_t
 * @brief Set video format
 *
 * See video_set_format() for argument descriptions.
 */
typedef int (*video_api_set_format_t)(const struct device *dev, enum video_endpoint_id ep,
				      struct video_format *fmt);

/**
 * @typedef video_api_get_format_t
 * @brief Get current video format
 *
 * See video_get_format() for argument descriptions.
 */
typedef int (*video_api_get_format_t)(const struct device *dev, enum video_endpoint_id ep,
				      struct video_format *fmt);

/**
 * @typedef video_api_set_frmival_t
 * @brief Set video frame interval
 *
 * See video_set_frmival() for argument descriptions.
 */
typedef int (*video_api_set_frmival_t)(const struct device *dev, enum video_endpoint_id ep,
				       struct video_frmival *frmival);

/**
 * @typedef video_api_get_frmival_t
 * @brief Get current video frame interval
 *
 * See video_get_frmival() for argument descriptions.
 */
typedef int (*video_api_get_frmival_t)(const struct device *dev, enum video_endpoint_id ep,
				       struct video_frmival *frmival);

/**
 * @typedef video_api_enum_frmival_t
 * @brief List all supported frame intervals of a given format
 *
 * See video_enum_frmival() for argument descriptions.
 */
typedef int (*video_api_enum_frmival_t)(const struct device *dev, enum video_endpoint_id ep,
					struct video_frmival_enum *fie);

/**
 * @typedef video_api_enqueue_t
 * @brief Enqueue a buffer in the driver’s incoming queue.
 *
 * See video_enqueue() for argument descriptions.
 */
typedef int (*video_api_enqueue_t)(const struct device *dev, enum video_endpoint_id ep,
				   struct video_buffer *buf);

/**
 * @typedef video_api_dequeue_t
 * @brief Dequeue a buffer from the driver’s outgoing queue.
 *
 * See video_dequeue() for argument descriptions.
 */
typedef int (*video_api_dequeue_t)(const struct device *dev, enum video_endpoint_id ep,
				   struct video_buffer **buf, k_timeout_t timeout);

/**
 * @typedef video_api_flush_t
 * @brief Flush endpoint buffers, buffer are moved from incoming queue to
 *        outgoing queue.
 *
 * See video_flush() for argument descriptions.
 */
typedef int (*video_api_flush_t)(const struct device *dev, enum video_endpoint_id ep, bool cancel);

/**
 * @typedef video_api_stream_start_t
 * @brief Start the capture or output process.
 *
 * See video_stream_start() for argument descriptions.
 */
typedef int (*video_api_stream_start_t)(const struct device *dev);

/**
 * @typedef video_api_stream_stop_t
 * @brief Stop the capture or output process.
 *
 * See video_stream_stop() for argument descriptions.
 */
typedef int (*video_api_stream_stop_t)(const struct device *dev);

/**
 * @typedef video_api_set_ctrl_t
 * @brief Set a video control value.
 *
 * See video_set_ctrl() for argument descriptions.
 */
typedef int (*video_api_set_ctrl_t)(const struct device *dev, unsigned int cid, void *value);

/**
 * @typedef video_api_get_ctrl_t
 * @brief Get a video control value.
 *
 * See video_get_ctrl() for argument descriptions.
 */
typedef int (*video_api_get_ctrl_t)(const struct device *dev, unsigned int cid, void *value);

/**
 * @typedef video_api_get_caps_t
 * @brief Get capabilities of a video endpoint.
 *
 * See video_get_caps() for argument descriptions.
 */
typedef int (*video_api_get_caps_t)(const struct device *dev, enum video_endpoint_id ep,
				    struct video_caps *caps);

/**
 * @typedef video_api_set_signal_t
 * @brief Register/Unregister poll signal for buffer events.
 *
 * See video_set_signal() for argument descriptions.
 */
typedef int (*video_api_set_signal_t)(const struct device *dev, enum video_endpoint_id ep,
				      struct k_poll_signal *signal);

__subsystem struct video_driver_api {
	/* mandatory callbacks */
	video_api_set_format_t set_format;
	video_api_get_format_t get_format;
	video_api_stream_start_t stream_start;
	video_api_stream_stop_t stream_stop;
	video_api_get_caps_t get_caps;
	/* optional callbacks */
	video_api_enqueue_t enqueue;
	video_api_dequeue_t dequeue;
	video_api_flush_t flush;
	video_api_set_ctrl_t set_ctrl;
	video_api_get_ctrl_t get_ctrl;
	video_api_set_signal_t set_signal;
	video_api_set_frmival_t set_frmival;
	video_api_get_frmival_t get_frmival;
	video_api_enum_frmival_t enum_frmival;
};

/**
 * @brief Set video format.
 *
 * Configure video device with a specific format.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param ep Endpoint ID.
 * @param fmt Pointer to a video format struct.
 *
 * @retval 0 Is successful.
 * @retval -EINVAL If parameters are invalid.
 * @retval -ENOTSUP If format is not supported.
 * @retval -EIO General input / output error.
 */
static inline int video_set_format(const struct device *dev, enum video_endpoint_id ep,
				   struct video_format *fmt)
{
	const struct video_driver_api *api = (const struct video_driver_api *)dev->api;

	if (api->set_format == NULL) {
		return -ENOSYS;
	}

	return api->set_format(dev, ep, fmt);
}

/**
 * @brief Get video format.
 *
 * Get video device current video format.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param ep Endpoint ID.
 * @param fmt Pointer to video format struct.
 *
 * @retval pointer to video format
 */
static inline int video_get_format(const struct device *dev, enum video_endpoint_id ep,
				   struct video_format *fmt)
{
	const struct video_driver_api *api = (const struct video_driver_api *)dev->api;

	if (api->get_format == NULL) {
		return -ENOSYS;
	}

	return api->get_format(dev, ep, fmt);
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
 * @param ep Endpoint ID.
 * @param frmival Pointer to a video frame interval struct.
 *
 * @retval 0 If successful.
 * @retval -ENOSYS If API is not implemented.
 * @retval -EINVAL If parameters are invalid.
 * @retval -EIO General input / output error.
 */
static inline int video_set_frmival(const struct device *dev, enum video_endpoint_id ep,
				    struct video_frmival *frmival)
{
	const struct video_driver_api *api = (const struct video_driver_api *)dev->api;

	if (api->set_frmival == NULL) {
		return -ENOSYS;
	}

	return api->set_frmival(dev, ep, frmival);
}

/**
 * @brief Get video frame interval.
 *
 * Get current frame interval of the video device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param ep Endpoint ID.
 * @param frmival Pointer to a video frame interval struct.
 *
 * @retval 0 If successful.
 * @retval -ENOSYS If API is not implemented.
 * @retval -EINVAL If parameters are invalid.
 * @retval -EIO General input / output error.
 */
static inline int video_get_frmival(const struct device *dev, enum video_endpoint_id ep,
				    struct video_frmival *frmival)
{
	const struct video_driver_api *api = (const struct video_driver_api *)dev->api;

	if (api->get_frmival == NULL) {
		return -ENOSYS;
	}

	return api->get_frmival(dev, ep, frmival);
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
 * @param ep Endpoint ID.
 * @param fie Pointer to a video frame interval enumeration struct.
 *
 * @retval 0 If successful.
 * @retval -ENOSYS If API is not implemented.
 * @retval -EINVAL If parameters are invalid.
 * @retval -EIO General input / output error.
 */
static inline int video_enum_frmival(const struct device *dev, enum video_endpoint_id ep,
				     struct video_frmival_enum *fie)
{
	const struct video_driver_api *api = (const struct video_driver_api *)dev->api;

	if (api->enum_frmival == NULL) {
		return -ENOSYS;
	}

	return api->enum_frmival(dev, ep, fie);
}

/**
 * @brief Enqueue a video buffer.
 *
 * Enqueue an empty (capturing) or filled (output) video buffer in the driver’s
 * endpoint incoming queue.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param ep Endpoint ID.
 * @param buf Pointer to the video buffer.
 *
 * @retval 0 Is successful.
 * @retval -EINVAL If parameters are invalid.
 * @retval -EIO General input / output error.
 */
static inline int video_enqueue(const struct device *dev, enum video_endpoint_id ep,
				struct video_buffer *buf)
{
	const struct video_driver_api *api = (const struct video_driver_api *)dev->api;

	if (api->enqueue == NULL) {
		return -ENOSYS;
	}

	return api->enqueue(dev, ep, buf);
}

/**
 * @brief Dequeue a video buffer.
 *
 * Dequeue a filled (capturing) or displayed (output) buffer from the driver’s
 * endpoint outgoing queue.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param ep Endpoint ID.
 * @param buf Pointer a video buffer pointer.
 * @param timeout Timeout
 *
 * @retval 0 Is successful.
 * @retval -EINVAL If parameters are invalid.
 * @retval -EIO General input / output error.
 */
static inline int video_dequeue(const struct device *dev, enum video_endpoint_id ep,
				struct video_buffer **buf, k_timeout_t timeout)
{
	const struct video_driver_api *api = (const struct video_driver_api *)dev->api;

	if (api->dequeue == NULL) {
		return -ENOSYS;
	}

	return api->dequeue(dev, ep, buf, timeout);
}

/**
 * @brief Flush endpoint buffers.
 *
 * A call to flush finishes when all endpoint buffers have been moved from
 * incoming queue to outgoing queue. Either because canceled or fully processed
 * through the video function.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param ep Endpoint ID.
 * @param cancel If true, cancel buffer processing instead of waiting for
 *        completion.
 *
 * @retval 0 Is successful, -ERRNO code otherwise.
 */
static inline int video_flush(const struct device *dev, enum video_endpoint_id ep, bool cancel)
{
	const struct video_driver_api *api = (const struct video_driver_api *)dev->api;

	if (api->flush == NULL) {
		return -ENOSYS;
	}

	return api->flush(dev, ep, cancel);
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
 * @retval 0 Is successful.
 * @retval -EIO General input / output error.
 */
static inline int video_stream_start(const struct device *dev)
{
	const struct video_driver_api *api = (const struct video_driver_api *)dev->api;

	if (api->stream_start == NULL) {
		return -ENOSYS;
	}

	return api->stream_start(dev);
}

/**
 * @brief Stop the video device function.
 *
 * On video_stream_stop, driver must stop any transactions or wait until they
 * finish.
 *
 * @retval 0 Is successful.
 * @retval -EIO General input / output error.
 */
static inline int video_stream_stop(const struct device *dev)
{
	const struct video_driver_api *api = (const struct video_driver_api *)dev->api;
	int ret;

	if (api->stream_stop == NULL) {
		return -ENOSYS;
	}

	ret = api->stream_stop(dev);
	video_flush(dev, VIDEO_EP_ALL, true);

	return ret;
}

/**
 * @brief Get the capabilities of a video endpoint.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param ep Endpoint ID.
 * @param caps Pointer to the video_caps struct to fill.
 *
 * @retval 0 Is successful, -ERRNO code otherwise.
 */
static inline int video_get_caps(const struct device *dev, enum video_endpoint_id ep,
				 struct video_caps *caps)
{
	const struct video_driver_api *api = (const struct video_driver_api *)dev->api;

	if (api->get_caps == NULL) {
		return -ENOSYS;
	}

	return api->get_caps(dev, ep, caps);
}

/**
 * @brief Set the value of a control.
 *
 * This set the value of a video control, value type depends on control ID, and
 * must be interpreted accordingly.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param cid Control ID.
 * @param value Pointer to the control value.
 *
 * @retval 0 Is successful.
 * @retval -EINVAL If parameters are invalid.
 * @retval -ENOTSUP If format is not supported.
 * @retval -EIO General input / output error.
 */
static inline int video_set_ctrl(const struct device *dev, unsigned int cid, void *value)
{
	const struct video_driver_api *api = (const struct video_driver_api *)dev->api;

	if (api->set_ctrl == NULL) {
		return -ENOSYS;
	}

	return api->set_ctrl(dev, cid, value);
}

/**
 * @brief Get the current value of a control.
 *
 * This retrieve the value of a video control, value type depends on control ID,
 * and must be interpreted accordingly.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param cid Control ID.
 * @param value Pointer to the control value.
 *
 * @retval 0 Is successful.
 * @retval -EINVAL If parameters are invalid.
 * @retval -ENOTSUP If format is not supported.
 * @retval -EIO General input / output error.
 */
static inline int video_get_ctrl(const struct device *dev, unsigned int cid, void *value)
{
	const struct video_driver_api *api = (const struct video_driver_api *)dev->api;

	if (api->get_ctrl == NULL) {
		return -ENOSYS;
	}

	return api->get_ctrl(dev, cid, value);
}

/**
 * @brief Register/Unregister k_poll signal for a video endpoint.
 *
 * Register a poll signal to the endpoint, which will be signaled on frame
 * completion (done, aborted, error). Registering a NULL poll signal
 * unregisters any previously registered signal.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param ep Endpoint ID.
 * @param signal Pointer to k_poll_signal
 *
 * @retval 0 Is successful, -ERRNO code otherwise.
 */
static inline int video_set_signal(const struct device *dev, enum video_endpoint_id ep,
				   struct k_poll_signal *signal)
{
	const struct video_driver_api *api = (const struct video_driver_api *)dev->api;

	if (api->set_signal == NULL) {
		return -ENOSYS;
	}

	return api->set_signal(dev, ep, signal);
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
 * @param ep Video endpoint ID to query.
 * @param match Frame interval enumerator with the query, and loaded with the result.
 */
void video_closest_frmival(const struct device *dev, enum video_endpoint_id ep,
			   struct video_frmival_enum *match);

/**
 * @defgroup video_pixel_formats Video pixel formats
 * The @c | characters separate the pixels, and spaces separate the bytes.
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
 * @name Bayer formats (R, G, B channels).
 *
 * The full color information is spread over multiple pixels.
 *
 * @{
 */

/**
 * @verbatim
 * | Bbbbbbbb | Gggggggg | Bbbbbbbb | Gggggggg | Bbbbbbbb | Gggggggg | ...
 * | Gggggggg | Rrrrrrrr | Gggggggg | Rrrrrrrr | Gggggggg | Rrrrrrrr | ...
 * @endverbatim
 */
#define VIDEO_PIX_FMT_BGGR8 VIDEO_FOURCC('B', 'A', '8', '1')

/**
 * @verbatim
 * | Gggggggg | Bbbbbbbb | Gggggggg | Bbbbbbbb | Gggggggg | Bbbbbbbb | ...
 * | Rrrrrrrr | Gggggggg | Rrrrrrrr | Gggggggg | Rrrrrrrr | Gggggggg | ...
 * @endverbatim
 */
#define VIDEO_PIX_FMT_GBRG8 VIDEO_FOURCC('G', 'B', 'R', 'G')

/**
 * @verbatim
 * | Gggggggg | Rrrrrrrr | Gggggggg | Rrrrrrrr | Gggggggg | Rrrrrrrr | ...
 * | Bbbbbbbb | Gggggggg | Bbbbbbbb | Gggggggg | Bbbbbbbb | Gggggggg | ...
 * @endverbatim
 */
#define VIDEO_PIX_FMT_GRBG8 VIDEO_FOURCC('G', 'R', 'B', 'G')

/**
 * @verbatim
 * | Rrrrrrrr | Gggggggg | Rrrrrrrr | Gggggggg | Rrrrrrrr | Gggggggg | ...
 * | Gggggggg | Bbbbbbbb | Gggggggg | Bbbbbbbb | Gggggggg | Bbbbbbbb | ...
 * @endverbatim
 */
#define VIDEO_PIX_FMT_RGGB8 VIDEO_FOURCC('R', 'G', 'G', 'B')

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
 * @verbatim
 *   15.....8 7......0
 * | RrrrrGgg gggBbbbb | ...
 * @endverbatim
 */
#define VIDEO_PIX_FMT_RGB565X VIDEO_FOURCC('R', 'G', 'B', 'R')

/**
 * 5 red bits [15:11], 6 green bits [10:5], 5 blue bits [4:0].
 * This 16-bit integer is then packed in little endian format over two bytes:
 *
 * @verbatim
 *   7......0 15.....8
 * | gggBbbbb RrrrrGgg | ...
 * @endverbatim
 */
#define VIDEO_PIX_FMT_RGB565 VIDEO_FOURCC('R', 'G', 'B', 'P')

/**
 * The first byte is empty (X) for each pixel.
 *
 * @verbatim
 * | Xxxxxxxx Rrrrrrrr Gggggggg Bbbbbbbb | ...
 * @endverbatim
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
 * @verbatim
 * | Yyyyyyyy Uuuuuuuu | Yyyyyyyy Vvvvvvvv | ...
 * @endverbatim
 */
#define VIDEO_PIX_FMT_YUYV VIDEO_FOURCC('Y', 'U', 'Y', 'V')

/**
 * The first byte is empty (X) for each pixel.
 *
 * @verbatim
 * | Xxxxxxxx Yyyyyyyy Uuuuuuuu Vvvvvvvv | ...
 * @endverbatim
 */
#define VIDEO_PIX_FMT_XYUV32 VIDEO_FOURCC('X', 'Y', 'U', 'V')

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
 * @retval bit size of one pixel for this format
 */
static inline unsigned int video_bits_per_pixel(uint32_t pixfmt)
{
	switch (pixfmt) {
	case VIDEO_PIX_FMT_BGGR8:
	case VIDEO_PIX_FMT_GBRG8:
	case VIDEO_PIX_FMT_GRBG8:
	case VIDEO_PIX_FMT_RGGB8:
		return 8;
	case VIDEO_PIX_FMT_RGB565:
	case VIDEO_PIX_FMT_YUYV:
		return 16;
	case VIDEO_PIX_FMT_XRGB32:
	case VIDEO_PIX_FMT_XYUV32:
		return 32;
	default:
		/* Variable number of bits per pixel or unknown format */
		return 0;
	}
}

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
