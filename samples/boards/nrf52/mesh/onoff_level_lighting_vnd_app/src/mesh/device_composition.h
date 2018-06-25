/* Bluetooth: Mesh Generic OnOff, Generic Level, Lighting & Vendor Models
 *
 * Copyright (c) 2018 Vikrant More
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _DEVICE_COMPOSITION_H
#define _DEVICE_COMPOSITION_H

#define CID_ZEPHYR 0x0002

enum lightness { ONPOWERUP = 0x01, ONOFF, LEVEL, ACTUAL, LINEAR, CTL, IGNORE};
enum temperature { ONOFF_TEMP = 0x01, LEVEL_TEMP, CTL_TEMP, IGNORE_TEMP};

struct generic_onoff_state {
	u8_t onoff;
	u8_t previous;
	u8_t model_instance;
	u8_t last_tid;
	u16_t last_tx_addr;
};

struct generic_level_state {
	int level;
	u8_t model_instance;
	u8_t last_tid;
	u16_t last_tx_addr;
};

struct generic_onpowerup_state {
	u8_t onpowerup;
	u8_t model_instance;
	u8_t last_tid;
	u16_t last_tx_addr;
};

struct vendor_state {
	int current;
	int previous;
	u8_t model_instance;
	u8_t last_tid;
	u16_t last_tx_addr;
};

struct light_lightness_state {
	u16_t linear;
	u16_t actual;
	u16_t last;
	u16_t def;

	u8_t status_code;
	u16_t lightness_range_min;
	u16_t lightness_range_max;

	u8_t last_tid;
	u16_t last_tx_addr;
};

struct light_ctl_state {
	u16_t lightness;
	u16_t temp;
	s16_t delta_uv;

	u8_t status_code;
	u16_t temp_range_min;
	u16_t temp_range_max;

	u16_t lightness_def;
	u16_t temp_def;
	s16_t delta_uv_def;

	u16_t temp_last;

	u8_t last_tid;
	u16_t last_tx_addr;
};

extern struct generic_onoff_state gen_onoff_srv_root_user_data;
extern struct light_lightness_state light_lightness_srv_user_data;
extern struct generic_level_state gen_level_srv_s0_user_data;

extern struct bt_mesh_model root_models[];
extern struct bt_mesh_model vnd_models[];
extern struct bt_mesh_model s0_models[];

extern const struct bt_mesh_comp comp;

void light_default_status_init(void);

#endif
