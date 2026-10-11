/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>

#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/services/nus.h>
#include <zephyr/ztest.h>

BT_NUS_INST_DEFINE(test_nus);

static uint16_t nus_perm(const struct bt_uuid *uuid)
{
	for (size_t i = 0; i < test_nus.svc->attr_count; i++) {
		if (bt_uuid_cmp(test_nus.svc->attrs[i].uuid, uuid) == 0) {
			return test_nus.svc->attrs[i].perm;
		}
	}

	zassert_unreachable("attribute not found");
	return 0;
}

ZTEST_SUITE(nus_perm, NULL, NULL, NULL, NULL, NULL);

static ZTEST(nus_perm, test_ccc)
{
	uint16_t expect = BT_GATT_PERM_READ;

	if (IS_ENABLED(CONFIG_BT_ZEPHYR_NUS_SECURITY_AUTHEN)) {
		expect |= BT_GATT_PERM_READ_AUTHEN | BT_GATT_PERM_WRITE_AUTHEN;
	} else if (IS_ENABLED(CONFIG_BT_ZEPHYR_NUS_SECURITY_ENCRYPT)) {
		expect |= BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT;
	} else {
		expect |= BT_GATT_PERM_WRITE;
	}

	zassert_equal(nus_perm(BT_UUID_GATT_CCC), expect);
}

static ZTEST(nus_perm, test_tx)
{
	uint16_t expect = BT_GATT_PERM_NONE;

	if (IS_ENABLED(CONFIG_BT_ZEPHYR_NUS_SECURITY_AUTHEN)) {
		expect = BT_GATT_PERM_READ_AUTHEN;
	} else if (IS_ENABLED(CONFIG_BT_ZEPHYR_NUS_SECURITY_ENCRYPT)) {
		expect = BT_GATT_PERM_READ_ENCRYPT;
	}

	zassert_equal(nus_perm(BT_UUID_NUS_TX_CHAR), expect);
}

static ZTEST(nus_perm, test_rx)
{
	uint16_t expect = BT_GATT_PERM_WRITE;

	if (IS_ENABLED(CONFIG_BT_ZEPHYR_NUS_SECURITY_AUTHEN)) {
		expect = BT_GATT_PERM_WRITE_AUTHEN;
	} else if (IS_ENABLED(CONFIG_BT_ZEPHYR_NUS_SECURITY_ENCRYPT)) {
		expect = BT_GATT_PERM_WRITE_ENCRYPT;
	}

	zassert_equal(nus_perm(BT_UUID_NUS_RX_CHAR), expect);
}
