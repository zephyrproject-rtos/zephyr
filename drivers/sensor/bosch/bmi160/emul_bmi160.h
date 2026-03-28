/*
 * Copyright 2020 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BMI160_EMUL_BMI160_H_
#define ZEPHYR_DRIVERS_SENSOR_BMI160_EMUL_BMI160_H_

#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Check if I2C messages are touching a given register (R or W)
 *
 * @param[in] msgs The I2C messages in question
 * @param[in] num_msgs The number of messages in the @p msgs array
 * @param[in] reg The register to check for
 * @return True if @p reg is either read or written to
 * @return False otherwise
 */
__maybe_unused static bool emul_bmi160_i2c_is_touching_reg(struct i2c_msg *msgs, int num_msgs,
							   uint32_t reg)
{
	if (num_msgs != 2) {
		return false;
	}
	if (msgs[0].len != 1) {
		return false;
	}
	if (i2c_is_read_op(msgs)) {
		return false;
	}

	uint8_t start_reg = msgs[0].buf[0];
	uint8_t read_len = msgs[1].len;

	return (start_reg <= reg) && (reg < start_reg + read_len);
}

/**
 * @brief Check if I2C messages are reading a specific register.
 *
 * @param[in] msgs The I2C messages in question
 * @param[in] num_msgs The number of messages in the @p msgs array
 * @param[in] reg The register to check for
 * @return True if @p reg is read
 * @return False otherwise
 */
__maybe_unused static bool emul_bmi160_i2c_is_reading_reg(struct i2c_msg *msgs, int num_msgs,
							  uint32_t reg)
{
	if (!emul_bmi160_i2c_is_touching_reg(msgs, num_msgs, reg)) {
		return false;
	}
	return i2c_is_read_op(&msgs[1]);
}

/**
 * @brief Check if I2C messages are writing to a specific register.
 *
 * @param[in] msgs The I2C messages in question
 * @param[in] num_msgs The number of messages in the @p msgs array
 * @param[in] reg The register to check for
 * @return True if @p reg is written
 * @return False otherwise
 */
__maybe_unused static bool emul_bmi160_i2c_is_writing_reg(struct i2c_msg *msgs, int num_msgs,
							  uint32_t reg)
{
	if (!emul_bmi160_i2c_is_touching_reg(msgs, num_msgs, reg)) {
		return false;
	}
	return !i2c_is_read_op(&msgs[1]);
}

/**
 * @brief Get the internal register value of the emulator
 *
 * @param[in]  target The emulator in question
 * @param[in]  reg_number The register number to start reading at
 * @param[out] out Buffer to store the values into
 * @param[in]  count The number of registers to read
 * @return 0 on success
 * @return < 0 on error
 */
int emul_bmi160_get_reg_value(const struct emul *target, int reg_number, uint8_t *out,
			      size_t count);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_BMI160_EMUL_BMI160_H_ */
