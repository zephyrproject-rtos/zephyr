/* Bluetooth: Mesh Generic OnOff, Generic Level, Lighting & Vendor Models
 *
 * Copyright (c) 2018 Vikrant More
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ble_mesh.h"
#include "device_composition.h"

#include "storage.h"

static void unsolicitedly_publish_states_work_handler(struct k_work *work)
{
	gen_onoff_publish(&root_models[2]);
	gen_level_publish(&root_models[4]);
	light_lightness_publish(&root_models[11]);
	light_lightness_linear_publish(&root_models[11]);
	light_ctl_publish(&root_models[14]);

	gen_level_publish(&s0_models[0]);
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


static void save_lightness_temp_last_values_timer_handler(struct k_timer *dummy)
{
	save_on_flash(LIGHTNESS_TEMP_LAST_STATE);
}

K_TIMER_DEFINE(save_lightness_temp_last_values_timer,
	       save_lightness_temp_last_values_timer_handler, NULL);

static void no_transition_work_handler(struct k_work *work)
{
	k_timer_start(&unsolicitedly_publish_states_timer, K_MSEC(5000), 0);

	/* If Lightness & Temperature values remains stable for
	 * 10 Seconds then & then only get stored on SoC flash.
	 */
	if (gen_power_onoff_srv_user_data.onpowerup == STATE_RESTORE) {
		k_timer_start(&save_lightness_temp_last_values_timer,
			      K_MSEC(10000), 0);
	}
}

K_WORK_DEFINE(no_transition_work, no_transition_work_handler);
