/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest_assert.h>
#include <bluetooth/buf.h>
#include <bluetooth/hci.h>
#include <drivers/bluetooth/hci_driver.h>
#include <sys/byteorder.h>
#include "hci_mock.h"

/* @brief Storage for reference to user defined HCI command handlers. */
static struct {
	const struct hci_cmd_handler *handlers;	/* Pointer to handlers array */
	uint32_t num;				/* Handlers array size */
} user_cmd_handlers;

/* @brief The function is a mock handler for
 *  BT_HCI_OP_READ_LOCAL_FEATURES HCI command.
 *
 * @param[in] opcode	Opcode of request command
 * @param[in] cmd	Pointer to requect command
 * @param[out] evt	Pointer to memory where to store response event
 * @param[in] rp_len	Length of response event
 */
static void read_local_features(uint16_t opcode, struct net_buf *buf,
				struct net_buf **evt, uint8_t rp_len)
{
	struct bt_hci_rp_read_local_features *rp;

	rp = hci_cmd_complete(opcode, evt, sizeof(*rp));
	rp->status = 0x00;
	(void)memset(&rp->features[0], 0xFF, sizeof(rp->features));
}

/* @brief The function is a mock handler for
 *  BT_HCI_OP_READ_SUPPORTED_COMMANDS HCI command.
 *
 * @param[in] opcode	Opcode of request command
 * @param[in] cmd	Pointer to requect command
 * @param[out] evt	Pointer to memory where to store response event
 * @param[in] rp_len	Length of response event
 */
static void read_supported_commands(uint16_t opcode, struct net_buf *buf,
				    struct net_buf **evt, uint8_t rp_len)
{
	struct bt_hci_rp_read_supported_commands *rp;

	rp = hci_cmd_complete(opcode, evt, sizeof(*rp));
	(void)memset(&rp->commands[0], 0xFF, sizeof(rp->commands));
	rp->status = 0x00;
}

/* @brief The function is a mock handler for
 *  BT_HCI_OP_LE_READ_LOCAL_FEATURES HCI command.
 *
 * @param[in] opcode	Opcode of request command
 * @param[in] cmd	Pointer to requect command
 * @param[out] evt	Pointer to memory where to store response event
 * @param[in] rp_len	Length of response event
 */
static void le_read_local_features(uint16_t opcode, struct net_buf *buf,
				   struct net_buf **evt, uint8_t rp_len)
{
	struct bt_hci_rp_le_read_local_features *rp;

	rp = hci_cmd_complete(opcode, evt, sizeof(*rp));
	rp->status = 0x00;
	(void)memset(&rp->features[0], 0xFF, sizeof(rp->features));
}

/* @brief The function is a mock handler for
 *  BT_HCI_OP_LE_READ_SUPP_STATES HCI command.
 *
 * @param[in] opcode	Opcode of request command
 * @param[in] cmd	Pointer to requect command
 * @param[out] evt	Pointer to memory where to store response event
 * @param[in] rp_len	Length of response event
 */
static void le_read_supp_states(uint16_t opcode, struct net_buf *buf,
				struct net_buf **evt, uint8_t rp_len)
{
	struct bt_hci_rp_le_read_supp_states *rp;

	rp = hci_cmd_complete(opcode, evt, sizeof(*rp));
	rp->status = 0x00;
	(void)memset(&rp->le_states, 0xFF, sizeof(rp->le_states));
}

/* @brief Common HCI command handlers required by bt_enable function.
 *
 * Each handler may be overridden by user. Just provide a handler
 * with appropriate opcode of command.
 */
