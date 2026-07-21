/* Bluetooth: Mesh Generic OnOff, Generic Level, Lighting & Vendor Models
 *
 * Copyright (c) 2018 Vikrant More
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _PUBLISHER_H
#define _PUBLISHER_H

#include <zephyr/kernel.h>

/* Others */
#define LEVEL_S0   -32768
#define LEVEL_S25  -16384
#define LEVEL_S50  0
#define LEVEL_S75  16384
#define LEVEL_S100 32767

#define LEVEL_U0   0
#define LEVEL_U25  16384
#define LEVEL_U50  32768
#define LEVEL_U75  49152
#define LEVEL_U100 65535

void publish(struct k_work *work);

#endif
