/*
 * Copyright (c) 2026 The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup mfd_interface_m5pm1
 * @brief Header file for the M5Stack M5PM1 MFD driver.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MFD_M5PM1_H_
#define ZEPHYR_INCLUDE_DRIVERS_MFD_M5PM1_H_

#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup mfd_interface_m5pm1 MFD M5PM1 Interface
 * @ingroup mfd_interfaces
 * @brief M5Stack M5PM1 power management companion MCU interface.
 * @since 4.5
 * @version 0.1.0
 *
 * The M5PM1 is a small companion microcontroller used on M5Stack boards to provide power management
 * (battery charging, switchable rails), a small bank of GPIOs and an on-chip ADC. This MFD driver
 * owns the I2C transport and serializes register access; sibling GPIO, ADC and regulator drivers
 * implement their domain-specific functionality on top of these primitives.
 * @{
 */

/**
 * @brief Read a single 8-bit M5PM1 register.
 *
 * @param dev M5PM1 MFD device.
 * @param reg Register address.
 * @param[out] val Pointer that receives the register value.
 *
 * @retval 0 On success.
 * @retval -errno On I2C transfer error (see i2c_reg_read_byte_dt()).
 */
int mfd_m5pm1_read_reg(const struct device *dev, uint8_t reg, uint8_t *val);

/**
 * @brief Write a single 8-bit M5PM1 register.
 *
 * @param dev M5PM1 MFD device.
 * @param reg Register address.
 * @param val Value to write.
 *
 * @retval 0 On Success.
 * @retval -errno I2C transfer error (see i2c_reg_write_byte_dt()).
 */
int mfd_m5pm1_write_reg(const struct device *dev, uint8_t reg, uint8_t val);

/**
 * @brief Read-modify-write selected bits of an M5PM1 register.
 *
 * Performs an atomic (mutex-protected) read-modify-write of @p reg, replacing the bits selected by
 * @p mask with the corresponding bits from @p val.
 *
 * @param dev M5PM1 MFD device.
 * @param reg Register address.
 * @param mask Bitmask of bits to update.
 * @param val New value for the bits selected by @p mask (other bits ignored).
 *
 * @retval 0 Success.
 * @retval -errno I2C transfer error (see i2c_reg_update_byte_dt()).
 */
int mfd_m5pm1_update_reg(const struct device *dev, uint8_t reg, uint8_t mask, uint8_t val);

/**
 * @brief Toggle selected bits of an M5PM1 register.
 *
 * Performs an atomic (mutex-protected) read-modify-write of @p reg, inverting the bits selected by
 * @p mask. Holding the lock across the read and write makes the toggle atomic with respect to the
 * other mfd_m5pm1_*() accessors.
 *
 * @param dev M5PM1 MFD device.
 * @param reg Register address.
 * @param mask Bitmask of bits to invert.
 *
 * @retval 0 Success.
 * @retval -errno I2C transfer error.
 */
int mfd_m5pm1_toggle_reg(const struct device *dev, uint8_t reg, uint8_t mask);

/**
 * @brief Read multiple consecutive M5PM1 registers in a single I2C transaction.
 *
 * Uses a single repeated-start I2C transfer so the device sees the read as atomic, which matters
 * for register pairs latched on the low-byte read (e.g. ADC sample registers).
 *
 * @important The M5PM1 only allows continuous reads of certain register ranges, please refer to the
 * datasheet for more details.
 *
 * @param dev M5PM1 MFD device.
 * @param reg Address of the first register to read.
 * @param[out] buf Buffer that receives @p len consecutive register values.
 * @param len Number of bytes to read.
 *
 * @retval 0 Success.
 * @retval -errno I2C transfer error (see i2c_burst_read_dt()).
 */
int mfd_m5pm1_burst_read(const struct device *dev, uint8_t reg, uint8_t *buf, size_t len);

