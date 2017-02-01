/* hci.h - Bluetooth Host Control Interface definitions */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __BT_HCI_H
#define __BT_HCI_H

#include <toolchain.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <misc/util.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BT_ADDR_LE_PUBLIC       0x00
#define BT_ADDR_LE_RANDOM       0x01
#define BT_ADDR_LE_PUBLIC_ID    0x02
#define BT_ADDR_LE_RANDOM_ID    0x03

typedef struct {
	uint8_t  val[6];
} bt_addr_t;

typedef struct {
	uint8_t   type;
	bt_addr_t a;
} bt_addr_le_t;

#define BT_ADDR_ANY     (&(bt_addr_t) {{0, 0, 0, 0, 0, 0} })
#define BT_ADDR_LE_ANY  (&(bt_addr_le_t) { 0, { {0, 0, 0, 0, 0, 0} } })
#define BT_ADDR_LE_NONE (&(bt_addr_le_t) { 0, \
			 { { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff } } })

static inline int bt_addr_cmp(const bt_addr_t *a, const bt_addr_t *b)
{
	return memcmp(a, b, sizeof(*a));
}

static inline int bt_addr_le_cmp(const bt_addr_le_t *a, const bt_addr_le_t *b)
{
	return memcmp(a, b, sizeof(*a));
}

static inline void bt_addr_copy(bt_addr_t *dst, const bt_addr_t *src)
{
	memcpy(dst, src, sizeof(*dst));
}

static inline void bt_addr_le_copy(bt_addr_le_t *dst, const bt_addr_le_t *src)
{
	memcpy(dst, src, sizeof(*dst));
}

#define BT_ADDR_IS_RPA(addr)     (((addr)->val[5] & 0xc0) == 0x40)
#define BT_ADDR_IS_NRPA(addr)    (((addr)->val[5] & 0xc0) == 0x00)
#define BT_ADDR_IS_STATIC(addr)  (((addr)->val[5] & 0xc0) == 0xc0)

#define BT_ADDR_SET_RPA(addr)    ((addr)->val[5] = \
					(((addr)->val[5] & 0x3f) | 0x40))
#define BT_ADDR_SET_NRPA(addr)   ((addr)->val[5] &= 0x3f)
#define BT_ADDR_SET_STATIC(addr) ((addr)->val[5] |= 0xc0)

int bt_addr_le_create_nrpa(bt_addr_le_t *addr);
int bt_addr_le_create_static(bt_addr_le_t *addr);

static inline bool bt_addr_le_is_rpa(const bt_addr_le_t *addr)
{
	if (addr->type != BT_ADDR_LE_RANDOM) {
		return false;
	}

	return BT_ADDR_IS_RPA(&addr->a);
}

static inline bool bt_addr_le_is_identity(const bt_addr_le_t *addr)
{
	if (addr->type == BT_ADDR_LE_PUBLIC) {
		return true;
	}

	return BT_ADDR_IS_STATIC(&addr->a);
}

/* HCI Error Codes */
#define BT_HCI_ERR_UNKNOWN_CMD                  0x01
#define BT_HCI_ERR_UNKNOWN_CONN_ID              0x02
#define BT_HCI_ERR_AUTHENTICATION_FAIL          0x05
#define BT_HCI_ERR_PIN_OR_KEY_MISSING           0x06
#define BT_HCI_ERR_MEM_CAPACITY_EXCEEDED        0x07
#define BT_HCI_ERR_CMD_DISALLOWED               0x0c
#define BT_HCI_ERR_INSUFFICIENT_RESOURCES       0x0d
#define BT_HCI_ERR_UNSUPP_FEATURE_PARAMS_VAL    0x11
#define BT_HCI_ERR_INVALID_PARAMS               0x12
#define BT_HCI_ERR_REMOTE_USER_TERM_CONN        0x13
#define BT_HCI_ERR_PAIRING_NOT_ALLOWED          0x18
#define BT_HCI_ERR_UNSUPP_REMOTE_FEATURE        0x1a
#define BT_HCI_ERR_INVALID_LL_PARAMS            0x1e
#define BT_HCI_ERR_UNSPECIFIED                  0x1f
#define BT_HCI_ERR_PAIRING_NOT_SUPPORTED        0x29
#define BT_HCI_ERR_UNACCEPT_CONN_PARAMS         0x3b

/* EIR/AD data type definitions */
#define BT_DATA_FLAGS                   0x01 /* AD flags */
#define BT_DATA_UUID16_SOME             0x02 /* 16-bit UUID, more available */
#define BT_DATA_UUID16_ALL              0x03 /* 16-bit UUID, all listed */
#define BT_DATA_UUID32_SOME             0x04 /* 32-bit UUID, more available */
#define BT_DATA_UUID32_ALL              0x05 /* 32-bit UUID, all listed */
#define BT_DATA_UUID128_SOME            0x06 /* 128-bit UUID, more available */
#define BT_DATA_UUID128_ALL             0x07 /* 128-bit UUID, all listed */
#define BT_DATA_NAME_SHORTENED          0x08 /* Shortened name */
#define BT_DATA_NAME_COMPLETE           0x09 /* Complete name */
#define BT_DATA_TX_POWER                0x0a /* Tx Power */
#define BT_DATA_SOLICIT16               0x14 /* Solicit UUIDs, 16-bit */
#define BT_DATA_SOLICIT128              0x15 /* Solicit UUIDs, 128-bit */
#define BT_DATA_SVC_DATA16              0x16 /* Service data, 16-bit UUID */
#define BT_DATA_GAP_APPEARANCE          0x19 /* GAP appearance */
#define BT_DATA_SOLICIT32               0x1f /* Solicit UUIDs, 32-bit */
#define BT_DATA_SVC_DATA32              0x20 /* Service data, 32-bit UUID */
#define BT_DATA_SVC_DATA128             0x21 /* Service data, 128-bit UUID */
#define BT_DATA_MANUFACTURER_DATA       0xff /* Manufacturer Specific Data */

#define BT_LE_AD_LIMITED                0x01 /* Limited Discoverable */
#define BT_LE_AD_GENERAL                0x02 /* General Discoverable */
#define BT_LE_AD_NO_BREDR               0x04 /* BR/EDR not supported */

