/* Copyright (c) 2022 Google Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SUBSYS_EMUL_INCLUDE_ZEPHYR_EMUL_BMI160_EMUL_H_
#define SUBSYS_EMUL_INCLUDE_ZEPHYR_EMUL_BMI160_EMUL_H_

#include <zephyr/drivers/emul.h>

int bmi160_emul_get_bias(const struct emul *sensor, uint32_t sensor_type, int8_t *bias_x,
			 int8_t *bias_y, int8_t *bias_z);

int bmi160_emul_set_bias(const struct emul *sensor, uint32_t sensor_type, int8_t bias_x,
			 int8_t bias_y, int8_t bias_z);

int bmi160_emul_set_int_status_reg(const struct emul *sensor, int offset, uint8_t value);

int bmi160_emul_get_int_status_reg(const struct emul *sensor, int offset, uint8_t *value);

int bmi160_emul_set_watermark_reg(const struct emul *sensor, uint8_t watermark_val);

int bmi160_emul_get_watermark_reg(const struct emul *sensor, uint8_t *watermark_val);

#endif /* SUBSYS_EMUL_INCLUDE_ZEPHYR_EMUL_BMI160_EMUL_H_ */
