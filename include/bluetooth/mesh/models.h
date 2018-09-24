/** @file
 *  @brief Bluetooth Mesh Models API.
 */

/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_MESH_MODELS_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_MESH_MODELS_H_

/* GENERIC BATTERY LEVEL (0x100C) */

/* Generic Battery Level defines */
#define BT_MESH_MODEL_BATTERY_SRV_CHARGE_LEVEL_UNKNOWN 0xFF

/* Generic Battery Time To Discharge defines */
#define BT_MESH_MODEL_BATTERY_SRV_UNKNOWN_DISCHARGE_TIME 0xFFFFFF

/* Generic Battery Time To Charge defines */
#define BT_MESH_MODEL_BATTERY_SRV_UNKNOWN_CHARGE_TIME 0xFFFFFF

/* Generic Battery Enums */
/* Generic Battery Enum Presence */
#define BT_MESH_MODEL_BATTERY_SRV_PRES_NOT_PRES     (0 << 0)
#define BT_MESH_MODEL_BATTERY_SRV_PRES_PRES_REMOV   (1 << 0)
#define BT_MESH_MODEL_BATTERY_SRV_PRES_PRES_NOREMOV (2 << 0)
#define BT_MESH_MODEL_BATTERY_SRV_PRES_PRES_UNKNOWN (3 << 0)
/* Generic Battery Enum Indicator */
#define BT_MESH_MODEL_BATTERY_SRV_IND_CRITICAL (0 << 2)
#define BT_MESH_MODEL_BATTERY_SRV_IND_LOW      (1 << 2)
#define BT_MESH_MODEL_BATTERY_SRV_IND_GOOD     (2 << 2)
#define BT_MESH_MODEL_BATTERY_SRV_IND_UNKNOWN  (3 << 2)
/* Generic Battery Enum Charging */
#define BT_MESH_MODEL_BATTERY_SRV_CHAR_NOT_CHARGABLE (0 << 4)
#define BT_MESH_MODEL_BATTERY_SRV_CHAR_NOT_CHARGING  (1 << 4)
#define BT_MESH_MODEL_BATTERY_SRV_CHAR_CHARARGING    (2 << 4)
#define BT_MESH_MODEL_BATTERY_SRV_CHAR_UNKNOWN       (3 << 4)
/* Generic Battery Enum Serviceability */
#define BT_MESH_MODEL_BATTERY_SRV_SVC_NOT_REQ (1 << 6)
#define BT_MESH_MODEL_BATTERY_SRV_SVC_REQ     (2 << 6)
#define BT_MESH_MODEL_BATTERY_SRV_SVC_UNKNOWN (3 << 6)

#ifdef CONFIG_BT_MESH_MODEL_BATTERY_SRV_CNT
struct generic_battery_state {
	u8_t battery_level;
	u32_t time_to_discharge;
	u32_t time_to_charge;
	u8_t flags;

	u8_t last_tid;
	u16_t last_tx_addr;
};

extern const struct bt_mesh_model_op bt_mesh_model_gen_battery_srv_op[];
extern struct generic_battery_state
	generic_battery_state_user_data[CONFIG_BT_MESH_MODEL_BATTERY_SRV_CNT];
extern struct bt_mesh_model_pub
	generic_battery_pub[CONFIG_BT_MESH_MODEL_BATTERY_SRV_CNT];

#define BT_MESH_MODEL_GEN_BATTERY_SRV(id)                          \
		BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_BATTERY_SRV,    \
			      bt_mesh_model_gen_battery_srv_op,    \
			      generic_battery_pub + id,            \
			      generic_battery_state_user_data + id)

int bt_mesh_model_gen_battery_srv_state_update(u8_t id, u8_t battery_level,
					       u32_t time_to_discharge,
					       u32_t time_to_charge,
					       u8_t flags);
int bt_mesh_model_gen_battery_srv_init(u8_t id);
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_MESH_MODELS_H_ */