struct bt_hci_evt_hdr {
	uint8_t  evt;
	uint8_t  len;
} __packed;
#define BT_HCI_EVT_HDR_SIZE             2

#define BT_ACL_START_NO_FLUSH           0x00
#define BT_ACL_CONT                     0x01
#define BT_ACL_START                    0x02

#define bt_acl_handle(h)                ((h) & 0x0fff)
#define bt_acl_flags(h)                 ((h) >> 12)
#define bt_acl_handle_pack(h, f)        ((h) | ((f) << 12))

struct bt_hci_acl_hdr {
	uint16_t handle;
	uint16_t len;
} __packed;
#define BT_HCI_ACL_HDR_SIZE             4

struct bt_hci_cmd_hdr {
	uint16_t opcode;
	uint8_t  param_len;
} __packed;
#define BT_HCI_CMD_HDR_SIZE             3

/* Supported Commands */
#define BT_CMD_TEST(cmd, octet, bit)            (cmd[octet] & BIT(bit))
#define BT_CMD_LE_STATES(cmd)                   BT_CMD_TEST(cmd, 28, 3)

#define BT_FEAT_TEST(feat, page, octet, bit)    (feat[page][octet] & BIT(bit))

#define BT_FEAT_BREDR(feat)                     !BT_FEAT_TEST(feat, 0, 4, 5)
#define BT_FEAT_LE(feat)                        BT_FEAT_TEST(feat, 0, 4, 6)
#define BT_FEAT_EXT_FEATURES(feat)              BT_FEAT_TEST(feat, 0, 7, 7)
#define BT_FEAT_HOST_SSP(feat)                  BT_FEAT_TEST(feat, 1, 0, 0)
#define BT_FEAT_SC(feat)                        BT_FEAT_TEST(feat, 2, 1, 0)

/* LE features */
#define BT_LE_FEAT_BIT_ENC                      0
#define BT_LE_FEAT_BIT_CONN_PARAM_REQ           1
#define BT_LE_FEAT_BIT_EXT_REJ_IND              2
#define BT_LE_FEAT_BIT_SLAVE_FEAT_REQ           3
#define BT_LE_FEAT_BIT_PING                     4
#define BT_LE_FEAT_BIT_DLE                      5
#define BT_LE_FEAT_BIT_PRIVACY                  6
#define BT_LE_FEAT_BIT_EXT_SCAN                 7

#define BT_FEAT_LE_ENCR(feat)                   BT_FEAT_TEST(feat, 0, 0, \
						BT_LE_FEAT_BIT_ENC)
#define BT_FEAT_LE_CONN_PARAM_REQ_PROC(feat)    BT_FEAT_TEST(feat, 0, 0, \
						BT_LE_FEAT_BIT_CONN_PARAM_REQ)
#define BT_FEAT_LE_SLAVE_FEATURE_XCHG(feat)     BT_FEAT_TEST(feat, 0, 0, \
						BT_LE_FEAT_BIT_SLAVE_FEAT_REQ)
#define BT_FEAT_LE_DLE(feat)                    BT_FEAT_TEST(feat, 0, 0, \
						BT_LE_FEAT_BIT_DLE)

/* LE States */
#define BT_LE_STATES_SLAVE_CONN_ADV(states)     (states & 0x0000004000000000)

/* Bonding/authentication types */
#define BT_HCI_NO_BONDING                       0x00
#define BT_HCI_NO_BONDING_MITM                  0x01
#define BT_HCI_DEDICATED_BONDING                0x02
#define BT_HCI_DEDICATED_BONDING_MITM           0x03
#define BT_HCI_GENERAL_BONDING                  0x04
#define BT_HCI_GENERAL_BONDING_MITM             0x05

/*
 * MITM protection is enabled in SSP authentication requirements octet when
 * LSB bit is set.
 */
#define BT_MITM                                 0x01

/* I/O capabilities */
#define BT_IO_DISPLAY_ONLY                      0x00
#define BT_IO_DISPLAY_YESNO                     0x01
#define BT_IO_KEYBOARD_ONLY                     0x02
#define BT_IO_NO_INPUT_OUTPUT                   0x03

/* Defined GAP timers */
#define BT_GAP_SCAN_FAST_INTERVAL               0x0060  /* 60 ms    */
#define BT_GAP_SCAN_FAST_WINDOW                 0x0030  /* 30 ms    */
#define BT_GAP_SCAN_SLOW_INTERVAL_1             0x0800  /* 1.28 s   */
#define BT_GAP_SCAN_SLOW_WINDOW_1               0x0012  /* 11.25 ms */
#define BT_GAP_SCAN_SLOW_INTERVAL_2             0x1000  /* 2.56 s   */
#define BT_GAP_SCAN_SLOW_WINDOW_2               0x0012  /* 11.25 ms */
#define BT_GAP_ADV_FAST_INT_MIN_1               0x0030  /* 30 ms    */
#define BT_GAP_ADV_FAST_INT_MAX_1               0x0060  /* 60 ms    */
#define BT_GAP_ADV_FAST_INT_MIN_2               0x00a0  /* 100 ms   */
#define BT_GAP_ADV_FAST_INT_MAX_2               0x00f0  /* 150 ms   */
#define BT_GAP_ADV_SLOW_INT_MIN                 0x0640  /* 1 s      */
#define BT_GAP_ADV_SLOW_INT_MAX                 0x0780  /* 1.2 s    */
#define BT_GAP_INIT_CONN_INT_MIN                0x0018  /* 30 ms    */
#define BT_GAP_INIT_CONN_INT_MAX                0x0028  /* 50 ms    */

/* HCI BR/EDR link types */
#define BT_HCI_SCO                              0x00
#define BT_HCI_ACL                              0x01
#define BT_HCI_ESCO                             0x02

/* OpCode Group Fields */
#define BT_OGF_LINK_CTRL                        0x01
#define BT_OGF_BASEBAND                         0x03
#define BT_OGF_INFO                             0x04
#define BT_OGF_STATUS                           0x05
#define BT_OGF_LE                               0x08
#define BT_OGF_VS                               0x3f

/* Construct OpCode from OGF and OCF */
#define BT_OP(ogf, ocf)                         ((ocf) | ((ogf) << 10))

