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

u16_t constrain_lightness(u16_t light)
{
	if (light > 0 && light < ctl->light->range_min) {
		light = ctl->light->range_min;
	} else if (light > ctl->light->range_max) {
		light = ctl->light->range_max;
	}

	return light;
}

static void constrain_target_lightness2(void)
{
	/* This is as per Mesh Model Specification 3.3.2.2.3 */
	if (ctl->light->target > 0 &&
	    ctl->light->target < ctl->light->range_min) {
		if (ctl->light->delta < 0) {
			ctl->light->target = 0U;
		} else {
			ctl->light->target = ctl->light->range_min;
		}
	} else if (ctl->light->target > ctl->light->range_max) {
		ctl->light->target = ctl->light->range_max;
	}
}

u16_t constrain_temperature(u16_t temp)
{
	if (temp < ctl->temp->range_min) {
		temp = ctl->temp->range_min;
	} else if (temp > ctl->temp->range_max) {
		temp = ctl->temp->range_max;
	}

	return temp;
}

static s16_t light_ctl_temp_to_level(u16_t temp)
{
	float tmp;

	/* Mesh Model Specification 6.1.3.1.1 2nd formula start */

	tmp = (temp - ctl->temp->range_min) * UINT16_MAX;

	tmp = tmp / (ctl->temp->range_max - ctl->temp->range_min);

	return (s16_t) (tmp + INT16_MIN);

	/* 6.1.3.1.1 2nd formula end */
}

u16_t level_to_light_ctl_temp(s16_t level)
{
	u16_t tmp;
	float diff;

	/* Mesh Model Specification 6.1.3.1.1 1st formula start */
	diff = (float) (ctl->temp->range_max - ctl->temp->range_min) /
		       UINT16_MAX;

	tmp = (u16_t) ((level - INT16_MIN) * diff);

	return (ctl->temp->range_min + tmp);

	/* 6.1.3.1.1 1st formula end */
}

void set_target(u8_t type, void *dptr)
{
	switch (type) {
	case ONOFF: {
		u8_t onoff;

		onoff = *((u8_t *) dptr);
		if (onoff == STATE_OFF) {
			ctl->light->target = 0U;
		} else if (onoff == STATE_ON) {
			if (ctl->light->def == 0U) {
				ctl->light->target = ctl->light->last;
			} else {
				ctl->light->target = ctl->light->def;
			}
		}
	}
	break;
	case LEVEL_LIGHT:
		ctl->light->target = *((s16_t *) dptr) - INT16_MIN;
		ctl->light->target = constrain_lightness(ctl->light->target);
		break;
	case DELTA_LEVEL_LIGHT:
		ctl->light->target =  *((s16_t *) dptr) - INT16_MIN;
		constrain_target_lightness2();
		break;
	case MOVE_LIGHT:
		ctl->light->target = *((u16_t *) dptr);
		break;
	case ACTUAL:
		ctl->light->target = *((u16_t *) dptr);
		ctl->light->target = constrain_lightness(ctl->light->target);
		break;
	case LINEAR:
		ctl->light->target = linear_to_actual(*((u16_t *) dptr));
		ctl->light->target = constrain_lightness(ctl->light->target);
		break;
	case CTL_LIGHT:
		ctl->light->target = *((u16_t *) dptr);
		ctl->light->target = constrain_lightness(ctl->light->target);
		break;
	case LEVEL_TEMP:
		ctl->temp->target = level_to_light_ctl_temp(*((s16_t *) dptr));
		break;
	case MOVE_TEMP:
		ctl->temp->target = *((u16_t *) dptr);
		break;
	case CTL_TEMP:
		ctl->temp->target = *((u16_t *) dptr);
		ctl->temp->target = constrain_temperature(ctl->temp->target);
		break;
	case CTL_DELTA_UV:
		ctl->duv->target = *((s16_t *) dptr);
		break;
	default:
		return;
	}

	if (ctl->onpowerup == STATE_RESTORE) {
		save_on_flash(LAST_TARGET_STATES);
	}
}

int get_current(u8_t type)
{
	switch (type) {
	case ONOFF:
		if (ctl->light->current != 0U) {
			return STATE_ON;
		} else {
			if (ctl->light->target) {
				return STATE_ON;
			} else {
				return STATE_OFF;
			}
		}
	case LEVEL_LIGHT:
		return (s16_t) (ctl->light->current + INT16_MIN);
	case ACTUAL:
		return ctl->light->current;
	case LINEAR:
		return actual_to_linear(ctl->light->current);
	case CTL_LIGHT:
		return ctl->light->current;
	case LEVEL_TEMP:
		return light_ctl_temp_to_level(ctl->temp->current);
	case CTL_TEMP:
		return ctl->temp->current;
	case CTL_DELTA_UV:
		return ctl->duv->current;
	default:
		return 0U;
	}
}

int get_target(u8_t type)
{
	switch (type) {
	case ONOFF:
		if (ctl->light->target != 0U) {
			return STATE_ON;
		} else {
			return STATE_OFF;
		}
	case LEVEL_LIGHT:
		return (s16_t) (ctl->light->target + INT16_MIN);
	case ACTUAL:
		return ctl->light->target;
	case LINEAR:
		return actual_to_linear(ctl->light->target);
	case CTL_LIGHT:
		return ctl->light->target;
	case LEVEL_TEMP:
		return light_ctl_temp_to_level(ctl->temp->target);
	case CTL_TEMP:
		return ctl->temp->target;
	case CTL_DELTA_UV:
		return ctl->duv->target;
	default:
		return 0U;
	}
}
