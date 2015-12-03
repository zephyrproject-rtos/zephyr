/* hci.h - Bluetooth Host Control Interface definitions */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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

#define BT_ADDR_ANY    (&(bt_addr_t) {{0, 0, 0, 0, 0, 0} })
#define BT_ADDR_LE_ANY (&(bt_addr_le_t) { 0, {0, 0, 0, 0, 0, 0} })

/* HCI Error Codes */
#define BT_HCI_ERR_UNKNOWN_CONN_ID		0x02
#define BT_HCI_ERR_AUTHENTICATION_FAIL		0x05
#define BT_HCI_ERR_REMOTE_USER_TERM_CONN	0x13
#define BT_HCI_ERR_UNSUPP_REMOTE_FEATURE	0x1a
#define BT_HCI_ERR_INVALID_LL_PARAMS		0x1e
#define BT_HCI_ERR_PAIRING_NOT_SUPPORTED	0x29
#define BT_HCI_ERR_UNACCEPT_CONN_PARAMS		0x3b

/* EIR/AD definitions */
#define BT_EIR_FLAGS			0x01 /* AD flags */
#define BT_EIR_UUID16_SOME		0x02 /* 16-bit UUID, more available */
#define BT_EIR_UUID16_ALL		0x03 /* 16-bit UUID, all listed */
#define BT_EIR_UUID32_SOME		0x04 /* 32-bit UUID, more available */
#define BT_EIR_UUID32_ALL		0x05 /* 32-bit UUID, all listed */
#define BT_EIR_UUID128_SOME		0x06 /* 128-bit UUID, more available */
#define BT_EIR_UUID128_ALL		0x07 /* 128-bit UUID, all listed */
#define BT_EIR_NAME_COMPLETE		0x09 /* Complete name */
#define BT_EIR_TX_POWER			0x0a /* Tx Power */
#define BT_EIR_SOLICIT16		0x14 /* Solicit UUIDs, 16-bit */
#define BT_EIR_SOLICIT128		0x15 /* Solicit UUIDs, 128-bit */
#define BT_EIR_SVC_DATA16		0x16 /* Service data, 16-bit UUID */
#define BT_EIR_GAP_APPEARANCE		0x19 /* GAP appearance */
#define BT_EIR_SOLICIT32		0x1f /* Solicit UUIDs, 32-bit */
#define BT_EIR_SVC_DATA32		0x20 /* Service data, 32-bit UUID */
#define BT_EIR_SVC_DATA128		0x21 /* Service data, 128-bit UUID */
#define BT_EIR_MANUFACTURER_DATA	0xff /* Manufacturer Specific Data */

#define BT_LE_AD_LIMITED		0x01 /* Limited Discoverable */
#define BT_LE_AD_GENERAL		0x02 /* General Discoverable */
#define BT_LE_AD_NO_BREDR		0x04 /* BR/EDR not supported */

struct bt_hci_evt_hdr {
	uint8_t  evt;
	uint8_t  len;
} __packed;

#define BT_ACL_START_NO_FLUSH		0x00
#define BT_ACL_CONT			0x01
#define BT_ACL_START			0x02

#define bt_acl_handle(h)		((h) & 0x0fff)
#define bt_acl_flags(h)			((h) >> 12)
#define bt_acl_handle_pack(h, f)	((h) | ((f) << 12))

struct bt_hci_acl_hdr {
	uint16_t handle;
	uint16_t len;
} __packed;

struct bt_hci_cmd_hdr {
	uint16_t opcode;
	uint8_t  param_len;
} __packed;

/* LMP features */
#define BT_LMP_NO_BREDR				0x20
#define BT_LMP_LE				0x40

/* LE features */
#define BT_HCI_LE_ENCRYPTION			0x01
#define BT_HCI_LE_CONN_PARAM_REQ_PROC		0x02
#define BT_HCI_LE_SLAVE_FEATURES		0x08

