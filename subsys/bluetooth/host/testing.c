/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <stddef.h>

#include <zephyr/bluetooth/testing.h>

#if defined(CONFIG_BT_MESH)
#include "mesh/net.h"
#include "mesh/lpn.h"
#include "mesh/rpl.h"
#include "mesh/transport.h"
#endif /* CONFIG_BT_MESH */

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

#if defined(CONFIG_BT_MESH)
void bt_test_mesh_net_recv(uint8_t ttl, uint8_t ctl, uint16_t src, uint16_t dst,
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

void bt_test_mesh_model_recv(uint16_t src, uint16_t dst, const void *payload,
			     size_t payload_len)
{
	struct bt_test_cb *cb;

	SYS_SLIST_FOR_EACH_CONTAINER(&cb_slist, cb, node) {
		if (cb->mesh_model_recv) {
			cb->mesh_model_recv(src, dst, payload, payload_len);
		}
	}
}

void bt_test_mesh_model_bound(uint16_t addr, const struct bt_mesh_model *model,
			      uint16_t key_idx)
{
	struct bt_test_cb *cb;

	SYS_SLIST_FOR_EACH_CONTAINER(&cb_slist, cb, node) {
		if (cb->mesh_model_bound) {
			cb->mesh_model_bound(addr, model, key_idx);
		}
	}
}

void bt_test_mesh_model_unbound(uint16_t addr, const struct bt_mesh_model *model,
				uint16_t key_idx)
{
	struct bt_test_cb *cb;

	SYS_SLIST_FOR_EACH_CONTAINER(&cb_slist, cb, node) {
		if (cb->mesh_model_unbound) {
			cb->mesh_model_unbound(addr, model, key_idx);
		}
	}
}

void bt_test_mesh_prov_invalid_bearer(uint8_t opcode)
{
	struct bt_test_cb *cb;

	SYS_SLIST_FOR_EACH_CONTAINER(&cb_slist, cb, node) {
		if (cb->mesh_prov_invalid_bearer) {
			cb->mesh_prov_invalid_bearer(opcode);
		}
	}
}

void bt_test_mesh_trans_incomp_timer_exp(void)
{
	struct bt_test_cb *cb;

	SYS_SLIST_FOR_EACH_CONTAINER(&cb_slist, cb, node) {
		if (cb->mesh_trans_incomp_timer_exp) {
			cb->mesh_trans_incomp_timer_exp();
		}
	}
}

#if defined(CONFIG_BT_MESH_LOW_POWER)
int bt_test_mesh_lpn_group_add(uint16_t group)
{
	bt_mesh_lpn_group_add(group);

	return 0;
}

int bt_test_mesh_lpn_group_remove(uint16_t *groups, size_t groups_count)
{
	bt_mesh_lpn_group_del(groups, groups_count);

	return 0;
}
#endif /* CONFIG_BT_MESH_LOW_POWER */

int bt_test_mesh_rpl_clear(void)
{
	bt_mesh_rpl_clear();

	return 0;
}
#endif /* CONFIG_BT_MESH */
