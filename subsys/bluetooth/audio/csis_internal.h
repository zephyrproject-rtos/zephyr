/**
 * @file
 * @brief Internal APIs for Bluetooth CSIS
 *
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/audio/csis.h>


#define BT_CSIS_SIRK_TYPE_ENCRYPTED             0x00
#define BT_CSIS_SIRK_TYPE_PLAIN                 0x01

#define BT_CSIS_RELEASE_VALUE                   0x01
#define BT_CSIS_LOCK_VALUE                      0x02

struct csis_pending_notifications {
	bt_addr_le_t addr;
	bool pending;
	bool active;

/* Since there's a 1-to-1 connection between bonded devices, and devices in
 * the array containing this struct, if the security manager overwrites
 * the oldest keys, we also overwrite the oldest entry
 */
#if IS_ENABLED(CONFIG_BT_KEYS_OVERWRITE_OLDEST)
	uint32_t age;
#endif /* CONFIG_BT_KEYS_OVERWRITE_OLDEST */
};

struct bt_csis_set_sirk {
	uint8_t type;
	uint8_t value[BT_CSIS_SET_SIRK_SIZE];
} __packed;

struct bt_csis_client_svc_inst {
	uint8_t rank;
	uint8_t set_lock;

	uint16_t start_handle;
	uint16_t end_handle;
	uint16_t set_sirk_handle;
	uint16_t set_size_handle;
	uint16_t set_lock_handle;
	uint16_t rank_handle;

	uint8_t idx;
	struct bt_gatt_subscribe_params sirk_sub_params;
	struct bt_gatt_discover_params sirk_sub_disc_params;
	struct bt_gatt_subscribe_params size_sub_params;
	struct bt_gatt_discover_params size_sub_disc_params;
	struct bt_gatt_subscribe_params lock_sub_params;
	struct bt_gatt_discover_params lock_sub_disc_params;

	struct bt_conn *conn;
	struct bt_csis_client_set_member *member;
};

/* TODO: Rename to bt_csis_svc_inst */
struct bt_csis_server {
	struct bt_csis_set_sirk set_sirk;
	uint8_t set_size;
	uint8_t set_lock;
	uint8_t rank;
	struct bt_csis_cb *cb;
	struct k_work_delayable set_lock_timer;
	bt_addr_le_t lock_client_addr;
	struct bt_gatt_service *service_p;
	struct csis_pending_notifications pend_notify[CONFIG_BT_MAX_PAIRED];
#if IS_ENABLED(CONFIG_BT_KEYS_OVERWRITE_OLDEST)
	uint32_t age_counter;
#endif /* CONFIG_BT_KEYS_OVERWRITE_OLDEST */
};

struct bt_csis {
	bool client_instance;
	union {
#if defined(CONFIG_BT_CSIS)
		struct bt_csis_server srv;
#endif /* CONFIG_BT_CSIS */
#if defined(CONFIG_BT_CSIS_CLIENT)
		struct bt_csis_client_svc_inst cli;
#endif /* CONFIG_BT_CSIS_CLIENT */
	};
};
