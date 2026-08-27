/*
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TEST_DRIVERS_SENSOR_ACCEL_INCLUDE_CHECKS_H_
#define TEST_DRIVERS_SENSOR_ACCEL_INCLUDE_CHECKS_H_

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/spi.h>

#include "bmi160.h"

static inline bool bmi160_i2c_is_touching_reg(struct i2c_msg *msgs, int num_msgs, int reg)
{
	__ASSERT_NO_MSG(num_msgs == 2);
	__ASSERT_NO_MSG(msgs[0].len == 1);

	uint8_t start_reg = msgs[0].buf[0];

	return (start_reg <= reg) && (reg < start_reg + msgs[1].len);
}

static inline bool bmi160_spi_is_touching_reg(const struct spi_buf_set *tx_bufs,
					      const struct spi_buf_set *rx_bufs, int reg)
{
	__ASSERT_NO_MSG(tx_bufs->count == 2);
	const struct spi_buf *tx = &tx_bufs->buffers[0];
	const struct spi_buf *tx_data = &tx_bufs->buffers[1];
	uint32_t start_reg = ((uint8_t *)(tx->buf))[0] & BMI160_REG_MASK;

	return (start_reg <= reg) && (reg < start_reg + tx_data->len);
}

#endif /* TEST_DRIVERS_SENSOR_ACCEL_INCLUDE_CHECKS_H_ */
