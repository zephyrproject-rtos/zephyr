/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ST_SECURE_I2C_COMMON_H
#define ST_SECURE_I2C_COMMON_H

#include <stdint.h>

/* Shared between the secure partition and the non-secure client. */
struct i2c_probe_result {
	uint8_t hal_status;
	uint8_t isr_lo;
	uint8_t isr_hi;
};

#endif /* ST_SECURE_I2C_COMMON_H */
