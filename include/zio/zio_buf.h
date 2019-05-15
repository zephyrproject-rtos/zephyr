/*
 * Copyright (c) 2019 Thomas Burdick <thomas.burdick@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file zio_buf.h
 *
 * @brief ZIO Buffer provides a pollable interface to a FIFO like buffer
 *
 * Device drivers implement this interface using either a software or
 * hardware FIFO.
 *
 * The format of the buffer is device dependent and thus a device must
 * implement part of the API by describing the layout of the buffer. Each
 * device is responsible for self describing the position, byte size, byte
 * order, bit precision, and bit shift each value provides in a static
 * array of struct zio_chan values.
 *
 * Changing enabled/disabled channels while using the same zio_buf isn't
 * supported for that reason.
 *
 * In addition to the data itself a 32 bit trigger timestamp corresponding to
 * k_cycle_get_32() may be placed as the last set of bytes of each datum
 * pulled.
 *
 * The buffer is pollable using k_poll where the pollable event becomes
 * available when the zio_buf contains at least the designated watermark
 * number of datums. It will cease to become pollable once it contains
 * less than the designated watermark number of datums.
 *
 * Adjusting the watermark value may or may not be implemented depending
 * on the FIFO implementation.
 *
 * The ability to poll one or more buffers allows an application designer
 * to decide how to deal with incoming streams of data from one or more
 * sensors.
 *
 * This is heavily inspired by the fantastic work in Linux's IIO subsystem,
 * with a microcontroller focused RTOS in mind.
 *
 * As an example in the case of a 9 DoF IMU sensor where each channel
 * produces a 16 bit value the buffer after each trigger without timestamps
 * would contain 18 bytes of data, 22 bytes with the 32bit cycle counter
 * timestamp.
 */

#ifndef ZEPHYR_INCLUDE_ZIO_BUF_H_
#define ZEPHYR_INCLUDE_ZIO_BUF_H_

#include <zephyr/types.h>
#include <kernel.h>
#include <zio/zio_dev.h>
#include <misc/__assert.h>

struct zio_buf;

/**
 * @brief Function to pull from the zio_buf a single datum
 *
 * The size of the datum is decided by the device and its set of active
 * channels. Its expected the caller already knows the exact size of
 * sample.
 */
typedef int (*zio_buf_pull_t)(struct zio_buf *buf, void *datum);

/**
 * @brief Function to set watermark of zio_buf
 *
 * Optional function for a driver if watermark manipulation is possible
 */
typedef int (*zio_buf_set_watermark_t)(struct zio_buf *buf, u32_t watermark);

/**
 * @brief Function to get watermark of zio_buf
 *
 * Optional function for a driver to return the known watermark
 */
typedef u32_t (*zio_buf_get_watermark_t)(struct zio_buf *buf);

/**
 * @brief Function to get length of zio_buf
 *
 * Optional function for a driver to return the known length
 */
typedef u32_t (*zio_buf_get_length_t)(struct zio_buf *buf);

/**
 * @brief Function to get the capacity of a zio_buf
 *
 * Optional function for a driver to return the known length
 */
typedef u32_t (*zio_buf_get_capacity_t)(struct zio_buf *buf);

/**
 * @brief A pollable buffer interface for reading and writing to streams of data
 *
 * Most devices will want to simply use a fifo backed zio_buf but optionally
 * devices may provide a hardware fifo backed zio_buf.
 *
 * The interface should support either. In the case of a fifo backed zio_buf
 * devices should make a best effort to use DMA transfers rather than memcpy
 */
struct zio_buf_api {
	zio_buf_pull_t pull;
	zio_buf_set_watermark_t set_watermark;
	zio_buf_get_watermark_t get_watermark;
	zio_buf_get_length_t get_length;
	zio_buf_get_capacity_t get_capacity;
};

/**
 * @brief A pollable fifo-like buffer for reading and writing to streams of data
 *
 * An implementation should implement the api above statically and provide an
 * attach/detach function pair to attach a zio_buf struct to it.
 */
struct zio_buf {
	struct device *device;
	struct zio_buf_api *api;
	void *api_data;
	struct k_sem sem;
};


/**
 * @private
 * @brief ZIO_BUF static initializer
 */
#define Z_ZIO_BUF_INITIALIZER(name)		\
	{					\
		.device = NULL,			\
		.api = NULL,	                \
		.api_data = NULL		\
	}

/**
 * @brief Define and initialize a zio_buf
 *
 * @param name Name of the zio_buf
 */
#define ZIO_BUF_DEFINE(name) \
	struct zio_buf name \
	= Z_ZIO_BUF_INITIALIZER(name)

/**
 * @brief Attach a buffer to a device.
 *
 * @param buf Pointer to a zio_buf struct
 * @param dev Pointer to a zephyr device which implements the zio api
 *
 * @return 0 on success, -errno on failure.
 */
