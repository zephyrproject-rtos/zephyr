/**
 * @file
 *
 * @brief Public APIs for Video for Zephyr (V4Z).
 */

/*
 * Copyright (c) 2019 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_V4Z_H_
#define ZEPHYR_INCLUDE_V4Z_H_

#ifdef __cplusplus
extern "C" {
#endif

enum v4z_device_type {
	V4Z_CAPTURE_DEVICE,
	V4Z_OUTPUT_DEVICE,
	V4Z_CONTROL_DEVICE,
};

struct v4z_device_config {
	enum v4z_device_type;
};

struct v4z_format {
	u32_t pixelformat;
	u32_t width;
	u32_t height;
}

struct v4z_buffer {
	/** Data buffer in bytes */
	void *data;

	/** Length of buffer in bytes */
	size_t len;

	/** Flags for the buffer */
	u8_t flags;

	/** format **/
	struct v4z_format fmt;
};

/**
 * @typedef v4z_api_set_fmt_t
 * @brief Set video format
 * See @a v4z_api_set_fmt() for argument.
 * description.
 */
typedef int (*v4z_api_set_fmt_t)(struct device *dev, struct v4z_format *fmt);

/**
 * @typedef v4z_api_get_fmt_t
 * @brief get current video format
 * See @a v4z_api_set_fmt() for argument.
 * description.
 */
typedef struct v4z_format *(*v4z_api_get_fmt_t)(struct device *dev);

/**
 * @typedef v4z_api_enqueue_t
 * @brief Enqueue a buffer in the driver’s incoming queue.
 * See @a v4z_enqueue() for argument
 * description.
 */
typedef int (*v4z_api_enqueue_t)(struct device *dev, struct v4z_buffer *buf);

/**
 * @typedef v4z_api_dequeue_t
 * @brief Dequeue a buffer from the driver’s outgoing queue.
 * See @a v4z_api_dequeue() for argument.
 * description.
 */
typedef int (*v4z_api_dequeue_t)(struct device *dev, struct v4z_buffer **buf,
				 u32_t timeout);

/**
 * @typedef v4z_api_stream_start_t
 * @brief Start the capture or output process.
 */
typedef int (*v4z_api_stream_start_t)(struct device *dev);

/**
 * @typedef v4z_api_stream_stop_t
 * @brief Stop the capture or output process.
 */
typedef int (*v4z_api_stream_stop_t)(struct device *dev);

struct v4z_driver_api {
	v4z_api_set_fmt_t set_fmt;
	v4z_api_get_fmt_t get_fmt;
	v4z_api_enqueue_t enqueue;
	v4z_api_dequeue_t dequeue;
	v4z_api_stream_start_t stream_start;
	v4z_api_stream_stop_t stream_stop;
};

/**
 * @brief Set video format.
 *
 * Configure v4z device with a specific format
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param fmt Pointer to a v4z format struct.
 *
 * @retval 0 Is successful.
 * @retval -EINVAL If parameters are invalid.
 * @retval -ENOTSUP If format is not supported
 * @retval -EIO General input / output error.
 */
__syscall int v4z_set_fmt(struct device *dev, struct v4z_format *fmt);

static inline int z_impl_v4z_set_fmt(struct device *dev, struct v4z_format *fmt)
{
	const struct v4z_driver_api *api =
		(const struct v4z_driver_api *)dev->driver_api;

	return api->set_fmt(dev, buf);
}

/**
 * @brief Get video format.
 *
 * Get v4z device current video format
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval pointer to video format
 */
__syscall int v4z_get_fmt(struct device *dev, struct v4z_format *fmt);

static inline struct v4z_format *z_impl_v4z_get_fmt(struct device *dev)
{
	const struct v4z_driver_api *api =
		(const struct v4z_driver_api *)dev->driver_api;

	return api->get_fmt(dev, buf);
}

/**
 * @brief Enqueue a v4z buffer.
 *
 * enqueue an empty (capturing) or filled (output) buffer in the driver’s
 * incoming queue.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @buf Pointer to the v4z buffer.
 *
 * @retval 0 Is successful.
 * @retval -EINVAL If parameters are invalid.
 * @retval -EIO General input / output error.
 */
__syscall int v4z_enqueue(struct device *dev, struct v4z_buffer *buf);

static inline int z_impl_v4z_enqueue(struct device *dev, struct v4z_buffer *buf)
{
	const struct v4z_driver_api *api =
		(const struct v4z_driver_api *)dev->driver_api;

	return api->enqueue(dev, buf);
}

/**
 * @brief Dequeue a v4z buffer.
 *
 * Dequeue a filled (capturing) or displayed (output) buffer from the driver’s
 * outgoing queue.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @buf Pointer a v4z buffer pointer.
 * @timeout Timeout in milliseconds.
 *
 * @retval 0 Is successful.
 * @retval -EINVAL If parameters are invalid.
 * @retval -EIO General input / output error.
 */
__syscall int v4z_dequeue(struct device *dev, struct v4z_buffer **buf,
			  u32_t timeout);

static inline int z_impl_v4z_dequeue(struct device *dev,
				     struct v4z_buffer **buf, u32_t timeout)
{
	const struct v4z_driver_api *api =
		(const struct v4z_driver_api *)dev->driver_api;

	return api->enqueue(dev, buf);
}

/**
 * @brief Start the capture or output process.
 *
 * @retval 0 Is successful.
 * @retval -EIO General input / output error.
 */
__syscall int v4z_stream_start(struct device *dev);

static inline int z_impl_v4z_stream_start(struct device *dev)
{
	const struct v4z_driver_api *api =
		(const struct v4z_driver_api *)dev->driver_api;

	return api->stream_start(dev);
}

/**
 * @brief Stop the capture or output process
 *
 * @retval 0 Is successful.
 * @retval -EIO General input / output error.
 */
static inline int z_impl_v4z_stream_stop(struct device *dev)
{
	const struct v4z_driver_api *api =
		(const struct v4z_driver_api *)dev->driver_api;

	return api->stream_stop(dev);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_V4Z_H_ */
