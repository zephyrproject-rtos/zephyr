/*
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_I2C_PEC_TEST_EMUL_H_
#define ZEPHYR_INCLUDE_DRIVERS_I2C_PEC_TEST_EMUL_H_

#include <zephyr/drivers/emul.h>

#ifdef __cplusplus
extern "C" {
#endif

void i2c_pec_test_emul_set_corrupt(const struct emul *target, bool value);
bool i2c_pec_test_emul_get_corrupt(const struct emul *target);

bool i2c_pec_test_emul_is_idle(const struct emul *target);
uint8_t i2c_pec_test_emul_get_last_pec(const struct emul *target);
void i2c_pec_test_euml_reset(const struct emul *target);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_I2C_PEC_TEST_EMUL_H_ */
