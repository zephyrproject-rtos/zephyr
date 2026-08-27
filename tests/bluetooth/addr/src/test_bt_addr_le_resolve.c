/* Copyright (c) 2026 Demant A/S
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/ztest.h>

#include <common/rpa.h>
#include <host/keys.h>

static void bt_addr_le_rpa_resolve_before(void *f)
{
	ARG_UNUSED(f);

	bt_keys_reset();
}

ZTEST_SUITE(bt_addr_le_rpa_resolve, NULL, NULL, bt_addr_le_rpa_resolve_before, NULL, NULL);

static ZTEST(bt_addr_le_rpa_resolve, test_null_args)
{
	const bt_addr_le_t addr = {
		.type = BT_ADDR_LE_PUBLIC,
		.a = {{0x06, 0x05, 0x04, 0x03, 0x02, 0x01}},
	};
	bt_addr_le_t resolved;

	zassert_equal(bt_addr_le_rpa_resolve(BT_ID_DEFAULT, NULL, &resolved), -EINVAL);
	zassert_equal(bt_addr_le_rpa_resolve(BT_ID_DEFAULT, &addr, NULL), -EINVAL);
}

static ZTEST(bt_addr_le_rpa_resolve, test_invalid_id_is_rejected)
{
	const bt_addr_le_t input = {
		.type = BT_ADDR_LE_RANDOM,
		.a = {{0x01, 0x02, 0x03, 0x04, 0x05, 0x46}},
	};
	bt_addr_le_t resolved;

	zassert_true(bt_addr_le_is_rpa(&input));
	zassert_equal(bt_addr_le_rpa_resolve(CONFIG_BT_ID_MAX, &input, &resolved), -EINVAL);
}

static ZTEST(bt_addr_le_rpa_resolve, test_rpa_resolution_no_matching_irk)
{
	const bt_addr_le_t input = {
		.type = BT_ADDR_LE_RANDOM,
		.a = {{0x01, 0x02, 0x03, 0x04, 0x05, 0x46}},
	};
	bt_addr_le_t resolved;

	zassert_true(bt_addr_le_is_rpa(&input));
	zassert_equal(bt_addr_le_rpa_resolve(BT_ID_DEFAULT, &input, &resolved), -ENOENT);
}

static ZTEST(bt_addr_le_rpa_resolve, test_non_rpa_random_is_rejected)
{
	const bt_addr_le_t input = {
		.type = BT_ADDR_LE_RANDOM,
		.a = {{0x01, 0x02, 0x03, 0x04, 0x05, 0x06}},
	};
	bt_addr_le_t resolved;

	zassert_false(bt_addr_le_is_rpa(&input));
	zassert_equal(bt_addr_le_rpa_resolve(BT_ID_DEFAULT, &input, &resolved), -EINVAL);
}

static ZTEST(bt_addr_le_rpa_resolve, test_identity_is_rejected)
{
	const bt_addr_le_t input = {
		.type = BT_ADDR_LE_PUBLIC,
		.a = {{0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6}},
	};
	bt_addr_le_t resolved;

	zassert_equal(bt_addr_le_rpa_resolve(BT_ID_DEFAULT, &input, &resolved), -EINVAL);
}

static ZTEST(bt_addr_le_rpa_resolve, test_valid_irk_resolves_rpa_to_identity)
{
	const uint8_t irk[16] = {
		0x10, 0x32, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe,
		0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
	};
	const bt_addr_le_t identity = {
		.type = BT_ADDR_LE_PUBLIC,
		.a = {{0xc6, 0xc5, 0xc4, 0xc3, 0xc2, 0xc1}},
	};
	bt_addr_t rpa;
	bt_addr_le_t input = {
		.type = BT_ADDR_LE_RANDOM,
	};
	bt_addr_le_t resolved;
	struct bt_keys *keys;

	keys = bt_keys_get_addr(BT_ID_DEFAULT, &identity);
	zassert_not_null(keys, "Failed to allocate key entry");

	bt_keys_add_type(keys, BT_KEYS_IRK);
	(void)memcpy(keys->irk.val, irk, sizeof(irk));

	zassert_equal(bt_rpa_create(irk, &rpa), 0, "Failed to generate RPA");
	bt_addr_copy(&input.a, &rpa);
	zassert_true(bt_addr_le_is_rpa(&input), "Generated address is not an RPA");

	zassert_equal(bt_addr_le_rpa_resolve(BT_ID_DEFAULT, &input, &resolved), 0);
	zassert_true(bt_addr_le_eq(&resolved, &identity));
}

static ZTEST(bt_addr_le_rpa_resolve, test_rpa_for_different_identity_is_not_resolved)
{
	const uint8_t irk[16] = {
		0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
		0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xf0, 0x01,
	};
	const bt_addr_le_t identity = {
		.type = BT_ADDR_LE_PUBLIC,
		.a = {{0xd6, 0xd5, 0xd4, 0xd3, 0xd2, 0xd1}},
	};
	bt_addr_t rpa;
	bt_addr_le_t input = {
		.type = BT_ADDR_LE_RANDOM,
	};
	bt_addr_le_t resolved;
	struct bt_keys *keys;

	keys = bt_keys_get_addr(1, &identity);
	zassert_not_null(keys, "Failed to allocate key entry");

	bt_keys_add_type(keys, BT_KEYS_IRK);
	(void)memcpy(keys->irk.val, irk, sizeof(irk));

	zassert_equal(bt_rpa_create(irk, &rpa), 0, "Failed to generate RPA");
	bt_addr_copy(&input.a, &rpa);
	zassert_true(bt_addr_le_is_rpa(&input), "Generated address is not an RPA");

	zassert_equal(bt_addr_le_rpa_resolve(BT_ID_DEFAULT, &input, &resolved), -ENOENT);
}
