/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Holding SDA or SCL low through spare GPIOs wired to the bus.
 *
 * The bus is open drain, so a pin driving a line low holds it low whatever
 * the controller under test drives.  A hold lasts milliseconds and needs no
 * timing precision.
 *
 * The pins are sda-gpios and scl-gpios under the zephyr,user node.  They must
 * be spare GPIOs wired to the controller's own SDA and SCL nets, declared
 * GPIO_ACTIVE_HIGH so that a logical 0 is a physical low.
 */

#ifndef BUS_FORCE_H_
#define BUS_FORCE_H_

#include <stdint.h>

#include <zephyr/sys/util_macro.h>

/** Bus lines that can be held low, combined with a bitwise OR. */
#define BUS_FORCE_SDA BIT(0)
#define BUS_FORCE_SCL BIT(1)

/**
 * Configure both forcing GPIOs as inputs, leaving the lines released.
 *
 * Does nothing unless CONFIG_I2C_FAULT_TEST_BUS_FAULT_WIRED is set.
 *
 * @retval 0        Success, or the option is not set.
 * @retval -ENODEV  A forcing GPIO's controller is not ready.
 * @retval -errno   Error from the GPIO driver.
 */
int bus_force_init(void);

/**
 * Hold @p lines low until bus_force_release() is called.
 *
 * @param lines BUS_FORCE_SDA, BUS_FORCE_SCL, or both.
 *
 * @retval 0        Success.
 * @retval -EINVAL  @p lines is 0 or has a bit set that names no line.
 * @retval -ENODEV  CONFIG_I2C_FAULT_TEST_BUS_FAULT_WIRED is not set.
 * @retval -errno   Error from the GPIO driver.
 */
int bus_force_low(uint32_t lines);

/**
 * Release both lines.
 *
 * @retval 0        Success.
 * @retval -ENODEV  CONFIG_I2C_FAULT_TEST_BUS_FAULT_WIRED is not set.
 * @retval -errno   Error from the GPIO driver.
 */
int bus_force_release(void);

/**
 * Read the level of one or both lines.
 *
 * Each pin read is switched to an input first, so a line this helper holds
 * low is released by the read.
 *
 * @param lines BUS_FORCE_SDA, BUS_FORCE_SCL, or both.
 *
 * @retval >=0      The lines in @p lines that read high.
 * @retval -EINVAL  @p lines is 0 or has a bit set that names no line.
 * @retval -ENODEV  CONFIG_I2C_FAULT_TEST_BUS_FAULT_WIRED is not set.
 * @retval -errno   Error from the GPIO driver.
 */
int bus_force_level(uint32_t lines);

#endif /* BUS_FORCE_H_ */
