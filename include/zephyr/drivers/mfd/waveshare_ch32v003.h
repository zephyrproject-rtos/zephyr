/*
 * Copyright (c) 2026 Kim Bøndergaard <kim@fam-boendergaard.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MFD_WAVESHARE_CH32V003_H_
#define ZEPHYR_INCLUDE_DRIVERS_MFD_WAVESHARE_CH32V003_H_

#include <stdint.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

struct k_mutex;

/**
 * @brief CH32V003 register map.
 */
enum ch32v003_reg {
	CH32V003_REG_DIRECTION = 0x02,
	CH32V003_REG_OUTPUT = 0x03,
	CH32V003_REG_INPUT = 0x04,
	CH32V003_REG_PWM = 0x05,
	CH32V003_REG_ADC = 0x06,
};

/**
 * @brief Get the CH32V003 register access lock.
 *
 * @param dev Pointer to the CH32V003 MFD device.
 *
 * @return Pointer to the register access mutex.
 */
struct k_mutex *ch32v003_get_lock(const struct device *dev);

/**
 * @brief Read an 8-bit register from the CH32V003 expander.
 *
 * @param dev Pointer to the CH32V003 MFD device.
 * @param reg Register to read.
 * @param value Pointer to store the register value.
 *
 * @retval 0 on success.
 * @retval -EIO on I2C failure.
 */
int ch32v003_read_reg8(const struct device *dev, enum ch32v003_reg reg, uint8_t *value);

/**
 * @brief Write an 8-bit register on the CH32V003 expander.
 *
 * @param dev Pointer to the CH32V003 MFD device.
 * @param reg Register to write.
 * @param value Value to write.
 *
 * @retval 0 on success.
 * @retval -EIO on I2C failure.
 */
int ch32v003_write_reg8(const struct device *dev, enum ch32v003_reg reg, uint8_t value);

/**
 * @brief Read a 16-bit register from the CH32V003 expander.
 *
 * @param dev Pointer to the CH32V003 MFD device.
 * @param reg Register to read.
 * @param value Pointer to store the register value in host format.
 *
 * @retval 0 on success.
 * @retval -EIO on I2C failure.
 */
int ch32v003_read_reg16(const struct device *dev, enum ch32v003_reg reg, uint16_t *value);


#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MFD_WAVESHARE_CH32V003_H_ */
