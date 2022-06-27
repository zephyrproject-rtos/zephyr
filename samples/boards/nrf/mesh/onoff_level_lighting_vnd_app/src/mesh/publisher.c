/* Bluetooth: Mesh Generic OnOff, Generic Level, Lighting & Vendor Models
 *
 * Copyright (c) 2018 Vikrant More
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>

#include "app_gpio.h"
#include "ble_mesh.h"
#include "device_composition.h"
#include "publisher.h"

#define ONOFF
#define GENERIC_LEVEL

static uint8_t tid;

void publish(struct k_work *work)
{
	int err = 0;

#ifndef ONE_LED_ONE_BUTTON_BOARD
	if (gpio_pin_get(button_device[0],
			 DT_GPIO_PIN(DT_ALIAS(sw0), gpios)) == 1) {
#if defined(ONOFF)
		bt_mesh_model_msg_init(root_models[3].pub->msg,
				       BT_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK);
		net_buf_simple_add_u8(root_models[3].pub->msg, 0x01);
		net_buf_simple_add_u8(root_models[3].pub->msg, tid++);
		err = bt_mesh_model_publish(&root_models[3]);
#elif defined(ONOFF_TT)
		bt_mesh_model_msg_init(root_models[3].pub->msg,
				       BT_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK);
		net_buf_simple_add_u8(root_models[3].pub->msg, 0x01);
		net_buf_simple_add_u8(root_models[3].pub->msg, tid++);
		net_buf_simple_add_u8(root_models[3].pub->msg, 0x45);
		net_buf_simple_add_u8(root_models[3].pub->msg, 0x28);
		err = bt_mesh_model_publish(&root_models[3]);
#elif defined(VND_MODEL_TEST)
		bt_mesh_model_msg_init(vnd_models[0].pub->msg,
				       BT_MESH_MODEL_OP_3(0x03, CID_ZEPHYR));
		net_buf_simple_add_le16(vnd_models[0].pub->msg, 0x0001);
		net_buf_simple_add_u8(vnd_models[0].pub->msg, tid++);
		err = bt_mesh_model_publish(&vnd_models[0]);
#endif
	} else if (gpio_pin_get(button_device[1],
				DT_GPIO_PIN(DT_ALIAS(sw1), gpios)) == 1) {
#if defined(ONOFF)
		bt_mesh_model_msg_init(root_models[3].pub->msg,
				       BT_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK);
		net_buf_simple_add_u8(root_models[3].pub->msg, 0x00);
		net_buf_simple_add_u8(root_models[3].pub->msg, tid++);
		err = bt_mesh_model_publish(&root_models[3]);
#elif defined(ONOFF_TT)
		bt_mesh_model_msg_init(root_models[3].pub->msg,
				       BT_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK);
		net_buf_simple_add_u8(root_models[3].pub->msg, 0x00);
		net_buf_simple_add_u8(root_models[3].pub->msg, tid++);
		net_buf_simple_add_u8(root_models[3].pub->msg, 0x45);
		net_buf_simple_add_u8(root_models[3].pub->msg, 0x28);
		err = bt_mesh_model_publish(&root_models[3]);
#elif defined(VND_MODEL_TEST)
		bt_mesh_model_msg_init(vnd_models[0].pub->msg,
				       BT_MESH_MODEL_OP_3(0x03, CID_ZEPHYR));
		net_buf_simple_add_le16(vnd_models[0].pub->msg, 0x0000);
		net_buf_simple_add_u8(vnd_models[0].pub->msg, tid++);
		err = bt_mesh_model_publish(&vnd_models[0]);
#endif
	} else if (gpio_pin_get(button_device[2],
				DT_GPIO_PIN(DT_ALIAS(sw2), gpios)) == 1) {
#if defined(GENERIC_LEVEL)
		bt_mesh_model_msg_init(root_models[5].pub->msg,
				       BT_MESH_MODEL_OP_GEN_LEVEL_SET_UNACK);
		net_buf_simple_add_le16(root_models[5].pub->msg, LEVEL_S25);
		net_buf_simple_add_u8(root_models[5].pub->msg, tid++);
		err = bt_mesh_model_publish(&root_models[5]);
#elif defined(ONOFF_GET)
		bt_mesh_model_msg_init(root_models[3].pub->msg,
				       BT_MESH_MODEL_OP_GEN_ONOFF_GET);
		err = bt_mesh_model_publish(&root_models[3]);
#elif defined(GENERIC_DELTA_LEVEL)
		bt_mesh_model_msg_init(root_models[5].pub->msg,
				       BT_MESH_MODEL_OP_GEN_DELTA_SET_UNACK);
		net_buf_simple_add_le32(root_models[5].pub->msg, 100);
		net_buf_simple_add_u8(root_models[5].pub->msg, tid++);
		err = bt_mesh_model_publish(&root_models[5]);
#elif defined(GENERIC_MOVE_LEVEL_TT)
		bt_mesh_model_msg_init(root_models[5].pub->msg,
				       BT_MESH_MODEL_OP_GEN_MOVE_SET_UNACK);
		net_buf_simple_add_le16(root_models[5].pub->msg, 655);
		net_buf_simple_add_u8(root_models[5].pub->msg, tid++);
		net_buf_simple_add_u8(root_models[5].pub->msg, 0x41);
		net_buf_simple_add_u8(root_models[5].pub->msg, 0x00);
		err = bt_mesh_model_publish(&root_models[5]);
#elif defined(LIGHT_LIGHTNESS_TT)
		bt_mesh_model_msg_init(root_models[13].pub->msg,
				       BT_MESH_MODEL_LIGHT_LIGHTNESS_SET_UNACK);
		net_buf_simple_add_le16(root_models[13].pub->msg, LEVEL_U25);
		net_buf_simple_add_u8(root_models[13].pub->msg, tid++);
		net_buf_simple_add_u8(root_models[13].pub->msg, 0x45);
		net_buf_simple_add_u8(root_models[13].pub->msg, 0x28);
		err = bt_mesh_model_publish(&root_models[13]);
#elif defined(LIGHT_CTL)
		bt_mesh_model_msg_init(root_models[16].pub->msg,
				       BT_MESH_MODEL_LIGHT_CTL_SET_UNACK);
		/* Lightness */
		net_buf_simple_add_le16(root_models[16].pub->msg, LEVEL_U25);
		/* Temperature (value should be from 0x0320 to 0x4E20 */
		/* This is as per 6.1.3.1 in Mesh Model Specification */
		net_buf_simple_add_le16(root_models[16].pub->msg, 0x0320);
		/* Delta UV */
		net_buf_simple_add_le16(root_models[16].pub->msg, 0x0000);
		net_buf_simple_add_u8(root_models[16].pub->msg, tid++);
		err = bt_mesh_model_publish(&root_models[16]);
