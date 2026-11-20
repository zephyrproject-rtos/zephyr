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

struct i2s_fallback {
	const struct device *dev;
	struct i2s_rtio *ctx;
	struct k_mem_slab *rx_slab;
	struct k_mem_slab *tx_slab;
	uint8_t *tx_buf_refs[I2S_FALLBACK_BUF_COUNT];
	struct sys_ringq *submission_q;
	struct k_thread thread;
	struct k_sem start_sem;
	uint8_t op;
	size_t buf_len;

	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_I2S_FALLBACK_THREAD_STACK_SIZE);
};

/** @endcond */

/**
 * @brief Statically define an I2S fallback context
 *
 * The I2S fallback context allows implementing the I2S RTIO API in device drivers which implement
 * the non-RTIO I2S APIs.
 *
 * @param _name Symbolic name of the fallback context
 * @param _buf_align Buffer alignment
 */
#define I2S_FALLBACK_DEFINE(_name, _buf_align)							\
	I2S_RTIO_DEFINE(_i2s_fallback_ctx_##_name);						\
												\
	K_MEM_SLAB_DEFINE(									\
		_i2s_fallback_rx_slab_##_name,							\
		CONFIG_I2S_FALLBACK_MAX_BUF_LEN,						\
		I2S_FALLBACK_BUF_COUNT,								\
		_buf_align									\
	);											\
												\
	K_MEM_SLAB_DEFINE(									\
		_i2s_fallback_tx_slab_##_name,							\
		CONFIG_I2S_FALLBACK_MAX_BUF_LEN,						\
		I2S_FALLBACK_BUF_COUNT,								\
		_buf_align									\
	);											\
												\
	SYS_RINGQ_DEFINE(									\
		_i2s_fallback_submission_q_##_name,						\
		sizeof(struct rtio_iodev_sqe *),						\
		I2S_FALLBACK_BUF_COUNT								\
	);											\
												\
	static struct i2s_fallback _name = {							\
		.ctx = &_i2s_fallback_ctx_##_name,						\
		.rx_slab = &_i2s_fallback_rx_slab_##_name,					\
		.tx_slab = &_i2s_fallback_tx_slab_##_name,					\
		.submission_q = &_i2s_fallback_submission_q_##_name,				\
	}

/**
 * @brief Initialize an I2S fallback context
 *
 * @param fallback I2S fallback context to initialize
 */
void i2s_fallback_init(struct i2s_fallback *fallback, const struct device *dev);

/**
 * @brief Submit an I2S RTIO submission to the fallback context
 *
 * @param fallback I2S fallback context
 * @param iodev_sqe I2S RTIO submission
 */
void i2s_fallback_iodev_submit(struct i2s_fallback *fallback, struct rtio_iodev_sqe *iodev_sqe);

/**
 * @brief Signal that an I2S RTIO submission was cancelled
 *
 * @param fallback I2S fallback context
 */
void i2s_fallback_iodev_cancel(struct i2s_fallback *fallback);

#endif /* ZEPHYR_DRIVERS_I2S_FALLBACK_H_ */
