/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <bluetooth/addr.h>

/* BT (ID, Address) pair */
struct id_addr_pair {
	uint8_t id;			    /* ID */
	bt_addr_le_t *addr;		/* Pointer to the address */
};

/* keys.c declarations */
struct bt_keys *bt_keys_get_key_pool(void);
#if IS_ENABLED(CONFIG_BT_KEYS_OVERWRITE_OLDEST)
uint32_t bt_keys_get_aging_counter_val(void);
#endif

/* keys_help_utils.c declarations */
void clear_key_pool(void);
int fill_key_pool_by_id_addr(const struct id_addr_pair src[], int size, struct bt_keys *refs[]);
bool check_key_pool_is_empty(void);

/* Repeat test entries */
#define REGISTER_SETUP_TEARDOWN(i, ...) \
	ztest_unit_test_setup_teardown(__VA_ARGS__, unit_test_setup, unit_test_noop)

#define ztest_unit_test_setup(fn, setup) \
	ztest_unit_test_setup_teardown(fn, setup, unit_test_noop)
