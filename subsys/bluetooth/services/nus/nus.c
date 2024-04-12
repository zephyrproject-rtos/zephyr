/*
 * Copyright (c) 2024 Croxel, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/services/nus.h>
#include "nus_internal.h"

ssize_t nus_bt_chr_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
	const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	struct bt_nus_cb *listener = NULL;
	struct bt_nus_inst *instance = NULL;

	instance = bt_nus_inst_get_from_attr(attr);
	__ASSERT_NO_MSG(instance);

	SYS_SLIST_FOR_EACH_CONTAINER(instance->cbs, listener, _node) {
		if (listener->received) {
			listener->received(conn, buf, len, listener->ctx);
		}
	}

	return len;
}

void nus_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	struct bt_nus_cb *listener = NULL;
	struct bt_nus_inst *instance = NULL;

	instance = bt_nus_inst_get_from_attr(attr);
	__ASSERT_NO_MSG(instance);

	SYS_SLIST_FOR_EACH_CONTAINER(instance->cbs, listener, _node) {
		if (listener->notif_enabled) {
			listener->notif_enabled((value == 1), listener->ctx);
		}
	}
}

int bt_nus_inst_cb_register(struct bt_nus_inst *instance, struct bt_nus_cb *cb, void *ctx)
{
	if (!cb) {
		return -EINVAL;
	}

	if (!instance) {
		if (IS_ENABLED(CONFIG_BT_ZEPHYR_NUS_DEFAULT_INSTANCE)) {
			instance = bt_nus_inst_default();
		} else {
			return -ENOTSUP;
		}
	}

	cb->ctx = ctx;
	sys_slist_append(instance->cbs, &cb->_node);

	return 0;
}

int bt_nus_inst_send(struct bt_conn *conn,
		     struct bt_nus_inst *instance,
		     const void *data,
		     uint16_t len)
{
	if (!data || !len) {
		return -EINVAL;
	}

	if (!instance) {
		if (IS_ENABLED(CONFIG_BT_ZEPHYR_NUS_DEFAULT_INSTANCE)) {
			instance = bt_nus_inst_default();
		} else {
			return -ENOTSUP;
		}
	}

	return bt_gatt_notify(conn, &instance->svc->attrs[1], data, len);
}
