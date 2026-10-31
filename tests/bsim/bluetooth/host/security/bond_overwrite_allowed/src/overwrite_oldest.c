/*
 * Copyright (c) 2026 Mark Tamas
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>

#include "bs_bt_utils.h"

struct bond_list {
	bt_addr_le_t addrs[CONFIG_BT_MAX_PAIRED];
	size_t count;
};

static void bond_list_collect(const struct bt_bond_info *info, void *user_data)
{
	struct bond_list *list = user_data;

	if (list->count < ARRAY_SIZE(list->addrs)) {
		bt_addr_le_copy(&list->addrs[list->count], &info->addr);
	}
	list->count++;
}

static bool bond_list_contains(const struct bond_list *list, const bt_addr_le_t *addr)
{
	size_t n = MIN(list->count, ARRAY_SIZE(list->addrs));

	for (size_t i = 0; i < n; i++) {
		if (bt_addr_le_eq(&list->addrs[i], addr)) {
			return true;
		}
	}

	return false;
}

/* Bond with the currently advertising peer and return its address. */
static bt_addr_le_t bond_with_next_identity(const char *phase)
{
	bt_addr_le_t peer;

	printk("%s\n", phase);
	scan_connect_to_first_result();
	wait_connected();
	set_security(BT_SECURITY_L2);
	TAKE_FLAG(flag_pairing_complete);
	peer = *bt_conn_get_dst(g_conn);
	disconnect();
	wait_disconnected();
	clear_g_conn();

	return peer;
}

void central_overwrite_oldest(void)
{
	struct bond_list bonds = {0};
	bt_addr_le_t id_a, id_b, id_c, id_d;

	bs_bt_utils_setup();

	id_a = bond_with_next_identity("== Bonding id a ==");
	id_b = bond_with_next_identity("== Bonding id b ==");
	id_c = bond_with_next_identity("== Bonding id c ==");
	id_d = bond_with_next_identity("== Bonding id d ==");

	printk("== Verifying the eviction through the public bond API ==\n");
	bt_foreach_bond(BT_ID_DEFAULT, bond_list_collect, &bonds);
	TEST_ASSERT(bonds.count == 2, "Expected 2 bonds after eviction, got %zu", bonds.count);
	TEST_ASSERT(!bond_list_contains(&bonds, &id_a), "The oldest bond (id a) was not evicted");
	TEST_ASSERT(!bond_list_contains(&bonds, &id_b),
		    "The second oldest bond (id b) was not evicted");
	TEST_ASSERT(bond_list_contains(&bonds, &id_c), "Bond for id c is missing");
	TEST_ASSERT(bond_list_contains(&bonds, &id_d), "Bond for id d is missing");

	TEST_PASS("PASS");
}

/* Advertise as the given identity until a peer has bonded and left. */
static void advertise_and_pair_once(int id, const char *phase)
{
	printk("%s\n", phase);
	advertise_connectable(id, NULL);
	wait_connected();
	wait_disconnected();
	clear_g_conn();
}

void peripheral_overwrite_oldest(void)
{
	int id_a, id_b, id_c, id_d;

	bs_bt_utils_setup();

	id_a = bt_id_create(NULL, NULL);
	TEST_ASSERT(id_a >= 0, "bt_id_create id_a failed (err %d)", id_a);
	id_b = bt_id_create(NULL, NULL);
	TEST_ASSERT(id_b >= 0, "bt_id_create id_b failed (err %d)", id_b);
	id_c = bt_id_create(NULL, NULL);
	TEST_ASSERT(id_c >= 0, "bt_id_create id_c failed (err %d)", id_c);
	id_d = bt_id_create(NULL, NULL);
	TEST_ASSERT(id_d >= 0, "bt_id_create id_d failed (err %d)", id_d);

	advertise_and_pair_once(id_a, "== Bonding id a ==");
	advertise_and_pair_once(id_b, "== Bonding id b ==");
	advertise_and_pair_once(id_c, "== Bonding id c ==");
	advertise_and_pair_once(id_d, "== Bonding id d ==");

	TEST_PASS("PASS");
}
