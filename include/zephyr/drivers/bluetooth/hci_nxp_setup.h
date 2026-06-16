/** @file
 *  @brief NXP HCI extended API.
 */

/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_DRIVERS_BLUETOOTH_HCI_NXP_SETUP_H_
#define ZEPHYR_INCLUDE_ZEPHYR_DRIVERS_BLUETOOTH_HCI_NXP_SETUP_H_

#include <zephyr/device.h>

/**
 * @brief Trigger Independent Reset (IR) on NXP Bluetooth controllers.
 *
 * This function initiates an Independent Reset sequence on the NXP Bluetooth
 * controller. Independent Reset allows the Bluetooth controller to be reset
 * and re-initialized without requiring a full system reset. After triggering
 * the IR, the firmware is re-downloaded to the controller.
 *
 * @note This API is only available when @kconfig{CONFIG_HCI_NXP_CONFIG_IR} is enabled.
 *
 * @pre Bluetooth must be disabled before calling this function. Use @ref bt_disable()
 *      to disable the Bluetooth stack prior to triggering IR. Failure to disable
 *      Bluetooth will result in the function returning -EAGAIN.
 *
 * @post On successful completion, the Bluetooth controller is reset and firmware
 *       is re-downloaded. The application must call @ref bt_enable() to re-activate
 *       the Bluetooth interface.
 *
 * Example usage:
 * @code{.c}
 *     int err;
 *
 *     // Step 1: Disable Bluetooth stack
 *     err = bt_disable();
 *     if (err) {
 *         LOG_ERR("Failed to disable Bluetooth: %d", err);
 *         return err;
 *     }
 *
 *     // Step 2: Trigger Independent Reset
 *     err = bt_nxp_trigger_ir(uart_dev);
 *     if (err) {
 *         LOG_ERR("Failed to trigger IR: %d", err);
 *         return err;
 *     }
 *
 *     // Step 3: Re-enable Bluetooth stack
 *     err = bt_enable(NULL);
 *     if (err) {
 *         LOG_ERR("Failed to re-enable Bluetooth: %d", err);
 *         return err;
 *     }
 * @endcode
 *
 * @param dev Pointer to the UART device structure used for HCI communication.
 *
 * @retval 0 on success.
 * @retval -EAGAIN if Bluetooth is not disabled. Disable Bluetooth using
 *         @ref bt_disable() before calling this function.
 * @retval -ENODEV if the UART device is not ready or feature is not enabled.
 * @retval -EIO if firmware download fails after IR trigger.
 * @retval other Negative error code on other failures.
 */
int bt_nxp_trigger_ir(const struct device *dev);

#endif /* ZEPHYR_INCLUDE_ZEPHYR_DRIVERS_BLUETOOTH_HCI_NXP_SETUP_H_ */