/* Obtain OGF from OpCode */
#define BT_OGF(opcode)                          (((opcode) >> 10) & BIT_MASK(6))
/* Obtain OCF from OpCode */
#define BT_OCF(opcode)                          ((opcode) & BIT_MASK(10))

#define BT_HCI_OP_INQUIRY                       BT_OP(BT_OGF_LINK_CTRL, 0x0001)
struct bt_hci_op_inquiry {
	uint8_t lap[3];
	uint8_t length;
	uint8_t num_rsp;
} __packed;

#define BT_HCI_OP_INQUIRY_CANCEL                BT_OP(BT_OGF_LINK_CTRL, 0x0002)

#define BT_HCI_OP_CONNECT                       BT_OP(BT_OGF_LINK_CTRL, 0x0005)
struct bt_hci_cp_connect {
	bt_addr_t bdaddr;
	uint16_t  packet_type;
	uint8_t   pscan_rep_mode;
	uint8_t   reserved;
	uint16_t  clock_offset;
	uint8_t   allow_role_switch;
} __packed;

#define BT_HCI_OP_DISCONNECT                    BT_OP(BT_OGF_LINK_CTRL, 0x0006)
struct bt_hci_cp_disconnect {
	uint16_t handle;
	uint8_t  reason;
} __packed;

#define BT_HCI_OP_CONNECT_CANCEL                BT_OP(BT_OGF_LINK_CTRL, 0x0008)
struct bt_hci_cp_connect_cancel {
	bt_addr_t bdaddr;
} __packed;
struct bt_hci_rp_connect_cancel {
	uint8_t   status;
	bt_addr_t bdaddr;
} __packed;

#define BT_HCI_OP_ACCEPT_CONN_REQ               BT_OP(BT_OGF_LINK_CTRL, 0x0009)
struct bt_hci_cp_accept_conn_req {
	bt_addr_t bdaddr;
	uint8_t   role;
} __packed;

#define BT_HCI_OP_REJECT_CONN_REQ               BT_OP(BT_OGF_LINK_CTRL, 0x000a)
struct bt_hci_cp_reject_conn_req {
	bt_addr_t bdaddr;
	uint8_t   reason;
} __packed;

#define BT_HCI_OP_LINK_KEY_REPLY                BT_OP(BT_OGF_LINK_CTRL, 0x000b)
struct bt_hci_cp_link_key_reply {
	bt_addr_t bdaddr;
	uint8_t   link_key[16];
} __packed;

#define BT_HCI_OP_LINK_KEY_NEG_REPLY            BT_OP(BT_OGF_LINK_CTRL, 0x000c)
struct bt_hci_cp_link_key_neg_reply {
	bt_addr_t bdaddr;
} __packed;

#define BT_HCI_OP_PIN_CODE_REPLY                BT_OP(BT_OGF_LINK_CTRL, 0x000d)
struct bt_hci_cp_pin_code_reply {
	bt_addr_t bdaddr;
	uint8_t   pin_len;
	uint8_t   pin_code[16];
} __packed;
struct bt_hci_rp_pin_code_reply {
	uint8_t   status;
	bt_addr_t bdaddr;
} __packed;

#define BT_HCI_OP_PIN_CODE_NEG_REPLY            BT_OP(BT_OGF_LINK_CTRL, 0x000e)
struct bt_hci_cp_pin_code_neg_reply {
	bt_addr_t bdaddr;
} __packed;
struct bt_hci_rp_pin_code_neg_reply {
	uint8_t   status;
	bt_addr_t bdaddr;
} __packed;

#define BT_HCI_OP_AUTH_REQUESTED                BT_OP(BT_OGF_LINK_CTRL, 0x0011)
struct bt_hci_cp_auth_requested {
	uint16_t handle;
} __packed;

#define BT_HCI_OP_SET_CONN_ENCRYPT              BT_OP(BT_OGF_LINK_CTRL, 0x0013)
struct bt_hci_cp_set_conn_encrypt {
	uint16_t handle;
	uint8_t  encrypt;
} __packed;

#define BT_HCI_OP_REMOTE_NAME_REQUEST           BT_OP(BT_OGF_LINK_CTRL, 0x0019)
struct bt_hci_cp_remote_name_request {
	bt_addr_t bdaddr;
	uint8_t   pscan_rep_mode;
	uint8_t   reserved;
	uint16_t  clock_offset;
} __packed;

#define BT_HCI_OP_REMOTE_NAME_CANCEL            BT_OP(BT_OGF_LINK_CTRL, 0x001a)
struct bt_hci_cp_remote_name_cancel {
	bt_addr_t bdaddr;
} __packed;
struct bt_hci_rp_remote_name_cancel {
	uint8_t status;
	bt_addr_t bdaddr;
} __packed;

#define BT_HCI_OP_READ_REMOTE_FEATURES          BT_OP(BT_OGF_LINK_CTRL, 0x001b)
struct bt_hci_cp_read_remote_features {
	uint16_t handle;
} __packed;

#define BT_HCI_OP_READ_REMOTE_EXT_FEATURES      BT_OP(BT_OGF_LINK_CTRL, 0x001c)
struct bt_hci_cp_read_remote_ext_features {
	uint16_t handle;
	uint8_t  page;
} __packed;

#define BT_HCI_OP_READ_REMOTE_VERSION_INFO      BT_OP(BT_OGF_LINK_CTRL, 0x001d)
struct bt_hci_cp_read_remote_version_info {
	uint16_t handle;
} __packed;

#define BT_HCI_OP_IO_CAPABILITY_REPLY           BT_OP(BT_OGF_LINK_CTRL, 0x002b)
struct bt_hci_cp_io_capability_reply {
	bt_addr_t bdaddr;
	uint8_t   capability;
	uint8_t   oob_data;
	uint8_t   authentication;
} __packed;

#define BT_HCI_OP_USER_CONFIRM_REPLY            BT_OP(BT_OGF_LINK_CTRL, 0x002c)
#define BT_HCI_OP_USER_CONFIRM_NEG_REPLY        BT_OP(BT_OGF_LINK_CTRL, 0x002d)
struct bt_hci_cp_user_confirm_reply {
	bt_addr_t bdaddr;
} __packed;
struct bt_hci_rp_user_confirm_reply {
	uint8_t   status;
	bt_addr_t bdaddr;
} __packed;

