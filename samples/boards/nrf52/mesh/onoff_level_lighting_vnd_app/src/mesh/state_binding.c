/* Bluetooth: Mesh Generic OnOff, Generic Level, Lighting & Vendor Models
 *
 * Copyright (c) 2018 Vikrant More
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/gpio.h>

#include "ble_mesh.h"
#include "device_composition.h"
#include "state_binding.h"
#include "storage.h"
#include "transition.h"

#define MINDIFF 2.25e-308

u16_t lightness, target_lightness;
s16_t temperature, target_temperature;

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

static u16_t actual_to_linear(u16_t val)
{
	float tmp;

	tmp = ((float) val / UINT16_MAX);

	return (u16_t) ceiling(UINT16_MAX * tmp * tmp);
}

static u16_t linear_to_actual(u16_t val)
{
	return (u16_t) (UINT16_MAX * sqrt(((float) val / UINT16_MAX)));
}

static void constrain_lightness(u16_t var)
{
	if (var > 0 && var < light_lightness_srv_user_data.light_range_min) {
		var = light_lightness_srv_user_data.light_range_min;
	} else if (var > light_lightness_srv_user_data.light_range_max) {
		var = light_lightness_srv_user_data.light_range_max;
	}

	lightness = var;
}

static void constrain_lightness2(u16_t var)
{
	/* This is as per Mesh Model Specification 3.3.2.2.3 */
	if (var > 0 && var < light_lightness_srv_user_data.light_range_min) {
		if (gen_level_srv_root_user_data.last_delta < 0) {
			var = 0U;
		} else {
			var = light_lightness_srv_user_data.light_range_min;
		}
	} else if (var > light_lightness_srv_user_data.light_range_max) {
		var = light_lightness_srv_user_data.light_range_max;
	}

	lightness = var;
}

static void constrain_target_lightness(u16_t var)
{
	if (var > 0 &&
	    var < light_lightness_srv_user_data.light_range_min) {
		var = light_lightness_srv_user_data.light_range_min;
	} else if (var > light_lightness_srv_user_data.light_range_max) {
		var = light_lightness_srv_user_data.light_range_max;
	}

	target_lightness = var;
}

static s16_t light_ctl_temp_to_level(u16_t temp)
{
	float tmp;

	/* Mesh Model Specification 6.1.3.1.1 2nd formula start */

	tmp = (temp - light_ctl_srv_user_data.temp_range_min) * UINT16_MAX;

	tmp = tmp / (light_ctl_srv_user_data.temp_range_max -
		     light_ctl_srv_user_data.temp_range_min);

	return (s16_t) (tmp + INT16_MIN);

	/* 6.1.3.1.1 2nd formula end */
}

static u16_t level_to_light_ctl_temp(s16_t level)
{
	u16_t tmp;
	float diff;

	/* Mesh Model Specification 6.1.3.1.1 1st formula start */
	diff = (float) (light_ctl_srv_user_data.temp_range_max -
			light_ctl_srv_user_data.temp_range_min) / UINT16_MAX;


	tmp = (u16_t) ((level - INT16_MIN) * diff);

	return (light_ctl_srv_user_data.temp_range_min + tmp);

	/* 6.1.3.1.1 1st formula end */
}

void readjust_lightness(void)
{
	if (lightness != 0U) {
		light_lightness_srv_user_data.last = lightness;

		save_on_flash(LIGHTNESS_LAST_STATE);
	}

	if (lightness) {
		gen_onoff_srv_root_user_data.onoff = STATE_ON;
	} else {
		gen_onoff_srv_root_user_data.onoff = STATE_OFF;
	}

	gen_level_srv_root_user_data.level = lightness + INT16_MIN;
	light_lightness_srv_user_data.actual = lightness;
	light_lightness_srv_user_data.linear = actual_to_linear(lightness);
	light_ctl_srv_user_data.lightness = lightness;
}

void readjust_temperature(void)
{
	gen_level_srv_s0_user_data.level = temperature;
	light_ctl_srv_user_data.temp = level_to_light_ctl_temp(temperature);
}

