/* hci.h - Bluetooth Host Control Interface definitions */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __BT_HCI_H
#define __BT_HCI_H

#include <toolchain.h>
#include <stdint.h>

#define BT_ADDR_LE_PUBLIC  0x00
#define BT_ADDR_LE_RANDOM  0x01

typedef struct {
	uint8_t  val[6];
} bt_addr_t;

typedef struct {
	uint8_t  type;
	uint8_t  val[6];
} bt_addr_le_t;

#define BT_ADDR_ANY    (&(bt_addr_t) {{0, 0, 0, 0, 0, 0}})
#define BT_ADDR_LE_ANY (&(bt_addr_le_t) { 0, {0, 0, 0, 0, 0, 0}})

/* EIR/AD definitions */
#define BT_EIR_FLAGS			0x01 /* AD flags */
#define BT_EIR_UUID16_SOME		0x02 /* 16-bit UUID, more available */
#define BT_EIR_UUID16_ALL		0x03 /* 16-bit UUID, all listed */
#define BT_EIR_UUID32_SOME		0x04 /* 32-bit UUID, more available */
#define BT_EIR_UUID32_ALL		0x05 /* 32-bit UUID, all listed */
#define BT_EIR_UUID128_SOME		0x06 /* 128-bit UUID, more available */
#define BT_EIR_UUID128_ALL		0x07 /* 128-bit UUID, all listed */
#define BT_EIR_NAME_COMPLETE		0x09 /* Complete name */
#define BT_EIR_TX_POWER			0x0A /* Tx Power */
#define BT_EIR_SOLICIT16		0x14 /* Solicit UUIDs, 16-bit */
#define BT_EIR_SOLICIT128		0x15 /* Solicit UUIDs, 128-bit */
#define BT_EIR_SVC_DATA16		0x16 /* Service data, 16-bit UUID */
#define BT_EIR_GAP_APPEARANCE		0x19 /* GAP appearance */
#define BT_EIR_SOLICIT32		0x1F /* Solicit UUIDs, 32-bit */
#define BT_EIR_SVC_DATA32		0x20 /* Service data, 32-bit UUID */
#define BT_EIR_SVC_DATA128		0x21 /* Service data, 128-bit UUID */
#define BT_EIR_MANUFACTURER_DATA	0xFF /* Manufacturer Specific Data */

#define BT_LE_AD_GENERAL		0x02 /* General Discoverable */
#define BT_LE_AD_NO_BREDR		0x04 /* BR/EDR not supported */

struct bt_hci_evt_hdr {
	uint8_t  evt;
	uint8_t  len;
} PACK_STRUCT;

#define bt_acl_handle(h)		((h) & 0x0fff)

struct bt_hci_acl_hdr {
	uint16_t handle;
	uint16_t len;
} PACK_STRUCT;

struct bt_hci_cmd_hdr {
	uint16_t opcode;
	uint8_t  param_len;
} PACK_STRUCT;

/* LMP features */
#define BT_LMP_NO_BREDR				0x20
#define BT_LMP_LE				0x40

/* LE features */
#define BT_HCI_LE_ENCRYPTION			0x01

/* OpCode Group Fields */
#define BT_OGF_BASEBAND				0x03
#define BT_OGF_INFO				0x04
#define BT_OGF_LE				0x08

/* Construct OpCode from OGF and OCF */
#define BT_OP(ogf, ocf)				((ocf) | ((ogf) << 10))

#define BT_HCI_OP_SET_EVENT_MASK		BT_OP(BT_OGF_BASEBAND, 0x0001)
struct bt_hci_cp_set_event_mask {
	uint8_t  events[8];
} PACK_STRUCT;

#define BT_HCI_OP_RESET				BT_OP(BT_OGF_BASEBAND, 0x0003)

#define BT_HCI_OP_SET_CTL_TO_HOST_FLOW		BT_OP(BT_OGF_BASEBAND, 0x0031)

#define BT_HCI_OP_HOST_BUFFER_SIZE		BT_OP(BT_OGF_BASEBAND, 0x0033)
struct bt_hci_cp_host_buffer_size {
	uint16_t acl_mtu;
	uint8_t  sco_mtu;
	uint16_t acl_pkts;
	uint16_t sco_pkts;
} PACK_STRUCT;

struct bt_hci_handle_count {
	uint16_t handle;
	uint16_t count;
} PACK_STRUCT;