#define BT_HCI_OP_USER_PASSKEY_REPLY            BT_OP(BT_OGF_LINK_CTRL, 0x002e)
struct bt_hci_cp_user_passkey_reply {
	bt_addr_t bdaddr;
	uint32_t  passkey;
} __packed;

#define BT_HCI_OP_USER_PASSKEY_NEG_REPLY        BT_OP(BT_OGF_LINK_CTRL, 0x002f)
struct bt_hci_cp_user_passkey_neg_reply {
	bt_addr_t bdaddr;
} __packed;

#define BT_HCI_OP_IO_CAPABILITY_NEG_REPLY       BT_OP(BT_OGF_LINK_CTRL, 0x0034)
struct bt_hci_cp_io_capability_neg_reply {
	bt_addr_t bdaddr;
	uint8_t   reason;
} __packed;

#define BT_HCI_OP_SET_EVENT_MASK                BT_OP(BT_OGF_BASEBAND, 0x0001)
struct bt_hci_cp_set_event_mask {
	uint8_t  events[8];
} __packed;

#define BT_HCI_OP_RESET                         BT_OP(BT_OGF_BASEBAND, 0x0003)

#define BT_HCI_OP_WRITE_LOCAL_NAME              BT_OP(BT_OGF_BASEBAND, 0x0013)
struct bt_hci_write_local_name {
	uint8_t local_name[248];
} __packed;

#define BT_HCI_OP_WRITE_PAGE_TIMEOUT            BT_OP(BT_OGF_BASEBAND, 0x0018)

#define BT_HCI_OP_WRITE_SCAN_ENABLE             BT_OP(BT_OGF_BASEBAND, 0x001a)
#define BT_BREDR_SCAN_DISABLED                  0x00
#define BT_BREDR_SCAN_INQUIRY                   0x01
#define BT_BREDR_SCAN_PAGE                      0x02

#define BT_HCI_CTL_TO_HOST_FLOW_ENABLE          0x01
#define BT_HCI_OP_SET_CTL_TO_HOST_FLOW          BT_OP(BT_OGF_BASEBAND, 0x0031)

#define BT_HCI_OP_HOST_BUFFER_SIZE              BT_OP(BT_OGF_BASEBAND, 0x0033)
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

#define BT_HCI_OP_HOST_NUM_COMPLETED_PACKETS    BT_OP(BT_OGF_BASEBAND, 0x0035)
struct bt_hci_cp_host_num_completed_packets {
	uint8_t  num_handles;
	struct bt_hci_handle_count h[0];
} __packed;

#define BT_HCI_OP_WRITE_INQUIRY_MODE            BT_OP(BT_OGF_BASEBAND, 0x0045)
struct bt_hci_cp_write_inquiry_mode {
	uint8_t  mode;
} __packed;

#define BT_HCI_OP_WRITE_SSP_MODE                BT_OP(BT_OGF_BASEBAND, 0x0056)
struct bt_hci_cp_write_ssp_mode {
	uint8_t mode;
} __packed;

#define BT_HCI_OP_LE_WRITE_LE_HOST_SUPP         BT_OP(BT_OGF_BASEBAND, 0x006d)
struct bt_hci_cp_write_le_host_supp {
	uint8_t  le;
	uint8_t  simul;
} __packed;

#define BT_HCI_OP_WRITE_SC_HOST_SUPP            BT_OP(BT_OGF_BASEBAND, 0x007a)
struct bt_hci_cp_write_sc_host_supp {
	uint8_t  sc_support;
} __packed;

/* HCI version from Assigned Numbers */
#define BT_HCI_VERSION_1_0B                     0
#define BT_HCI_VERSION_1_1                      1
#define BT_HCI_VERSION_1_2                      2
#define BT_HCI_VERSION_2_0                      3
#define BT_HCI_VERSION_2_1                      4
#define BT_HCI_VERSION_3_0                      5
#define BT_HCI_VERSION_4_0                      6
#define BT_HCI_VERSION_4_1                      7
#define BT_HCI_VERSION_4_2                      8

#define BT_HCI_OP_READ_LOCAL_VERSION_INFO       BT_OP(BT_OGF_INFO, 0x0001)
struct bt_hci_rp_read_local_version_info {
	uint8_t  status;
	uint8_t  hci_version;
	uint16_t hci_revision;
	uint8_t  lmp_version;
	uint16_t manufacturer;
	uint16_t lmp_subversion;
} __packed;

#define BT_HCI_OP_READ_SUPPORTED_COMMANDS       BT_OP(BT_OGF_INFO, 0x0002)
struct bt_hci_rp_read_supported_commands {
	uint8_t  status;
	uint8_t  commands[64];
} __packed;

#define BT_HCI_OP_READ_LOCAL_EXT_FEATURES       BT_OP(BT_OGF_INFO, 0x0004)
struct bt_hci_cp_read_local_ext_features {
	uint8_t page;
};
struct bt_hci_rp_read_local_ext_features {
	uint8_t  status;
	uint8_t  page;
	uint8_t  max_page;
	uint8_t  ext_features[8];
} __packed;

#define BT_HCI_OP_READ_LOCAL_FEATURES           BT_OP(BT_OGF_INFO, 0x0003)
struct bt_hci_rp_read_local_features {
	uint8_t  status;
	uint8_t  features[8];
} __packed;

#define BT_HCI_OP_READ_BUFFER_SIZE              BT_OP(BT_OGF_INFO, 0x0005)
struct bt_hci_rp_read_buffer_size {
	uint8_t  status;
	uint16_t acl_max_len;
	uint8_t  sco_max_len;
	uint16_t acl_max_num;
	uint16_t sco_max_num;
} __packed;

#define BT_HCI_OP_READ_BD_ADDR                  BT_OP(BT_OGF_INFO, 0x0009)
struct bt_hci_rp_read_bd_addr {
	uint8_t   status;
	bt_addr_t bdaddr;
} __packed;

#define BT_HCI_OP_READ_ENCRYPTION_KEY_SIZE      BT_OP(BT_OGF_STATUS, 0x0008)
struct bt_hci_cp_read_encryption_key_size {
	uint16_t handle;
} __packed;
struct bt_hci_rp_read_encryption_key_size {
	uint8_t  status;
	uint16_t handle;
	uint8_t  key_size;
} __packed;

