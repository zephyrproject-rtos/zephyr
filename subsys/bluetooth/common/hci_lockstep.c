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

/* The helper's state: whether the controller allows a command, and where
 * the command in progress stands. One byte under a short spinlock, with a
 * single semaphore that only signals a change of it: a waiter re-checks the
 * state it waits for, so a stale wakeup or a NOP received while a response
 * is awaited cannot pass for what it waits for.
 */
#define CREDIT_MASK      BIT_MASK(2)
/* The controller allows no command */
#define CREDIT_NONE      0U
/* The controller allows one command */
#define CREDIT_AVAILABLE BIT(0)
/* The allowance went to the command in progress */
#define CREDIT_RESERVED  BIT(1)
/* A command is outstanding, its response not yet received */
#define PHASE_WAITING    BIT(2)
/* The response has been delivered to the sender */
#define PHASE_DONE       BIT(3)

void bt_hci_lockstep_init(struct bt_hci_lockstep *ls, const struct device *dev,
			  bt_hci_lockstep_send_t send)
{
	*ls = (struct bt_hci_lockstep){
		.dev = dev,
		.send = send,
		.opcode = BT_OP_NOP,
		/* Initially the controller allows one command (Core
		 * Specification 6.3, Vol 4, Part E, Section 4.4).
		 */
		.state = CREDIT_AVAILABLE,
		.status = BT_HCI_ERR_SUCCESS,
		.timeout = LOCKSTEP_CMD_TIMEOUT,
	};
	k_sem_init(&ls->changed, 0, 1);
}

bool bt_hci_lockstep_feed(struct bt_hci_lockstep *ls, const uint8_t *pkt, size_t len)
{
	struct bt_hci_pkt_cmd_rsp rsp;
	k_spinlock_key_t key;
	bool consumed = false;
	int err;

	/* A Command Complete without its status (-ENODATA) is still a command
	 * response: rsp then carries BT_HCI_ERR_UNSPECIFIED.
	 */
	err = bt_hci_pkt_parse_cmd_rsp(pkt, len, &rsp);
	if (err != 0 && err != -ENODATA) {
		return false;
	}

	key = k_spin_lock(&ls->lock);

	/* Every command response tells whether the controller allows another
	 * command, whether or not it is the awaited one: the NOP Command
	 * Complete is sent for that alone. The latest word counts.
	 */
	ls->state = (ls->state & ~CREDIT_MASK) |
		    ((rsp.ncmd > 0) ? CREDIT_AVAILABLE : CREDIT_NONE);

	if ((ls->state & PHASE_WAITING) != 0 && rsp.opcode == ls->opcode) {
		ls->status = rsp.status;

		/* Delivered under the lock, so that the sender cannot return
		 * to its caller while the caller's buffer is being written.
		 */
		if (ls->rsp != NULL) {
			net_buf_simple_add_mem(ls->rsp, rsp.rp,
					       MIN(rsp.rp_len, net_buf_simple_tailroom(ls->rsp)));
		}

		ls->state = (ls->state & CREDIT_MASK) | PHASE_DONE;
		consumed = true;
	}

	k_sem_give(&ls->changed);

	k_spin_unlock(&ls->lock, key);

	return consumed;
}

int bt_hci_lockstep_cmd_send_sync(struct bt_hci_lockstep *ls, uint16_t opcode,
				  struct net_buf_simple *cmd, struct net_buf_simple *rsp)
{
	uint8_t no_params[BT_HCI_PKT_CMD_HDR_SIZE];
	struct net_buf_simple no_params_buf;
	k_spinlock_key_t key;
	k_timepoint_t end;
	uint8_t status;
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

	/* Command flow control: a response that allowed no further command
	 * holds this one until the controller allows it, if only with the NOP
	 * Command Complete it sends for that purpose.
	 */
	end = sys_timepoint_calc(ls->timeout);
	while (true) {
		key = k_spin_lock(&ls->lock);

		if ((ls->state & CREDIT_MASK) == CREDIT_AVAILABLE) {
			/* Armed before sending: the response may arrive, and be
			 * fed, before the send function returns.
			 */
			ls->state = CREDIT_RESERVED | PHASE_WAITING;
			ls->opcode = opcode;
			ls->rsp = rsp;
			ls->status = BT_HCI_ERR_SUCCESS;
			if (rsp != NULL) {
				net_buf_simple_reset(rsp);
			}

			k_spin_unlock(&ls->lock, key);
			break;
		}

		k_spin_unlock(&ls->lock, key);

		if (sys_timepoint_expired(end) ||
		    k_sem_take(&ls->changed, sys_timepoint_timeout(end)) != 0) {
			LOG_ERR("opcode 0x%04x: controller allows no command", opcode);
			return -EAGAIN;
		}
	}

	err = ls->send(ls->dev, cmd->data, cmd->len);
	if (err != 0) {
		bool responded;

		key = k_spin_lock(&ls->lock);

		responded = (ls->state & PHASE_DONE) != 0;
		if (!responded) {
			/* Nothing reached the controller: the allowance taken is
			 * intact, unless the controller has spoken since.
			 */
			ls->state &= CREDIT_MASK;
			if (ls->state == CREDIT_RESERVED) {
				ls->state = CREDIT_AVAILABLE;
			}
			ls->rsp = NULL;
		}

		k_spin_unlock(&ls->lock, key);

		if (!responded) {
			LOG_ERR("opcode 0x%04x: send failed (err %d)", opcode, err);
			return err;
		}

		/* The command evidently reached the controller: its response
		 * counts, not the transport's verdict.
		 */
		LOG_WRN("opcode 0x%04x: send failed (err %d) after the response arrived",
			opcode, err);
	}

	end = sys_timepoint_calc(ls->timeout);
	while (true) {
		key = k_spin_lock(&ls->lock);

		if ((ls->state & PHASE_DONE) != 0) {
			status = ls->status;
			ls->state &= CREDIT_MASK;
			ls->rsp = NULL;
			k_spin_unlock(&ls->lock, key);
			break;
		}

		if (sys_timepoint_expired(end)) {
			/* The unanswered command used up the allowance: the next
			 * one waits for the controller to allow it again.
			 */
			ls->state &= CREDIT_MASK;
			ls->rsp = NULL;
			k_spin_unlock(&ls->lock, key);
			LOG_ERR("opcode 0x%04x: no response", opcode);
			return -EAGAIN;
		}

		k_spin_unlock(&ls->lock, key);

		/* A stale wakeup or a NOP only leads back to the check */
		(void)k_sem_take(&ls->changed, sys_timepoint_timeout(end));
	}

	if (status != BT_HCI_ERR_SUCCESS) {
		LOG_ERR("opcode 0x%04x: status 0x%02x", opcode, status);
		return -EIO;
	}

	return 0;
}
