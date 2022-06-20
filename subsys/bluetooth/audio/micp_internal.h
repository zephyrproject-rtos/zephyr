/*
 * Copyright (c) 2020-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_MICP_INTERNAL_
#define ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_MICP_INTERNAL_
#include <zephyr/types.h>
#include <zephyr/bluetooth/gatt.h>

#if defined(CONFIG_BT_MICP)
struct bt_micp_server {
	uint8_t mute;
	struct bt_micp_cb *cb;
	struct bt_gatt_service *service_p;
	struct bt_aics *aics_insts[CONFIG_BT_MICP_AICS_INSTANCE_COUNT];
};
#endif /* CONFIG_BT_MICP */

#if defined(CONFIG_BT_MICP_CLIENT)
struct bt_micp_client {
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
	struct bt_aics *aics[CONFIG_BT_MICP_CLIENT_MAX_AICS_INST];
};
#endif /* CONFIG_BT_MICP_CLIENT */

/* Struct used as a common type for the api */
struct bt_micp {
	bool client_instance;
	union {
#if defined(CONFIG_BT_MICP)
		struct bt_micp_server srv;
#endif /* CONFIG_BT_MICP */
#if defined(CONFIG_BT_MICP_CLIENT)
		struct bt_micp_client cli;
#endif /* CONFIG_BT_MICP_CLIENT */
	};
};

int bt_micp_client_included_get(struct bt_micp *micp,
				struct bt_micp_included  *included);
int bt_micp_client_mute_get(struct bt_micp *micp);
int bt_micp_client_mute(struct bt_micp *micp);
int bt_micp_client_unmute(struct bt_micp *micp);

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_MICS_INTERNAL_ */