/**
 * @brief Write multiple consecutive M5PM1 registers in a single I2C transaction.
 *
 * Counterpart to mfd_m5pm1_burst_read().
 *
 * @important The M5PM1 only allows continuous writes of certain register ranges, please refer to
 * the datasheet for more details.
 *
 * @param dev M5PM1 MFD device.
 * @param reg Address of the first register to write.
 * @param buf Buffer holding @p len consecutive register values.
 * @param len Number of bytes to write.
 *
 * @retval 0 Success.
 * @retval -errno I2C transfer error (see i2c_burst_write_dt()).
 */
int mfd_m5pm1_burst_write(const struct device *dev, uint8_t reg, const uint8_t *buf, size_t len);

/**
 * @brief Semantic function of an M5PM1 multiplexed pin.
 *
 * Each pin selects its function through a 2-bit field in GPIO_FUNC0/GPIO_FUNC1. The standard GPIO
 * and IRQ functions exist on every pin, but the "special" function (register value 0b11) means a
 * different peripheral on each pin, so validity is checked per-pin (see mfd_m5pm1_pin_request()):
 *
 *  - IO0: ::M5PM1_PIN_FUNC_NEOPIXEL
 *  - IO1, IO2: ::M5PM1_PIN_FUNC_ADC
 *  - IO3, IO4: ::M5PM1_PIN_FUNC_PWM
 */
enum m5pm1_pin_func {
	M5PM1_PIN_FUNC_GPIO = 0, /**< Standard GPIO (FUNC field 0b00). */
	M5PM1_PIN_FUNC_IRQ,      /**< IRQ input (FUNC field 0b01). */
	M5PM1_PIN_FUNC_ADC,      /**< ADC input (FUNC field 0b11, IO1/IO2 only). */
	M5PM1_PIN_FUNC_PWM,      /**< PWM output (FUNC field 0b11, IO3/IO4 only). */
	M5PM1_PIN_FUNC_NEOPIXEL, /**< NeoPixel output (FUNC field 0b11, IO0 only). */
};

/**
 * @brief Claim a pin for a given function on behalf of a sibling driver.
 *
 * The M5PM1 pins are multiplexed between several peripherals (GPIO, ADC, PWM, NeoPixel) whose
 * sibling drivers all share the GPIO_FUNC0/GPIO_FUNC1 select registers. This routine centralizes
 * that mux: it validates that @p func is available on @p pin, programs the pin's function field and
 * records @p client as the pin's owner. A pin can only be owned by a single client; this guards
 * against a board accidentally wiring the same pin to two incompatible consumers.
 *
 * Requests are idempotent for the current owner: the same @p client may call again (e.g. to change
 * its own function) without error.
 *
 * @param dev M5PM1 MFD device.
 * @param client Device requesting the pin (used as the ownership token).
 * @param pin Pin index (0..M5PM1_GPIO_COUNT-1).
 * @param func Function to configure (see @ref m5pm1_pin_func).
 *
 * @retval 0 Success.
 * @retval -EINVAL @p pin is out of range.
 * @retval -ENOTSUP @p func is not available on @p pin.
 * @retval -EBUSY @p pin is already owned by a different client.
 * @retval -errno I2C transfer error.
 */
int mfd_m5pm1_pin_request(const struct device *dev, const struct device *client, uint8_t pin,
			  enum m5pm1_pin_func func);

/**
 * @brief Release a pin previously claimed with mfd_m5pm1_pin_request().
 *
 * If @p client currently owns @p pin, its function field is reset to standard GPIO and the
 * ownership is cleared. Releasing a pin not owned by @p client is a no-op.
 *
 * @param dev M5PM1 MFD device.
 * @param client Device that owns the pin.
 * @param pin Pin index (0..M5PM1_GPIO_COUNT-1).
 *
 * @retval 0 Success (including the no-op case).
 * @retval -EINVAL @p pin is out of range.
 * @retval -errno I2C transfer error.
 */
int mfd_m5pm1_pin_release(const struct device *dev, const struct device *client, uint8_t pin);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MFD_M5PM1_H_ */
