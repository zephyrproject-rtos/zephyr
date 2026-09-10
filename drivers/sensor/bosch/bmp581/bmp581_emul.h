/*
 * Copyright (c) 2026 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BMP581_BMP581_EMUL_H_
#define ZEPHYR_DRIVERS_SENSOR_BMP581_BMP581_EMUL_H_

#include <zephyr/drivers/emul.h>
#include <stdint.h>

/**
 * @brief Set the raw temperature sample registers.
 *
 * @param target Pointer to the emulator instance.
 * @param raw_temp Raw 24-bit temperature value (LSB = 1/65536 degC).
 */
void bmp581_emul_set_temp_raw(const struct emul *target, int32_t raw_temp);

/**
 * @brief Set the raw pressure sample registers.
 *
 * @param target Pointer to the emulator instance.
 * @param raw_press Raw 24-bit pressure value (LSB = 1/64 Pa).
 */
void bmp581_emul_set_press_raw(const struct emul *target, uint32_t raw_press);

#endif /* ZEPHYR_DRIVERS_SENSOR_BMP581_BMP581_EMUL_H_ */
