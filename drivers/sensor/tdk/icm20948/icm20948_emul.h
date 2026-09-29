/*
 * Copyright (c) 2026 Ali Talha Yurtseven
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICM20948_EMUL_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM20948_EMUL_H_

#include <stddef.h>
#include <stdint.h>

#include <zephyr/drivers/emul.h>

/*
 * Place a value in the register file of the emulated device, so that a test
 * can hand the driver a sample and check what it makes of it. The register
 * carries its bank in the high byte, the same way the driver names it.
 */
void icm20948_emul_set_reg(const struct emul *target, uint16_t reg, const uint8_t *val, size_t len);

/* Reads back what the driver left in the register file */
void icm20948_emul_get_reg(const struct emul *target, uint16_t reg, uint8_t *val, size_t len);

#endif /* ZEPHYR_DRIVERS_SENSOR_ICM20948_EMUL_H_ */
