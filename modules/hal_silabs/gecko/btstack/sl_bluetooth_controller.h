/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Bluetooth controller API
 */

#ifndef SL_BLUETOOTH_CONTROLLER_H_
#define SL_BLUETOOTH_CONTROLLER_H_

#include <sl_status.h>

sl_status_t sl_btctrl_init_zephyr(void);
void sl_btctrl_deinit(void);

#endif /* SL_BLUETOOTH_CONTROLLER_H_ */
