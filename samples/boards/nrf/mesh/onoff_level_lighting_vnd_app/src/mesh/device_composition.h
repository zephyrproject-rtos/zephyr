/* Bluetooth: Mesh Generic OnOff, Generic Level, Lighting & Vendor Models
 *
 * Copyright (c) 2018 Vikrant More
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _DEVICE_COMPOSITION_H
#define _DEVICE_COMPOSITION_H

#define CID_ZEPHYR 0x0002

#define STATE_OFF       0x00
#define STATE_ON        0x01
#define STATE_DEFAULT   0x01
#define STATE_RESTORE   0x02

/* Following 4 values are as per Mesh Model specification */
#define LIGHTNESS_MIN   0x0001
#define LIGHTNESS_MAX   0xFFFF
#define TEMP_MIN	0x0320
#define TEMP_MAX	0x4E20
#define DELTA_UV_DEF	0x0000

/* Refer 7.2 of Mesh Model Specification */
#define RANGE_SUCCESSFULLY_UPDATED      0x00
#define CANNOT_SET_RANGE_MIN            0x01
#define CANNOT_SET_RANGE_MAX            0x02

struct vendor_state {
	int current;
	uint32_t response;
	uint8_t last_tid;
	uint16_t last_src_addr;
	uint16_t last_dst_addr;
	int64_t last_msg_timestamp;
};

struct lightness {
	uint16_t current;
	uint16_t target;

	uint16_t def;
	uint16_t last;

	uint8_t status_code;
	uint16_t range_min;
	uint16_t range_max;
	uint32_t range;

	int delta;
};

struct temperature {
	uint16_t current;
	uint16_t target;

	uint16_t def;

	uint8_t status_code;
	uint16_t range_min;
	uint16_t range_max;
	uint32_t range;

	int delta;
};


struct delta_uv {
	int16_t current;
	int16_t target;

	int16_t def;

	int delta;
};

struct light_ctl_state {
	struct lightness *light;
	struct temperature *temp;
	struct delta_uv *duv;

	uint8_t onpowerup, tt;

	uint8_t last_tid;
	uint16_t last_src_addr;
	uint16_t last_dst_addr;
	int64_t last_msg_timestamp;

	struct transition *transition;
};

extern struct vendor_state vnd_user_data;
extern struct light_ctl_state *const ctl;

extern struct bt_mesh_model root_models[];
extern struct bt_mesh_model vnd_models[];
extern struct bt_mesh_model s0_models[];

extern const struct bt_mesh_comp comp;

void gen_onoff_publish(struct bt_mesh_model *model);
void gen_level_publish(struct bt_mesh_model *model);
void light_lightness_publish(struct bt_mesh_model *model);
void light_lightness_linear_publish(struct bt_mesh_model *model);
void light_ctl_publish(struct bt_mesh_model *model);
void light_ctl_temp_publish(struct bt_mesh_model *model);
void gen_level_publish_temp(struct bt_mesh_model *model);

#endif
