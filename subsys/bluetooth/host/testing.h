/**
 * @file testing.h
 * @brief Internal API for Bluetooth testing.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct bt_test_cb {
	void (*mesh_net_recv)(u8_t ttl, u8_t ctl, u16_t src, u16_t dst,
			      const void *payload, size_t payload_len);

	sys_snode_t node;
};

void bt_test_cb_register(struct bt_test_cb *cb);
void bt_test_cb_unregister(struct bt_test_cb *cb);

void bt_test_mesh_net_recv(u8_t ttl, u8_t ctl, u16_t src, u16_t dst,
			   const void *payload, size_t payload_len);
