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

uint16_t constrain_lightness(uint16_t light);
uint16_t constrain_temperature(uint16_t temp);
uint16_t level_to_light_ctl_temp(int16_t level);

void set_target(uint8_t type, void *dptr);
int get_current(uint8_t type);
int get_target(uint8_t type);

#endif