/* Defined GAP timers */
#define BT_GAP_SCAN_FAST_INTERVAL		0x0060	/* 60 ms    */
#define BT_GAP_SCAN_FAST_WINDOW			0x0030  /* 30 ms    */
#define BT_GAP_SCAN_SLOW_INTERVAL_1		0x0800  /* 1.28 s   */
#define BT_GAP_SCAN_SLOW_WINDOW_1		0x0012  /* 11.25 ms */
#define BT_GAP_SCAN_SLOW_INTERVAL_2		0x1000  /* 2.56 s   */
#define BT_GAP_SCAN_SLOW_WINDOW_2		0x0012  /* 11.25 ms */
#define BT_GAP_ADV_FAST_INT_MIN_1		0x0030	/* 30 ms    */
#define BT_GAP_ADV_FAST_INT_MAX_1		0x0060  /* 60 ms    */
#define BT_GAP_ADV_FAST_INT_MIN_2		0x00a0  /* 100 ms   */
#define BT_GAP_ADV_FAST_INT_MAX_2		0x00f0  /* 150 ms   */
#define BT_GAP_ADV_SLOW_INT_MIN			0x0640  /* 1 s      */
#define BT_GAP_ADV_SLOW_INT_MAX			0x0780  /* 1.2 s    */
#define BT_GAP_INIT_CONN_INT_MIN		0x0018  /* 30 ms    */
#define BT_GAP_INIT_CONN_INT_MAX		0x0028  /* 50 ms    */

/* OpCode Group Fields */
#define BT_OGF_LINK_CTRL			0x01
#define BT_OGF_BASEBAND				0x03
#define BT_OGF_INFO				0x04
#define BT_OGF_LE				0x08

/* Construct OpCode from OGF and OCF */
#define BT_OP(ogf, ocf)				((ocf) | ((ogf) << 10))

#define BT_HCI_OP_DISCONNECT			BT_OP(BT_OGF_LINK_CTRL, 0x0006)
struct bt_hci_cp_disconnect {
	uint16_t handle;
	uint8_t  reason;
} __packed;

#define BT_HCI_OP_SET_EVENT_MASK		BT_OP(BT_OGF_BASEBAND, 0x0001)
struct bt_hci_cp_set_event_mask {
	uint8_t  events[8];
} __packed;

#define BT_HCI_OP_RESET				BT_OP(BT_OGF_BASEBAND, 0x0003)

#define BT_HCI_OP_WRITE_SCAN_ENABLE		BT_OP(BT_OGF_BASEBAND, 0x001a)
#define BT_BREDR_SCAN_DISABLED			0x00
#define BT_BREDR_SCAN_INQUIRY			0x01
#define BT_BREDR_SCAN_PAGE			0x02

#define BT_HCI_OP_SET_CTL_TO_HOST_FLOW		BT_OP(BT_OGF_BASEBAND, 0x0031)

#define BT_HCI_OP_HOST_BUFFER_SIZE		BT_OP(BT_OGF_BASEBAND, 0x0033)
struct bt_hci_cp_host_buffer_size {
	uint16_t acl_mtu;
	uint8_t  sco_mtu;
	uint16_t acl_pkts;
	uint16_t sco_pkts;
} __packed;

struct bt_hci_handle_count {
	uint16_t handle;
	uint16_t count;
} __packed;

#define BT_HCI_OP_HOST_NUM_COMPLETED_PACKETS	BT_OP(BT_OGF_BASEBAND, 0x0035)
struct bt_hci_cp_host_num_completed_packets {
	uint8_t  num_handles;
	struct bt_hci_handle_count h[0];
} __packed;

#define BT_HCI_OP_LE_WRITE_LE_HOST_SUPP		BT_OP(BT_OGF_BASEBAND, 0x006d)
struct bt_hci_cp_write_le_host_supp {
	uint8_t  le;
	uint8_t  simul;
} __packed;

#define BT_HCI_OP_READ_LOCAL_VERSION_INFO	BT_OP(BT_OGF_INFO, 0x0001)
struct bt_hci_rp_read_local_version_info {
	uint8_t  status;
	uint8_t  hci_version;
	uint16_t hci_revision;
	uint8_t  lmp_version;
	uint16_t manufacturer;
	uint16_t lmp_subversion;
} __packed;

