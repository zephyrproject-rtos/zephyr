/** @file
 *  @brief Bluetooth subsystem controller APIs.
 */

/*
 * Copyright (c) 2018 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_CONTROLLER_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_CONTROLLER_H_

/**
 * @brief Bluetooth Controller
 * @defgroup bt_ctrl Bluetooth Controller
 * @ingroup bluetooth
 * @{
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Set public address for controller
 *
 *  Should be called before bt_enable().
 *
 *  @param addr Public address
 */
void bt_ctlr_set_public_addr(const uint8_t *addr);

#ifdef __cplusplus
}
#endif
/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_CONTROLLER_H_ */