static inline int zio_buf_attach(struct zio_buf *buf, struct device *dev)
{
	const struct zio_dev_api *api = dev->driver_api;

	if (!api->attach_buf) {
		return -ENOTSUP;
	}
	return api->attach_buf(dev, buf);
}

/**
 * @brief Detach the buffer from the device
 *
 * @param buf Pointer to a zio_buf struct
 * @return 0 on success, -errno on failure.
 */
static inline int zio_buf_detach(struct zio_buf *buf)
{
	struct device *dev = buf->device;
	const struct zio_dev_api *api = dev->driver_api;

	if (!api->detach_buf) {
		return -ENOTSUP;
	}
	return api->detach_buf(dev);
}

/**
 * @brief Pull a sample from the buffer
 *
 * The size of the sample is the sum of bytesizes of all enabled channels for a
 * device
 *
 * @param buf Pointer to a zio_buf struct
 * @param sample Pointer to sample to move sample into
 * @return 1 on success, 0 if nothing to pull, -errno on failure.
 */
static inline int zio_buf_pull(struct zio_buf *buf, void *sample)
{
	const struct zio_buf_api *api = buf->api;

	if (!api->pull) {
		return -ENOTSUP;
	}

	int res = k_sem_take(&buf->sem, K_NO_WAIT);

	if (res != 0) {
		return 0;
	}

	return api->pull(buf, sample);
}

/**
 * @brief Set the desired watermark
 *
 * Not all zio_buf implementations provide watermark manipulation as
 * many hardware backed implementations might not provide this functionality.
 *
 * @param buf Pointer to a zio_buf struct
 * @param watermark Desired watermark to cause a pollable signal to be set
 * @return 0 on success, -errno on failure.
 */
static inline int zio_buf_set_watermark(struct zio_buf *buf, u32_t watermark)
{
	const struct zio_buf_api *api = buf->api;

	if (!api->set_watermark) {
		return -ENOTSUP;
	}

	return api->set_watermark(buf, watermark);
}

/**
 * @brief Get the watermark value
 *
 * The watermark determines the minimum number of samples needed in the buffer
 * to cause k_poll on this zio_buf to return it as ready
 *
 * @param buf Pointer to a zio_buf struct
 * @param watermark Watermark to cause a signal being set
 * @return 0 on success, -errno on failure.
 */
static inline int zio_buf_get_watermark(struct zio_buf *buf, u32_t *watermark)
{
	const struct zio_buf_api *api = buf->api;

	if (!api->get_watermark) {
		return -ENOTSUP;
	}
	*watermark = api->get_watermark(buf);
	return 0;
}

/**
 * @brief Get the number of samples contained in the buffer
 *
 * @param buf Pointer to a zio_buf struct
 * @return length.
 */
static inline u32_t zio_buf_get_length(struct zio_buf *buf)
{
	const struct zio_buf_api *api = buf->api;

	__ASSERT_NO_MSG(api->get_length != NULL);

	return api->get_length(buf);
}

/**
 * @brief Get the maximum number of samples that the buffer can contain
 *
 * @param buf Pointer to a zio_buf struct
 * @return capacity
 */
static inline int zio_buf_get_capacity(struct zio_buf *buf)
{
	const struct zio_buf_api *api = buf->api;

	__ASSERT_NO_MSG(api->get_capacity != NULL);

	return api->get_capacity(buf);
}

/**
 * @private
 * @brief Give the zio_buf sem n times
 *
 */
static inline void z_zio_buf_sem_give(struct zio_buf *buf, u32_t n)
{
	for (u32_t i = 0; i < n; i++) {
		k_sem_give(&buf->sem);
	}
}

/**
 * @private
 * @brief Attach an implementation
 */
static inline void z_zio_buf_attach(struct zio_buf *buf,
	struct zio_buf_api *api, void *api_data)
{
	buf->api = api;
	buf->api_data = api_data;
	u32_t length = zio_buf_get_length(buf);
	u32_t capacity = zio_buf_get_capacity(buf);

	k_sem_init(&buf->sem, length, capacity);
}

/**
 * @private
 * @brief Detach an implementation
 */
static inline void z_zio_buf_detach(struct zio_buf *buf)
{
	buf->api = NULL;
	buf->api_data = NULL;
	k_sem_reset(&buf->sem);
}



/* TODO
 * - define functions for manipulating attributes by their type, assuming one of
 *   each type of attribute exists
 * - define functions for accessing each channel or groups of channels in a
 *   more meaningful way from interleaved samples
 *   ie accel x,y,z gyro x,y,z, timestamp etc
 * - define helpers for obtaining SI unit converted values when possible
 */

#endif /* ZEPHYR_INCLUDE_ZIO_BUF_H_ */