#define BT_HCI_OP_HOST_NUM_COMPLETED_PACKETS	BT_OP(BT_OGF_BASEBAND, 0x0035)
struct bt_hci_cp_host_num_completed_packets {
	uint8_t  num_handles;
	struct bt_hci_handle_count h[0];
} PACK_STRUCT;

#define BT_HCI_OP_LE_WRITE_LE_HOST_SUPP		BT_OP(BT_OGF_BASEBAND, 0x006d)
struct bt_hci_cp_write_le_host_supp {
	uint8_t  le;
	uint8_t  simul;
} PACK_STRUCT;

#define BT_HCI_OP_READ_LOCAL_VERSION_INFO	BT_OP(BT_OGF_INFO, 0x0001)
struct bt_hci_rp_read_local_version_info {
	uint8_t  status;
	uint8_t  hci_version;
	uint16_t hci_revision;
	uint8_t  lmp_version;
	uint16_t manufacturer;
	uint16_t lmp_subversion;
} PACK_STRUCT;

#define BT_HCI_OP_READ_LOCAL_FEATURES		BT_OP(BT_OGF_INFO, 0x0003)
struct bt_hci_rp_read_local_features {
	uint8_t  status;
	uint8_t  features[8];
} PACK_STRUCT;

#define BT_HCI_OP_READ_BUFFER_SIZE		BT_OP(BT_OGF_INFO, 0x0005)
struct bt_hci_rp_read_buffer_size {
	uint8_t  status;
	uint16_t acl_max_len;
	uint8_t  sco_max_len;
	uint16_t acl_max_num;
	uint16_t sco_max_num;
} PACK_STRUCT;

#define BT_HCI_OP_READ_BD_ADDR			BT_OP(BT_OGF_INFO, 0x0009)
struct bt_hci_rp_read_bd_addr {
	uint8_t   status;
	bt_addr_t bdaddr;
} PACK_STRUCT;

#define BT_HCI_OP_LE_READ_BUFFER_SIZE		BT_OP(BT_OGF_LE, 0x0002)
struct bt_hci_rp_le_read_buffer_size {
	uint8_t  status;
	uint16_t le_max_len;
	uint8_t  le_max_num;
} PACK_STRUCT;

#define BT_HCI_OP_LE_READ_LOCAL_FEATURES	BT_OP(BT_OGF_LE, 0x0003)
struct bt_hci_rp_le_read_local_features {
	uint8_t  status;
	uint8_t  features[8];
} PACK_STRUCT;

/* Advertising types */
#define BT_LE_ADV_IND				0x00
#define BT_LE_ADV_DIRECT_IND			0x01
#define BT_LE_ADV_SCAN_IND			0x02
#define BT_LE_ADV_NONCONN_IND			0x03
/* Needed in advertising reports when getting info about */
#define BT_LE_ADV_SCAN_RSP			0x04

#define BT_HCI_OP_LE_SET_ADV_PARAMETERS		BT_OP(BT_OGF_LE, 0x0006)
struct bt_hci_cp_le_set_adv_parameters {
	uint16_t     min_interval;
	uint16_t     max_interval;
	uint8_t      type;
	uint8_t      own_addr_type;
	bt_addr_le_t direct_addr;
	uint8_t      channel_map;
	uint8_t      filter_policy;
} PACK_STRUCT;

#define BT_HCI_OP_LE_SET_ADV_DATA		BT_OP(BT_OGF_LE, 0x0008)
struct bt_hci_cp_le_set_adv_data {
	uint8_t  len;
	uint8_t  data[31];
} PACK_STRUCT;

#define BT_HCI_OP_LE_SET_SCAN_RSP_DATA		BT_OP(BT_OGF_LE, 0x0009)
struct bt_hci_cp_le_set_scan_rsp_data {
	uint8_t  len;
	uint8_t  data[31];
} PACK_STRUCT;

#define BT_HCI_OP_LE_SET_ADV_ENABLE		BT_OP(BT_OGF_LE, 0x000a)
struct bt_hci_cp_le_set_adv_enable {
	uint8_t  enable;
} PACK_STRUCT;

/* Scan types */
#define BT_HCI_OP_LE_SET_SCAN_PARAMS		BT_OP(BT_OGF_LE, 0x000b)
#define BT_LE_SCAN_PASSIVE			0x00
#define BT_LE_SCAN_ACTIVE			0x01

