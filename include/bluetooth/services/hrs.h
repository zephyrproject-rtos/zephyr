/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_HRS_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_HRS_H_

/**
 * @brief Heart Rate Service (HRS)
 * @defgroup bt_gatt_hrs Heart Rate Service (HRS)
 * @ingroup bluetooth
 * @{
 *
 * [Experimental] Users should note that the APIs can change
 * as a part of ongoing development.
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Notify heart rate measurement.
 *
 * This will send a GATT notification to all current subscribers.
 *
 *  @param heartrate The heartrate measurement in beats per minute.
 *
 *  @return Zero in case of success and error code in case of error.
 */
int bt_gatt_hrs_notify(u16_t heartrate);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_HRS_H_ */
