/* hci_pkt.c - Bluetooth HCI packet helpers */

/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/hci_pkt.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>

void bt_hci_pkt_reset_cmd(struct net_buf_simple *buf)
{
	__ASSERT_NO_MSG(buf->size >= BT_HCI_PKT_CMD_HDR_SIZE);

	net_buf_simple_reset(buf);
	net_buf_simple_reserve(buf, BT_HCI_PKT_CMD_HDR_SIZE);
}

int bt_hci_pkt_push_cmd_hdr(struct net_buf_simple *buf, uint16_t opcode)
{
	struct bt_hci_cmd_hdr *hdr;

	if (net_buf_simple_headroom(buf) < BT_HCI_PKT_CMD_HDR_SIZE) {
		return -EINVAL;
	}

	if (buf->len > UINT8_MAX) {
		return -EMSGSIZE;
	}

	hdr = (struct bt_hci_cmd_hdr *)net_buf_simple_push(buf, sizeof(*hdr));
	hdr->opcode = sys_cpu_to_le16(opcode);
	hdr->param_len = buf->len - sizeof(*hdr);

	net_buf_simple_push_u8(buf, BT_HCI_H4_CMD);

	return 0;
}

/* Decode the event parameters of an HCI_Command_Complete event */
static int decode_cmd_complete(const uint8_t *params, size_t len, struct bt_hci_pkt_cmd_rsp *rsp)
{
	const struct bt_hci_evt_cmd_complete *evt;

	if (len < sizeof(*evt)) {
		return -EINVAL;
	}

	evt = (const struct bt_hci_evt_cmd_complete *)params;
	rsp->opcode = sys_le16_to_cpu(evt->opcode);
	rsp->ncmd = evt->ncmd;
	rsp->rp = &params[sizeof(*evt)];
	rsp->rp_len = len - sizeof(*evt);

	/* Only an event that responds to no command (NOP) carries no return
	 * parameters; those of every command start with the status.
	 */
	if (rsp->opcode == BT_OP_NOP) {
		rsp->status = BT_HCI_ERR_SUCCESS;
		return 0;
	}

	if (rsp->rp_len == 0) {
		rsp->status = BT_HCI_ERR_UNSPECIFIED;
		return -ENODATA;
	}

	rsp->status = rsp->rp[0];

	return 0;
}

int bt_hci_pkt_pull_cmd_complete(struct net_buf_simple *buf, struct bt_hci_pkt_cmd_rsp *rsp)
{
	int err;

	err = decode_cmd_complete(buf->data, buf->len, rsp);
	if (err != 0 && err != -ENODATA) {
		return err;
	}

	net_buf_simple_pull(buf, sizeof(struct bt_hci_evt_cmd_complete));

	return err;
}

/* Decode the event parameters of an HCI_Command_Status event */
static int decode_cmd_status(const uint8_t *params, size_t len, struct bt_hci_pkt_cmd_rsp *rsp)
{
	const struct bt_hci_evt_cmd_status *evt;

	if (len < sizeof(*evt)) {
		return -EINVAL;
	}

	evt = (const struct bt_hci_evt_cmd_status *)params;
	rsp->opcode = sys_le16_to_cpu(evt->opcode);
	rsp->ncmd = evt->ncmd;
	rsp->status = evt->status;
	rsp->rp = &params[sizeof(*evt)];
	rsp->rp_len = 0;

	return 0;
}

int bt_hci_pkt_pull_cmd_status(struct net_buf_simple *buf, struct bt_hci_pkt_cmd_rsp *rsp)
{
	int err;

	err = decode_cmd_status(buf->data, buf->len, rsp);
	if (err != 0) {
		return err;
	}

	net_buf_simple_pull(buf, sizeof(struct bt_hci_evt_cmd_status));

	return 0;
}

int bt_hci_pkt_parse_cmd_rsp(const uint8_t *pkt, size_t len, struct bt_hci_pkt_cmd_rsp *rsp)
{
	const struct bt_hci_evt_hdr *hdr;
	const uint8_t *params;

	if (len < sizeof(uint8_t) || pkt[0] != BT_HCI_H4_EVT) {
		return -ENOMSG;
	}

	if (len < sizeof(uint8_t) + sizeof(*hdr)) {
		return -EINVAL;
	}

	hdr = (const struct bt_hci_evt_hdr *)&pkt[sizeof(uint8_t)];
	if (hdr->len > len - sizeof(uint8_t) - sizeof(*hdr)) {
		return -EINVAL;
	}

	params = &pkt[sizeof(uint8_t) + sizeof(*hdr)];

	switch (hdr->evt) {
	case BT_HCI_EVT_CMD_COMPLETE:
		return decode_cmd_complete(params, hdr->len, rsp);
	case BT_HCI_EVT_CMD_STATUS:
		return decode_cmd_status(params, hdr->len, rsp);
	default:
		return -ENOMSG;
	}
}
