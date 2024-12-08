/* Bluetooth: Mesh Generic OnOff, Generic Level, Lighting & Vendor Models
 *
 * Copyright (c) 2018 Vikrant More
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _APP_GPIO_H
#define _APP_GPIO_H

/* GPIO */
extern const struct gpio_dt_spec button_device[];
extern const struct gpio_dt_spec led_device[];

void app_gpio_init(void);

#endif
