/*
 * Copyright (c) 2025 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup regulator_parent_max20362
 * @brief Public API for the MAX20362 PMIC regulator driver.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_REGULATOR_MAX20362_H_
#define ZEPHYR_INCLUDE_DRIVERS_REGULATOR_MAX20362_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

/**
 * @defgroup regulator_parent_max20362 MAX20362 API
 * @ingroup regulator_parent_interface
 * @brief Public API for the MAX20362 regulator driver.
 * @{
 */

/**
 * @name MAX20362 INT / INT_MASK register bits (0x14 / 0x18)
 * @{
 */
/** @brief Cap over-voltage lockout */
#define MAX20362_INT_CAPOVLO_MASK  BIT(7)
/** @brief Cap under-voltage lockout */
#define MAX20362_INT_CAPUVLO_MASK  BIT(6)
/** @brief Buck-boost on */
#define MAX20362_INT_BBSTON_MASK   BIT(5)
/** @brief Buck-boost off */
#define MAX20362_INT_BBSTOFF_MASK  BIT(4)
/** @brief Buck-boost input UVLO */
#define MAX20362_INT_BBINUVLO_MASK BIT(3)
/** @brief Undervoltage lockout */
#define MAX20362_INT_UVLO_MASK     BIT(2)
/** @brief Boost fault */
#define MAX20362_INT_BSTFLT_MASK   BIT(1)
/** @brief Thermal fault */
#define MAX20362_INT_THMFLT_MASK   BIT(0)
/** @} */

/**
 * @name MAX20362 LDO_INT / LDO_INT_MASK register bits (0x17 / 0x1B)
 * @{
 */
/** @brief Divider fault */
#define MAX20362_LDOINT_DIV_MASK BIT(3)
/** @brief Short circuit */
#define MAX20362_LDOINT_SHR_MASK BIT(2)
/** @brief Thermal fault */
#define MAX20362_LDOINT_THM_MASK BIT(1)
/** @brief Current limit / clipping fault */
#define MAX20362_LDOINT_CLP_MASK BIT(0)
/** @} */

/**
 * @name MAX20362 INGEN_INT / INGEN_INT_MASK register bits (0x16 / 0x1A)
 * @{
 */
/** @brief Output timeout */
#define MAX20362_INGENINT_OUTTMO_MASK  BIT(4)
/** @brief Droop min */
#define MAX20362_INGENINT_DRPMIN_MASK  BIT(3)
/** @brief Tank timeout */
#define MAX20362_INGENINT_TNKTMO_MASK  BIT(2)
/** @brief SIMO pin */
#define MAX20362_INGENINT_SIMOPIN_MASK BIT(1)
/** @brief Droop max */
#define MAX20362_INGENINT_DRPMAX_MASK  BIT(0)
/** @} */

/**
 * @name MAX20362 interrupt mask convenience values
 * @{
 */
/** @brief Mask all INT bits */
#define MAX20362_INT_MASK_ALL       0xFF
/** @brief Mask all LDO_INT bits (DIV | SHR | THM | CLP) */
#define MAX20362_LDO_INT_MASK_ALL   0x0F
/** @brief Mask all INGEN_INT bits (OUTTMO | DRPMIN | TNKTMO | SIMOPIN | DRPMAX) */
#define MAX20362_INGEN_INT_MASK_ALL 0x1F
/** @} */

/**
 * @brief Sets the main interrupt mask (INT_MASK register) of the MAX20362 device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param mask Interrupt mask value to write (1 = masked/disabled, 0 = enabled).
 * @return 0 on success, negative errno code on failure.
 */
int regulator_max20362_set_int_mask(const struct device *dev, uint8_t mask);

/**
 * @brief Sets the Ingenuity interrupt mask (INGEN_INT_MASK register) of the MAX20362 device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param mask Interrupt mask value to write (1 = masked/disabled, 0 = enabled).
 * @return 0 on success, negative errno code on failure.
 */
int regulator_max20362_set_ingen_int_mask(const struct device *dev, uint8_t mask);

/**
 * @brief Sets the LDO interrupt mask (LDO_INT_MASK register) of the MAX20362 device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param mask Interrupt mask value to write (1 = masked/disabled, 0 = enabled).
 * @return 0 on success, negative errno code on failure.
 */
int regulator_max20362_set_ldo_int_mask(const struct device *dev, uint8_t mask);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_REGULATOR_MAX20362_H_ */
