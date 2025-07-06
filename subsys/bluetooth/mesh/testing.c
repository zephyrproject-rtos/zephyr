/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/mesh/access.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/slist.h>

#include "net.h"
#include "lpn.h"
#include "rpl.h"
#include "testing.h"
#include "transport.h"

static sys_slist_t cb_slist;

int bt_mesh_test_cb_register(struct bt_mesh_test_cb *cb)
{
	if (sys_slist_find(&cb_slist, &cb->node, NULL)) {
		return -EEXIST;
	}

	sys_slist_append(&cb_slist, &cb->node);

	return 0;
}

void bt_mesh_test_cb_unregister(struct bt_mesh_test_cb *cb)
{
	sys_slist_find_and_remove(&cb_slist, &cb->node);
}

void bt_mesh_test_net_recv(uint8_t ttl, uint8_t ctl, uint16_t src, uint16_t dst,
			   const void *payload, size_t payload_len)
{
	struct bt_mesh_test_cb *cb;

	SYS_SLIST_FOR_EACH_CONTAINER(&cb_slist, cb, node) {
		if (cb->net_recv) {
			cb->net_recv(ttl, ctl, src, dst, payload, payload_len);
		}
	}
}

void bt_mesh_test_model_recv(uint16_t src, uint16_t dst, const void *payload, size_t payload_len)
{
	struct bt_mesh_test_cb *cb;

	SYS_SLIST_FOR_EACH_CONTAINER(&cb_slist, cb, node) {
		if (cb->model_recv) {
			cb->model_recv(src, dst, payload, payload_len);
		}
	}
}

void bt_mesh_test_model_bound(uint16_t addr, const struct bt_mesh_model *model, uint16_t key_idx)
{
	struct bt_mesh_test_cb *cb;

	SYS_SLIST_FOR_EACH_CONTAINER(&cb_slist, cb, node) {
		if (cb->model_bound) {
			cb->model_bound(addr, model, key_idx);
		}
	}
}

void bt_mesh_test_model_unbound(uint16_t addr, const struct bt_mesh_model *model, uint16_t key_idx)
{
	struct bt_mesh_test_cb *cb;

	SYS_SLIST_FOR_EACH_CONTAINER(&cb_slist, cb, node) {
		if (cb->model_unbound) {
			cb->model_unbound(addr, model, key_idx);
		}
	}
}

void bt_mesh_test_prov_invalid_bearer(uint8_t opcode)
{
	struct bt_mesh_test_cb *cb;

	SYS_SLIST_FOR_EACH_CONTAINER(&cb_slist, cb, node) {
		if (cb->prov_invalid_bearer) {
			cb->prov_invalid_bearer(opcode);
		}
	}
}

void bt_mesh_test_trans_incomp_timer_exp(void)
{
	struct bt_mesh_test_cb *cb;

	SYS_SLIST_FOR_EACH_CONTAINER(&cb_slist, cb, node) {
		if (cb->trans_incomp_timer_exp) {
			cb->trans_incomp_timer_exp();
		}
	}
}

#if defined(CONFIG_BT_MESH_LOW_POWER)
int bt_mesh_test_lpn_group_add(uint16_t group)
{
	bt_mesh_lpn_group_add(group);

	return 0;
}

int bt_mesh_test_lpn_group_remove(uint16_t *groups, size_t groups_count)
{
	bt_mesh_lpn_group_del(groups, groups_count);

	return 0;
}
#endif /* CONFIG_BT_MESH_LOW_POWER */

int bt_mesh_test_rpl_clear(void)
{
	bt_mesh_rpl_clear();

	return 0;
}
