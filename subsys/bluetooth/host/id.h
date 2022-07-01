/*
 * Copyright (c) 2017-2021 Nordic Semiconductor ASA
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define RPA_TIMEOUT_MS(_rpa_timeout) (_rpa_timeout * MSEC_PER_SEC)

static inline bool bt_id_rpa_is_new(void)
{
#if defined(CONFIG_BT_PRIVACY)
	uint32_t remaining_ms = k_ticks_to_ms_floor32(
		k_work_delayable_remaining_get(&bt_dev.rpa_update));
	/* RPA is considered new if there is less than half a second since the
	 * timeout was started.
	 */
	return remaining_ms > (RPA_TIMEOUT_MS(bt_dev.rpa_timeout) - 500);
#else
	return false;
#endif
}

int bt_id_init(void);

uint8_t bt_id_read_public_addr(bt_addr_le_t *addr);

int bt_id_set_create_conn_own_addr(bool use_filter, uint8_t *own_addr_type);

int bt_id_set_scan_own_addr(bool active_scan, uint8_t *own_addr_type);

int bt_id_set_adv_own_addr(struct bt_le_ext_adv *adv, uint32_t options,
			   bool dir_adv, uint8_t *own_addr_type);

bool bt_id_adv_random_addr_check(const struct bt_le_adv_param *param);

bool bt_id_scan_random_addr_check(void);

int bt_id_set_adv_random_addr(struct bt_le_ext_adv *adv,
			      const bt_addr_t *addr);
int bt_id_set_adv_private_addr(struct bt_le_ext_adv *adv);

int bt_id_set_private_addr(uint8_t id);

void bt_id_pending_keys_update(void);