static const struct hci_cmd_handler common_handlers[] = {
	{ BT_HCI_OP_READ_LOCAL_VERSION_INFO,
	  sizeof(struct bt_hci_rp_read_local_version_info),
	  hci_cmd_complete_success },
	{ BT_HCI_OP_READ_SUPPORTED_COMMANDS,
	  sizeof(struct bt_hci_rp_read_supported_commands),
	  read_supported_commands },
	{ BT_HCI_OP_READ_LOCAL_FEATURES,
	  sizeof(struct bt_hci_rp_read_local_features),
	  read_local_features },
	{ BT_HCI_OP_READ_BD_ADDR,
	  sizeof(struct bt_hci_rp_read_bd_addr),
	  hci_cmd_complete_success },
	{ BT_HCI_OP_SET_EVENT_MASK,
	  sizeof(struct bt_hci_evt_cc_status),
	  hci_cmd_complete_success },
	{ BT_HCI_OP_LE_SET_EVENT_MASK,
	  sizeof(struct bt_hci_evt_cc_status),
	  hci_cmd_complete_success },
	{ BT_HCI_OP_LE_READ_LOCAL_FEATURES,
	  sizeof(struct bt_hci_rp_le_read_local_features),
	  le_read_local_features },
	{ BT_HCI_OP_LE_READ_SUPP_STATES,
	  sizeof(struct bt_hci_rp_le_read_supp_states),
	  le_read_supp_states },
	{ BT_HCI_OP_LE_RAND,
	  sizeof(struct bt_hci_rp_le_rand),
	  hci_cmd_complete_success },
	{ BT_HCI_OP_LE_SET_RANDOM_ADDRESS,
	  sizeof(struct bt_hci_cp_le_set_random_address),
	  hci_cmd_complete_success },
};

void hci_set_handlers(const struct hci_cmd_handler *handlers,
		     uint32_t num_handlers)
{
	zassert_true(handlers != NULL, "HCI command handlers pointer is null");
	zassert_true(num_handlers != 0, "HCI command handlers number is zero");

	user_cmd_handlers.handlers = handlers;
	user_cmd_handlers.num = num_handlers;
}

void *hci_cmd_complete(uint16_t opcode, struct net_buf **buf, uint8_t rp_len)
{
	*buf = bt_hci_cmd_complete_create(opcode, rp_len);

	return net_buf_add(*buf, rp_len);
}

void hci_cmd_complete_success(uint16_t opcode, struct net_buf *cmd,
			      struct net_buf **evt, uint8_t rp_len)
{
	struct bt_hci_evt_cc_status *ccst;

	ccst = hci_cmd_complete(opcode, evt, rp_len);

	(void)memset(ccst, 0, rp_len);

	ccst->status = BT_HCI_ERR_SUCCESS;
}

struct net_buf *hci_cmd_handle(struct net_buf *cmd)
{
	struct bt_hci_cmd_hdr *chdr;
	struct net_buf *evt = NULL;
	uint16_t opcode;
	bool cmd_handled;

	if (cmd->len < sizeof(*chdr)) {
		return NULL;
	}

	chdr = net_buf_pull_mem(cmd, sizeof(*chdr));
	if (cmd->len < chdr->param_len) {
		return NULL;
	}

	opcode = sys_le16_to_cpu(chdr->opcode);
	cmd_handled = false;

	const struct hci_cmd_handler *handler;

	if (user_cmd_handlers.handlers != NULL && user_cmd_handlers.num > 0) {
		uint32_t num = user_cmd_handlers.num;

		for (uint32_t handl_idx = 0; handl_idx < num; ++handl_idx) {
			handler = &user_cmd_handlers.handlers[handl_idx];
			if (handler->opcode == opcode) {
				handler->handl_cb(opcode, cmd, &evt,
						  handler->rp_len);
				cmd_handled = true;
				break;
			}
		}
	}

	if (cmd_handled == false) {
		uint32_t num = ARRAY_SIZE(common_handlers);

		for (uint32_t handl_idx = 0; handl_idx < num; ++handl_idx) {
			handler = &common_handlers[handl_idx];
			if (handler->opcode == opcode) {
				handler->handl_cb(opcode, cmd, &evt,
						  handler->rp_len);
				cmd_handled = true;
				break;
			}
		}
	}

	if (cmd_handled == false) {
		evt = bt_hci_cmd_status_create(opcode, BT_HCI_ERR_UNKNOWN_CMD);
	}

	return evt;
}