#define BT_HCI_OP_READ_SUPPORTED_COMMANDS	BT_OP(BT_OGF_INFO, 0x0002)
struct bt_hci_rp_read_supported_commands {
	uint8_t  status;
	uint8_t  commands[36];
} __packed;

#define BT_HCI_OP_READ_LOCAL_FEATURES		BT_OP(BT_OGF_INFO, 0x0003)
struct bt_hci_rp_read_local_features {
	uint8_t  status;
	uint8_t  features[8];
} __packed;

#define BT_HCI_OP_READ_BUFFER_SIZE		BT_OP(BT_OGF_INFO, 0x0005)
struct bt_hci_rp_read_buffer_size {
	uint8_t  status;
	uint16_t acl_max_len;
	uint8_t  sco_max_len;
	uint16_t acl_max_num;
	uint16_t sco_max_num;
} __packed;

#define BT_HCI_OP_READ_BD_ADDR			BT_OP(BT_OGF_INFO, 0x0009)
struct bt_hci_rp_read_bd_addr {
	uint8_t   status;
	bt_addr_t bdaddr;
} __packed;

#define BT_HCI_OP_LE_SET_EVENT_MASK		BT_OP(BT_OGF_LE, 0x0001)
struct bt_hci_cp_le_set_event_mask {
	uint8_t events[8];
} __packed;
struct bt_hci_rp_le_set_event_mask {
	uint8_t status;
} __packed;

#define BT_HCI_OP_LE_READ_BUFFER_SIZE		BT_OP(BT_OGF_LE, 0x0002)
struct bt_hci_rp_le_read_buffer_size {
	uint8_t  status;
	uint16_t le_max_len;
	uint8_t  le_max_num;
} __packed;

#define BT_HCI_OP_LE_READ_LOCAL_FEATURES	BT_OP(BT_OGF_LE, 0x0003)
struct bt_hci_rp_le_read_local_features {
	uint8_t  status;
	uint8_t  features[8];
} __packed;

#define BT_HCI_OP_LE_SET_RANDOM_ADDRESS		BT_OP(BT_OGF_LE, 0x0005)

/* Advertising types */
#define BT_LE_ADV_IND				0x00
#define BT_LE_ADV_DIRECT_IND			0x01
#define BT_LE_ADV_SCAN_IND			0x02
#define BT_LE_ADV_NONCONN_IND			0x03
#define BT_LE_ADV_DIRECT_IND_LOW_DUTY		0x04
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
} __packed;

#define BT_HCI_OP_LE_SET_ADV_DATA		BT_OP(BT_OGF_LE, 0x0008)
struct bt_hci_cp_le_set_adv_data {
	uint8_t  len;
	uint8_t  data[31];
} __packed;

#define BT_HCI_OP_LE_SET_SCAN_RSP_DATA		BT_OP(BT_OGF_LE, 0x0009)
struct bt_hci_cp_le_set_scan_rsp_data {
	uint8_t  len;
	uint8_t  data[31];
} __packed;

#define BT_HCI_OP_LE_SET_ADV_ENABLE		BT_OP(BT_OGF_LE, 0x000a)
struct bt_hci_cp_le_set_adv_enable {
	uint8_t  enable;
} __packed;

/* Scan types */
#define BT_HCI_OP_LE_SET_SCAN_PARAMS		BT_OP(BT_OGF_LE, 0x000b)
#define BT_HCI_LE_SCAN_PASSIVE			0x00
#define BT_HCI_LE_SCAN_ACTIVE			0x01

struct bt_hci_cp_le_set_scan_params {
	uint8_t  scan_type;
	uint16_t interval;
	uint16_t window;
	uint8_t  addr_type;
	uint8_t  filter_policy;
} __packed;

#define BT_HCI_OP_LE_SET_SCAN_ENABLE		BT_OP(BT_OGF_LE, 0x000c)

