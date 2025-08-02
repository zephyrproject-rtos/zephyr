/*
 * Copyright (c) 2025 Nick Ward
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/modem/pipe.h>
#include <zephyr/modem/stats.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/types.h>

#ifndef ZEPHYR_MODEM_BACKEND_QUECTEL_I2C_
#define ZEPHYR_MODEM_BACKEND_QUECTEL_I2C_
struct modem_backend_quectel_i2c_config {
	const struct i2c_dt_spec i2c;
	uint16_t i2c_poll_interval_ms;
	uint8_t *receive_buf;
	uint8_t *transmit_buf;
	uint16_t receive_buf_size;
	uint16_t transmit_buf_size;
};

struct modem_backend_quectel_i2c {
	struct i2c_dt_spec i2c;
	uint16_t i2c_poll_interval_ms;
	uint8_t *transmit_buf;
	uint16_t transmit_buf_size;
	uint16_t transmit_i;
	struct modem_pipe pipe;
	struct k_work_delayable poll_work;
	struct k_work notify_receive_ready_work;
	struct k_work notify_transmit_idle_work;
	struct k_work notify_closed_work;
	struct ring_buf receive_ring_buf;
	struct k_spinlock receive_rb_lock;
	bool suppress_next_lf;
	int64_t next_cmd_earliest_time;
	bool open;

#if CONFIG_MODEM_STATS
	struct modem_stats_buffer receive_buf_stats;
	struct modem_stats_buffer transmit_buf_stats;
#endif
};

struct modem_pipe *
modem_backend_quectel_i2c_init(struct modem_backend_quectel_i2c *backend,
			       const struct modem_backend_quectel_i2c_config *config);

#endif /* ZEPHYR_MODEM_BACKEND_QUECTEL_I2C_ */