#elif defined(LIGHT_CTL_TT)
		bt_mesh_model_msg_init(root_models[16].pub->msg,
				       BT_MESH_MODEL_LIGHT_CTL_SET_UNACK);
		/* Lightness */
		net_buf_simple_add_le16(root_models[16].pub->msg, LEVEL_U25);
		/* Temperature (value should be from 0x0320 to 0x4E20 */
		/* This is as per 6.1.3.1 in Mesh Model Specification */
		net_buf_simple_add_le16(root_models[16].pub->msg, 0x0320);
		/* Delta UV */
		net_buf_simple_add_le16(root_models[16].pub->msg, 0x0000);
		net_buf_simple_add_u8(root_models[16].pub->msg, tid++);
		net_buf_simple_add_u8(root_models[16].pub->msg, 0x45);
		net_buf_simple_add_u8(root_models[16].pub->msg, 0x00);
		err = bt_mesh_model_publish(&root_models[16]);
#elif defined(LIGHT_CTL_TEMP)
		bt_mesh_model_msg_init(root_models[16].pub->msg,
				       BT_MESH_MODEL_LIGHT_CTL_TEMP_SET_UNACK);
		/* Temperature (value should be from 0x0320 to 0x4E20 */
		/* This is as per 6.1.3.1 in Mesh Model Specification */
		net_buf_simple_add_le16(root_models[16].pub->msg, 0x0320);
		/* Delta UV */
		net_buf_simple_add_le16(root_models[16].pub->msg, 0x0000);
		net_buf_simple_add_u8(root_models[16].pub->msg, tid++);
		err = bt_mesh_model_publish(&root_models[16]);
