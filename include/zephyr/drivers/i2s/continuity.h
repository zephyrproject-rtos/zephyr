/*
 * SPDX-FileCopyrightText: Copyright 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_I2S_CONTINUITY_H_
#define ZEPHYR_DRIVERS_I2S_CONTINUITY_H_

/**
 * @file
 * @brief Header file for the I2S continuity RTIO iodev
 * @in_driverbackendgroup{i2s_interface}
 * @{
 */

#include <zephyr/drivers/i2s.h>
#include <zephyr/sys/atomic.h>

/** @cond INTERNAL_HIDDEN */
struct i2s_continuity_iodev_data {
	struct rtio *r;
	struct rtio_iodev *iodev;
	const uint8_t *tx_buf;
	uint8_t *rx_buf;
	size_t buf_len;
	struct mpsc io_q;
	struct k_work work;
	uint16_t sq_count;
	uint8_t op;
	uint8_t prio;
	bool started;
	bool stop;
};
/** @endcond */

/**
 * We need 1 active submission, 1 buffered submission, and 1 queued submission to maintain a
 * stream.
 */
#define I2S_CONTINUITY_STREAM_SUBMISSIONS 3

/** @cond INTERNAL_HIDDEN */
extern const struct rtio_iodev_api i2s_continuity_iodev_api;
/** @endcond */

/**
 * @brief Define an I2S continuity iodev for an I2S device iodev
 *
 * This iodev will maintain an I2S device iodev stream by continuously submitting a static buffer
 * to an I2S device iodev if the I2S continuity iodev is starved. This can be used to "pause"/"mute"
 * audio by submitting a "zeroed" buffer in case of tx or txrx, or maintaining the stream while
 * discarding all data in case of rx.
 *
 * The I2S continuity iodev is started by submitting to it. Every submission will be passed to the
 * I2S device iodev. If the I2S device iodev nears starvation, the I2S continuity iodev will
 * submit static submissions created from the provided static buffers, maintaining the stream. The
 * I2S continuity iodev is stopped by calling @ref i2s_continuity_iodev_stop.
 *
 * @param _name Symbolic name to use for defining the continuity iodev.
 * @param _iodev Pointer to I2S device iodev to maintain continuity for.
 * @param _sq_sz Submission queue size. Must be max in-flight submissions for I2S device iodev.
 * @param _tx_buf Pointer to tx buffer which will be continuously submitted on TX underrun.
 * @param _rx_buf Pointer to rx buffer which will be continuously discarded on RX overrun.
 * @param _buf_len Length of tx and/or rx buffers.
 */
#define I2S_CONTINUTY_IODEV_DEFINE(_name, _iodev, _sq_sz, _tx_buf, _rx_buf, _buf_len)		\
	RTIO_DEFINE(										\
		CONCAT(_i2s_continuity_r, _name),						\
		(_sq_sz) + I2S_CONTINUITY_STREAM_SUBMISSIONS,					\
		(_sq_sz) + I2S_CONTINUITY_STREAM_SUBMISSIONS					\
	);											\
												\
	struct i2s_continuity_iodev_data CONCAT(_i2s_continuity_iodev_data, _name) = {		\
		.r = &CONCAT(_i2s_continuity_r, _name),						\
		.iodev = _iodev,								\
		.tx_buf = _tx_buf,								\
		.rx_buf = _rx_buf,								\
		.buf_len = _buf_len,								\
	};											\
												\
	RTIO_IODEV_DEFINE(									\
		_name,										\
		&i2s_continuity_iodev_api,							\
		&CONCAT(_i2s_continuity_iodev_data, _name)					\
	)

/**
 * @brief Initialize continuity iodev
 */
void i2s_continuity_iodev_init(const struct rtio_iodev *i2s_continuity_iodev);

/**
 * @brief Validate that I2S continuity iodev is ready
 */
bool i2s_continuity_iodev_is_ready(const struct rtio_iodev *i2s_continuity_iodev);

/**
 * @brief Stop a continuity iodev from maintaining an I2S iodev stream
 */
void i2s_continuity_iodev_stop(const struct rtio_iodev *i2s_continuity_iodev);

/** @} */

#endif /* ZEPHYR_DRIVERS_I2S_CONTINUITY_H_ */