/* BLE */

#define BT_HCI_OP_LE_SET_EVENT_MASK             BT_OP(BT_OGF_LE, 0x0001)
struct bt_hci_cp_le_set_event_mask {
	uint8_t events[8];
} __packed;

#define BT_HCI_OP_LE_READ_BUFFER_SIZE           BT_OP(BT_OGF_LE, 0x0002)
struct bt_hci_rp_le_read_buffer_size {
	uint8_t  status;
	uint16_t le_max_len;
	uint8_t  le_max_num;
} __packed;

#define BT_HCI_OP_LE_READ_LOCAL_FEATURES        BT_OP(BT_OGF_LE, 0x0003)
struct bt_hci_rp_le_read_local_features {
	uint8_t  status;
	uint8_t  features[8];
} __packed;

#define BT_HCI_OP_LE_SET_RANDOM_ADDRESS         BT_OP(BT_OGF_LE, 0x0005)
struct bt_hci_cp_le_set_random_address {
	bt_addr_t bdaddr;
} __packed;

/* Advertising types */
#define BT_LE_ADV_IND                           0x00
#define BT_LE_ADV_DIRECT_IND                    0x01
#define BT_LE_ADV_SCAN_IND                      0x02
#define BT_LE_ADV_NONCONN_IND                   0x03
#define BT_LE_ADV_DIRECT_IND_LOW_DUTY           0x04
/* Needed in advertising reports when getting info about */
#define BT_LE_ADV_SCAN_RSP                      0x04

#define BT_HCI_OP_LE_SET_ADV_PARAM              BT_OP(BT_OGF_LE, 0x0006)
struct bt_hci_cp_le_set_adv_param {
	uint16_t     min_interval;
	uint16_t     max_interval;
	uint8_t      type;
	uint8_t      own_addr_type;
	bt_addr_le_t direct_addr;
	uint8_t      channel_map;
	uint8_t      filter_policy;
} __packed;

#define BT_HCI_OP_LE_READ_ADV_CH_TX_POWER       BT_OP(BT_OGF_LE, 0x0007)
struct bt_hci_rp_le_read_ch_tx_power {
	uint8_t  status;
	int8_t   tx_power_level;
} __packed;

#define BT_HCI_OP_LE_SET_ADV_DATA               BT_OP(BT_OGF_LE, 0x0008)
struct bt_hci_cp_le_set_adv_data {
	uint8_t  len;
	uint8_t  data[31];
} __packed;

#define BT_HCI_OP_LE_SET_SCAN_RSP_DATA          BT_OP(BT_OGF_LE, 0x0009)
struct bt_hci_cp_le_set_scan_rsp_data {
	uint8_t  len;
	uint8_t  data[31];
} __packed;

#define BT_HCI_LE_ADV_DISABLE                   0x00
#define BT_HCI_LE_ADV_ENABLE                    0x01

#define BT_HCI_OP_LE_SET_ADV_ENABLE             BT_OP(BT_OGF_LE, 0x000a)
struct bt_hci_cp_le_set_adv_enable {
	uint8_t  enable;
} __packed;

/* Scan types */
#define BT_HCI_OP_LE_SET_SCAN_PARAMS            BT_OP(BT_OGF_LE, 0x000b)
#define BT_HCI_LE_SCAN_PASSIVE                  0x00
#define BT_HCI_LE_SCAN_ACTIVE                   0x01

struct bt_hci_cp_le_set_scan_params {
	uint8_t  scan_type;
	uint16_t interval;
	uint16_t window;
	uint8_t  addr_type;
	uint8_t  filter_policy;
} __packed;

#define BT_HCI_OP_LE_SET_SCAN_ENABLE            BT_OP(BT_OGF_LE, 0x000c)

#define BT_HCI_LE_SCAN_DISABLE                  0x00
#define BT_HCI_LE_SCAN_ENABLE                   0x01

#define BT_HCI_LE_SCAN_FILTER_DUP_DISABLE       0x00
#define BT_HCI_LE_SCAN_FILTER_DUP_ENABLE        0x01

struct bt_hci_cp_le_set_scan_enable {
	uint8_t  enable;
	uint8_t  filter_dup;
} __packed;

#define BT_HCI_OP_LE_CREATE_CONN                BT_OP(BT_OGF_LE, 0x000d)
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

#define BT_HCI_OP_LE_CREATE_CONN_CANCEL         BT_OP(BT_OGF_LE, 0x000e)

#define BT_HCI_OP_LE_READ_WL_SIZE               BT_OP(BT_OGF_LE, 0x000f)
struct bt_hci_rp_le_read_wl_size {
	uint8_t  status;
	uint8_t  wl_size;
} __packed;

#define BT_HCI_OP_LE_CLEAR_WL                   BT_OP(BT_OGF_LE, 0x0010)

#define BT_HCI_OP_LE_ADD_DEV_TO_WL              BT_OP(BT_OGF_LE, 0x0011)
struct bt_hci_cp_le_add_dev_to_wl {
	bt_addr_le_t  addr;
} __packed;

#define BT_HCI_OP_LE_REM_DEV_FROM_WL            BT_OP(BT_OGF_LE, 0x0012)
struct bt_hci_cp_le_rem_dev_from_wl {
	bt_addr_le_t  addr;
} __packed;

#define BT_HCI_OP_LE_CONN_UPDATE                BT_OP(BT_OGF_LE, 0x0013)
struct hci_cp_le_conn_update {
	uint16_t handle;
	uint16_t conn_interval_min;
	uint16_t conn_interval_max;
	uint16_t conn_latency;
	uint16_t supervision_timeout;
	uint16_t min_ce_len;
	uint16_t max_ce_len;
} __packed;

#define BT_HCI_OP_LE_SET_HOST_CH_CLASSIF        BT_OP(BT_OGF_LE, 0x0014)
struct bt_hci_cp_le_set_host_ch_classif {
	uint8_t  ch_map[5];
} __packed;

#define BT_HCI_OP_LE_READ_CH_MAP                BT_OP(BT_OGF_LE, 0x0015)
struct bt_hci_cp_le_read_ch_map {
	uint16_t handle;
} __packed;
struct bt_hci_rp_le_read_ch_map {
	uint8_t  status;
	uint16_t handle;
	uint8_t  ch_map[5];
} __packed;

