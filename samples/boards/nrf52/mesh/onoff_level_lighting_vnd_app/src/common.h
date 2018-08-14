/* Bluetooth: Mesh Generic OnOff, Generic Level, Lighting & Vendor Models
 *
 * Copyright (c) 2018 Vikrant More
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _COMMON_H
#define _COMMON_H

/* GPIO */
extern struct device *button_device[4];
extern struct device *led_device[4];

void update_light_state(void);
void light_default_status_init(void);

#endif