void state_binding(u8_t light, u8_t temp)
{
	switch (temp) {
	case ONOFF_TEMP:
	case CTL_TEMP:
		temperature =
			light_ctl_temp_to_level(light_ctl_srv_user_data.temp);

		gen_level_srv_s0_user_data.level = temperature;
		break;
	case LEVEL_TEMP:
		temperature = gen_level_srv_s0_user_data.level;
		light_ctl_srv_user_data.temp =
			level_to_light_ctl_temp(temperature);
		break;
	default:
		break;
	}

	switch (light) {
	case ONPOWERUP:
		lightness = light_ctl_srv_user_data.lightness;
		break;
	case ONOFF:
		if (gen_onoff_srv_root_user_data.onoff == STATE_OFF) {
			lightness = 0U;
		} else if (gen_onoff_srv_root_user_data.onoff == STATE_ON) {
			if (light_lightness_srv_user_data.def == 0U) {
				lightness = light_lightness_srv_user_data.last;
			} else {
				lightness = light_lightness_srv_user_data.def;
			}
		}
		break;
	case LEVEL:
		lightness = gen_level_srv_root_user_data.level - INT16_MIN;
		break;
	case DELTA_LEVEL:
		lightness = gen_level_srv_root_user_data.level - INT16_MIN;
		constrain_lightness2(lightness);
		goto jump;
	case ACTUAL:
		lightness = light_lightness_srv_user_data.actual;
		break;
	case LINEAR:
		lightness =
			linear_to_actual(light_lightness_srv_user_data.linear);
		break;
	case CTL:
		lightness = light_ctl_srv_user_data.lightness;
		break;
	default:
		break;
	}

	constrain_lightness(lightness);

jump:
	readjust_lightness();
}

void init_lightness_target_values(void)
{
	target_lightness = lightness;

	if (target_lightness) {
		gen_onoff_srv_root_user_data.target_onoff = STATE_ON;
	} else {
		gen_onoff_srv_root_user_data.target_onoff = STATE_OFF;
	}

	gen_level_srv_root_user_data.target_level = target_lightness + INT16_MIN;

	light_lightness_srv_user_data.target_actual = target_lightness;

	light_lightness_srv_user_data.target_linear =
		actual_to_linear(target_lightness);

	light_ctl_srv_user_data.target_lightness = target_lightness;
}

void calculate_lightness_target_values(u8_t type)
{
	u16_t tmp;

	switch (type) {
	case ONOFF:
		if (gen_onoff_srv_root_user_data.target_onoff == 0U) {
			tmp = 0U;
		} else {
			if (light_lightness_srv_user_data.def == 0U) {
				tmp = light_lightness_srv_user_data.last;
			} else {
				tmp = light_lightness_srv_user_data.def;
			}
		}
		break;
	case LEVEL:
		tmp = gen_level_srv_root_user_data.target_level - INT16_MIN;
		break;
	case ACTUAL:
		tmp = light_lightness_srv_user_data.target_actual;
		break;
	case LINEAR:
		tmp = linear_to_actual(light_lightness_srv_user_data.target_linear);
		break;
	case CTL:
		tmp = light_ctl_srv_user_data.target_lightness;

		target_temperature = light_ctl_temp_to_level(light_ctl_srv_user_data.target_temp);
		gen_level_srv_s0_user_data.target_level = target_temperature;
		break;
	default:
		return;
	}

	constrain_target_lightness(tmp);

	if (target_lightness) {
		gen_onoff_srv_root_user_data.target_onoff = STATE_ON;
	} else {
		gen_onoff_srv_root_user_data.target_onoff = STATE_OFF;
	}

	gen_level_srv_root_user_data.target_level = target_lightness + INT16_MIN;

	light_lightness_srv_user_data.target_actual = target_lightness;

	light_lightness_srv_user_data.target_linear =
		actual_to_linear(target_lightness);

	light_ctl_srv_user_data.target_lightness = target_lightness;

	if (gen_power_onoff_srv_user_data.onpowerup == STATE_RESTORE) {
		save_on_flash(LIGHTNESS_TEMP_LAST_TARGET_STATE);
	}
}

void init_temp_target_values(void)
{
	target_temperature = temperature;
	gen_level_srv_s0_user_data.target_level = target_temperature;
	light_ctl_srv_user_data.target_temp =
			level_to_light_ctl_temp(target_temperature);
}

void calculate_temp_target_values(u8_t type)
{
	bool set_light_ctl_delta_uv_target_value;

	set_light_ctl_delta_uv_target_value = true;

	switch (type) {
	case LEVEL_TEMP:
		target_temperature = gen_level_srv_s0_user_data.target_level;
		light_ctl_srv_user_data.target_temp =
			level_to_light_ctl_temp(target_temperature);
		break;
	case CTL_TEMP:
		set_light_ctl_delta_uv_target_value = false;

		target_temperature = light_ctl_temp_to_level(light_ctl_srv_user_data.target_temp);
		gen_level_srv_s0_user_data.target_level = target_temperature;
		break;
	default:
		return;
	}

	if (set_light_ctl_delta_uv_target_value) {

		light_ctl_srv_user_data.target_delta_uv =
			light_ctl_srv_user_data.delta_uv;
	}

	if (gen_power_onoff_srv_user_data.onpowerup == STATE_RESTORE) {
		save_on_flash(LIGHTNESS_TEMP_LAST_TARGET_STATE);
	}
}

