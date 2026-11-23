/* Bluetooth: Mesh Generic OnOff, Generic Level, Lighting & Vendor Models
 *
 * Copyright (c) 2018 Vikrant More
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common.h"
#include "ble_mesh.h"
#include "device_composition.h"
#include "state_binding.h"
#include "storage.h"

static void unsolicitedly_publish_states_work_handler(struct k_work *work)
{
	gen_onoff_publish(&root_models[2]);
	gen_level_publish(&root_models[4]);
	light_lightness_publish(&root_models[11]);
	light_lightness_linear_publish(&root_models[11]);
	light_ctl_publish(&root_models[14]);

	gen_level_publish_temp(&s0_models[0]);
	light_ctl_temp_publish(&s0_models[2]);
}

K_WORK_DEFINE(unsolicitedly_publish_states_work,
	      unsolicitedly_publish_states_work_handler);

static void unsolicitedly_publish_states_timer_handler(struct k_timer *dummy)
{
	k_work_submit(&unsolicitedly_publish_states_work);
}

K_TIMER_DEFINE(unsolicitedly_publish_states_timer,
	       unsolicitedly_publish_states_timer_handler, NULL);

static void no_transition_work_handler(struct k_work *work)
{
	bool readjust_light_state;

	readjust_light_state = false;

	if (!bt_mesh_is_provisioned()) {
		return;
	}

	if (ctl->light->target != ctl->light->current) {
		ctl->light->current = ctl->light->target;
		readjust_light_state = true;
	}

	if (ctl->temp->target != ctl->temp->current) {
		ctl->temp->current = ctl->temp->target;
		readjust_light_state = true;
	}

	if (ctl->duv->target != ctl->duv->current) {
		ctl->duv->current = ctl->duv->target;
		readjust_light_state = true;
	}

	if (ctl->light->current) {
		ctl->light->last = ctl->light->current;
		save_on_flash(LIGHTNESS_LAST_STATE);
	}

	if (readjust_light_state) {
		update_led_gpio();
	}

	k_timer_start(&unsolicitedly_publish_states_timer,
		      K_MSEC(5000),
		      K_NO_WAIT);
}

K_WORK_DEFINE(no_transition_work, no_transition_work_handler);
