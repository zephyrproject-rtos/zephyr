/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/addr.h>

/* BT (ID, Address) pair */
struct id_addr_pair {
	uint8_t id;	    /* ID */
	bt_addr_le_t *addr; /* Pointer to the address */
};

/* BT Key (ID, Address, type) info */
struct id_addr_type {
	uint8_t id;	    /*	ID */
	bt_addr_le_t *addr; /*	Pointer to the address	*/
	int type;	    /*	Key type */
};

/* keys.c declarations */
struct bt_keys *bt_keys_get_key_pool(void);
#if defined(CONFIG_BT_KEYS_OVERWRITE_OLDEST)
uint32_t bt_keys_get_aging_counter_val(void);
struct bt_keys *bt_keys_get_last_keys_updated(void);
#endif

/* keys_help_utils.c declarations */
void clear_key_pool(void);
int fill_key_pool_by_id_addr(const struct id_addr_pair src[], int size, struct bt_keys *refs[]);
int fill_key_pool_by_id_addr_type(const struct id_addr_type src[], int size,
				  struct bt_keys *refs[]);
bool check_key_pool_is_empty(void);