#define BT_HCI_OP_LE_READ_REMOTE_FEATURES       BT_OP(BT_OGF_LE, 0x0016)
struct bt_hci_cp_le_read_remote_features {
	uint16_t  handle;
} __packed;

#define BT_HCI_OP_LE_ENCRYPT                    BT_OP(BT_OGF_LE, 0x0017)
struct bt_hci_cp_le_encrypt {
	uint8_t  key[16];
	uint8_t  plaintext[16];
} __packed;
struct bt_hci_rp_le_encrypt {
	uint8_t  status;
	uint8_t  enc_data[16];
} __packed;

#define BT_HCI_OP_LE_RAND                       BT_OP(BT_OGF_LE, 0x0018)
struct bt_hci_rp_le_rand {
	uint8_t  status;
	uint8_t  rand[8];
} __packed;

#define BT_HCI_OP_LE_START_ENCRYPTION           BT_OP(BT_OGF_LE, 0x0019)
struct bt_hci_cp_le_start_encryption {
	uint16_t handle;
	uint64_t rand;
	uint16_t ediv;
	uint8_t  ltk[16];
} __packed;

#define BT_HCI_OP_LE_LTK_REQ_REPLY              BT_OP(BT_OGF_LE, 0x001a)
struct bt_hci_cp_le_ltk_req_reply {
	uint16_t handle;
	uint8_t  ltk[16];
} __packed;
struct bt_hci_rp_le_ltk_req_reply {
	uint8_t  status;
	uint16_t handle;
} __packed;

#define BT_HCI_OP_LE_LTK_REQ_NEG_REPLY          BT_OP(BT_OGF_LE, 0x001b)
struct bt_hci_cp_le_ltk_req_neg_reply {
	uint16_t handle;
} __packed;
struct bt_hci_rp_le_ltk_req_neg_reply {
	uint8_t  status;
	uint16_t handle;
} __packed;

#define BT_HCI_OP_LE_READ_SUPP_STATES           BT_OP(BT_OGF_LE, 0x001c)
struct bt_hci_rp_le_read_supp_states {
	uint8_t  status;
	uint8_t  le_states[8];
} __packed;

#define BT_HCI_OP_LE_RX_TEST                    BT_OP(BT_OGF_LE, 0x001d)
struct bt_hci_cp_le_rx_test {
	uint8_t  rx_ch;
} __packed;

#define BT_HCI_OP_LE_TX_TEST                    BT_OP(BT_OGF_LE, 0x001e)
struct bt_hci_cp_le_tx_test {
	uint8_t  tx_ch;
	uint8_t  test_data_len;
	uint8_t  pkt_payload;
} __packed;

#define BT_HCI_OP_LE_TEST_END                   BT_OP(BT_OGF_LE, 0x001f)
struct bt_hci_rp_le_test_end {
	uint8_t  status;
	uint16_t rx_pkt_count;
} __packed;

#define BT_HCI_OP_LE_CONN_PARAM_REQ_REPLY       BT_OP(BT_OGF_LE, 0x0020)
struct bt_hci_cp_le_conn_param_req_reply {
	uint16_t handle;
	uint16_t interval_min;
	uint16_t interval_max;
	uint16_t latency;
	uint16_t timeout;
	uint16_t min_ce_len;
	uint16_t max_ce_len;
} __packed;
struct bt_hci_rp_le_conn_param_req_reply {
	uint8_t  status;
	uint16_t handle;
} __packed;

#define BT_HCI_OP_LE_CONN_PARAM_REQ_NEG_REPLY   BT_OP(BT_OGF_LE, 0x0021)
struct bt_hci_cp_le_conn_param_req_neg_reply {
	uint16_t handle;
	uint8_t  reason;
} __packed;
struct bt_hci_rp_le_conn_param_req_neg_reply {
	uint8_t  status;
	uint16_t handle;
} __packed;

#define BT_HCI_OP_LE_SET_DATA_LEN               BT_OP(BT_OGF_LE, 0x0022)
struct bt_hci_cp_le_set_data_len {
	uint16_t handle;
	uint16_t tx_octets;
	uint16_t tx_time;
} __packed;
struct bt_hci_rp_le_set_data_len {
	uint8_t  status;
	uint16_t handle;
} __packed;

#define BT_HCI_OP_LE_READ_DEFAULT_DATA_LEN      BT_OP(BT_OGF_LE, 0x0023)
struct bt_hci_rp_le_read_default_data_len {
	uint8_t  status;
	uint16_t max_tx_octets;
	uint16_t max_tx_time;
} __packed;

#define BT_HCI_OP_LE_WRITE_DEFAULT_DATA_LEN     BT_OP(BT_OGF_LE, 0x0024)
struct bt_hci_cp_le_write_default_data_len {
	uint16_t max_tx_octets;
	uint16_t max_tx_time;
} __packed;

#define BT_HCI_OP_LE_P256_PUBLIC_KEY            BT_OP(BT_OGF_LE, 0x0025)

#define BT_HCI_OP_LE_GENERATE_DHKEY             BT_OP(BT_OGF_LE, 0x0026)
struct bt_hci_cp_le_generate_dhkey {
	uint8_t key[64];
} __packed;

#define BT_HCI_OP_LE_ADD_DEV_TO_RL              BT_OP(BT_OGF_LE, 0x0027)
struct bt_hci_cp_le_add_dev_to_rl {
	bt_addr_le_t  peer_id_addr;
	uint8_t       peer_irk[16];
	uint8_t       local_irk[16];
} __packed;

#define BT_HCI_OP_LE_REM_DEV_FROM_RL            BT_OP(BT_OGF_LE, 0x0028)
struct bt_hci_cp_le_rem_dev_from_rl {
	bt_addr_le_t  peer_id_addr;
} __packed;

#define BT_HCI_OP_LE_CLEAR_RL                   BT_OP(BT_OGF_LE, 0x0029)

#define BT_HCI_OP_LE_READ_RL_SIZE               BT_OP(BT_OGF_LE, 0x002a)
struct bt_hci_rp_le_read_rl_size {
	uint8_t  status;
	uint8_t  rl_size;
} __packed;

