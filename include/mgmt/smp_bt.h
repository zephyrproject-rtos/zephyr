/*
 * Copyright Runtime.io 2018. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Bluetooth transport for the mcumgr SMP protocol.
 */

#ifndef ZEPHYR_INCLUDE_MGMT_SMP_BT_H_
#define ZEPHYR_INCLUDE_MGMT_SMP_BT_H_

#include <zephyr/types.h>
struct bt_conn;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Registers the SMP Bluetooth service.
 *
 * @return 0 on success; negative error code on failure.
 */
int smp_bt_register(void);

#ifdef __cplusplus
}
#endif

#endif
