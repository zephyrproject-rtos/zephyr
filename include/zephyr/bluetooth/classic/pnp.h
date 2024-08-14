/** @file
 *  @brief PNP infomation Protocol handling.
 */

/*
 * Copyright 2024 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_PNP_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_PNP_H_

#include <sys/types.h>

#include <zephyr/bluetooth/conn.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t (*bt_pnp_discover_func_t)(struct bt_conn *conn, uint16_t type, uint16_t value);

int bt_pnp_discover(struct bt_conn *conn, bt_pnp_discover_func_t cb);

#ifdef __cplusplus
}
#endif
/**
 * @}
 */
#endif /* ZEPHYR_INCLUDE_BLUETOOTH_PNP_H_ */
