/* Bluetooth: Mesh Generic OnOff, Generic Level, Lighting & Vendor Models
 *
 * Copyright (c) 2018 Vikrant More
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _STORAGE_H
#define _STORAGE_H

enum ps_variables_id {
	RESET_COUNTER = 0x01,
	GEN_DEF_TRANS_TIME_STATE,
	GEN_ONPOWERUP_STATE,
	LIGHTNESS_TEMP_DEF_STATE,
	LIGHTNESS_LAST_STATE,
	LIGHTNESS_TEMP_LAST_TARGET_STATE,
	LIGHTNESS_RANGE,
	TEMPERATURE_RANGE
};

extern u8_t reset_counter;

extern struct k_work storage_work;

int ps_settings_init(void);
void save_on_flash(u8_t id);

#endif
