/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BT_MAC_H
#define BT_MAC_H

#include <stdint.h>

/* Public APIs */
__attribute__((noinline)) void telink_bt_blc_mac_init(uint8_t *bt_mac);

#endif /* BT_MAC_H */
