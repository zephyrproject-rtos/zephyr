/** @file
 *  @brief Bluetooth BAS Service
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BT_BAS_H
#define __BT_BAS_H

#ifdef __cplusplus
extern "C" {
#endif

/** @typedef bt_bas_subscribe_func_t
 *  @brief Subscribe callback function
 *
 *  @param active Indicate if there is a subcription active.
 */
typedef void (*bt_bas_subscribe_func_t)(bool active);

/** @brief Register BAS Service
 *
 *  This registers BAS attributes into GATT database.
 *
 *  @param level Initial battery level in percentage
 *  @param func Callback to be called when subscription changes
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_bas_register(u8_t level, bt_bas_subscribe_func_t func);

/** @brief Unregister BAS Service
 *
 *  This unregisters BAS attributes from GATT database.
 */
void bt_bas_unregister(void);

/** @brief Set Battery Level
 *
 *  Set current battery level, if there are connected clients subscribed for
 *  notifications they will be notified about the change.
 *
 *  @param level Battery level in percentage.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_bas_set_level(u8_t level);

/** @brief Simulate Battery Level changes
 *
 *  Simulate battery level changes.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_bas_simulate(void);

#ifdef __cplusplus
}
#endif

#endif /* __BT_BAS_H */
