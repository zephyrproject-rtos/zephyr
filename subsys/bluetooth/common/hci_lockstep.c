/* hci_lockstep.c - Bluetooth HCI lockstep command helper */

/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/hci_lockstep.h>
#include <zephyr/bluetooth/hci_pkt.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

/* The log level symbol exists only in builds with the Bluetooth Kconfig
 * tree; the helper also builds without it (see tests/bluetooth/hci_pkt).
 */
#if defined(CONFIG_BT_HCI_DRIVER_LOG_LEVEL)
#define LOG_LEVEL CONFIG_BT_HCI_DRIVER_LOG_LEVEL
#endif
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_hci_lockstep);

/* The Bluetooth Host's own command timeout */
#define LOCKSTEP_CMD_TIMEOUT K_SECONDS(10)

void bt_hci_lockstep_init(struct bt_hci_lockstep *ls, const struct device *dev,
			  bt_hci_lockstep_send_t send)
{
	ls->status = BT_HCI_ERR_SUCCESS;
	ls->timeout = LOCKSTEP_CMD_TIMEOUT;
	ls->dev = dev;
	ls->send = send;
	ls->rsp = NULL;
	ls->opcode = BT_OP_NOP;
	atomic_clear(&ls->waiting);
	k_sem_init(&ls->rsp_sem, 0, 1);
}

bool bt_hci_lockstep_feed(struct bt_hci_lockstep *ls, const uint8_t *pkt, size_t len)
{
	struct bt_hci_pkt_cmd_rsp rsp;

	if (!atomic_get(&ls->waiting)) {
		return false;
	}

	if (bt_hci_pkt_parse_cmd_rsp(pkt, len, &rsp) != 0 || rsp.opcode != ls->opcode) {
		return false;
	}

	/* Claim the response: either this call or a waiter giving up on its
	 * timeout disarms the helper, never both.
	 */
	if (!atomic_cas(&ls->waiting, 1, 0)) {
		return false;
	}

	ls->status = rsp.status;

	if (ls->rsp != NULL) {
		net_buf_simple_add_mem(ls->rsp, rsp.rp,
				       MIN(rsp.rp_len, net_buf_simple_tailroom(ls->rsp)));
	}

	k_sem_give(&ls->rsp_sem);

	return true;
}

int bt_hci_lockstep_cmd_send_sync(struct bt_hci_lockstep *ls, uint16_t opcode,
				  struct net_buf_simple *cmd, struct net_buf_simple *rsp)
{
	uint8_t no_params[BT_HCI_PKT_CMD_HDR_SIZE];
	struct net_buf_simple no_params_buf;
	int err;

	if (cmd == NULL) {
		net_buf_simple_init_with_data(&no_params_buf, no_params, sizeof(no_params));
		bt_hci_pkt_reset_cmd(&no_params_buf);
		cmd = &no_params_buf;
	}

	err = bt_hci_pkt_push_cmd_hdr(cmd, opcode);
	if (err != 0) {
		LOG_ERR("opcode 0x%04x: invalid command buffer (err %d)", opcode, err);
		return err;
	}

	if (rsp != NULL) {
		net_buf_simple_reset(rsp);
	}

	ls->opcode = opcode;
	ls->rsp = rsp;
	ls->status = BT_HCI_ERR_SUCCESS;
	k_sem_reset(&ls->rsp_sem);

	/* Armed before sending: the response may arrive, and be fed, before
	 * the send function returns.
	 */
	atomic_set(&ls->waiting, 1);

	err = ls->send(ls->dev, cmd->data, cmd->len);
	if (err != 0) {
		atomic_clear(&ls->waiting);
		ls->rsp = NULL;
		LOG_ERR("opcode 0x%04x: send failed (err %d)", opcode, err);
		return err;
	}

	err = k_sem_take(&ls->rsp_sem, ls->timeout);
	if (err != 0) {
		if (atomic_cas(&ls->waiting, 1, 0)) {
			ls->rsp = NULL;
			LOG_ERR("opcode 0x%04x: no response", opcode);
			return -EAGAIN;
		}

		/* bt_hci_lockstep_feed() claimed the response just as the
		 * wait expired: let it finish delivering.
		 */
		k_sem_take(&ls->rsp_sem, K_FOREVER);
	}

	ls->rsp = NULL;

	if (ls->status != BT_HCI_ERR_SUCCESS) {
		LOG_ERR("opcode 0x%04x: status 0x%02x", opcode, ls->status);
		return -EIO;
	}

	return 0;
}