#define BT_HCI_OP_LE_READ_PEER_RPA              BT_OP(BT_OGF_LE, 0x002b)
struct bt_hci_cp_le_read_peer_rpa {
	bt_addr_le_t  peer_id_addr;
} __packed;
struct bt_hci_rp_le_read_peer_rpa {
	uint8_t    status;
	bt_addr_t  peer_rpa;
} __packed;

#define BT_HCI_OP_LE_READ_LOCAL_RPA             BT_OP(BT_OGF_LE, 0x002c)
struct bt_hci_cp_le_read_local_rpa {
	bt_addr_le_t  peer_id_addr;
} __packed;
struct bt_hci_rp_le_read_local_rpa {
	uint8_t    status;
	bt_addr_t  local_rpa;
} __packed;

#define BT_HCI_ADDR_RES_DISABLE                 0x00
#define BT_HCI_ADDR_RES_ENABLE                  0x01

#define BT_HCI_OP_LE_SET_ADDR_RES_ENABLE        BT_OP(BT_OGF_LE, 0x002d)
struct bt_hci_cp_le_set_addr_res_enable {
	uint8_t  enable;
} __packed;

#define BT_HCI_OP_LE_SET_RPA_TIMEOUT            BT_OP(BT_OGF_LE, 0x002e)
struct bt_hci_cp_le_set_rpa_timeout {
	uint8_t  rpa_timeout;
} __packed;

#define BT_HCI_OP_LE_READ_MAX_DATA_LEN          BT_OP(BT_OGF_LE, 0x002f)
struct bt_hci_rp_le_read_max_data_len {
	uint8_t  status;
	uint16_t max_tx_octets;
	uint16_t max_tx_time;
	uint16_t max_rx_octets;
	uint16_t max_rx_time;
} __packed;

/* Event definitions */

#define BT_HCI_EVT_VENDOR                       0xff

#define BT_HCI_EVT_INQUIRY_COMPLETE             0x01
struct bt_hci_evt_inquiry_complete {
	uint8_t status;
} __packed;

#define BT_HCI_EVT_CONN_COMPLETE                0x03
struct bt_hci_evt_conn_complete {
	uint8_t   status;
	uint16_t  handle;
	bt_addr_t bdaddr;
	uint8_t   link_type;
	uint8_t   encr_enabled;
} __packed;

#define BT_HCI_EVT_CONN_REQUEST                 0x04
struct bt_hci_evt_conn_request {
	bt_addr_t bdaddr;
	uint8_t   dev_class[3];
	uint8_t   link_type;
} __packed;

#define BT_HCI_EVT_DISCONN_COMPLETE             0x05
struct bt_hci_evt_disconn_complete {
	uint8_t  status;
	uint16_t handle;
	uint8_t  reason;
} __packed;

#define BT_HCI_EVT_AUTH_COMPLETE                0x06
struct bt_hci_evt_auth_complete {
	uint8_t  status;
	uint16_t handle;
} __packed;

#define BT_HCI_EVT_REMOTE_NAME_REQ_COMPLETE     0x07
struct bt_hci_evt_remote_name_req_complete {
	uint8_t status;
	bt_addr_t bdaddr;
	uint8_t name[248];
} __packed;

#define BT_HCI_EVT_ENCRYPT_CHANGE               0x08
struct bt_hci_evt_encrypt_change {
	uint8_t  status;
	uint16_t handle;
	uint8_t  encrypt;
} __packed;

#define BT_HCI_EVT_REMOTE_FEATURES              0x0b
struct bt_hci_evt_remote_features {
	uint8_t  status;
	uint16_t handle;
	uint8_t  features[8];
} __packed;

#define BT_HCI_EVT_REMOTE_VERSION_INFO          0x0c
struct bt_hci_evt_remote_version_info {
	uint8_t  status;
	uint16_t handle;
	uint8_t  version;
	uint16_t manufacturer;
	uint8_t  subversion;
} __packed;

#define BT_HCI_EVT_CMD_COMPLETE                 0x0e
struct bt_hci_evt_cmd_complete {
	uint8_t  ncmd;
	uint16_t opcode;
} __packed;

struct bt_hci_evt_cc_status {
	uint8_t  status;
} __packed;

#define BT_HCI_EVT_CMD_STATUS                   0x0f
struct bt_hci_evt_cmd_status {
	uint8_t  status;
	uint8_t  ncmd;
	uint16_t opcode;
} __packed;

#define BT_HCI_EVT_ROLE_CHANGE                  0x12
struct bt_hci_evt_role_change {
	uint8_t   status;
	bt_addr_t bdaddr;
	uint8_t   role;
} __packed;

#define BT_HCI_EVT_NUM_COMPLETED_PACKETS        0x13
struct bt_hci_evt_num_completed_packets {
	uint8_t  num_handles;
	struct bt_hci_handle_count h[0];
} __packed;

#define BT_HCI_EVT_PIN_CODE_REQ                 0x16
struct bt_hci_evt_pin_code_req {
	bt_addr_t bdaddr;
} __packed;

#define BT_HCI_EVT_LINK_KEY_REQ                 0x17
struct bt_hci_evt_link_key_req {
	bt_addr_t bdaddr;
} __packed;

/* Link Key types */
#define BT_LK_COMBINATION                       0x00
#define BT_LK_LOCAL_UNIT                        0x01
#define BT_LK_REMOTE_UNIT                       0x02
#define BT_LK_DEBUG_COMBINATION                 0x03
#define BT_LK_UNAUTH_COMBINATION_P192           0x04
#define BT_LK_AUTH_COMBINATION_P192             0x05
#define BT_LK_CHANGED_COMBINATION               0x06
#define BT_LK_UNAUTH_COMBINATION_P256           0x07
#define BT_LK_AUTH_COMBINATION_P256             0x08

#define BT_HCI_EVT_LINK_KEY_NOTIFY              0x18
struct bt_hci_ev_link_key_notify {
	bt_addr_t bdaddr;
	uint8_t   link_key[16];
	uint8_t   key_type;
} __packed;

#define BT_HCI_EVT_INQUIRY_RESULT_WITH_RSSI     0x22
struct bt_hci_evt_inquiry_result_with_rssi {
	bt_addr_t addr;
	uint8_t   pscan_rep_mode;
	uint8_t   reserved;
	uint8_t   cod[3];
	uint16_t  clock_offset;
	int8_t    rssi;
} __packed;

