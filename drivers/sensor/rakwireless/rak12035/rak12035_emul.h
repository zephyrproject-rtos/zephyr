/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_RAKWIRELESS_RAK12035_EMUL_H_
#define ZEPHYR_DRIVERS_SENSOR_RAKWIRELESS_RAK12035_EMUL_H_

#include <stdint.h>

#include <zephyr/drivers/emul.h>

void rak12035_emul_reset(const struct emul *target);
void rak12035_emul_set_version(const struct emul *target, uint8_t version);
void rak12035_emul_set_temperature(const struct emul *target, int16_t temperature);
void rak12035_emul_set_capacitance(const struct emul *target, uint16_t capacitance);
void rak12035_emul_set_moisture(const struct emul *target, uint8_t moisture);
void rak12035_emul_set_calibration(const struct emul *target, uint16_t dry, uint16_t wet);
void rak12035_emul_fail_next_transfer(const struct emul *target, int error);

#endif /* ZEPHYR_DRIVERS_SENSOR_RAKWIRELESS_RAK12035_EMUL_H_ */