#endif
	} else if (gpio_pin_get(button_device[3],
				DT_GPIO_PIN(DT_ALIAS(sw3), gpios)) == 1) {
#if defined(GENERIC_LEVEL)
		bt_mesh_model_msg_init(root_models[5].pub->msg,
				       BT_MESH_MODEL_OP_GEN_LEVEL_SET_UNACK);
		net_buf_simple_add_le16(root_models[5].pub->msg, LEVEL_S100);
		net_buf_simple_add_u8(root_models[5].pub->msg, tid++);
		err = bt_mesh_model_publish(&root_models[5]);
#elif defined(GENERIC_DELTA_LEVEL)
		bt_mesh_model_msg_init(root_models[5].pub->msg,
				       BT_MESH_MODEL_OP_GEN_DELTA_SET_UNACK);
		net_buf_simple_add_le32(root_models[5].pub->msg, -100);
		net_buf_simple_add_u8(root_models[5].pub->msg, tid++);
		err = bt_mesh_model_publish(&root_models[5]);
#elif defined(GENERIC_MOVE_LEVEL_TT)
		bt_mesh_model_msg_init(root_models[5].pub->msg,
				       BT_MESH_MODEL_OP_GEN_MOVE_SET_UNACK);
		net_buf_simple_add_le16(root_models[5].pub->msg, -655);
		net_buf_simple_add_u8(root_models[5].pub->msg, tid++);
		net_buf_simple_add_u8(root_models[5].pub->msg, 0x41);
		net_buf_simple_add_u8(root_models[5].pub->msg, 0x00);
		err = bt_mesh_model_publish(&root_models[5]);
#elif defined(LIGHT_LIGHTNESS_TT)
		bt_mesh_model_msg_init(root_models[13].pub->msg,
				       BT_MESH_MODEL_LIGHT_LIGHTNESS_SET_UNACK);
		net_buf_simple_add_le16(root_models[13].pub->msg, LEVEL_U100);
		net_buf_simple_add_u8(root_models[13].pub->msg, tid++);
		net_buf_simple_add_u8(root_models[13].pub->msg, 0x45);
		net_buf_simple_add_u8(root_models[13].pub->msg, 0x28);
		err = bt_mesh_model_publish(&root_models[13]);
#elif defined(LIGHT_CTL)
		bt_mesh_model_msg_init(root_models[16].pub->msg,
				       BT_MESH_MODEL_LIGHT_CTL_SET_UNACK);
		/* Lightness */
		net_buf_simple_add_le16(root_models[16].pub->msg, LEVEL_U100);
		/* Temperature (value should be from 0x0320 to 0x4E20 */
		/* This is as per 6.1.3.1 in Mesh Model Specification */
		net_buf_simple_add_le16(root_models[16].pub->msg, 0x4E20);
		/* Delta UV */
		net_buf_simple_add_le16(root_models[16].pub->msg, 0x0000);
		net_buf_simple_add_u8(root_models[16].pub->msg, tid++);
		err = bt_mesh_model_publish(&root_models[16]);
#elif defined(LIGHT_CTL_TT)
		bt_mesh_model_msg_init(root_models[16].pub->msg,
				       BT_MESH_MODEL_LIGHT_CTL_SET_UNACK);
		/* Lightness */
		net_buf_simple_add_le16(root_models[16].pub->msg, LEVEL_U100);
		/* Temperature (value should be from 0x0320 to 0x4E20 */
		/* This is as per 6.1.3.1 in Mesh Model Specification */
		net_buf_simple_add_le16(root_models[16].pub->msg, 0x4E20);
		/* Delta UV */
		net_buf_simple_add_le16(root_models[16].pub->msg, 0x0000);
		net_buf_simple_add_u8(root_models[16].pub->msg, tid++);
		net_buf_simple_add_u8(root_models[16].pub->msg, 0x45);
		net_buf_simple_add_u8(root_models[16].pub->msg, 0x00);
		err = bt_mesh_model_publish(&root_models[16]);
#elif defined(LIGHT_CTL_TEMP)
		bt_mesh_model_msg_init(root_models[16].pub->msg,
				       BT_MESH_MODEL_LIGHT_CTL_TEMP_SET_UNACK);
		/* Temperature (value should be from 0x0320 to 0x4E20 */
		/* This is as per 6.1.3.1 in Mesh Model Specification */
		net_buf_simple_add_le16(root_models[16].pub->msg, 0x4E20);
		/* Delta UV */
		net_buf_simple_add_le16(root_models[16].pub->msg, 0x0000);
		net_buf_simple_add_u8(root_models[16].pub->msg, tid++);
		err = bt_mesh_model_publish(&root_models[16]);
#endif
	}
#else
	if (gpio_pin_get(button_device[0],
			 DT_GPIO_PIN(DT_ALIAS(sw0), gpios)) == 1) {
#if defined(ONOFF)
		static uint8_t state = STATE_ON;

		bt_mesh_model_msg_init(root_models[3].pub->msg,
				       BT_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK);
		net_buf_simple_add_u8(root_models[3].pub->msg,
				      state = state ^ 0x01);
		net_buf_simple_add_u8(root_models[3].pub->msg, tid++);
		err = bt_mesh_model_publish(&root_models[3]);
#endif
	}
#endif

	if (err) {
		printk("bt_mesh_model_publish: err: %d\n", err);
	}
}
