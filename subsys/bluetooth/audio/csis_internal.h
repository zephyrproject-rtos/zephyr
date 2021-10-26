/**
 * @file
 * @brief Internal APIs for Bluetooth CSIS
 *
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <bluetooth/audio/csis.h>

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

struct bt_csis_server {
	struct bt_csis_set_sirk set_sirk;
	uint8_t psri[BT_CSIS_PSRI_SIZE];
	uint8_t set_size;
	uint8_t set_lock;
	uint8_t rank;
	bool adv_enabled;
	struct k_work_delayable set_lock_timer;
	bt_addr_le_t lock_client_addr;
	struct bt_gatt_service *service_p;
	struct csis_pending_notifications pend_notify[CONFIG_BT_MAX_PAIRED];
#if IS_ENABLED(CONFIG_BT_KEYS_OVERWRITE_OLDEST)
	uint32_t age_counter;
#endif /* CONFIG_BT_KEYS_OVERWRITE_OLDEST */
#if defined(CONFIG_BT_EXT_ADV)
	uint8_t conn_cnt;
	struct bt_le_ext_adv *adv;
	struct bt_le_ext_adv_cb adv_cb;
	struct k_work work;
#endif /* CONFIG_BT_EXT_ADV */
};

struct bt_csis {
	union {
		struct bt_csis_server srv;
		/* TODO: Add cli */
	};
};
