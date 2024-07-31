/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The I2C context provides an event driven framework for I2C
 * device driver transfer implementations.
 *
 * Drivers implementing this framework will work with both the
 * synchronous i2c_transfer() API, and the asynchronous
 * i2c_transfer_cb() and i2c_transfer_signal() APIs.
 *
 * If I2C_CALLBACK=n, the I2C context runs in context of the
 * caller to i2c_transfer(). If I2C_CALLBACK=y, all
 * i2c_transfer*() APIs run in the context of the system
 * workqueue.
 */

#ifndef ZEPHYR_DRIVERS_I2C_I2C_CONTEXT_H_
#define ZEPHYR_DRIVERS_I2C_I2C_CONTEXT_H_

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>

struct i2c_context;

typedef int (*i2c_context_init_transfer_handler)(struct i2c_context *ctx);
typedef void (*i2c_context_start_transfer_handler)(struct i2c_context *ctx);
typedef void (*i2c_context_post_transfer_handler)(struct i2c_context *ctx);
typedef void (*i2c_context_deinit_transfer_handler)(struct i2c_context *ctx);

struct i2c_context {
	const struct device *dev;

	i2c_context_init_transfer_handler init_transfer_handler;
	i2c_context_start_transfer_handler start_transfer_handler;
	i2c_context_post_transfer_handler post_transfer_handler;
	i2c_context_deinit_transfer_handler deinit_transfer_handler;

	struct k_sem transfer_lock;
	struct k_sem transfer_sync;

	struct i2c_msg *transfer_msgs;
	uint8_t transfer_num_msgs;
	uint16_t transfer_addr;
	uint8_t transfer_msg_idx;
	int transfer_result;

#if CONFIG_I2C_CALLBACK
	bool transfer_started;

	struct k_work start_transfer_work;
	struct k_work continue_transfer_work;
	struct k_work cancel_transfer_work;
	struct k_work_delayable transfer_timeout_dwork;

	i2c_callback_t transfer_callback;
	void *transfer_callback_userdata;
#endif
};

void i2c_context_init(struct i2c_context *ctx,
		      const struct device *dev,
		      i2c_context_init_transfer_handler init_transfer_handler,
		      i2c_context_start_transfer_handler start_transfer_handler,
		      i2c_context_post_transfer_handler post_transfer_handler,
		      i2c_context_deinit_transfer_handler deinit_transfer_handler);

int i2c_context_start_transfer(struct i2c_context *ctx,
			       struct i2c_msg *msgs,
			       uint8_t num_msgs,
			       uint16_t addr);

#if CONFIG_I2C_CALLBACK
int i2c_context_start_transfer_cb(struct i2c_context *ctx,
				  struct i2c_msg *msgs,
				  uint8_t num_msgs,
				  uint16_t addr,
				  i2c_callback_t cb,
				  void *userdata);
#endif

void i2c_context_continue_transfer(struct i2c_context *ctx);

void i2c_context_cancel_transfer(struct i2c_context *ctx);

static inline const struct device *i2c_context_get_dev(struct i2c_context *ctx)
{
	return ctx->dev;
}

static inline struct i2c_msg *i2c_context_get_transfer_msgs(struct i2c_context *ctx)
{
	return ctx->transfer_msgs;
}

static inline uint8_t i2c_context_get_transfer_msg_idx(struct i2c_context *ctx)
{
	return ctx->transfer_msg_idx;
}

static inline void i2c_context_set_transfer_msg_idx(struct i2c_context *ctx, uint8_t idx)
{
	ctx->transfer_msg_idx = idx;
}

static inline uint8_t i2c_context_get_transfer_num_msgs(struct i2c_context *ctx)
{
	return ctx->transfer_num_msgs;
}

static inline uint16_t i2c_context_get_transfer_addr(struct i2c_context *ctx)
{
	return ctx->transfer_addr;
}

static inline int i2c_context_get_transfer_result(struct i2c_context *ctx)
{
	return ctx->transfer_result;
}

#endif /* ZEPHYR_DRIVERS_I2C_I2C_CONTEXT_H_ */
