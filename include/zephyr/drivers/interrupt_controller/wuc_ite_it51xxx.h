/*
 * Copyright (c) 2025 ITE Corporation. All Rights Reserved
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_IT51XXX_WUC_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_IT51XXX_WUC_H_

#include <zephyr/device.h>

/**
 * @name wakeup controller flags
 * @{
 */
/** WUC rising edge trigger mode */
#define WUC_TYPE_EDGE_RISING  BIT(0)
/** WUC falling edge trigger mode */
#define WUC_TYPE_EDGE_FALLING BIT(1)
/** WUC both edge trigger mode */
#define WUC_TYPE_EDGE_BOTH    (WUC_TYPE_EDGE_RISING | WUC_TYPE_EDGE_FALLING)

#define WUC_TYPE_LEVEL_TRIG BIT(2)
/** WUC level high trigger mode */
#define WUC_TYPE_LEVEL_HIGH BIT(3)
/** WUC level low trigger mode */
#define WUC_TYPE_LEVEL_LOW  BIT(4)

/** @} */

/**
 * @brief A trigger condition on the corresponding input generates
 *        a wake-up signal to the power management control of EC
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param mask Pin mask of WUC group
 */
void it51xxx_wuc_enable(const struct device *dev, uint8_t mask);

/**
 * @brief A trigger condition on the corresponding input doesn't
 *        assert the wake-up signal (canceled not pending)
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param mask Pin mask of WUC group
 */
void it51xxx_wuc_disable(const struct device *dev, uint8_t mask);

/**
 * @brief Write-1-clear a trigger condition that occurs on the
 *        corresponding input
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param mask Pin mask of WUC group
 */
void it51xxx_wuc_clear_status(const struct device *dev, uint8_t mask);

/**
 * @brief Select the trigger edge mode on the corresponding input
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param mask Pin mask of WUC group
 * @param flags Select the trigger edge mode
 */
void it51xxx_wuc_set_polarity(const struct device *dev, uint8_t mask, uint32_t flags);

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_IT51XXX_WUC_H_ */