#define BT_HCI_LE_SCAN_DISABLE			0x00
#define BT_HCI_LE_SCAN_ENABLE			0x01

#define BT_HCI_LE_SCAN_FILTER_DUP_DISABLE	0x00
#define BT_HCI_LE_SCAN_FILTER_DUP_ENABLE	0x01

struct bt_hci_cp_le_set_scan_enable {
	uint8_t  enable;
	uint8_t  filter_dup;
} __packed;

#define BT_HCI_OP_LE_CREATE_CONN		BT_OP(BT_OGF_LE, 0x000d)
struct bt_hci_cp_le_create_conn {
	uint16_t     scan_interval;
	uint16_t     scan_window;
	uint8_t      filter_policy;
	bt_addr_le_t peer_addr;
	uint8_t      own_addr_type;
	uint16_t     conn_interval_min;
	uint16_t     conn_interval_max;
	uint16_t     conn_latency;
	uint16_t     supervision_timeout;
	uint16_t     min_ce_len;
	uint16_t     max_ce_len;
} __packed;

#define BT_HCI_OP_LE_CREATE_CONN_CANCEL		BT_OP(BT_OGF_LE, 0x000e)

#define BT_HCI_OP_LE_CONN_UPDATE		BT_OP(BT_OGF_LE, 0x0013)
struct hci_cp_le_conn_update {
	uint16_t handle;
	uint16_t conn_interval_min;
	uint16_t conn_interval_max;
	uint16_t conn_latency;
	uint16_t supervision_timeout;
	uint16_t min_ce_len;
	uint16_t max_ce_len;
} __packed;

#define BT_HCI_OP_LE_READ_REMOTE_FEATURES	BT_OP(BT_OGF_LE, 0x0016)
struct bt_hci_cp_le_read_remote_features {
	uint16_t  handle;
} __packed;

#define BT_HCI_OP_LE_ENCRYPT			BT_OP(BT_OGF_LE, 0x0017)
struct bt_hci_cp_le_encrypt {
	uint8_t  key[16];
	uint8_t  plaintext[16];
} __packed;
struct bt_hci_rp_le_encrypt {
	uint8_t  status;
	uint8_t  enc_data[16];
} __packed;

#define BT_HCI_OP_LE_RAND			BT_OP(BT_OGF_LE, 0x0018)
struct bt_hci_rp_le_rand {
	uint8_t  status;
	uint8_t  rand[8];
} __packed;

#define BT_HCI_OP_LE_START_ENCRYPTION		BT_OP(BT_OGF_LE, 0x0019)
struct bt_hci_cp_le_start_encryption {
	uint16_t handle;
	uint64_t rand;
	uint16_t ediv;
	uint8_t  ltk[16];
} __packed;

#define BT_HCI_OP_LE_LTK_REQ_REPLY		BT_OP(BT_OGF_LE, 0x001a)
struct bt_hci_cp_le_ltk_req_reply {
	uint16_t handle;
	uint8_t  ltk[16];
} __packed;

#define BT_HCI_OP_LE_LTK_REQ_NEG_REPLY		BT_OP(BT_OGF_LE, 0x001b)
struct bt_hci_cp_le_ltk_req_neg_reply {
	uint16_t handle;
} __packed;

#define BT_HCI_OP_LE_CONN_PARAM_REQ_REPLY	BT_OP(BT_OGF_LE, 0x0020)
struct bt_hci_cp_le_conn_param_req_reply {
	uint16_t handle;
	uint16_t interval_min;
	uint16_t interval_max;
	uint16_t latency;
	uint16_t timeout;
	uint16_t min_ce_len;
	uint16_t max_ce_len;
} __packed;

#define BT_HCI_OP_LE_CONN_PARAM_REQ_NEG_REPLY	BT_OP(BT_OGF_LE, 0x0021)
struct bt_hci_cp_le_conn_param_req_neg_reply {
	uint16_t handle;
	uint8_t  reason;
} __packed;