#define BT_HCI_EVT_REMOTE_EXT_FEATURES          0x23
struct bt_hci_evt_remote_ext_features {
	uint8_t  status;
	uint16_t handle;
	uint8_t  page;
	uint8_t  max_page;
	uint8_t  features[8];
} __packed;

#define BT_HCI_EVT_EXTENDED_INQUIRY_RESULT      0x2f
struct bt_hci_evt_extended_inquiry_result {
	uint8_t   num_reports;
	bt_addr_t addr;
	uint8_t   pscan_rep_mode;
	uint8_t   reserved;
	uint8_t   cod[3];
	uint16_t  clock_offset;
	int8_t    rssi;
	uint8_t   eir[240];
} __packed;

#define BT_HCI_EVT_ENCRYPT_KEY_REFRESH_COMPLETE 0x30
struct bt_hci_evt_encrypt_key_refresh_complete {
	uint8_t  status;
	uint16_t handle;
} __packed;

#define BT_HCI_EVT_IO_CAPA_REQ                  0x31
struct bt_hci_evt_io_capa_req {
	bt_addr_t bdaddr;
} __packed;

#define BT_HCI_EVT_IO_CAPA_RESP                 0x32
struct bt_hci_evt_io_capa_resp {
	bt_addr_t bdaddr;
	uint8_t   capability;
	uint8_t   oob_data;
	uint8_t   authentication;
} __packed;

#define BT_HCI_EVT_USER_CONFIRM_REQ             0x33
struct bt_hci_evt_user_confirm_req {
	bt_addr_t bdaddr;
	uint32_t  passkey;
} __packed;

#define BT_HCI_EVT_USER_PASSKEY_REQ             0x34
struct bt_hci_evt_user_passkey_req {
	bt_addr_t bdaddr;
} __packed;

#define BT_HCI_EVT_SSP_COMPLETE                 0x36
struct bt_hci_evt_ssp_complete {
	uint8_t   status;
	bt_addr_t bdaddr;
} __packed;

#define BT_HCI_EVT_USER_PASSKEY_NOTIFY          0x3b
struct bt_hci_evt_user_passkey_notify {
	bt_addr_t bdaddr;
	uint32_t  passkey;
} __packed;

#define BT_HCI_EVT_LE_META_EVENT                0x3e
struct bt_hci_evt_le_meta_event {
	uint8_t  subevent;
} __packed;

#define BT_HCI_EVT_AUTH_PAYLOAD_TIMEOUT_EXP     0x57
struct bt_hci_evt_auth_payload_timeout_exp {
	uint16_t handle;
} __packed;

#define BT_HCI_ROLE_MASTER                      0x00
#define BT_HCI_ROLE_SLAVE                       0x01

#define BT_HCI_EVT_LE_CONN_COMPLETE             0x01
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

#define BT_HCI_EVT_LE_ADVERTISING_REPORT        0x02
struct bt_hci_ev_le_advertising_info {
	uint8_t      evt_type;
	bt_addr_le_t addr;
	uint8_t      length;
	uint8_t      data[0];
} __packed;
struct bt_hci_ev_le_advertising_report {
	uint8_t num_reports;
	struct bt_hci_ev_le_advertising_info adv_info[0];
} __packed;

#define BT_HCI_EVT_LE_CONN_UPDATE_COMPLETE      0x03
struct bt_hci_evt_le_conn_update_complete {
	uint8_t  status;
	uint16_t handle;
	uint16_t interval;
	uint16_t latency;
	uint16_t supv_timeout;
} __packed;

#define BT_HCI_EV_LE_REMOTE_FEAT_COMPLETE       0x04
struct bt_hci_ev_le_remote_feat_complete {
	uint8_t  status;
	uint16_t handle;
	uint8_t  features[8];
} __packed;

#define BT_HCI_EVT_LE_LTK_REQUEST               0x05
struct bt_hci_evt_le_ltk_request {
	uint16_t handle;
	uint64_t rand;
	uint16_t ediv;
} __packed;

#define BT_HCI_EVT_LE_CONN_PARAM_REQ            0x06
struct bt_hci_evt_le_conn_param_req {
	uint16_t handle;
	uint16_t interval_min;
	uint16_t interval_max;
	uint16_t latency;
	uint16_t timeout;
} __packed;

#define BT_HCI_EVT_LE_DATA_LEN_CHANGE           0x07
struct bt_hci_evt_le_data_len_change {
	uint16_t handle;
	uint16_t max_tx_octets;
	uint16_t max_tx_time;
	uint16_t max_rx_octets;
	uint16_t max_rx_time;
} __packed;

#define BT_HCI_EVT_LE_P256_PUBLIC_KEY_COMPLETE  0x08
struct bt_hci_evt_le_p256_public_key_complete {
	uint8_t status;
	uint8_t key[64];
} __packed;

#define BT_HCI_EVT_LE_GENERATE_DHKEY_COMPLETE   0x09
struct bt_hci_evt_le_generate_dhkey_complete {
	uint8_t status;
	uint8_t dhkey[32];
} __packed;

#define BT_HCI_EVT_LE_ENH_CONN_COMPLETE         0x0a
struct bt_hci_evt_le_enh_conn_complete {
	uint8_t      status;
	uint16_t     handle;
	uint8_t      role;
	bt_addr_le_t peer_addr;
	bt_addr_t    local_rpa;
	bt_addr_t    peer_rpa;
	uint16_t     interval;
	uint16_t     latency;
	uint16_t     supv_timeout;
	uint8_t      clock_accuracy;
} __packed;

#define BT_HCI_EVT_LE_DIRECT_ADV_REPORT         0x0b
struct bt_hci_ev_le_direct_adv_info {
	uint8_t      evt_type;
	bt_addr_le_t dir_addr;
	bt_addr_le_t addr;
	int8_t       rssi;
} __packed;
struct bt_hci_ev_le_direct_adv_report {
	uint8_t num_reports;
	struct bt_hci_ev_le_direct_adv_info direct_adv_info[0];
} __packed;

#ifdef __cplusplus
}
#endif

#endif /* __BT_HCI_H */
