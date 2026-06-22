/*
 * Copyright (c) 2022 ITE Corporation. All Rights Reserved
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_IT8XXX2_WUC_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_IT8XXX2_WUC_H_

#include <zephyr/device.h>
#include <stdint.h>

/**
 * @brief A trigger condition on the corresponding input generates
 *        a wake-up signal to the power management control of EC
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param mask Pin mask of WUC group
 */
void it8xxx2_wuc_enable(const struct device *dev, uint8_t mask);

/**
 * @brief A trigger condition on the corresponding input doesn't
 *        assert the wake-up signal (canceled not pending)
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param mask Pin mask of WUC group
 */
void it8xxx2_wuc_disable(const struct device *dev, uint8_t mask);

/**
 * @brief Write-1-clear a trigger condition that occurs on the
 *        corresponding input
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param mask Pin mask of WUC group
 */
void it8xxx2_wuc_clear_status(const struct device *dev, uint8_t mask);

/**
 * @brief Select the trigger edge mode on the corresponding input
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param mask Pin mask of WUC group
 * @param flags Select the trigger edge mode
 */
void it8xxx2_wuc_set_polarity(const struct device *dev, uint8_t mask,
			      uint32_t flags);

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_IT8XXX2_WUC_H_ */