#define BT_HCI_OP_LE_P256_PUBLIC_KEY		BT_OP(BT_OGF_LE, 0x0025)

#define BT_HCI_OP_LE_GENERATE_DHKEY		BT_OP(BT_OGF_LE, 0x0026)
struct bt_hci_cp_le_generate_dhkey {
	uint8_t key[64];
} __packed;

/* Event definitions */

#define BT_HCI_EVT_CONN_REQUEST			0x04
struct bt_hci_evt_conn_request {
	bt_addr_t bdaddr;
	uint8_t   dev_class[3];
	uint8_t   link_type;
} __packed;

#define BT_HCI_EVT_DISCONN_COMPLETE		0x05
struct bt_hci_evt_disconn_complete {
	uint8_t  status;
	uint16_t handle;
	uint8_t  reason;
} __packed;

#define BT_HCI_EVT_ENCRYPT_CHANGE		0x08
struct bt_hci_evt_encrypt_change {
	uint8_t  status;
	uint16_t handle;
	uint8_t  encrypt;
} __packed;

#define BT_HCI_EVT_CMD_COMPLETE			0x0e
struct hci_evt_cmd_complete {
	uint8_t  ncmd;
	uint16_t opcode;
} __packed;

#define BT_HCI_EVT_CMD_STATUS			0x0f
struct bt_hci_evt_cmd_status {
	uint8_t  status;
	uint8_t  ncmd;
	uint16_t opcode;
} __packed;

#define BT_HCI_EVT_NUM_COMPLETED_PACKETS	0x13
struct bt_hci_evt_num_completed_packets {
	uint8_t  num_handles;
	struct bt_hci_handle_count h[0];
} __packed;

#define BT_HCI_EVT_ENCRYPT_KEY_REFRESH_COMPLETE	0x30
struct bt_hci_evt_encrypt_key_refresh_complete {
	uint8_t  status;
	uint16_t handle;
} __packed;

#define BT_HCI_EVT_LE_META_EVENT		0x3e
struct bt_hci_evt_le_meta_event {
	uint8_t  subevent;
} __packed;

#define BT_HCI_ROLE_MASTER			0x00
#define BT_HCI_ROLE_SLAVE			0x01

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
} __packed;

#define BT_HCI_EVT_LE_ADVERTISING_REPORT	0x02
struct bt_hci_ev_le_advertising_info {
	uint8_t      evt_type;
	bt_addr_le_t addr;
	uint8_t      length;
	uint8_t      data[0];
} __packed;

#define BT_HCI_EVT_LE_CONN_UPDATE_COMPLETE	0x03
struct bt_hci_evt_le_conn_update_complete {
	uint8_t  status;
	uint16_t handle;
	uint16_t interval;
	uint16_t latency;
	uint16_t supv_timeout;
} __packed;

#define BT_HCI_EV_LE_REMOTE_FEAT_COMPLETE	0x04
struct bt_hci_ev_le_remote_feat_complete {
	uint8_t  status;
	uint16_t handle;
	uint8_t  features[8];
} __packed;

#define BT_HCI_EVT_LE_LTK_REQUEST		0x05
struct bt_hci_evt_le_ltk_request {
	uint16_t handle;
	uint64_t rand;
	uint16_t ediv;
} __packed;

#define BT_HCI_EVT_LE_CONN_PARAM_REQ		0x06
struct bt_hci_evt_le_conn_param_req {
	uint16_t handle;
	uint16_t interval_min;
	uint16_t interval_max;
	uint16_t latency;
	uint16_t timeout;
} __packed;

#define BT_HCI_EVT_LE_P256_PUBLIC_KEY_COMPLETE	0x08
struct bt_hci_evt_le_p256_public_key_complete {
	uint8_t status;
	uint8_t key[64];
} __packed;

#define BT_HCI_EVT_LE_GENERATE_DHKEY_COMPLETE	0x09
struct bt_hci_evt_le_generate_dhkey_complete {
	uint8_t status;
	uint8_t dhkey[32];
} __packed;

#endif /* __BT_HCI_H */
