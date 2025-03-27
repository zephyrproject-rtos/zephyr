/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BSIM_BTP_H_
#define BSIM_BTP_H_

#include <stdint.h>
#include <string.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>

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

static inline void bsim_btp_gap_pair(const bt_addr_le_t *address)
{
	struct btp_gap_pair_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_GAP;
	cmd_hdr->opcode = BTP_GAP_PAIR;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	bt_addr_le_copy(&cmd->address, address);

	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_wait_for_gap_sec_level_changed(bt_addr_le_t *address,
							   uint8_t *sec_level)
{
	struct btp_gap_sec_level_changed_ev *ev;
	struct net_buf *buf;

	bsim_btp_wait_for_evt(BTP_SERVICE_ID_GAP, BTP_GAP_EV_SEC_LEVEL_CHANGED, &buf);
	ev = net_buf_pull_mem(buf, sizeof(*ev));

	if (address != NULL) {
		bt_addr_le_copy(address, &ev->address);
	}

	if (sec_level != NULL) {
		*sec_level = ev->sec_level;
	}

	net_buf_unref(buf);
}

static inline void bsim_btp_hauc_init(void)
{
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_HAP;
	cmd_hdr->opcode = BTP_HAP_HAUC_INIT;
	cmd_hdr->index = BTP_INDEX;

	/* command is empty */

	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_hauc_discover(const bt_addr_le_t *address)
{
	struct btp_hap_hauc_discover_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_HAP;
	cmd_hdr->opcode = BTP_HAP_HAUC_DISCOVER;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	bt_addr_le_copy(&cmd->address, address);

	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_wait_for_hauc_discovery_complete(bt_addr_le_t *address)
{
	struct btp_hap_hauc_discovery_complete_ev *ev;
	struct net_buf *buf;

	bsim_btp_wait_for_evt(BTP_SERVICE_ID_HAP, BT_HAP_EV_HAUC_DISCOVERY_COMPLETE, &buf);
	ev = net_buf_pull_mem(buf, sizeof(*ev));

	TEST_ASSERT(ev->status == BT_ATT_ERR_SUCCESS);

	if (address != NULL) {
		bt_addr_le_copy(address, &ev->address);
	}

	net_buf_unref(buf);
}

static inline void bsim_btp_vcp_discover(const bt_addr_le_t *address)
{
	struct btp_vcp_discover_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_VCP;
	cmd_hdr->opcode = BTP_VCP_VOL_CTLR_DISCOVER;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	bt_addr_le_copy(&cmd->address, address);

	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_wait_for_vcp_discovered(bt_addr_le_t *address)
{
	struct btp_vcp_discovered_ev *ev;
	struct net_buf *buf;

	bsim_btp_wait_for_evt(BTP_SERVICE_ID_VCP, BTP_VCP_DISCOVERED_EV, &buf);
	ev = net_buf_pull_mem(buf, sizeof(*ev));

	TEST_ASSERT(ev->att_status == BT_ATT_ERR_SUCCESS);

	if (address != NULL) {
		bt_addr_le_copy(address, &ev->address);
	}

	net_buf_unref(buf);
}

static inline void bsim_btp_vcp_ctlr_set_vol(const bt_addr_le_t *address, uint8_t volume)
{
	struct btp_vcp_ctlr_set_vol_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_VCP;
	cmd_hdr->opcode = BTP_VCP_VOL_CTLR_SET_VOL;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	bt_addr_le_copy(&cmd->address, address);
	cmd->volume = volume;

	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_wait_for_vcp_state(bt_addr_le_t *address, uint8_t *volume)
{
	struct btp_vcp_state_ev *ev;
	struct net_buf *buf;

	bsim_btp_wait_for_evt(BTP_SERVICE_ID_VCP, BTP_VCP_STATE_EV, &buf);
	ev = net_buf_pull_mem(buf, sizeof(*ev));

	TEST_ASSERT(ev->att_status == BT_ATT_ERR_SUCCESS);

	if (address != NULL) {
		bt_addr_le_copy(address, &ev->address);
	}

	if (volume != NULL) {
		*volume = ev->volume;
	}

	net_buf_unref(buf);
}

static inline void bsim_btp_vocs_state_set(const bt_addr_le_t *address, int16_t offset)
{
	struct btp_vocs_offset_set_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_VOCS;
	cmd_hdr->opcode = BTP_VOCS_OFFSET_STATE_SET;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	bt_addr_le_copy(&cmd->address, address);
	cmd->offset = sys_cpu_to_le16(offset);

	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_wait_for_vocs_state(bt_addr_le_t *address, int16_t *offset)
{
	struct btp_vocs_offset_state_ev *ev;
	struct net_buf *buf;

	bsim_btp_wait_for_evt(BTP_SERVICE_ID_VOCS, BTP_VOCS_OFFSET_STATE_EV, &buf);
	ev = net_buf_pull_mem(buf, sizeof(*ev));

	TEST_ASSERT(ev->att_status == BT_ATT_ERR_SUCCESS);

	if (address != NULL) {
		bt_addr_le_copy(address, &ev->address);
	}

	if (offset != NULL) {
		*offset = sys_le16_to_cpu(ev->offset);
	}

	net_buf_unref(buf);
}

static inline void bsim_btp_aics_set_gain(const bt_addr_le_t *address, int8_t gain)
{
	struct btp_aics_set_gain_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_AICS;
	cmd_hdr->opcode = BTP_AICS_SET_GAIN;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	bt_addr_le_copy(&cmd->address, address);
	cmd->gain = gain;

	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_wait_for_aics_state(bt_addr_le_t *address, int8_t *gain)
{
	struct btp_aics_state_ev *ev;
	struct net_buf *buf;

	bsim_btp_wait_for_evt(BTP_SERVICE_ID_AICS, BTP_AICS_STATE_EV, &buf);
	ev = net_buf_pull_mem(buf, sizeof(*ev));

	TEST_ASSERT(ev->att_status == BT_ATT_ERR_SUCCESS);

	if (address != NULL) {
		bt_addr_le_copy(address, &ev->address);
	}

	if (gain != NULL) {
		*gain = ev->gain;
	}

	net_buf_unref(buf);
}

static inline void bsim_btp_micp_discover(const bt_addr_le_t *address)
{
	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	struct btp_hdr *cmd_hdr;
	struct btp_micp_discover_cmd *cmd;

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_MICP;
	cmd_hdr->opcode = BTP_MICP_CTLR_DISCOVER;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	bt_addr_le_copy(&cmd->address, address);

	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_wait_for_micp_discovered(bt_addr_le_t *address)
{
	struct btp_micp_discovered_ev *ev;
	struct net_buf *buf;

	bsim_btp_wait_for_evt(BTP_SERVICE_ID_MICP, BTP_MICP_DISCOVERED_EV, &buf);
	ev = net_buf_pull_mem(buf, sizeof(*ev));

	TEST_ASSERT(ev->att_status == BT_ATT_ERR_SUCCESS);

	if (address != NULL) {
		bt_addr_le_copy(address, &ev->address);
	}

	net_buf_unref(buf);
}

static inline void bsim_btp_micp_ctlr_mute(const bt_addr_le_t *address)
{
	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	struct btp_hdr *cmd_hdr;
	struct btp_micp_mute_cmd *cmd;

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_MICP;
	cmd_hdr->opcode = BTP_MICP_CTLR_MUTE;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	bt_addr_le_copy(&cmd->address, address);

	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_wait_for_micp_state(bt_addr_le_t *address, uint8_t *mute)
{
	struct btp_micp_mute_state_ev *ev;
	struct net_buf *buf;

	bsim_btp_wait_for_evt(BTP_SERVICE_ID_MICP, BTP_MICP_MUTE_STATE_EV, &buf);
	ev = net_buf_pull_mem(buf, sizeof(*ev));

	TEST_ASSERT(ev->att_status == BT_ATT_ERR_SUCCESS);

	if (address != NULL) {
		bt_addr_le_copy(address, &ev->address);
	}

	if (mute != NULL) {
		*mute = ev->mute;
	}

	net_buf_unref(buf);
}
#endif /* BSIM_BTP_H_ */
