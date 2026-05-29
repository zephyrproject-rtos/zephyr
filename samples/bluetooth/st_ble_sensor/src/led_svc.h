/** @file
 *  @brief LED Service sample
 */

/*
 * Copyright (c) 2019 Marcio Montenegro <mtuxpe@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ST_BLE_SENSOR_LED_SVC_H_
#define ST_BLE_SENSOR_LED_SVC_H_

#ifdef __cplusplus
extern "C" {
#endif

void led_update(void);
int led_init(void);

#ifdef __cplusplus
}
#endif

#endif
