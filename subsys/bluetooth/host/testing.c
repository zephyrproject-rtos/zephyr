/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <stddef.h>

#include "testing.h"

static sys_slist_t cb_slist;

void bt_test_cb_register(struct bt_test_cb *cb)
{
	sys_slist_append(&cb_slist, &cb->node);
}

void bt_test_cb_unregister(struct bt_test_cb *cb)
{
	sys_slist_find_and_remove(&cb_slist, &cb->node);
}

void bt_test_mesh_net_recv(u8_t ttl, u8_t ctl, u16_t src, u16_t dst,
			   const void *payload, size_t payload_len)
{
	struct bt_test_cb *cb;

	SYS_SLIST_FOR_EACH_CONTAINER(&cb_slist, cb, node) {
		if (cb->mesh_net_recv) {
			cb->mesh_net_recv(ttl, ctl, src, dst, payload,
					  payload_len);
		}
	}
}
