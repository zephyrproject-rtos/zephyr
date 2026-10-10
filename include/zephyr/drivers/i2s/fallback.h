/*
 * SPDX-FileCopyrightText: Copyright 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_I2S_FALLBACK_H_
#define ZEPHYR_DRIVERS_I2S_FALLBACK_H_

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/i2s/rtio.h>
#include <zephyr/sys/ringq.h>

/** @cond INTERNAL_HIDDEN */

/*
 * We need 1 active buffer, 1 buffered buffer, and 1 available buffer, per direction, to maintain a
 * stream.
 */
#define I2S_FALLBACK_BUF_COUNT 3

struct i2s_fallback_iodev_data {
	const struct device *dev;
	struct i2s_iodev_config config;
	struct i2s_rtio *ctx;
	struct k_mem_slab *rx_slab;
	struct k_mem_slab *tx_slab;
	size_t max_buf_len;
	uint8_t *tx_buf_refs[I2S_FALLBACK_BUF_COUNT];
	struct sys_ringq *submission_q;
	struct k_thread thread;
	struct k_sem start_sem;
	uint8_t op;

	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_I2S_FALLBACK_THREAD_STACK_SIZE);
};

extern const struct rtio_iodev_api i2s_fallback_iodev_api;

/** @endcond */

/**
 * @brief Statically define an I2S fallback iodev
 *
 * The I2S fallback iodev allows using the I2S RTIO API with device drivers which implement
 * the non-RTIO I2S API.
 *
 * @param _name Symbolic name of the iodev
 * @param _node_id Node identifier of the I2S device
 * @param _word_size Number of bits representing one data word
 * @param _channels Number of words per frame
 * @param _format Data stream format as defined by I2S_FMT_* constants
 * @param _options Configuration options as defined by I2S_OPT_* constants
 * @param _frame_clk_freq Frame clock (WS) frequency, this is the sampling rate
 * @param _preload_count Number of submissions to preload before starting the stream
 * @param _max_buf_len Maximum supported buffer size in bytes
 * @param _buf_align Buffer alignment
 */
#define I2S_DT_FALLBACK_IODEV_DEFINE(_name,							\
				     _node_id,							\
				     _word_size,						\
				     _channels,							\
				     _format,							\
				     _options,							\
				     _frame_clk_freq,						\
				     _preload_count,						\
				     _max_buf_len,						\
				     _buf_align)						\
	I2S_RTIO_DEFINE(_i2s_fallback_iodev_ctx_##_name);					\
												\
	K_MEM_SLAB_DEFINE(									\
		_i2s_fallback_iodev_rx_slab_##_name,						\
		_max_buf_len,									\
		I2S_FALLBACK_BUF_COUNT,								\
		_buf_align									\
	);											\
												\
	K_MEM_SLAB_DEFINE(									\
		_i2s_fallback_iodev_tx_slab_##_name,						\
		_max_buf_len,									\
		I2S_FALLBACK_BUF_COUNT,								\
		_buf_align									\
	);											\
												\
	SYS_RINGQ_DEFINE(									\
		_i2s_fallback_iodev_submission_q_##_name,					\
		sizeof(struct rtio_iodev_sqe *),						\
		I2S_FALLBACK_BUF_COUNT								\
	);											\
												\
	struct i2s_fallback_iodev_data CONCAT(_i2s_fallback_iodev_data_, _name) = {		\
		.dev = DEVICE_DT_GET(_node_id),							\
		.config = {									\
			.word_size = _word_size,						\
			.channels = _channels,							\
			.format = _format,							\
			.options = _options,							\
			.frame_clk_freq = _frame_clk_freq,					\
			.preload_count = _preload_count,					\
		},										\
		.ctx = &_i2s_fallback_iodev_ctx_##_name,					\
		.rx_slab = &_i2s_fallback_iodev_rx_slab_##_name,				\
		.tx_slab = &_i2s_fallback_iodev_tx_slab_##_name,				\
		.max_buf_len = _max_buf_len,							\
		.submission_q = &_i2s_fallback_iodev_submission_q_##_name,			\
	};											\
												\
	RTIO_IODEV_DEFINE(									\
		_name,										\
		&i2s_fallback_iodev_api,							\
		&_i2s_fallback_iodev_data_##_name						\
	)

/**
 * @brief Initialize an I2S fallback iodev
 *
 * @param iodev I2S fallback iodev to initialize
 */
void i2s_fallback_iodev_init(struct rtio_iodev *iodev);

/**
 * @brief Validate that I2S fallback iodev is ready
 */
bool i2s_fallback_iodev_is_ready(const struct rtio_iodev *iodev);

#endif /* ZEPHYR_DRIVERS_I2S_FALLBACK_H_ */
