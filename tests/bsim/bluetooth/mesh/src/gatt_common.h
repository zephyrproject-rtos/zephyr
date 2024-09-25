/*
 * Copyright (c) 2023 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mesh_test.h"

enum bt_mesh_test_gatt_service {
	MESH_SERVICE_PROVISIONING,
	MESH_SERVICE_PROXY,
};

struct bt_mesh_test_gatt {
	uint8_t transmits; /* number of frame (pb gatt or proxy beacon) transmits */
	int64_t interval;  /* interval of transmitted frames */
	enum bt_mesh_test_gatt_service service;
};

struct bt_mesh_test_adv {
	uint8_t retr;     /* number of retransmits of adv frame */
	int64_t interval; /* interval of transmitted frames */
};

void bt_mesh_test_parse_mesh_gatt_preamble(struct net_buf_simple *buf);
void bt_mesh_test_parse_mesh_pb_gatt_service(struct net_buf_simple *buf);
void bt_mesh_test_parse_mesh_proxy_service(struct net_buf_simple *buf);
