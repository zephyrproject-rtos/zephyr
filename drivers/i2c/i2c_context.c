/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "i2c_context.h"

/**
 * @brief Implements the i2c stats for i2c transfers
 *
 * @param dev I2C device to update stats for
 * @param msgs Array of struct i2c_msg
 * @param num_msgs Number of i2c_msgs
 */
void z_i2c_xfer_stats(const struct device *dev, struct i2c_msg *msgs,
				  uint8_t num_msgs)
{
#ifdef CONFIG_I2C_STATS
	struct i2c_device_state *state =
		CONTAINER_OF(dev->state, struct i2c_device_state, devstate);

	/* Check if the magic exists so we can use the common data */
	if (state->magic != Z_I2C_MAGIC) {
		return;
	}

	uint32_t bytes_read = 0U;
	uint32_t bytes_written = 0U;

	STATS_INC(state->stats, transfer_call_count);
	STATS_INCN(state->stats, message_count, num_msgs);
	for (uint8_t i = 0U; i < num_msgs; i++) {
		if (msgs[i].flags & I2C_MSG_READ) {
			bytes_read += msgs[i].len;
		}
		if (msgs[i].flags & I2C_MSG_WRITE) {
			bytes_written += msgs[i].len;
		}
	}
	STATS_INCN(state->stats, bytes_read, bytes_read);
	STATS_INCN(state->stats, bytes_written, bytes_written);
#endif
}
