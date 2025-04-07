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

static inline void bsim_btp_ascs_configure_codec(const bt_addr_le_t *address, uint8_t ase_id,
						 uint8_t coding_format, uint16_t vid, uint16_t cid,
						 uint8_t cc_ltvs_len, uint8_t cc_ltvs[])
{
	struct btp_ascs_configure_codec_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_ASCS;
	cmd_hdr->opcode = BTP_ASCS_CONFIGURE_CODEC;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	bt_addr_le_copy(&cmd->address, address);
	cmd->ase_id = ase_id;
	cmd->coding_format = coding_format;
	cmd->vid = sys_cpu_to_le16(vid);
	cmd->cid = sys_cpu_to_le16(cid);
	cmd->cc_ltvs_len = cc_ltvs_len;
	if (cc_ltvs_len != 0) {
		(void)net_buf_simple_add_mem(&cmd_buffer, cc_ltvs, cc_ltvs_len);
	}

	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_ascs_configure_qos(const bt_addr_le_t *address, uint8_t ase_id,
					       uint8_t cig_id, uint8_t cis_id,
					       uint32_t sdu_interval, uint8_t framing,
					       uint16_t max_sdu, uint8_t rtn, uint16_t max_latency,
					       uint32_t presentation_delay)
{
	struct btp_ascs_configure_qos_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_ASCS;
	cmd_hdr->opcode = BTP_ASCS_CONFIGURE_QOS;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	bt_addr_le_copy(&cmd->address, address);
	cmd->ase_id = ase_id;
	cmd->cig_id = cig_id;
	cmd->cis_id = cis_id;
	sys_put_le24(sdu_interval, cmd->sdu_interval);
	cmd->framing = framing;
	cmd->max_sdu = sys_cpu_to_le16(max_sdu);
	cmd->retransmission_num = rtn;
	cmd->max_transport_latency = sys_cpu_to_le16(max_latency);
	sys_put_le24(presentation_delay, cmd->presentation_delay);

	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_ascs_enable(const bt_addr_le_t *address, uint8_t ase_id)
{
	struct btp_ascs_enable_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_ASCS;
	cmd_hdr->opcode = BTP_ASCS_ENABLE;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	bt_addr_le_copy(&cmd->address, address);
	cmd->ase_id = ase_id;

	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_ascs_receiver_start_ready(const bt_addr_le_t *address, uint8_t ase_id)
{
	struct btp_ascs_receiver_start_ready_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_ASCS;
	cmd_hdr->opcode = BTP_ASCS_RECEIVER_START_READY;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	bt_addr_le_copy(&cmd->address, address);
	cmd->ase_id = ase_id;

	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_ascs_receiver_stop_ready(const bt_addr_le_t *address, uint8_t ase_id)
{
	struct btp_ascs_receiver_stop_ready_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_ASCS;
	cmd_hdr->opcode = BTP_ASCS_RECEIVER_STOP_READY;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	bt_addr_le_copy(&cmd->address, address);
	cmd->ase_id = ase_id;

	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_ascs_disable(const bt_addr_le_t *address, uint8_t ase_id)
{
	struct btp_ascs_disable_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_ASCS;
	cmd_hdr->opcode = BTP_ASCS_DISABLE;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	bt_addr_le_copy(&cmd->address, address);
	cmd->ase_id = ase_id;

	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_ascs_release(const bt_addr_le_t *address, uint8_t ase_id)
{
	struct btp_ascs_release_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_ASCS;
	cmd_hdr->opcode = BTP_ASCS_RELEASE;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	bt_addr_le_copy(&cmd->address, address);
	cmd->ase_id = ase_id;

	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_wait_for_ascs_operation_complete(void)
{
	struct btp_ascs_operation_completed_ev *ev;
	struct net_buf *buf;

	bsim_btp_wait_for_evt(BTP_SERVICE_ID_ASCS, BTP_ASCS_EV_OPERATION_COMPLETED, &buf);
	ev = net_buf_pull_mem(buf, sizeof(*ev));

	net_buf_unref(buf);
}

static inline void bsim_btp_ascs_add_ase_to_cis(const bt_addr_le_t *address, uint8_t ase_id,
						uint8_t cig_id, uint8_t cis_id)
{
	struct btp_ascs_add_ase_to_cis *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_ASCS;
	cmd_hdr->opcode = BTP_ASCS_ADD_ASE_TO_CIS;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	bt_addr_le_copy(&cmd->address, address);
	cmd->ase_id = ase_id;
	cmd->cig_id = cig_id;
	cmd->cis_id = cis_id;

	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void btp_ascs_preconfigure_qos(uint8_t cig_id, uint8_t cis_id, uint32_t sdu_interval,
					     uint8_t framing, uint16_t max_sdu, uint8_t rtn,
					     uint16_t max_latency, uint32_t presentation_delay)
{
	struct btp_ascs_preconfigure_qos_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_ASCS;
	cmd_hdr->opcode = BTP_ASCS_PRECONFIGURE_QOS;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	cmd->cig_id = cig_id;
	cmd->cis_id = cis_id;
	sys_put_le24(sdu_interval, cmd->sdu_interval);
	cmd->framing = framing;
	cmd->max_sdu = sys_cpu_to_le16(max_sdu);
	cmd->retransmission_num = rtn;
	cmd->max_transport_latency = sys_cpu_to_le16(max_latency);
	sys_put_le24(presentation_delay, cmd->presentation_delay);

	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_wait_for_ascs_ase_state_changed(void)
{
	struct btp_ascs_ase_state_changed_ev *ev;
	struct net_buf *buf;

	bsim_btp_wait_for_evt(BTP_SERVICE_ID_ASCS, BTP_ASCS_EV_ASE_STATE_CHANGED, &buf);
	ev = net_buf_pull_mem(buf, sizeof(*ev));

	net_buf_unref(buf);
}

static inline void bsim_btp_bap_discover(const bt_addr_le_t *address)
{
	struct btp_bap_discover_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_BAP;
	cmd_hdr->opcode = BTP_BAP_DISCOVER;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	bt_addr_le_copy(&cmd->address, address);

	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_wait_for_bap_discovered(void)
{
	struct btp_bap_discovery_completed_ev *ev;
	struct net_buf *buf;

	bsim_btp_wait_for_evt(BTP_SERVICE_ID_BAP, BTP_BAP_EV_DISCOVERY_COMPLETED, &buf);
	ev = net_buf_pull_mem(buf, sizeof(*ev));

	TEST_ASSERT(ev->status == BT_ATT_ERR_SUCCESS);

	net_buf_unref(buf);
}

static inline void bsim_btp_bap_broadcast_adv_start(uint32_t broadcast_id)
{
	struct btp_bap_broadcast_adv_start_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_BAP;
	cmd_hdr->opcode = BTP_BAP_BROADCAST_ADV_START;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	sys_put_le24(broadcast_id, cmd->broadcast_id);

	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_bap_broadcast_source_start(uint32_t broadcast_id)
{
	struct btp_bap_broadcast_source_start_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_BAP;
	cmd_hdr->opcode = BTP_BAP_BROADCAST_SOURCE_START;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	sys_put_le24(broadcast_id, cmd->broadcast_id);

	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_bap_broadcast_sink_setup(void)
{
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_BAP;
	cmd_hdr->opcode = BTP_BAP_BROADCAST_SINK_SETUP;
	cmd_hdr->index = BTP_INDEX;

	/* The command for this is empty */

	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_bap_broadcast_sink_release(void)
{
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_BAP;
	cmd_hdr->opcode = BTP_BAP_BROADCAST_SINK_RELEASE;
	cmd_hdr->index = BTP_INDEX;

	/* The command for this is empty */

	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_bap_broadcast_scan_start(void)
{
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_BAP;
	cmd_hdr->opcode = BTP_BAP_BROADCAST_SCAN_START;
	cmd_hdr->index = BTP_INDEX;

	/* The command for this is empty */

	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_bap_broadcast_scan_stop(void)
{
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_BAP;
	cmd_hdr->opcode = BTP_BAP_BROADCAST_SCAN_STOP;
	cmd_hdr->index = BTP_INDEX;

	/* The command for this is empty */

	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_bap_broadcast_sink_sync(const bt_addr_le_t *address,
						    uint32_t broadcast_id, uint8_t advertiser_sid,
						    uint16_t skip, uint16_t sync_timeout,
						    bool past_avail, uint8_t src_id)
{
	struct btp_bap_broadcast_sink_sync_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_BAP;
	cmd_hdr->opcode = BTP_BAP_BROADCAST_SINK_SYNC;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	bt_addr_le_copy(&cmd->address, address);
	sys_put_le24(broadcast_id, cmd->broadcast_id);
	cmd->advertiser_sid = advertiser_sid;
	cmd->skip = sys_cpu_to_le16(skip);
	cmd->sync_timeout = sys_cpu_to_le16(sync_timeout);
	cmd->past_avail = past_avail ? 1U : 0U;
	cmd->src_id = src_id;

	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_bap_broadcast_sink_stop(const bt_addr_le_t *address,
						    uint32_t broadcast_id)
{
	struct btp_bap_broadcast_sink_stop_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_BAP;
	cmd_hdr->opcode = BTP_BAP_BROADCAST_SINK_STOP;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	bt_addr_le_copy(&cmd->address, address);
	sys_put_le24(broadcast_id, cmd->broadcast_id);

	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_bap_broadcast_sink_bis_sync(const bt_addr_le_t *address,
							uint32_t broadcast_id,
							uint32_t requested_bis_sync)
{
	struct btp_bap_broadcast_sink_bis_sync_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_BAP;
	cmd_hdr->opcode = BTP_BAP_BROADCAST_SINK_BIS_SYNC;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	bt_addr_le_copy(&cmd->address, address);
	sys_put_le24(broadcast_id, cmd->broadcast_id);
	cmd->requested_bis_sync = sys_cpu_to_le32(requested_bis_sync);

	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_bap_broadcast_source_setup_v2(
	uint32_t broadcast_id, uint8_t streams_per_subgroup, uint8_t subgroups,
	uint32_t sdu_interval, uint8_t framing, uint16_t max_sdu, uint8_t rtn, uint16_t max_latency,
	uint32_t presentation_delay, uint8_t coding_format, uint16_t vid, uint16_t cid,
	uint8_t cc_ltvs_len, const uint8_t cc_ltvs[])
{
	struct btp_bap_broadcast_source_setup_v2_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_BAP;
	cmd_hdr->opcode = BTP_BAP_BROADCAST_SOURCE_SETUP_V2;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	sys_put_le24(broadcast_id, cmd->broadcast_id);
	cmd->streams_per_subgroup = streams_per_subgroup;
	cmd->subgroups = subgroups;
	sys_put_le24(sdu_interval, cmd->sdu_interval);
	cmd->framing = framing;
	cmd->max_sdu = sys_cpu_to_le16(max_sdu);
	cmd->retransmission_num = rtn;
	cmd->max_transport_latency = sys_cpu_to_le16(max_latency);
	sys_put_le24(presentation_delay, cmd->presentation_delay);
	cmd->coding_format = coding_format;
	cmd->vid = sys_cpu_to_le16(vid);
	cmd->cid = sys_cpu_to_le16(cid);
	cmd->cc_ltvs_len = cc_ltvs_len;
	if (cc_ltvs_len > 0U) {
		(void)net_buf_simple_add_mem(&cmd_buffer, cc_ltvs, cc_ltvs_len);
	}

	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_wait_for_bap_ase_found(uint8_t *ase_id)
{
	struct btp_bap_ase_found_ev *ev;
	struct net_buf *buf;

	bsim_btp_wait_for_evt(BTP_SERVICE_ID_BAP, BTP_BAP_EV_ASE_FOUND, &buf);
	ev = net_buf_pull_mem(buf, sizeof(*ev));

	if (ase_id != NULL) {
		*ase_id = ev->ase_id;
	}

	net_buf_unref(buf);
}

static inline void bsim_btp_wait_for_bap_baa_found(bt_addr_le_t *address, uint32_t *broadcast_id,
						   uint8_t *advertiser_sid)
{
	struct btp_bap_baa_found_ev *ev;
	struct net_buf *buf;

	bsim_btp_wait_for_evt(BTP_SERVICE_ID_BAP, BTP_BAP_EV_BAA_FOUND, &buf);
	ev = net_buf_pull_mem(buf, sizeof(*ev));

	if (address != NULL) {
		bt_addr_le_copy(address, &ev->address);
	}

	if (broadcast_id != NULL) {
		*broadcast_id = sys_get_le24(ev->broadcast_id);
	}

	if (advertiser_sid != NULL) {
		*advertiser_sid = ev->advertiser_sid;
	}

	net_buf_unref(buf);
}

static inline void bsim_btp_wait_for_bap_bap_bis_found(uint8_t *bis_id)
{
	struct btp_bap_bis_found_ev *ev;
	struct net_buf *buf;

	bsim_btp_wait_for_evt(BTP_SERVICE_ID_BAP, BTP_BAP_EV_BIS_FOUND, &buf);
	ev = net_buf_pull_mem(buf, sizeof(*ev));

	if (bis_id != NULL) {
		*bis_id = ev->bis_id;
	}

	net_buf_unref(buf);
}

static inline void bsim_btp_wait_for_btp_bap_bis_synced(void)
{
	struct btp_bap_bis_syned_ev *ev;
	struct net_buf *buf;

	bsim_btp_wait_for_evt(BTP_SERVICE_ID_BAP, BTP_BAP_EV_BIS_SYNCED, &buf);
	ev = net_buf_pull_mem(buf, sizeof(*ev));

	net_buf_unref(buf);
}

static inline void bsim_btp_wait_for_bap_bis_stream_received(void)
{
	struct btp_bap_bis_stream_received_ev *ev;
	struct net_buf *buf;

	bsim_btp_wait_for_evt(BTP_SERVICE_ID_BAP, BTP_BAP_EV_BIS_STREAM_RECEIVED, &buf);
	ev = net_buf_pull_mem(buf, sizeof(*ev));

	net_buf_unref(buf);
}

static inline void bsim_btp_cap_discover(const bt_addr_le_t *address)
{
	struct btp_cap_discover_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_CAP;
	cmd_hdr->opcode = BTP_CAP_DISCOVER;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	bt_addr_le_copy(&cmd->address, address);

	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_cap_unicast_setup_ase_cmd(
	const bt_addr_le_t *address, uint8_t ase_id, uint8_t cis_id, uint8_t cig_id,
	uint8_t coding_format, uint16_t vid, uint16_t cid, uint32_t sdu_interval, uint8_t framing,
	uint16_t max_sdu, uint8_t rtn, uint16_t max_latency, uint32_t presentation_delay,
	uint8_t cc_ltvs_len, const uint8_t cc_ltvs[], uint8_t metadata_ltvs_len,
	const uint8_t metadata_ltvs[])
{
	struct btp_cap_unicast_setup_ase_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_CAP;
	cmd_hdr->opcode = BTP_CAP_UNICAST_SETUP_ASE;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	bt_addr_le_copy(&cmd->address, address);
	cmd->ase_id = ase_id;
	cmd->cig_id = cig_id;
	cmd->cis_id = cis_id;
	cmd->coding_format = coding_format;
	cmd->vid = sys_cpu_to_le16(vid);
	cmd->cid = sys_cpu_to_le16(cid);
	sys_put_le24(sdu_interval, cmd->sdu_interval);
	cmd->framing = framing;
	cmd->max_sdu = sys_cpu_to_le16(max_sdu);
	cmd->retransmission_num = rtn;
	cmd->max_transport_latency = sys_cpu_to_le16(max_latency);
	sys_put_le24(presentation_delay, cmd->presentation_delay);
	cmd->cc_ltvs_len = cc_ltvs_len;
	if (cc_ltvs_len > 0U) {
		net_buf_simple_add_mem(&cmd_buffer, cc_ltvs, cc_ltvs_len);
	}
	cmd->metadata_ltvs_len = metadata_ltvs_len;
	if (metadata_ltvs_len > 0U) {
		net_buf_simple_add_mem(&cmd_buffer, metadata_ltvs, metadata_ltvs_len);
	}

	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_cap_unicast_audio_start(uint8_t cig_id, uint8_t set_type)
{
	struct btp_cap_unicast_audio_start_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_CAP;
	cmd_hdr->opcode = BTP_CAP_UNICAST_AUDIO_START;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	cmd->cig_id = cig_id;
	cmd->set_type = set_type;

	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void
bsim_btp_cap_broadcast_source_setup_stream(uint8_t source_id, uint8_t subgroup_id,
					   uint8_t coding_format, uint16_t vid, uint16_t cid,
					   uint8_t cc_ltvs_len, const uint8_t cc_ltvs[],
					   uint8_t metadata_ltvs_len, const uint8_t metadata_ltvs[])
{
	struct btp_cap_broadcast_source_setup_stream_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_CAP;
	cmd_hdr->opcode = BTP_CAP_BROADCAST_SOURCE_SETUP_STREAM;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	cmd->source_id = source_id;
	cmd->subgroup_id = subgroup_id;
	cmd->coding_format = coding_format;
	cmd->vid = sys_cpu_to_le16(vid);
	cmd->cid = sys_cpu_to_le16(cid);
	cmd->cc_ltvs_len = cc_ltvs_len;
	if (cc_ltvs_len > 0U) {
		net_buf_simple_add_mem(&cmd_buffer, cc_ltvs, cc_ltvs_len);
	}
	cmd->metadata_ltvs_len = metadata_ltvs_len;
	if (metadata_ltvs_len > 0U) {
		net_buf_simple_add_mem(&cmd_buffer, metadata_ltvs, metadata_ltvs_len);
	}

	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_cap_broadcast_source_setup_subgroup(
	uint8_t source_id, uint8_t subgroup_id, uint8_t coding_format, uint16_t vid, uint16_t cid,
	uint8_t cc_ltvs_len, const uint8_t cc_ltvs[], uint8_t metadata_ltvs_len,
	const uint8_t metadata_ltvs[])
{
	struct btp_cap_broadcast_source_setup_subgroup_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_CAP;
	cmd_hdr->opcode = BTP_CAP_BROADCAST_SOURCE_SETUP_SUBGROUP;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	cmd->source_id = source_id;
	cmd->subgroup_id = subgroup_id;
	cmd->coding_format = coding_format;
	cmd->vid = sys_cpu_to_le16(vid);
	cmd->cid = sys_cpu_to_le16(cid);
	cmd->cc_ltvs_len = cc_ltvs_len;
	if (cc_ltvs_len > 0U) {
		net_buf_simple_add_mem(&cmd_buffer, cc_ltvs, cc_ltvs_len);
	}
	cmd->metadata_ltvs_len = metadata_ltvs_len;
	if (metadata_ltvs_len > 0U) {
		net_buf_simple_add_mem(&cmd_buffer, metadata_ltvs, metadata_ltvs_len);
	}

	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_cap_broadcast_source_setup(uint8_t source_id, uint32_t broadcast_id,
						       uint32_t sdu_interval, uint8_t framing,
						       uint16_t max_sdu, uint8_t rtn,
						       uint16_t max_latency,
						       uint32_t presentation_delay, uint8_t flags)
{
	struct btp_cap_broadcast_source_setup_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_CAP;
	cmd_hdr->opcode = BTP_CAP_BROADCAST_SOURCE_SETUP;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	sys_put_le24(broadcast_id, cmd->broadcast_id);
	sys_put_le24(sdu_interval, cmd->sdu_interval);
	cmd->framing = framing;
	cmd->max_sdu = sys_cpu_to_le16(max_sdu);
	cmd->retransmission_num = rtn;
	cmd->max_transport_latency = sys_cpu_to_le16(max_latency);
	sys_put_le24(presentation_delay, cmd->presentation_delay);
	cmd->flags = flags;

	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_cap_broadcast_adv_start(uint8_t source_id)
{
	struct btp_cap_broadcast_adv_start_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_CAP;
	cmd_hdr->opcode = BTP_CAP_BROADCAST_ADV_START;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	cmd->source_id = source_id;

	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_cap_broadcast_source_start(uint8_t source_id)
{
	struct btp_cap_broadcast_source_start_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_CAP;
	cmd_hdr->opcode = BTP_CAP_BROADCAST_SOURCE_START;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	cmd->source_id = source_id;

	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_wait_for_cap_discovered(void)
{
	struct btp_cap_discovery_completed_ev *ev;
	struct net_buf *buf;

	bsim_btp_wait_for_evt(BTP_SERVICE_ID_CAP, BTP_CAP_EV_DISCOVERY_COMPLETED, &buf);
	ev = net_buf_pull_mem(buf, sizeof(*ev));

	TEST_ASSERT(ev->status == BTP_CAP_DISCOVERY_STATUS_SUCCESS);

	net_buf_unref(buf);
}

static inline void bsim_btp_wait_for_cap_unicast_start_completed(void)
{
	struct btp_cap_unicast_start_completed_ev *ev;
	struct net_buf *buf;

	bsim_btp_wait_for_evt(BTP_SERVICE_ID_CAP, BTP_CAP_EV_UNICAST_START_COMPLETED, &buf);
	ev = net_buf_pull_mem(buf, sizeof(*ev));

	TEST_ASSERT(ev->status == BTP_CAP_UNICAST_START_STATUS_SUCCESS);

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

static inline void bsim_btp_pbp_set_public_broadcast_announcement(uint8_t features)
{
	struct btp_pbp_set_public_broadcast_announcement_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_PBP;
	cmd_hdr->opcode = BTP_PBP_SET_PUBLIC_BROADCAST_ANNOUNCEMENT;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	cmd->features = features;

	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_pbp_set_broadcast_name(const char *broadcast_name)
{
	struct btp_pbp_set_broadcast_name_cmd *cmd;
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_PBP;
	cmd_hdr->opcode = BTP_PBP_SET_BROADCAST_NAME;
	cmd_hdr->index = BTP_INDEX;
	cmd = net_buf_simple_add(&cmd_buffer, sizeof(*cmd));
	cmd->name_len = strlen(broadcast_name) + 1 /* NULL terminator */;
	net_buf_simple_add_mem(&cmd_buffer, broadcast_name, cmd->name_len);

	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_pbp_broadcast_scan_start(void)
{
	struct btp_hdr *cmd_hdr;

	NET_BUF_SIMPLE_DEFINE(cmd_buffer, BTP_MTU);

	cmd_hdr = net_buf_simple_add(&cmd_buffer, sizeof(*cmd_hdr));
	cmd_hdr->service = BTP_SERVICE_ID_PBP;
	cmd_hdr->opcode = BTP_PBP_BROADCAST_SCAN_START;
	cmd_hdr->index = BTP_INDEX;

	/* The command for this is empty */
	cmd_hdr->len = cmd_buffer.len - sizeof(*cmd_hdr);

	bsim_btp_send_to_tester(cmd_buffer.data, cmd_buffer.len);
}

static inline void bsim_btp_wait_for_pbp_public_broadcast_announcement_found(bt_addr_le_t *address)
{
	struct btp_pbp_ev_public_broadcast_announcement_found_ev *ev;
	struct net_buf *buf;

	bsim_btp_wait_for_evt(BTP_SERVICE_ID_PBP, BTP_PBP_EV_PUBLIC_BROADCAST_ANNOUNCEMENT_FOUND,
			      &buf);
	ev = net_buf_pull_mem(buf, sizeof(*ev));

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
#endif /* BSIM_BTP_H_ */
