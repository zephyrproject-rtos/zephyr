/*
 * Copyright (c) 2026 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef TEST_DRIVERS_I3C_I3C_EMUL_I2C_TEST_TARGET_EMUL_H_
#define TEST_DRIVERS_I3C_I3C_EMUL_I2C_TEST_TARGET_EMUL_H_

#include <stdint.h>

#include <zephyr/drivers/emul.h>

uint8_t i2c_test_target_get_reg(const struct emul *target, uint8_t idx);

#endif /* TEST_DRIVERS_I3C_I3C_EMUL_I2C_TEST_TARGET_EMUL_H_ */
