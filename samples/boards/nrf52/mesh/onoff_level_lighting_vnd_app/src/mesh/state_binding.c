/* Bluetooth: Mesh Generic OnOff, Generic Level, Lighting & Vendor Models
 *
 * Copyright (c) 2018 Vikrant More
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <board.h>
#include <gpio.h>

#include "common.h"
#include "ble_mesh.h"
#include "device_composition.h"

#define MINDIFF 2.25e-308

static float sqrt(float square)
{
	float root, last, diff;

	root = square / 3.0;
	diff = 1;

	if (square <= 0) {
		return 0;
	}

	do {
		last = root;
		root = (root + square / root) / 2.0;
		diff = root - last;
	} while (diff > MINDIFF || diff < -MINDIFF);

	return root;
}

static s32_t ceiling(float num)
{
	s32_t inum;

	inum = (s32_t) num;
	if (num == (float) inum) {
		return inum;
	}

	return inum + 1;
}

static bool constrain_light_actual_state(u16_t var)
{
	bool is_value_within_range;

	is_value_within_range = false;

	if (var > 0 &&
	    var < light_lightness_srv_user_data.light_range_min) {
		var = light_lightness_srv_user_data.light_range_min;
	} else if (var > light_lightness_srv_user_data.light_range_max) {
		var = light_lightness_srv_user_data.light_range_max;
	} else {
		is_value_within_range = true;
	}

	light_lightness_srv_user_data.actual = var;

	return is_value_within_range;
}

static void update_gen_onoff_state(void)
{
	if (light_lightness_srv_user_data.actual == 0) {
		gen_onoff_srv_root_user_data.onoff = STATE_OFF;
	} else {
		gen_onoff_srv_root_user_data.onoff = STATE_ON;
	}
}

void state_binding(u8_t lightness, u8_t temperature)
{
	bool is_value_within_range;
	u16_t tmp16;
	float tmp;

	switch (temperature) {
	case ONOFF_TEMP:/* Temp. update as per Light CTL temp. default state */
	case CTL_TEMP:	/* Temp. update as per Light CTL temp. state */
		/* Mesh Model Specification 6.1.3.1.1 2nd formula start */
		tmp = (light_ctl_srv_user_data.temp -
		       light_ctl_srv_user_data.temp_range_min) * 65535;
		tmp = tmp / (light_ctl_srv_user_data.temp_range_max -
			     light_ctl_srv_user_data.temp_range_min);
		gen_level_srv_s0_user_data.level = tmp - 32768;
		/* 6.1.3.1.1 2nd formula end */
		break;
	case LEVEL_TEMP:/* Temp. update as per Generic Level (s0) state */
		/* Mesh Model Specification 6.1.3.1.1 1st formula start */
		tmp = (float) (light_ctl_srv_user_data.temp_range_max -
			       light_ctl_srv_user_data.temp_range_min) / 65535;
		tmp = (gen_level_srv_s0_user_data.level + 32768) * tmp;
		light_ctl_srv_user_data.temp =
			light_ctl_srv_user_data.temp_range_min + tmp;
		/* 6.1.3.1.1 1st formula end */
		break;
	default:
		break;
	}

	tmp16 = 0;

	switch (lightness) {
	case ONPOWERUP: /* Lightness update as per Generic OnPowerUp state */
		if (gen_onoff_srv_root_user_data.onoff == STATE_OFF) {
			light_lightness_srv_user_data.actual = 0;
			light_lightness_srv_user_data.linear = 0;
			gen_level_srv_root_user_data.level = -32768;
			light_ctl_srv_user_data.lightness = 0;
			return;
		} else if (gen_onoff_srv_root_user_data.onoff == STATE_ON) {
			light_lightness_srv_user_data.actual =
				light_lightness_srv_user_data.last;
		}

		break;
	case ONOFF: /* Lightness update as per Generic OnOff (root) state */
		if (gen_onoff_srv_root_user_data.onoff == STATE_OFF) {
			light_lightness_srv_user_data.actual = 0;
			light_lightness_srv_user_data.linear = 0;
			gen_level_srv_root_user_data.level = -32768;
			light_ctl_srv_user_data.lightness = 0;
			return;
		} else if (gen_onoff_srv_root_user_data.onoff == STATE_ON) {
			if (light_lightness_srv_user_data.def == 0) {
				light_lightness_srv_user_data.actual =
					light_lightness_srv_user_data.last;
			} else {
				light_lightness_srv_user_data.actual =
					light_lightness_srv_user_data.def;
			}
		}

		break;
	case LEVEL: /* Lightness update as per Generic Level (root) state */
		tmp16 = gen_level_srv_root_user_data.level + 32768;
		constrain_light_actual_state(tmp16);
		update_gen_onoff_state();
		break;
	case DELTA_LEVEL: /* Lightness update as per Gen. Level (root) state */
		/* This is as per Mesh Model Specification 3.3.2.2.3 */
		tmp16 = gen_level_srv_root_user_data.level + 32768;
		if (tmp16 > 0 &&
		    tmp16 < light_lightness_srv_user_data.light_range_min) {
			if (gen_level_srv_root_user_data.last_delta < 0) {
				tmp16 = 0;
			} else {
				tmp16 =
				light_lightness_srv_user_data.light_range_min;
			}
		} else if (tmp16 >
			   light_lightness_srv_user_data.light_range_max) {
			tmp16 =
			light_lightness_srv_user_data.light_range_max;
		}

		light_lightness_srv_user_data.actual = tmp16;

		update_gen_onoff_state();
		break;
	case ACTUAL: /* Lightness update as per Light Lightness Actual state */
		update_gen_onoff_state();
		break;
	case LINEAR: /* Lightness update as per Light Lightness Linear state */
		tmp16 = (u16_t) 65535 *
			sqrt(((float) light_lightness_srv_user_data.linear /
			      65535));

		is_value_within_range = constrain_light_actual_state(tmp16);
		update_gen_onoff_state();

		if (is_value_within_range) {
			goto ignore_linear_update_others;
		}

		break;
	case CTL: /* Lightness update as per Light CTL Lightness state */
		constrain_light_actual_state(light_ctl_srv_user_data.lightness);
		update_gen_onoff_state();
		break;
	default:
		return;
	}

	tmp = ((float) light_lightness_srv_user_data.actual / 65535);
	light_lightness_srv_user_data.linear = ceiling(65535 * tmp * tmp);

ignore_linear_update_others:
	if (light_lightness_srv_user_data.actual != 0) {
		light_lightness_srv_user_data.last =
			light_lightness_srv_user_data.actual;
	}

	gen_level_srv_root_user_data.level =
		light_lightness_srv_user_data.actual - 32768;

	light_ctl_srv_user_data.lightness =
		light_lightness_srv_user_data.actual;
}
