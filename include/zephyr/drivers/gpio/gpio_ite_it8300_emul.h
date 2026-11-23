/*
 * Copyright (c) 2026 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for the ITE IT8300 GPIO emulator.
 * @ingroup it8300_emul
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_ITE_IT8300_EMUL_H_
#define ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_ITE_IT8300_EMUL_H_

#include <stdint.h>

#include <zephyr/drivers/emul.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IT8300 emulator
 * @defgroup it8300_emul IT8300 emulator
 * @ingroup io_emulators
 * @{
 */

/** Reset all emulated IT8300 registers. */
void it8300_emul_reset(const struct emul *target);

/**
 * Set an emulated IT8300 register.
 *
 * This is primarily used to inject values into read-only hardware state, such
 * as a GPIO pin data mirror register.
 *
 * @retval 0 on success
 * @retval -EINVAL if @p reg is outside the emulated register map
 */
int it8300_emul_set_reg(const struct emul *target, uint8_t reg, uint8_t value);

/**
 * Read an emulated IT8300 register.
 *
 * @retval 0 on success
 * @retval -EINVAL if @p reg is outside the emulated register map
 */
int it8300_emul_get_reg(const struct emul *target, uint8_t reg, uint8_t *value);

/**
 * Set the sampled input value for one GPIO group.
 *
 * Changed bits are latched in the group's interrupt status register. If alert
 * generation is enabled, the emulated ALERT# GPIO is asserted.
 *
 * @param target Emulator instance.
 * @param group GPIO group index: 0 for A through 4 for E.
 * @param value New sampled input value.
 *
 * @retval 0 on success
 * @retval -EINVAL if @p group is invalid
 */
int it8300_emul_set_input(const struct emul *target, uint8_t group, uint8_t value);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_ITE_IT8300_EMUL_H_ */
