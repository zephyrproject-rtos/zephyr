/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>
#include <string.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/net_buf.h>

#include "babblekit/testcase.h"

#include "btp/btp.h"

void bsim_btp_uart_init(void);
void bsim_btp_send_to_tester(const uint8_t *data, size_t len);
void bsim_btp_wait_for_evt(uint8_t service, uint8_t opcode, struct net_buf **out_buf);

static inline void bsim_btp_core_register(uint8_t id)
{
	struct btp_core_register_service_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_CORE;
	cmd_hdr->opcode = BTP_CORE_REGISTER_SERVICE;
	cmd_hdr->index = BTP_INDEX_NONE;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	cmd->id = id;

	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_gap_set_discoverable(uint8_t discoverable)
{
	struct btp_gap_set_discoverable_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_GAP;
	cmd_hdr->opcode = BTP_GAP_SET_DISCOVERABLE;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	cmd->discoverable = discoverable;

	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_gap_start_advertising(uint8_t adv_data_len, uint8_t scan_rsp_len,
						  const uint8_t adv_sr_data[],
						  uint8_t own_addr_type)
{
	struct btp_gap_start_advertising_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_GAP;
	cmd_hdr->opcode = BTP_GAP_START_ADVERTISING;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	cmd->adv_data_len = adv_data_len;
	cmd->scan_rsp_len = scan_rsp_len;
	if (adv_data_len > 0U) {
		net_buf_simple_add_mem(&cmd_buffer, adv_sr_data, adv_data_len);
	}
	if (scan_rsp_len > 0U) {
		net_buf_simple_add_mem(&cmd_buffer, adv_sr_data, scan_rsp_len);
	}
	net_buf_simple_add_le32(&cmd_buffer, 0xFFFF); /* unused in Zephyr */
	net_buf_simple_add_u8(&cmd_buffer, own_addr_type);

	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_gap_start_discovery(uint8_t flags)
{
	struct btp_gap_start_discovery_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_GAP;
	cmd_hdr->opcode = BTP_GAP_START_DISCOVERY;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	cmd->flags = flags;

	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_gap_stop_discovery(void)
{
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_GAP;
	cmd_hdr->opcode = BTP_GAP_STOP_DISCOVERY;
	cmd_hdr->index = BTP_INDEX;
	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_wait_for_gap_device_found(bt_addr_le_t *address)
{
	struct btp_gap_device_found_ev *ev;
	struct net_buf *buf;

	bsim_btp_wait_for_evt(BTP_SERVICE_ID_GAP, BTP_GAP_EV_DEVICE_FOUND, &buf);
	ev = net_buf_pull_mem(buf, sizeof(*ev));
	if (address != NULL) {
		bt_addr_le_copy(address, &ev->address);
	}

	net_buf_unref(buf);
}

static inline void bsim_btp_gap_connect(const bt_addr_le_t *address, uint8_t own_addr_type)
{
	struct btp_gap_connect_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_GAP;
	cmd_hdr->opcode = BTP_GAP_CONNECT;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	bt_addr_le_copy(&cmd->address, address);
	cmd->own_addr_type = own_addr_type;

	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_wait_for_gap_device_connected(bt_addr_le_t *address)
{
	struct btp_gap_device_connected_ev *ev;
	struct net_buf *buf;

	bsim_btp_wait_for_evt(BTP_SERVICE_ID_GAP, BTP_GAP_EV_DEVICE_CONNECTED, &buf);
	ev = net_buf_pull_mem(buf, sizeof(*ev));
	if (address != NULL) {
		bt_addr_le_copy(address, &ev->address);
	}

	net_buf_unref(buf);
}

static inline void bsim_btp_gap_disconnect(const bt_addr_le_t *address)
{
	struct btp_gap_disconnect_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_GAP;
	cmd_hdr->opcode = BTP_GAP_DISCONNECT;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	bt_addr_le_copy(&cmd->address, address);

	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_wait_for_gap_device_disconnected(bt_addr_le_t *address)
{
	struct btp_gap_device_disconnected_ev *ev;
	struct net_buf *buf;

	bsim_btp_wait_for_evt(BTP_SERVICE_ID_GAP, BTP_GAP_EV_DEVICE_DISCONNECTED, &buf);
	ev = net_buf_pull_mem(buf, sizeof(*ev));
	if (address != NULL) {
		bt_addr_le_copy(address, &ev->address);
	}

	net_buf_unref(buf);
}
