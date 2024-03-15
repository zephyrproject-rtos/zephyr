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
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/device.h>
#include <stddef.h>
#include <zephyr/kernel.h>

#include <zephyr/types.h>

#include <zephyr/drivers/video-controls.h>

#ifdef __cplusplus
extern "C" {
#endif


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
};

/**
 * @brief video_endpoint_id enum
 *
 * Identify the video device endpoint.
 */
enum video_endpoint_id {
	VIDEO_EP_NONE,
	VIDEO_EP_ANY,
	VIDEO_EP_IN,
	VIDEO_EP_OUT,
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
typedef int (*video_api_set_format_t)(const struct device *dev,
				      enum video_endpoint_id ep,
				      struct video_format *fmt);

/**
 * @typedef video_api_get_format_t
 * @brief Get current video format
 *
 * See video_get_format() for argument descriptions.
 */
typedef int (*video_api_get_format_t)(const struct device *dev,
				      enum video_endpoint_id ep,
				      struct video_format *fmt);

/**
 * @typedef video_api_enqueue_t
 * @brief Enqueue a buffer in the driver’s incoming queue.
 *
 * See video_enqueue() for argument descriptions.
 */
typedef int (*video_api_enqueue_t)(const struct device *dev,
				   enum video_endpoint_id ep,
				   struct video_buffer *buf);

/**
 * @typedef video_api_dequeue_t
 * @brief Dequeue a buffer from the driver’s outgoing queue.
 *
 * See video_dequeue() for argument descriptions.
 */
typedef int (*video_api_dequeue_t)(const struct device *dev,
				   enum video_endpoint_id ep,
				   struct video_buffer **buf,
				   k_timeout_t timeout);

/**
 * @typedef video_api_flush_t
 * @brief Flush endpoint buffers, buffer are moved from incoming queue to
 *        outgoing queue.
 *
 * See video_flush() for argument descriptions.
 */
typedef int (*video_api_flush_t)(const struct device *dev,
				 enum video_endpoint_id ep,
				 bool cancel);

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
typedef int (*video_api_set_ctrl_t)(const struct device *dev,
				    unsigned int cid,
				    void *value);

/**
 * @typedef video_api_get_ctrl_t
 * @brief Get a video control value.
 *
 * See video_get_ctrl() for argument descriptions.
 */
typedef int (*video_api_get_ctrl_t)(const struct device *dev,
				    unsigned int cid,
				    void *value);

/**
 * @typedef video_api_get_caps_t
 * @brief Get capabilities of a video endpoint.
 *
 * See video_get_caps() for argument descriptions.
 */
typedef int (*video_api_get_caps_t)(const struct device *dev,
				    enum video_endpoint_id ep,
				    struct video_caps *caps);

/**
 * @typedef video_api_set_signal_t
 * @brief Register/Unregister poll signal for buffer events.
 *
 * See video_set_signal() for argument descriptions.
 */
typedef int (*video_api_set_signal_t)(const struct device *dev,
				      enum video_endpoint_id ep,
				      struct k_poll_signal *signal);

struct video_driver_api {
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
	video_api_set_ctrl_t get_ctrl;
	video_api_set_signal_t set_signal;
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
static inline int video_set_format(const struct device *dev,
				   enum video_endpoint_id ep,
				   struct video_format *fmt)
{
	const struct video_driver_api *api =
		(const struct video_driver_api *)dev->api;

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
static inline int video_get_format(const struct device *dev,
				   enum video_endpoint_id ep,
				   struct video_format *fmt)
{
	const struct video_driver_api *api =
		(const struct video_driver_api *)dev->api;

	if (api->get_format == NULL) {
		return -ENOSYS;
	}

