/*
 * Copyright (c) 2022 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MODULES_H
#define _MODULES_H

int init_led_device(void);
void init_timer_object(void);
void init_temp_sensor(void);
void init_firmware_update(void);

#if defined(CONFIG_LWM2M_IPSO_HUMIDITY_SENSOR)
void init_humidity_sensor(void);
#endif

#endif
