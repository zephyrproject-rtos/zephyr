/*
 * Copyright (c) 2023 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "gatt_common.h"
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gatt_common);

void bt_mesh_test_parse_mesh_gatt_preamble(struct net_buf_simple *buf)
{
	ASSERT_EQUAL(0x0201, net_buf_simple_pull_be16(buf));
	/* flags */
	(void)net_buf_simple_pull_u8(buf);
	ASSERT_EQUAL(0x0303, net_buf_simple_pull_be16(buf));
}

void bt_mesh_test_parse_mesh_pb_gatt_service(struct net_buf_simple *buf)
{
	/* MshPRT Figure 7.1: PB-GATT Advertising Data */
	/* mesh provisioning service */
	ASSERT_EQUAL(0x2718, net_buf_simple_pull_be16(buf));
	ASSERT_EQUAL(0x1516, net_buf_simple_pull_be16(buf));
	/* mesh provisioning service */
	ASSERT_EQUAL(0x2718, net_buf_simple_pull_be16(buf));
}

void bt_mesh_test_parse_mesh_proxy_service(struct net_buf_simple *buf)
{
	/* MshPRT Figure 7.2: Advertising with Network ID (Identification Type 0x00) */
	/* mesh proxy service */
	ASSERT_EQUAL(0x2818, net_buf_simple_pull_be16(buf));
	ASSERT_EQUAL(0x0c16, net_buf_simple_pull_be16(buf));
	/* mesh proxy service */
	ASSERT_EQUAL(0x2818, net_buf_simple_pull_be16(buf));
	/* network ID */
	ASSERT_EQUAL(0x00, net_buf_simple_pull_u8(buf));
}
