/* Bluetooth: Mesh Generic OnOff, Generic Level, Lighting & Vendor Models
 *
 * Copyright (c) 2018 Vikrant More
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _STATE_BINDING_H
#define _STATE_BINDING_H

enum state_binding {
	ONPOWERUP = 0x01,
	ONOFF,
	LEVEL,
	DELTA_LEVEL,
	MOVE_LEVEL,
	ACTUAL,
	LINEAR,
	CTL,

	LEVEL_TEMP,
	MOVE_LEVEL_TEMP,
	CTL_TEMP,

	CTL_DELTA_UV
};

u16_t constrain_lightness(u16_t light);
u16_t level_to_light_ctl_temp(s16_t level);

void set_target(u8_t type, void *dptr);
int get_current(u8_t type);
int get_target(u8_t type);

#endif
