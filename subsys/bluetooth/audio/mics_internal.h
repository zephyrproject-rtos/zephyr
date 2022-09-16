/*
 * Copyright (c) 2020-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_MICS_INTERNAL_
#define ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_MICS_INTERNAL_
#include <zephyr/types.h>
#include <zephyr/bluetooth/gatt.h>

#if defined(CONFIG_BT_MICS)
struct bt_mics_server {
	uint8_t mute;
	struct bt_mics_cb *cb;
	struct bt_gatt_service *service_p;
	struct bt_aics *aics_insts[CONFIG_BT_MICS_AICS_INSTANCE_COUNT];
};
#endif /* CONFIG_BT_MICS */

#if defined(CONFIG_BT_MICS_CLIENT)
struct bt_mics_client {
	uint16_t start_handle;
	uint16_t end_handle;
	uint16_t mute_handle;
	struct bt_gatt_subscribe_params mute_sub_params;
	struct bt_gatt_discover_params mute_sub_disc_params;

	bool busy;
	uint8_t mute_val_buf[1]; /* Mute value is a single octet */
	struct bt_gatt_write_params write_params;
	struct bt_gatt_read_params read_params;
	struct bt_gatt_discover_params discover_params;
	struct bt_conn *conn;

	uint8_t aics_inst_cnt;
	struct bt_aics *aics[CONFIG_BT_MICS_CLIENT_MAX_AICS_INST];
};
#endif /* CONFIG_BT_MICS_CLIENT */

/* Struct used as a common type for the api */
struct bt_mics {
	bool client_instance;
	union {
#if defined(CONFIG_BT_MICS)
		struct bt_mics_server srv;
#endif /* CONFIG_BT_MICS */
#if defined(CONFIG_BT_MICS_CLIENT)
		struct bt_mics_client cli;
#endif /* CONFIG_BT_MICS_CLIENT */
	};
};

int bt_mics_client_included_get(struct bt_mics *mics,
				struct bt_mics_included  *included);
int bt_mics_client_mute_get(struct bt_mics *mics);
int bt_mics_client_mute(struct bt_mics *mics);
int bt_mics_client_unmute(struct bt_mics *mics);
bool bt_mics_client_valid_aics_inst(struct bt_mics *mics, struct bt_aics *aics);

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_MICS_INTERNAL_ */
