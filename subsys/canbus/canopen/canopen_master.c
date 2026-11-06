/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/sys/byteorder.h>

#include "canopen_internal.h"

#define MASTER_NODE_ID_MAX   127U
#define MASTER_OD_DEVICE_TYPE 0x1000U

static bool master_cmd_valid(enum canopen_nmt_cmd cmd)
{
	switch (cmd) {
	case CANOPEN_NMT_CMD_START:
	case CANOPEN_NMT_CMD_STOP:
	case CANOPEN_NMT_CMD_ENTER_PRE_OPERATIONAL:
	case CANOPEN_NMT_CMD_RESET_NODE:
	case CANOPEN_NMT_CMD_RESET_COMMUNICATION:
		return true;
	default:
		return false;
	}
}

int canopen_nmt_send(struct canopen_node *node, uint8_t node_id, enum canopen_nmt_cmd cmd)
{
	struct can_frame frame = {
		.id = CANOPEN_COB_NMT,
		.dlc = 2U,
		.data = {(uint8_t)cmd, node_id},
	};
	int err;

	if ((node_id > MASTER_NODE_ID_MAX) || !master_cmd_valid(cmd)) {
		return -EINVAL;
	}

	k_mutex_lock(&node->lock, K_FOREVER);
	err = canopen_send(node, &frame);
	k_mutex_unlock(&node->lock);

	return (err == 0) ? 0 : -EIO;
}

static int master_boot_slave(struct canopen_node *node, const struct canopen_boot_slave *slave,
			     k_timeout_t timeout)
{
	k_timepoint_t end = sys_timepoint_calc(timeout);
	uint8_t value[sizeof(uint32_t)];
	size_t len;
	int err;

	do {
		len = sizeof(value);
		err = canopen_sdo_upload(node, slave->node_id, MASTER_OD_DEVICE_TYPE, 0U, value,
					 &len, NULL);
	} while ((err == -ETIMEDOUT) && !sys_timepoint_expired(end));

	if (err != 0) {
		return err;
	}

	for (uint8_t i = 0U; i < slave->config_count; i++) {
		const struct canopen_boot_entry *entry = &slave->config[i];

		if ((entry->len == 0U) || (entry->len > sizeof(value))) {
			return -EINVAL;
		}

		sys_put_le32(entry->value, value);
		err = canopen_sdo_download(node, slave->node_id, entry->index, entry->sub, value,
					   entry->len, NULL);
		if (err != 0) {
			return err;
		}
	}

	return canopen_nmt_send(node, slave->node_id, CANOPEN_NMT_CMD_START);
}

int canopen_master_boot(struct canopen_node *node, const struct canopen_boot_slave *slaves,
			size_t count, k_timeout_t timeout)
{
	int err;

	for (size_t i = 0U; i < count; i++) {
		if ((slaves[i].node_id == 0U) || (slaves[i].node_id > MASTER_NODE_ID_MAX)) {
			return -EINVAL;
		}

		err = master_boot_slave(node, &slaves[i], timeout);
		if (err != 0) {
			return err;
		}
	}

	return 0;
}
