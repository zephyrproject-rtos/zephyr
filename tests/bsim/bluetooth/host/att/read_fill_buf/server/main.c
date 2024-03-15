/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <argparse.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>

#include <testlib/adv.h>

#include "../bs_macro.h"
#include "../common_defs.h"

LOG_MODULE_REGISTER(server, LOG_LEVEL_DBG);

static ssize_t read_mtu_validation_chrc(struct bt_conn *conn, const struct bt_gatt_attr *attr,
					void *buf, uint16_t buf_len, uint16_t offset)
{
	LOG_INF("buf_len %u", buf_len);

	/* Fill the requested read size. */
	memset(buf, 0, buf_len);

	/* Echo back the requested read size in the first two bytes. */
	sys_put_le16(buf_len, buf);

	return buf_len;
}

BT_GATT_SERVICE_DEFINE(long_attr_svc, BT_GATT_PRIMARY_SERVICE(MTU_VALIDATION_SVC),
		       BT_GATT_CHARACTERISTIC(MTU_VALIDATION_CHRC, BT_GATT_CHRC_READ,
					      BT_GATT_PERM_READ, read_mtu_validation_chrc, NULL,
					      NULL));

void the_test(void)
{
	int err;

	err = bt_enable(NULL);
	__ASSERT_NO_MSG(!err);

	__ASSERT_NO_MSG(get_device_nbr() == 1);
	err = bt_set_name("d1");
	__ASSERT_NO_MSG(!err);

	err = bt_testlib_adv_conn(NULL, BT_ID_DEFAULT,
				  (BT_LE_ADV_OPT_USE_NAME | BT_LE_ADV_OPT_FORCE_NAME_IN_AD));
	__ASSERT_NO_MSG(!err);

	PASS("PASS\n");
}
