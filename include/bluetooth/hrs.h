/** @file
 *  @brief HRS Service sample
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BT_HRS_H
#define __BT_HRS_H

#ifdef __cplusplus
extern "C" {
#endif

/** @typedef bt_hrs_subscribe_func_t
 *  @brief Subscribe callback function
 *
 *  @param active Indicate if there is a subcription active.
 */
typedef void (*bt_hrs_subscribe_func_t)(bool active);

/** @brief Register HRS Service
 *
 *  This registers HRS attributes into GATT database.
 *
 *  @param blsc Body sensor location
 *  @param func Callback to be called when subscription changes
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hrs_register(u8_t blsc, bt_hrs_subscribe_func_t func);

/** @brief Set Heart Rate Measurement
 *
 *  Set current heartrate measurement, if there are connected clients subscribed
 *  for notifications they will be notified about the change.
 *
 *  @param type Sensor contact type
 *  @param heartrate Heart rate measurement
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hrs_set_rate(u8_t type, u8_t heartrate);

/** @brief Simulate Heart Rate change
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hrs_simulate(void);

#ifdef __cplusplus
}
#endif

#endif /* __BT_HRS_H */
