/*
 * Copyright (c) 2024 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BMA4XX_BMA4XX_EMUL_H_
#define ZEPHYR_DRIVERS_SENSOR_BMA4XX_BMA4XX_EMUL_H_

#include <zephyr/drivers/emul.h>
#include "zephyr/dsp/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Set the sensor's current acceleration reading. */
void bma4xx_emul_set_accel_data(const struct emul *target, q31_t value, int8_t shift, int8_t reg);

/**
 * Return the current interrupt configuration.
 *
 * Provided pointers are out-parameters for the INT1_IO_CTRL register and
 * whether interrupts are in latched mode. The return value is the current value
 * of the INT_MAP_DATA register.
 */
uint8_t bma4xx_emul_get_interrupt_config(const struct emul *emul, uint8_t *int1_io_ctrl,
					 bool *latched_mode);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_BMA4XX_BMA4XX_EMUL_H_ */