	return api->get_format(dev, ep, fmt);
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
static inline int video_enqueue(const struct device *dev,
				enum video_endpoint_id ep,
				struct video_buffer *buf)
{
	const struct video_driver_api *api =
		(const struct video_driver_api *)dev->api;

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
static inline int video_dequeue(const struct device *dev,
				enum video_endpoint_id ep,
				struct video_buffer **buf,
				k_timeout_t timeout)
{
	const struct video_driver_api *api =
		(const struct video_driver_api *)dev->api;

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
static inline int video_flush(const struct device *dev,
			      enum video_endpoint_id ep,
			      bool cancel)
{
	const struct video_driver_api *api =
		(const struct video_driver_api *)dev->api;

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
	const struct video_driver_api *api =
		(const struct video_driver_api *)dev->api;

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
	const struct video_driver_api *api =
		(const struct video_driver_api *)dev->api;
	int ret;

	if (api->stream_stop == NULL) {
		return -ENOSYS;
	}

	ret = api->stream_stop(dev);
	video_flush(dev, VIDEO_EP_ANY, true);

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
static inline int video_get_caps(const struct device *dev,
				 enum video_endpoint_id ep,
				 struct video_caps *caps)
{
	const struct video_driver_api *api =
		(const struct video_driver_api *)dev->api;

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
static inline int video_set_ctrl(const struct device *dev, unsigned int cid,
				 void *value)
{
	const struct video_driver_api *api =
		(const struct video_driver_api *)dev->api;

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
static inline int video_get_ctrl(const struct device *dev, unsigned int cid,
				 void *value)
{
	const struct video_driver_api *api =
		(const struct video_driver_api *)dev->api;

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
static inline int video_set_signal(const struct device *dev,
				   enum video_endpoint_id ep,
				   struct k_poll_signal *signal)
{
	const struct video_driver_api *api =
		(const struct video_driver_api *)dev->api;

	if (api->set_signal == NULL) {
		return -ENOSYS;
	}

	return api->set_signal(dev, ep, signal);
}

/**
 * @brief Allocate video buffer.
 *
 * @param size Size of the video buffer.
 *
 * @retval pointer to allocated video buffer
 */
struct video_buffer *video_buffer_alloc(size_t size);

/**
 * @brief Release a video buffer.
 *
 * @param buf Pointer to the video buffer to release.
 */
void video_buffer_release(struct video_buffer *buf);


/* fourcc - four-character-code */
#define video_fourcc(a, b, c, d)\
	((uint32_t)(a) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))


/**
 * @defgroup video_pixel_formats Video pixel formats
 * @{
 */

/**
 * @name Bayer formats
 * @{
 */

/** BGGR8 pixel format */
#define VIDEO_PIX_FMT_BGGR8  video_fourcc('B', 'G', 'G', 'R') /*  8  BGBG.. GRGR.. */
/** GBRG8 pixel format */
#define VIDEO_PIX_FMT_GBRG8  video_fourcc('G', 'B', 'R', 'G') /*  8  GBGB.. RGRG.. */
/** GRBG8 pixel format */
#define VIDEO_PIX_FMT_GRBG8  video_fourcc('G', 'R', 'B', 'G') /*  8  GRGR.. BGBG.. */
/** RGGB8 pixel format */
#define VIDEO_PIX_FMT_RGGB8  video_fourcc('R', 'G', 'G', 'B') /*  8  RGRG.. GBGB.. */

/**
 * @}
 */

/**
 * @name RGB formats
 * @{
 */

/** RGB565 pixel format */
#define VIDEO_PIX_FMT_RGB565 video_fourcc('R', 'G', 'B', 'P') /* 16  RGB-5-6-5 */

/** XRGB32 pixel format */
#define VIDEO_PIX_FMT_XRGB32 video_fourcc('B', 'X', '2', '4') /* 32  XRGB-8-8-8-8 */

/**
 * @}
 */

/**
 * @name YUV formats
 * @{
 */

/** YUYV pixel format */
#define VIDEO_PIX_FMT_YUYV video_fourcc('Y', 'U', 'Y', 'V') /* 16  Y0-Cb0 Y1-Cr0 */

/**
 *
 * @}
 */

/**
 * @name JPEG formats
 * @{
 */

/** JPEG pixel format */
#define VIDEO_PIX_FMT_JPEG   video_fourcc('J', 'P', 'E', 'G') /*  8  JPEG */

/**
 * @}
 */

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
