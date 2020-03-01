/* Bluetooth: Mesh Generic OnOff, Generic Level, Lighting & Vendor Models
 *
 * Copyright (c) 2018 Vikrant More
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _STATE_BINDING_H
#define _STATE_BINDING_H

enum state_binding {
	ONOFF,
	LEVEL_LIGHT,
	DELTA_LEVEL_LIGHT,
	MOVE_LIGHT,
	ACTUAL,
	LINEAR,
	CTL_LIGHT,

	LEVEL_TEMP,
	MOVE_TEMP,
	CTL_TEMP,

	CTL_DELTA_UV
};

u16_t constrain_lightness(u16_t light);
u16_t constrain_temperature(u16_t temp);
u16_t level_to_light_ctl_temp(s16_t level);

void set_target(u8_t type, void *dptr);
int get_current(u8_t type);
int get_target(u8_t type);

#endif
