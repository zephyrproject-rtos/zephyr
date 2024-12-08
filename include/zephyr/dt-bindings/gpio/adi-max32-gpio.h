/*
 * Copyright (c) 2023-2024 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_ADI_MAX32_GPIO_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_ADI_MAX32_GPIO_H_

/**
 * @brief MAX32-specific GPIO Flags
 * @defgroup gpio_interface_max32 MAX32-specific GPIO Flags
 * @ingroup gpio_interface
 * @{
 */

/**
 * @name MAX32 GPIO drive flags
 * @brief MAX32 GPIO drive flags
 *
 * The drive flags are encoded in the 8 upper bits of @ref gpio_dt_flags_t as
 * follows:
 *
 * - Bit 8: GPIO Supply Voltage Select
 *          Selects the voltage rail used for the pin.
 *          0: VDDIO
 *          1: VDDIOH
 *
 * - Bit 9: GPIO Drive Strength Select,
 *          MAX32_GPIO_DRV_STRENGTH_0 = 1mA
 *          MAX32_GPIO_DRV_STRENGTH_1 = 2mA
 *          MAX32_GPIO_DRV_STRENGTH_2 = 4mA
 *          MAX32_GPIO_DRV_STRENGTH_3 = 8mA
 *
 * - Bit 10: Weak pull up selection, Weak Pullup to VDDIO (1MOhm)
 *          0: Disable
 *          1: Enable
 *
 * - Bit 11: Weak pull down selection, Weak Pulldown to VDDIOH (1MOhm)
 *          0: Disable
 *          1: Enable
 * @{
 */

/** GPIO Voltage Select */
#define MAX32_GPIO_VSEL_POS    (8U)
#define MAX32_GPIO_VSEL_MASK   (0x01U << MAX32_GPIO_VSEL_POS)
#define MAX32_GPIO_VSEL_VDDIO  (0U << MAX32_GPIO_VSEL_POS)
#define MAX32_GPIO_VSEL_VDDIOH (1U << MAX32_GPIO_VSEL_POS)

/** GPIO Drive Strength Select */
#define MAX32_GPIO_DRV_STRENGTH_POS  (9U)
#define MAX32_GPIO_DRV_STRENGTH_MASK (0x03U << MAX32_GPIO_DRV_STRENGTH_POS)
#define MAX32_GPIO_DRV_STRENGTH_0    (0U << MAX32_GPIO_DRV_STRENGTH_POS)
#define MAX32_GPIO_DRV_STRENGTH_1    (1U << MAX32_GPIO_DRV_STRENGTH_POS)
#define MAX32_GPIO_DRV_STRENGTH_2    (2U << MAX32_GPIO_DRV_STRENGTH_POS)
#define MAX32_GPIO_DRV_STRENGTH_3    (3U << MAX32_GPIO_DRV_STRENGTH_POS)

/** GPIO bias weak pull up selection, to VDDIO (1MOhm) */
#define MAX32_GPIO_WEAK_PULL_UP_POS   (10U)
#define MAX32_GPIO_WEAK_PULL_UP       (1U << MAX32_GPIO_WEAK_PULL_UP_POS)
/** GPIO bias weak pull down selection, to VDDIOH (1MOhm) */
#define MAX32_GPIO_WEAK_PULL_DOWN_POS (11U)
#define MAX32_GPIO_WEAK_PULL_DOWN     (1U << MAX32_GPIO_WEAK_PULL_DOWN_POS)

/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_ADI_MAX32_GPIO_H_ */
