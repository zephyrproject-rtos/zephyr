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
#include "publisher.h"
#include "device_composition.h"

#define ONOFF
#define GENERIC_LEVEL

static bool is_randomization_of_TIDs_done;

#if (defined(ONOFF) || defined(ONOFF_TT))
static u8_t tid_onoff;
#elif defined(VND_MODEL_TEST)
static u8_t tid_vnd;
#endif

static u8_t tid_level;

void randomize_publishers_TID(void)
{
#if (defined(ONOFF) || defined(ONOFF_TT))
	bt_rand(&tid_onoff, sizeof(tid_onoff));
#elif defined(VND_MODEL_TEST)
	bt_rand(&tid_vnd, sizeof(tid_vnd));
#endif

	bt_rand(&tid_level, sizeof(tid_level));

	is_randomization_of_TIDs_done = true;
}

static u32_t button_read(struct device *port, u32_t pin)
{
	u32_t val = 0;

	gpio_pin_read(port, pin, &val);
	return val;
}

void publish(struct k_work *work)
{
	int err = 0;

	if (is_randomization_of_TIDs_done == false) {
		return;
	}

	if (button_read(button_device[0], SW0_GPIO_PIN) == 0) {
#if defined(ONOFF)
		bt_mesh_model_msg_init(root_models[3].pub->msg,
				       BT_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK);
		net_buf_simple_add_u8(root_models[3].pub->msg, 0x01);
		net_buf_simple_add_u8(root_models[3].pub->msg, tid_onoff++);
		err = bt_mesh_model_publish(&root_models[3]);
#elif defined(ONOFF_TT)
		bt_mesh_model_msg_init(root_models[3].pub->msg,
				       BT_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK);
		net_buf_simple_add_u8(root_models[3].pub->msg, 0x01);
		net_buf_simple_add_u8(root_models[3].pub->msg, tid_onoff++);
		net_buf_simple_add_u8(root_models[3].pub->msg, 0x45);
		net_buf_simple_add_u8(root_models[3].pub->msg, 0x28);
		err = bt_mesh_model_publish(&root_models[3]);
#elif defined(VND_MODEL_TEST)
		bt_mesh_model_msg_init(vnd_models[0].pub->msg,
				       BT_MESH_MODEL_OP_3(0x02, CID_ZEPHYR));
		net_buf_simple_add_le16(vnd_models[0].pub->msg, 0x0001);
		net_buf_simple_add_u8(vnd_models[0].pub->msg, tid_vnd++);
		err = bt_mesh_model_publish(&vnd_models[0]);
#endif
	} else if (button_read(button_device[1], SW1_GPIO_PIN) == 0) {
#if defined(ONOFF)
		bt_mesh_model_msg_init(root_models[3].pub->msg,
				       BT_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK);
		net_buf_simple_add_u8(root_models[3].pub->msg, 0x00);
		net_buf_simple_add_u8(root_models[3].pub->msg, tid_onoff++);
		err = bt_mesh_model_publish(&root_models[3]);
#elif defined(ONOFF_TT)
		bt_mesh_model_msg_init(root_models[3].pub->msg,
				       BT_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK);
		net_buf_simple_add_u8(root_models[3].pub->msg, 0x00);
		net_buf_simple_add_u8(root_models[3].pub->msg, tid_onoff++);
		net_buf_simple_add_u8(root_models[3].pub->msg, 0x45);
		net_buf_simple_add_u8(root_models[3].pub->msg, 0x28);
		err = bt_mesh_model_publish(&root_models[3]);
#elif defined(VND_MODEL_TEST)
		bt_mesh_model_msg_init(vnd_models[0].pub->msg,
				       BT_MESH_MODEL_OP_3(0x02, CID_ZEPHYR));
		net_buf_simple_add_le16(vnd_models[0].pub->msg, 0x0000);
		net_buf_simple_add_u8(vnd_models[0].pub->msg, tid_vnd++);
		err = bt_mesh_model_publish(&vnd_models[0]);
#endif
	} else if (button_read(button_device[2], SW2_GPIO_PIN) == 0) {
#if defined(GENERIC_LEVEL)
		bt_mesh_model_msg_init(root_models[5].pub->msg,
				       BT_MESH_MODEL_OP_GEN_LEVEL_SET_UNACK);
		net_buf_simple_add_le16(root_models[5].pub->msg, LEVEL_S25);
		net_buf_simple_add_u8(root_models[5].pub->msg, tid_level++);
		err = bt_mesh_model_publish(&root_models[5]);
#elif defined(ONOFF_GET)
		bt_mesh_model_msg_init(root_models[3].pub->msg,
				       BT_MESH_MODEL_OP_GEN_ONOFF_GET);
		err = bt_mesh_model_publish(&root_models[3]);
#elif defined(GENERIC_DELTA_LEVEL)
		bt_mesh_model_msg_init(root_models[5].pub->msg,
				       BT_MESH_MODEL_OP_GEN_DELTA_SET_UNACK);
		net_buf_simple_add_le32(root_models[5].pub->msg, 100);
		net_buf_simple_add_u8(root_models[5].pub->msg, tid_level++);
		err = bt_mesh_model_publish(&root_models[5]);
#elif defined(GENERIC_MOVE_LEVEL_TT)
		bt_mesh_model_msg_init(root_models[5].pub->msg,
				       BT_MESH_MODEL_OP_GEN_MOVE_SET_UNACK);
		net_buf_simple_add_le16(root_models[5].pub->msg, 13100);
		net_buf_simple_add_u8(root_models[5].pub->msg, tid_level++);
		net_buf_simple_add_u8(root_models[5].pub->msg, 0x45);
		net_buf_simple_add_u8(root_models[5].pub->msg, 0x00);
		err = bt_mesh_model_publish(&root_models[5]);
#elif defined(LIGHT_LIGHTNESS_TT)
		bt_mesh_model_msg_init(root_models[13].pub->msg,
				       BT_MESH_MODEL_OP_2(0x82, 0x4D));
		net_buf_simple_add_le16(root_models[13].pub->msg, LEVEL_U25);
		net_buf_simple_add_u8(root_models[13].pub->msg, tid_level++);
		net_buf_simple_add_u8(root_models[13].pub->msg, 0x45);
		net_buf_simple_add_u8(root_models[13].pub->msg, 0x28);
		err = bt_mesh_model_publish(&root_models[13]);
#elif defined(LIGHT_CTL)
		bt_mesh_model_msg_init(root_models[16].pub->msg,
				       BT_MESH_MODEL_OP_2(0x82, 0x5F));
		/* Lightness */
		net_buf_simple_add_le16(root_models[16].pub->msg, LEVEL_U25);
		/* Temperature (value should be from 0x0320 to 0x4E20 */
		/* This is as per 6.1.3.1 in Mesh Model Specification */
		net_buf_simple_add_le16(root_models[16].pub->msg, 0x0320);
		/* Delta UV */
		net_buf_simple_add_le16(root_models[16].pub->msg, 0x0000);
		net_buf_simple_add_u8(root_models[16].pub->msg, tid_level++);
		err = bt_mesh_model_publish(&root_models[16]);
#elif defined(LIGHT_CTL_TT)
		bt_mesh_model_msg_init(root_models[16].pub->msg,
				       BT_MESH_MODEL_OP_2(0x82, 0x5F));
		/* Lightness */
		net_buf_simple_add_le16(root_models[16].pub->msg, LEVEL_U25);
		/* Temperature (value should be from 0x0320 to 0x4E20 */
		/* This is as per 6.1.3.1 in Mesh Model Specification */
		net_buf_simple_add_le16(root_models[16].pub->msg, 0x0320);
		/* Delta UV */
		net_buf_simple_add_le16(root_models[16].pub->msg, 0x0000);
		net_buf_simple_add_u8(root_models[16].pub->msg, tid_level++);
		net_buf_simple_add_u8(root_models[16].pub->msg, 0x45);
		net_buf_simple_add_u8(root_models[16].pub->msg, 0x00);
		err = bt_mesh_model_publish(&root_models[16]);
#elif defined(LIGHT_CTL_TEMP)
		bt_mesh_model_msg_init(root_models[16].pub->msg,
				       BT_MESH_MODEL_OP_2(0x82, 0x65));
		/* Temperature (value should be from 0x0320 to 0x4E20 */
		/* This is as per 6.1.3.1 in Mesh Model Specification */
		net_buf_simple_add_le16(root_models[16].pub->msg, 0x0320);
		/* Delta UV */
		net_buf_simple_add_le16(root_models[16].pub->msg, 0x0000);
		net_buf_simple_add_u8(root_models[16].pub->msg, tid_level++);
		err = bt_mesh_model_publish(&root_models[16]);
#endif
	} else if (button_read(button_device[3], SW3_GPIO_PIN) == 0) {
#if defined(GENERIC_LEVEL)
		bt_mesh_model_msg_init(root_models[5].pub->msg,
				       BT_MESH_MODEL_OP_GEN_LEVEL_SET_UNACK);
		net_buf_simple_add_le16(root_models[5].pub->msg, LEVEL_S100);
		net_buf_simple_add_u8(root_models[5].pub->msg, tid_level++);
		err = bt_mesh_model_publish(&root_models[5]);
#elif defined(GENERIC_DELTA_LEVEL)
		bt_mesh_model_msg_init(root_models[5].pub->msg,
				       BT_MESH_MODEL_OP_GEN_DELTA_SET_UNACK);
		net_buf_simple_add_le32(root_models[5].pub->msg, -100);
		net_buf_simple_add_u8(root_models[5].pub->msg, tid_level++);
		err = bt_mesh_model_publish(&root_models[5]);
#elif defined(GENERIC_MOVE_LEVEL_TT)
		bt_mesh_model_msg_init(root_models[5].pub->msg,
				       BT_MESH_MODEL_OP_GEN_MOVE_SET_UNACK);
		net_buf_simple_add_le16(root_models[5].pub->msg, -13100);
		net_buf_simple_add_u8(root_models[5].pub->msg, tid_level++);
		net_buf_simple_add_u8(root_models[5].pub->msg, 0x45);
		net_buf_simple_add_u8(root_models[5].pub->msg, 0x00);
		err = bt_mesh_model_publish(&root_models[5]);
#elif defined(LIGHT_LIGHTNESS_TT)
		bt_mesh_model_msg_init(root_models[13].pub->msg,
				       BT_MESH_MODEL_OP_2(0x82, 0x4D));
		net_buf_simple_add_le16(root_models[13].pub->msg, LEVEL_U100);
		net_buf_simple_add_u8(root_models[13].pub->msg, tid_level++);
		net_buf_simple_add_u8(root_models[13].pub->msg, 0x45);
		net_buf_simple_add_u8(root_models[13].pub->msg, 0x28);
		err = bt_mesh_model_publish(&root_models[13]);
#elif defined(LIGHT_CTL)
		bt_mesh_model_msg_init(root_models[16].pub->msg,
				       BT_MESH_MODEL_OP_2(0x82, 0x5F));
		/* Lightness */
		net_buf_simple_add_le16(root_models[16].pub->msg, LEVEL_U100);
		/* Temperature (value should be from 0x0320 to 0x4E20 */
		/* This is as per 6.1.3.1 in Mesh Model Specification */
		net_buf_simple_add_le16(root_models[16].pub->msg, 0x4E20);
		/* Delta UV */
		net_buf_simple_add_le16(root_models[16].pub->msg, 0x0000);
		net_buf_simple_add_u8(root_models[16].pub->msg, tid_level++);
		err = bt_mesh_model_publish(&root_models[16]);
#elif defined(LIGHT_CTL_TT)
		bt_mesh_model_msg_init(root_models[16].pub->msg,
				       BT_MESH_MODEL_OP_2(0x82, 0x5F));
		/* Lightness */
		net_buf_simple_add_le16(root_models[16].pub->msg, LEVEL_U100);
		/* Temperature (value should be from 0x0320 to 0x4E20 */
		/* This is as per 6.1.3.1 in Mesh Model Specification */
		net_buf_simple_add_le16(root_models[16].pub->msg, 0x4E20);
		/* Delta UV */
		net_buf_simple_add_le16(root_models[16].pub->msg, 0x0000);
		net_buf_simple_add_u8(root_models[16].pub->msg, tid_level++);
		net_buf_simple_add_u8(root_models[16].pub->msg, 0x45);
		net_buf_simple_add_u8(root_models[16].pub->msg, 0x00);
		err = bt_mesh_model_publish(&root_models[16]);
#elif defined(LIGHT_CTL_TEMP)
		bt_mesh_model_msg_init(root_models[16].pub->msg,
				       BT_MESH_MODEL_OP_2(0x82, 0x65));
		/* Temperature (value should be from 0x0320 to 0x4E20 */
		/* This is as per 6.1.3.1 in Mesh Model Specification */
		net_buf_simple_add_le16(root_models[16].pub->msg, 0x4E20);
		/* Delta UV */
		net_buf_simple_add_le16(root_models[16].pub->msg, 0x0000);
		net_buf_simple_add_u8(root_models[16].pub->msg, tid_level++);
		err = bt_mesh_model_publish(&root_models[16]);
#endif
	}

	if (err) {
		printk("bt_mesh_model_publish: err: %d\n", err);
	}
}

