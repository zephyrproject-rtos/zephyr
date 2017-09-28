/** @file
 *  @brief Bluetooth CTS Service
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BT_CTS_H
#define __BT_CTS_H

#ifdef __cplusplus
extern "C" {
#endif

/** @typedef bt_cts_subscribe_func_t
 *  @brief Subscribe callback function
 *
 *  @param active Indicate if there is a subcription active.
 */
typedef void (*bt_cts_subscribe_func_t)(bool active);

/** @brief Register CTS Service
 *
 *  This registers CTS attributes into GATT database.
 *
 *  @param time Current time
 *  @param func Callback to be called when subscription changes
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_cts_register(u8_t time[10], bt_cts_subscribe_func_t func);

/** @brief Unregister CTS Service
 *
 *  This unregisters CTS attributes from GATT database.
 */
void bt_cts_unregister(void);

/** @brief Set Current Time
 *
 *  Set current time, if there are connected clients subscribed for
 *  notifications they will be notified about the change.
 *
 *  @param time Current time
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_cts_set_time(u8_t time[10]);

/** @brief Simulate Current Time change
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_cts_simulate(void);

#ifdef __cplusplus
}
#endif

#endif /* __BT_CTS_H */
