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
#include <zephyr/bluetooth/iso.h>
#include <zephyr/bluetooth/audio/mcs.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>

#include "babblekit/testcase.h"

#include "btp/btp.h"

void bsim_btp_uart_init(void);
void bsim_btp_send_to_tester(const uint8_t *data, size_t len);
void bsim_btp_wait_for_evt(uint8_t service, uint8_t opcode, struct net_buf **out_buf);

static inline void bsim_btp_ccp_discover(const bt_addr_le_t *address)
{
	struct btp_ccp_discover_tbs_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_CCP;
	cmd_hdr->opcode = BTP_CCP_DISCOVER_TBS;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	bt_addr_le_copy(&cmd->address, address);

	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_ccp_originate_call(const bt_addr_le_t *address, uint8_t inst_index,
					       const char *uri)
{
	struct btp_ccp_originate_call_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_CCP;
	cmd_hdr->opcode = BTP_CCP_ORIGINATE_CALL;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	bt_addr_le_copy(&cmd->address, address);
	cmd->inst_index = inst_index;
	cmd->uri_len = strlen(uri) + 1 /* NULL terminator */;
	net_buf_simple_add_mem(&cmd_buffer, uri, cmd->uri_len);

	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_wait_for_ccp_discovered(void)
{
	struct btp_ccp_discovered_ev *ev;
	struct net_buf *buf;

	bsim_btp_wait_for_evt(BTP_SERVICE_ID_CCP, BTP_CCP_EV_DISCOVERED, &buf);
	ev = net_buf_pull_mem(buf, sizeof(*ev));

	TEST_ASSERT(ev->status == BT_ATT_ERR_SUCCESS);

	net_buf_unref(buf);
}

static inline void bsim_btp_wait_for_ccp_call_states(void)
{
	struct btp_ccp_call_states_ev *ev;
	struct net_buf *buf;

	bsim_btp_wait_for_evt(BTP_SERVICE_ID_CCP, BTP_CCP_EV_CALL_STATES, &buf);
	ev = net_buf_pull_mem(buf, sizeof(*ev));

	TEST_ASSERT(ev->status == BT_ATT_ERR_SUCCESS);

	net_buf_unref(buf);
}

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

	cmd_hdr->len = sys_cpu_to_le16(cmd_buffer.len - sizeof(*cmd_hdr));

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_csip_discover(const bt_addr_le_t *address)
{
	struct btp_csip_discover_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_CSIP;
	cmd_hdr->opcode = BTP_CSIP_DISCOVER;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	bt_addr_le_copy(&cmd->address, address);

	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_csip_set_coordinator_lock(void)
{
	struct btp_csip_set_coordinator_lock_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_CSIP;
	cmd_hdr->opcode = BTP_CSIP_SET_COORDINATOR_LOCK;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	cmd->addr_cnt = 0U; /* Zephyr BT Tester only supports this value being 0 */

	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_wait_for_csip_discovered(bt_addr_le_t *address)
{
	struct btp_csip_discovered_ev *ev;
	struct net_buf *buf;

	bsim_btp_wait_for_evt(BTP_SERVICE_ID_CSIP, BTP_CSIP_DISCOVERED_EV, &buf);
	ev = net_buf_pull_mem(buf, sizeof(*ev));

	TEST_ASSERT(ev->status == BT_ATT_ERR_SUCCESS);

	if (address != NULL) {
		bt_addr_le_copy(address, &ev->address);
	}

	net_buf_unref(buf);
}

static inline void bsim_btp_wait_for_lock(void)
{
	struct btp_csip_lock_ev *ev;
	struct net_buf *buf;

	bsim_btp_wait_for_evt(BTP_SERVICE_ID_CSIP, BTP_CSIP_LOCK_EV, &buf);
	ev = net_buf_pull_mem(buf, sizeof(*ev));

	TEST_ASSERT(ev->status == BT_ATT_ERR_SUCCESS);

	net_buf_unref(buf);
}

static inline void bsim_btp_gap_set_connectable(bool enable)
{
	struct btp_gap_set_connectable_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_GAP;
	cmd_hdr->opcode = BTP_GAP_SET_CONNECTABLE;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	cmd->connectable = enable ? 1 : 0;

	cmd_hdr->len = sys_cpu_to_le16(cmd_buffer.len - sizeof(*cmd_hdr));

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

	cmd_hdr->len = sys_cpu_to_le16(cmd_buffer.len - sizeof(*cmd_hdr));

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

	cmd_hdr->len = sys_cpu_to_le16(cmd_buffer.len - sizeof(*cmd_hdr));

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

	cmd_hdr->len = sys_cpu_to_le16(cmd_buffer.len - sizeof(*cmd_hdr));

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
	cmd_hdr->len = sys_cpu_to_le16(cmd_buffer.len - sizeof(*cmd_hdr));

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

	cmd_hdr->len = sys_cpu_to_le16(cmd_buffer.len - sizeof(*cmd_hdr));

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

	cmd_hdr->len = sys_cpu_to_le16(cmd_buffer.len - sizeof(*cmd_hdr));

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

	cmd_hdr->len = sys_cpu_to_le16(cmd_buffer.len - sizeof(*cmd_hdr));

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
static inline void bsim_btp_mcp_discover(const bt_addr_le_t *address)
{
	struct btp_mcp_discover_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_MCP;
	cmd_hdr->opcode = BTP_MCP_DISCOVER;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	bt_addr_le_copy(&cmd->address, address);

	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_mcp_send_cmd(const bt_addr_le_t *address, uint8_t opcode,
					 bool use_param, int32_t param)
{
	struct btp_mcp_send_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_MCP;
	cmd_hdr->opcode = BTP_MCP_CMD_SEND;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	bt_addr_le_copy(&cmd->address, address);
	cmd->opcode = opcode;
	cmd->use_param = use_param ? 1U : 0U;
	cmd->param = sys_cpu_to_le32(param);

	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_wait_for_mcp_discovered(bt_addr_le_t *address)
{
	struct btp_mcp_discovered_ev *ev;
	struct net_buf *buf;

	bsim_btp_wait_for_evt(BTP_SERVICE_ID_MCP, BTP_MCP_DISCOVERED_EV, &buf);
	ev = net_buf_pull_mem(buf, sizeof(*ev));

	TEST_ASSERT(ev->status == BT_ATT_ERR_SUCCESS);

	if (address != NULL) {
		bt_addr_le_copy(address, &ev->address);
	}

	net_buf_unref(buf);
}

static inline void bsim_btp_wait_for_mcp_cmd_ntf(uint8_t *requested_opcode)
{
	struct btp_mcp_cmd_ntf_ev *ev;
	struct net_buf *buf;

	bsim_btp_wait_for_evt(BTP_SERVICE_ID_MCP, BTP_MCP_NTF_EV, &buf);
	ev = net_buf_pull_mem(buf, sizeof(*ev));

	TEST_ASSERT(ev->status == BT_ATT_ERR_SUCCESS);
	TEST_ASSERT(ev->result_code == BT_MCS_OPC_NTF_SUCCESS);

	if (requested_opcode != NULL) {
		*requested_opcode = ev->requested_opcode;
	}

	net_buf_unref(buf);
}

static inline void bsim_btp_tmap_discover(const bt_addr_le_t *address)
{
	struct btp_tmap_discover_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_TMAP;
	cmd_hdr->opcode = BTP_TMAP_DISCOVER;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	bt_addr_le_copy(&cmd->address, address);

	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_wait_for_tmap_discovery_complete(void)
{
	struct btp_tmap_discovery_complete_ev *ev;
	struct net_buf *buf;

	bsim_btp_wait_for_evt(BTP_SERVICE_ID_TMAP, BT_TMAP_EV_DISCOVERY_COMPLETE, &buf);
	ev = net_buf_pull_mem(buf, sizeof(*ev));

	TEST_ASSERT(ev->status == BT_ATT_ERR_SUCCESS);

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

	cmd_hdr->len = sys_cpu_to_le16(cmd_buffer.len - sizeof(*cmd_hdr));

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

	cmd_hdr->len = sys_cpu_to_le16(cmd_buffer.len - sizeof(*cmd_hdr));

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

	cmd_hdr->len = sys_cpu_to_le16(cmd_buffer.len - sizeof(*cmd_hdr));

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

	cmd_hdr->len = sys_cpu_to_le16(cmd_buffer.len - sizeof(*cmd_hdr));

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

	cmd_hdr->len = sys_cpu_to_le16(cmd_buffer.len - sizeof(*cmd_hdr));

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

	cmd_hdr->len = sys_cpu_to_le16(cmd_buffer.len - sizeof(*cmd_hdr));

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

static inline void bsim_btp_gap_set_extended_advertising(bool enable)
{
	struct btp_gap_set_extended_advertising_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_GAP;
	cmd_hdr->opcode = BTP_GAP_SET_EXTENDED_ADVERTISING;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	cmd->settings = enable ? BIT(0) : 0;

	cmd_hdr->len = sys_cpu_to_le16(cmd_buffer.len - sizeof(*cmd_hdr));

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_gap_padv_configure(uint8_t flags, uint16_t interval_min,
					       uint16_t interval_max)
{
	struct btp_gap_padv_configure_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_GAP;
	cmd_hdr->opcode = BTP_GAP_PADV_CONFIGURE;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	cmd->flags = flags;
	cmd->interval_min = sys_cpu_to_le16(interval_min);
	cmd->interval_max = sys_cpu_to_le16(interval_max);

	cmd_hdr->len = sys_cpu_to_le16(cmd_buffer.len - sizeof(*cmd_hdr));

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_gap_padv_start(void)
{
	struct btp_gap_padv_start_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_GAP;
	cmd_hdr->opcode = BTP_GAP_PADV_START;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	cmd->flags = 0;

	cmd_hdr->len = sys_cpu_to_le16(cmd_buffer.len - sizeof(*cmd_hdr));

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_gap_padv_stop(void)
{
	struct btp_gap_padv_stop_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_GAP;
	cmd_hdr->opcode = BTP_GAP_PADV_STOP;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));

	cmd_hdr->len = sys_cpu_to_le16(cmd_buffer.len - sizeof(*cmd_hdr));

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_gap_padv_create_sync(bt_addr_le_t *addr, uint8_t sid, uint16_t skip,
						 uint16_t sync_timeout, uint8_t flags)
{
	struct btp_gap_padv_create_sync_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_GAP;
	cmd_hdr->opcode = BTP_GAP_PADV_CREATE_SYNC;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	bt_addr_le_copy(&cmd->address, addr);
	cmd->advertiser_sid = sid;
	cmd->skip = sys_cpu_to_le16(skip);
	cmd->sync_timeout = sys_cpu_to_le16(sync_timeout);
	cmd->flags = flags;

	cmd_hdr->len = sys_cpu_to_le16(cmd_buffer.len - sizeof(*cmd_hdr));

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_wait_for_gap_periodic_sync_established(bt_addr_le_t *address,
								   uint16_t *sync_handle,
								   uint8_t *status)
{
	struct btp_gap_ev_periodic_sync_established_ev *ev;
	struct net_buf *buf;

	bsim_btp_wait_for_evt(BTP_SERVICE_ID_GAP, BTP_GAP_EV_PERIODIC_SYNC_ESTABLISHED, &buf);
	ev = net_buf_pull_mem(buf, sizeof(*ev));
	if (address != NULL) {
		bt_addr_le_copy(address, &ev->address);
	}

	if (sync_handle != NULL) {
		*sync_handle = ev->sync_handle;
	}

	if (status != NULL) {
		*status = ev->status;
	}

	net_buf_unref(buf);
}

static inline void bsim_btp_wait_for_gap_periodic_sync_lost(uint16_t *sync_handle, uint8_t *reason)
{
	struct btp_gap_ev_periodic_sync_lost_ev *ev;
	struct net_buf *buf;

	bsim_btp_wait_for_evt(BTP_SERVICE_ID_GAP, BTP_GAP_EV_PERIODIC_SYNC_LOST, &buf);
	ev = net_buf_pull_mem(buf, sizeof(*ev));
	if (sync_handle != NULL) {
		*sync_handle = ev->sync_handle;
	}

	if (reason != NULL) {
		*reason = ev->reason;
	}

	net_buf_unref(buf);
}

static inline void bsim_btp_wait_for_gap_periodic_biginfo(bt_addr_le_t *address, uint8_t *sid,
							  uint8_t *num_bis, uint8_t *encryption)
{
	struct btp_gap_periodic_biginfo_ev *ev;
	struct net_buf *buf;

	bsim_btp_wait_for_evt(BTP_SERVICE_ID_GAP, BTP_GAP_EV_PERIODIC_BIGINFO, &buf);
	ev = net_buf_pull_mem(buf, sizeof(*ev));
	if (address != NULL) {
		bt_addr_le_copy(address, &ev->address);
	}

	if (sid != NULL) {
		*sid = ev->sid;
	}

	if (num_bis != NULL) {
		*num_bis = ev->num_bis;
	}

	if (encryption != NULL) {
		*encryption = ev->encryption;
	}

	net_buf_unref(buf);
}

static inline void bsim_btp_gap_big_create_sync(bt_addr_le_t *address, uint8_t sid, uint8_t num_bis,
						uint32_t bis_bitfield, uint32_t mse,
						uint16_t sync_timeout, bool encryption,
						uint8_t broadcast_code[BT_ISO_BROADCAST_CODE_SIZE])
{
	struct btp_gap_big_create_sync_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_GAP;
	cmd_hdr->opcode = BTP_GAP_BIG_CREATE_SYNC;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	bt_addr_le_copy(&cmd->address, address);
	cmd->sid = sid;
	cmd->num_bis = num_bis;
	cmd->bis_bitfield = sys_cpu_to_le32(bis_bitfield);
	cmd->mse = sys_cpu_to_le32(mse);
	cmd->sync_timeout = sys_cpu_to_le16(sync_timeout);
	cmd->encryption = encryption;
	if (encryption) {
		net_buf_simple_add(&cmd_buffer, BT_ISO_BROADCAST_CODE_SIZE);
		memcpy(cmd->broadcast_code, broadcast_code, BT_ISO_BROADCAST_CODE_SIZE);
	}

	cmd_hdr->len = sys_cpu_to_le16(cmd_buffer.len - sizeof(*cmd_hdr));

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_wait_for_gap_big_sync_established(bt_addr_le_t *address)
{
	struct btp_gap_big_sync_established_ev *ev;
	struct net_buf *buf;

	bsim_btp_wait_for_evt(BTP_SERVICE_ID_GAP, BTP_GAP_EV_BIG_SYNC_ESTABLISHED, &buf);
	ev = net_buf_pull_mem(buf, sizeof(*ev));
	if (address != NULL) {
		bt_addr_le_copy(address, &ev->address);
	}

	net_buf_unref(buf);
}

static inline void bsim_btp_gap_create_big(uint8_t num_bis, uint32_t interval, uint16_t latency,
					   bool encryption,
					   uint8_t broadcast_code[BT_ISO_BROADCAST_CODE_SIZE])
{
	struct btp_gap_create_big_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_GAP;
	cmd_hdr->opcode = BTP_GAP_CREATE_BIG;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	cmd->id = 0;
	cmd->num_bis = num_bis;
	cmd->interval = sys_cpu_to_le32(interval);
	cmd->latency = sys_cpu_to_le16(latency);
	cmd->rtn = 2;
	cmd->phy = BT_GAP_LE_PHY_2M;
	cmd->packing = BT_ISO_PACKING_SEQUENTIAL;
	cmd->framing = BT_ISO_FRAMING_UNFRAMED;
	cmd->encryption = encryption;
	if (encryption) {
		net_buf_simple_add(&cmd_buffer, BT_ISO_BROADCAST_CODE_SIZE);
		memcpy(cmd->broadcast_code, broadcast_code, BT_ISO_BROADCAST_CODE_SIZE);
	}

	cmd_hdr->len = sys_cpu_to_le16(cmd_buffer.len - sizeof(*cmd_hdr));

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_wait_for_gap_bis_data_path_setup(bt_addr_le_t *address, uint8_t *bis_id)
{
	struct btp_gap_bis_data_path_setup_ev *ev;
	struct net_buf *buf;

	bsim_btp_wait_for_evt(BTP_SERVICE_ID_GAP, BTP_GAP_EV_BIS_DATA_PATH_SETUP, &buf);
	ev = net_buf_pull_mem(buf, sizeof(*ev));
	if (address != NULL) {
		bt_addr_le_copy(address, &ev->address);
	}

	if (bis_id != NULL) {
		*bis_id = ev->bis_id;
	}

	net_buf_unref(buf);
}

static inline void bsim_btp_gap_bis_broadcast(uint8_t bis_id, struct net_buf_simple *buf)
{
	struct btp_gap_bis_broadcast_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_GAP;
	cmd_hdr->opcode = BTP_GAP_BIS_BROADCAST;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	__ASSERT(buf->len <= net_buf_simple_tailroom(&cmd_buffer), "No more tail room");
	cmd->bis_id = bis_id;
	cmd->data_len = buf->len;
	net_buf_simple_add_mem(&cmd_buffer, buf->data, buf->len);

	cmd_hdr->len = sys_cpu_to_le16(cmd_buffer.len - sizeof(*cmd_hdr));

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_wait_for_gap_bis_stream_received(struct net_buf_simple *rx)
{
	struct net_buf *buf;

	bsim_btp_wait_for_evt(BTP_SERVICE_ID_GAP, BTP_GAP_EV_BIS_STREAM_RECEIVED, &buf);
	__ASSERT(buf->len <= net_buf_simple_tailroom(rx), "No more tail room");
	net_buf_simple_add_mem(rx, buf->data, buf->len);
	net_buf_unref(buf);
}

static inline void bsim_btp_wait_for_gap_big_sync_lost(bt_addr_le_t *address, uint8_t *reason)
{
	struct btp_gap_big_sync_lost_ev *ev;
	struct net_buf *buf;

	bsim_btp_wait_for_evt(BTP_SERVICE_ID_GAP, BTP_GAP_EV_BIG_SYNC_LOST, &buf);
	ev = net_buf_pull_mem(buf, sizeof(*ev));
	if (address != NULL) {
		bt_addr_le_copy(address, &ev->address);
	}

	if (reason != NULL) {
		*reason = ev->reason;
	}

	net_buf_unref(buf);
}

#endif /* BSIM_BTP_H_ */
