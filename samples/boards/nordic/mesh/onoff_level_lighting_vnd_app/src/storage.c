/* Bluetooth: Mesh Generic OnOff, Generic Level, Lighting & Vendor Models
 *
 * Copyright (c) 2018 Vikrant More
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ble_mesh.h"
#include "device_composition.h"
#include "storage.h"

static uint8_t storage_id;
uint8_t reset_counter;

static void save_reset_counter(void)
{
	settings_save_one("ps/rc", &reset_counter, sizeof(reset_counter));
}

static void save_gen_def_trans_time_state(void)
{
	settings_save_one("ps/gdtt", &ctl->tt, sizeof(ctl->tt));
}

static void save_gen_onpowerup_state(void)
{
	settings_save_one("ps/gpo", &ctl->onpowerup, sizeof(ctl->onpowerup));

	if (ctl->onpowerup == 0x02) {
		save_on_flash(LAST_TARGET_STATES);
	}
}

static void save_def_states(void)
{
	settings_save_one("ps/ld", &ctl->light->def, sizeof(ctl->light->def));
	settings_save_one("ps/td", &ctl->temp->def, sizeof(ctl->temp->def));
	settings_save_one("ps/dd", &ctl->duv->def, sizeof(ctl->duv->def));
}

static void save_lightness_last_state(void)
{
	settings_save_one("ps/ll", &ctl->light->last, sizeof(ctl->light->last));

	printk("Lightness Last values have been saved !!\n");
}

static void save_last_target_states(void)
{
	settings_save_one("ps/llt", &ctl->light->target,
			  sizeof(ctl->light->target));

	settings_save_one("ps/tlt", &ctl->temp->target,
			  sizeof(ctl->temp->target));

	settings_save_one("ps/dlt", &ctl->duv->target,
			  sizeof(ctl->duv->target));
}

static void save_lightness_range(void)
{
	ctl->light->range = (uint32_t) ((ctl->light->range_max << 16) |
				     ctl->light->range_min);

	settings_save_one("ps/lr", &ctl->light->range,
			  sizeof(ctl->light->range));
}

static void save_temperature_range(void)
{
	ctl->temp->range = (uint32_t) ((ctl->temp->range_max << 16) |
				    ctl->temp->range_min);

	settings_save_one("ps/tr", &ctl->temp->range, sizeof(ctl->temp->range));
}

static void storage_work_handler(struct k_work *work)
{
	switch (storage_id) {
	case RESET_COUNTER:
		save_reset_counter();
		break;
	case GEN_DEF_TRANS_TIME_STATE:
		save_gen_def_trans_time_state();
		break;
	case GEN_ONPOWERUP_STATE:
		save_gen_onpowerup_state();
		break;
	case DEF_STATES:
		save_def_states();
		break;
	case LIGHTNESS_LAST_STATE:
		save_lightness_last_state();
		break;
	case LAST_TARGET_STATES:
		save_last_target_states();
		break;
	case LIGHTNESS_RANGE:
		save_lightness_range();
		break;
	case TEMPERATURE_RANGE:
		save_temperature_range();
		break;
	}
}

K_WORK_DEFINE(storage_work, storage_work_handler);

void save_on_flash(uint8_t id)
{
	storage_id = id;
	k_work_submit(&storage_work);
}

static int ps_set(const char *key, size_t len_rd,
		  settings_read_cb read_cb, void *cb_arg)
{
	ssize_t len = 0;
	int key_len;
	const char *next;

	key_len = settings_name_next(key, &next);

	if (!next) {
		if (!strncmp(key, "rc", key_len)) {
			len = read_cb(cb_arg, &reset_counter,
				      sizeof(reset_counter));
		}

		if (!strncmp(key, "gdtt", key_len)) {
			len = read_cb(cb_arg, &ctl->tt, sizeof(ctl->tt));
		}

		if (!strncmp(key, "gpo", key_len)) {
			len = read_cb(cb_arg, &ctl->onpowerup,
				      sizeof(ctl->onpowerup));
		}

		if (!strncmp(key, "ld", key_len)) {
			len = read_cb(cb_arg, &ctl->light->def,
				      sizeof(ctl->light->def));
		}

		if (!strncmp(key, "td", key_len)) {
			len = read_cb(cb_arg, &ctl->temp->def,
				      sizeof(ctl->temp->def));
		}

		if (!strncmp(key, "dd", key_len)) {
			len = read_cb(cb_arg, &ctl->duv->def,
				      sizeof(ctl->duv->def));
		}

		if (!strncmp(key, "ll", key_len)) {
			len = read_cb(cb_arg, &ctl->light->last,
				      sizeof(ctl->light->last));
		}

		if (!strncmp(key, "llt", key_len)) {
			len = read_cb(cb_arg, &ctl->light->target,
				      sizeof(ctl->light->target));
		}

		if (!strncmp(key, "tlt", key_len)) {
			len = read_cb(cb_arg, &ctl->temp->target,
				      sizeof(ctl->temp->target));
		}

		if (!strncmp(key, "dlt", key_len)) {
			len = read_cb(cb_arg, &ctl->duv->target,
				      sizeof(ctl->duv->target));
		}

		if (!strncmp(key, "lr", key_len)) {
			len = read_cb(cb_arg, &ctl->light->range,
				      sizeof(ctl->light->range));
		}

		if (!strncmp(key, "tr", key_len)) {
			len = read_cb(cb_arg, &ctl->temp->range,
				      sizeof(ctl->temp->range));
		}

		return (len < 0) ? len : 0;
	}

	return -ENOENT;
}

static struct settings_handler ps_settings = {
	.name = "ps",
	.h_set = ps_set,
};

int ps_settings_init(void)
{
	int err;

	err = settings_subsys_init();
	if (err) {
		printk("settings_subsys_init failed (err %d)", err);
		return err;
	}

	err = settings_register(&ps_settings);
	if (err) {
		printk("ps_settings_register failed (err %d)", err);
		return err;
	}

	return 0;
}
