/** @file
 *  @brief BlueNRG HCI extended API.
 */

/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_BLUETOOTH_HCI_DRIVER_BLUENRG_H_
#define ZEPHYR_INCLUDE_DRIVERS_BLUETOOTH_HCI_DRIVER_BLUENRG_H_

/**
 * @brief BlueNRG HCI Driver-Specific API
 * @defgroup bluenrg_hci_driver BlueNRG HCI driver extended API
 * @ingroup bluetooth
 * @{
 */

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Hardware reset the BlueNRG network coprocessor.
 *
 * Performs hardware reset of the BLE network coprocessor.
 * It can also force to enter firmware updater mode.
 *
 * @param updater_mode flag to indicate whether updater mode needs to be entered.
 *
 * @return a non-negative value indicating success, or a
 * negative error code for failure
 */

int bluenrg_bt_reset(bool updater_mode);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_BLUETOOTH_HCI_DRIVER_BLUENRG_H_ */
