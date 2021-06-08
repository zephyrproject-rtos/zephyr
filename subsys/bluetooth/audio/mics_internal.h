/*
 * Copyright (c) 2020-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_MICS_INTERNAL_
#define ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_MICS_INTERNAL_
#include <zephyr/types.h>
#include <bluetooth/gatt.h>

struct bt_mics_server {
	uint8_t mute;
	struct bt_mics_cb *cb;
	struct bt_gatt_service *service_p;
	struct bt_aics *aics_insts[CONFIG_BT_MICS_AICS_INSTANCE_COUNT];
};

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

	uint8_t aics_inst_cnt;
	struct bt_aics *aics[CONFIG_BT_MICS_CLIENT_MAX_AICS_INST];
};

/* Struct used as a common type for the api */
struct bt_mics {
	union {
		struct bt_mics_server srv;
		struct bt_mics_client cli;
	};
};

int bt_mics_client_included_get(struct bt_conn *conn,
				struct bt_mics_included  *included);
int bt_mics_client_mute_get(struct bt_conn *conn);
int bt_mics_client_mute(struct bt_conn *conn);
int bt_mics_client_unmute(struct bt_conn *conn);
bool bt_mics_client_valid_aics_inst(struct bt_conn *conn, struct bt_aics *aics);

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_MICS_INTERNAL_ */