struct bt_hci_cp_le_set_scan_params {
	uint8_t  scan_type;
	uint16_t interval;
	uint16_t window;
	uint8_t  addr_type;
	uint8_t  filter_policy;
} PACK_STRUCT;

#define BT_HCI_OP_LE_SET_SCAN_ENABLE		BT_OP(BT_OGF_LE, 0x000c)
#define BT_LE_SCAN_DISABLE			0x00
#define BT_LE_SCAN_ENABLE			0x01
#define BT_LE_SCAN_FILTER_DUP_DISABLE		0x00
#define BT_LE_SCAN_FILTER_DUP_ENABLE		0x01

struct bt_hci_cp_le_set_scan_enable {
	uint8_t  enable;
	uint8_t  filter_dup;
} PACK_STRUCT;

#define BT_HCI_OP_LE_ENCRYPT			BT_OP(BT_OGF_LE, 0x0017)
struct bt_hci_cp_le_encrypt {
	uint8_t  key[16];
	uint8_t  plaintext[16];
} PACK_STRUCT;
struct bt_hci_rp_le_encrypt {
	uint8_t  status;
	uint8_t  enc_data[16];
} PACK_STRUCT;

#define BT_HCI_OP_LE_RAND			BT_OP(BT_OGF_LE, 0x0018)
struct bt_hci_rp_le_rand {
	uint8_t  status;
	uint8_t  rand[8];
} PACK_STRUCT;

#define BT_HCI_OP_LE_LTK_REQ_REPLY		BT_OP(BT_OGF_LE, 0x001a)
struct bt_hci_cp_le_ltk_req_reply {
	uint16_t handle;
	uint8_t  ltk[16];
} PACK_STRUCT;

#define BT_HCI_OP_LE_LTK_REQ_NEG_REPLY		BT_OP(BT_OGF_LE, 0x001b)
struct bt_hci_cp_le_ltk_req_neg_reply {
	uint16_t handle;
} PACK_STRUCT;

/* Event definitions */

#define BT_HCI_EVT_DISCONN_COMPLETE		0x05
struct bt_hci_evt_disconn_complete {
	uint8_t  status;
	uint16_t handle;
	uint8_t  reason;
} PACK_STRUCT;

#define BT_HCI_EVT_ENCRYPT_CHANGE		0x08
struct bt_hci_evt_encrypt_change {
	uint8_t  status;
	uint16_t handle;
	uint8_t  encrypt;
} PACK_STRUCT;

#define BT_HCI_EVT_CMD_COMPLETE			0x0e
struct hci_evt_cmd_complete {
	uint8_t  ncmd;
	uint16_t opcode;
} PACK_STRUCT;

#define BT_HCI_EVT_CMD_STATUS			0x0f
struct bt_hci_evt_cmd_status {
	uint8_t  status;
	uint8_t  ncmd;
	uint16_t opcode;
} PACK_STRUCT;

#define BT_HCI_EVT_NUM_COMPLETED_PACKETS	0x13
struct bt_hci_evt_num_completed_packets {
	uint8_t  num_handles;
	struct bt_hci_handle_count h[0];
} PACK_STRUCT;

#define BT_HCI_EVT_LE_META_EVENT		0x3e
struct bt_hci_evt_le_meta_event {
	uint8_t  subevent;
} PACK_STRUCT;

#define BT_HCI_EVT_LE_CONN_COMPLETE		0x01
struct bt_hci_evt_le_conn_complete {
	uint8_t      status;
	uint16_t     handle;
	uint8_t      role;
	bt_addr_le_t peer_addr;
	uint16_t     interval;
	uint16_t     latency;
	uint16_t     supv_timeout;
	uint8_t      clock_accuracy;
} PACK_STRUCT;

#define BT_HCI_EVT_LE_ADVERTISING_REPORT	0x02
struct bt_hci_ev_le_advertising_info {
	uint8_t      evt_type;
	bt_addr_le_t addr;
	uint8_t      length;
	uint8_t      data[0];
} PACK_STRUCT;

#define BT_HCI_EVT_LE_LTK_REQUEST		0x05
struct bt_hci_evt_le_ltk_request {
	uint16_t handle;
	uint64_t rand;
	uint16_t ediv;
} PACK_STRUCT;

#endif /* __BT_HCI_H */
