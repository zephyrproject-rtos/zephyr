/**
 * @file smp.h
 * Security Manager Protocol implementation header
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct bt_smp_hdr {
	uint8_t  code;
} __packed;

#define BT_SMP_ERR_PASSKEY_ENTRY_FAILED		0x01
#define BT_SMP_ERR_OOB_NOT_AVAIL		0x02
#define BT_SMP_ERR_AUTH_REQUIREMENTS		0x03
#define BT_SMP_ERR_CONFIRM_FAILED		0x04
#define BT_SMP_ERR_PAIRING_NOTSUPP		0x05
#define BT_SMP_ERR_ENC_KEY_SIZE			0x06
#define BT_SMP_ERR_CMD_NOTSUPP			0x07
#define BT_SMP_ERR_UNSPECIFIED			0x08
#define BT_SMP_ERR_REPEATED_ATTEMPTS		0x09
#define BT_SMP_ERR_INVALID_PARAMS		0x0a
#define BT_SMP_ERR_DHKEY_CHECK_FAILED		0x0b
#define BT_SMP_ERR_NUMERIC_COMP_FAILED		0x0c
#define BT_SMP_ERR_BREDR_PAIRING_IN_PROGRESS	0x0d
#define BT_SMP_ERR_CROSS_TRANSP_NOT_ALLOWED	0x0e

#define BT_SMP_IO_DISPLAY_ONLY			0x00
#define BT_SMP_IO_DISPLAY_YESNO			0x01
#define BT_SMP_IO_KEYBOARD_ONLY			0x02
#define BT_SMP_IO_NO_INPUT_OUTPUT		0x03
#define BT_SMP_IO_KEYBOARD_DISPLAY		0x04

#define BT_SMP_OOB_DATA_MASK			0x01
#define BT_SMP_OOB_NOT_PRESENT			0x00
#define BT_SMP_OOB_PRESENT			0x01

#define BT_SMP_MIN_ENC_KEY_SIZE			7
#define BT_SMP_MAX_ENC_KEY_SIZE			16

#define BT_SMP_DIST_ENC_KEY			0x01
#define BT_SMP_DIST_ID_KEY			0x02
#define BT_SMP_DIST_SIGN			0x04
#define BT_SMP_DIST_LINK_KEY			0x08

#define BT_SMP_DIST_MASK			0x0f

#define BT_SMP_AUTH_NONE			0x00
#define BT_SMP_AUTH_BONDING			0x01
#define BT_SMP_AUTH_MITM			0x04
#define BT_SMP_AUTH_SC				0x08
#define BT_SMP_AUTH_KEYPRESS			0x10
#define BT_SMP_AUTH_CT2				0x20

#define BT_SMP_CMD_PAIRING_REQ			0x01
#define BT_SMP_CMD_PAIRING_RSP			0x02
struct bt_smp_pairing {
	uint8_t  io_capability;
	uint8_t  oob_flag;
	uint8_t  auth_req;
	uint8_t  max_key_size;
	uint8_t  init_key_dist;
	uint8_t  resp_key_dist;
} __packed;

#define BT_SMP_CMD_PAIRING_CONFIRM		0x03
struct bt_smp_pairing_confirm {
	uint8_t  val[16];
} __packed;

#define BT_SMP_CMD_PAIRING_RANDOM		0x04
struct bt_smp_pairing_random {
	uint8_t  val[16];
} __packed;

#define BT_SMP_CMD_PAIRING_FAIL			0x05
struct bt_smp_pairing_fail {
	uint8_t  reason;
} __packed;

#define BT_SMP_CMD_ENCRYPT_INFO			0x06
struct bt_smp_encrypt_info {
	uint8_t  ltk[16];
} __packed;

#define BT_SMP_CMD_MASTER_IDENT			0x07
struct bt_smp_master_ident {
	uint8_t ediv[2];
	uint8_t rand[8];
} __packed;

#define BT_SMP_CMD_IDENT_INFO			0x08
struct bt_smp_ident_info {
	uint8_t  irk[16];
} __packed;

#define BT_SMP_CMD_IDENT_ADDR_INFO		0x09
struct bt_smp_ident_addr_info {
	bt_addr_le_t addr;
} __packed;

#define BT_SMP_CMD_SIGNING_INFO			0x0a
struct bt_smp_signing_info {
	uint8_t csrk[16];
} __packed;

#define BT_SMP_CMD_SECURITY_REQUEST		0x0b
struct bt_smp_security_request {
	uint8_t  auth_req;
} __packed;

#define BT_SMP_CMD_PUBLIC_KEY			0x0c
struct bt_smp_public_key {
	uint8_t x[32];
	uint8_t y[32];
} __packed;

#define BT_SMP_DHKEY_CHECK			0x0d
struct bt_smp_dhkey_check {
	uint8_t e[16];
} __packed;

int bt_smp_start_security(struct bt_conn *conn);
bool bt_smp_request_ltk(struct bt_conn *conn, uint64_t rand, uint16_t ediv,
			uint8_t *ltk);

void bt_smp_update_keys(struct bt_conn *conn);

int bt_smp_br_send_pairing_req(struct bt_conn *conn);

int bt_smp_init(void);

int bt_smp_auth_passkey_entry(struct bt_conn *conn, unsigned int passkey);
int bt_smp_auth_passkey_confirm(struct bt_conn *conn);
int bt_smp_auth_pairing_confirm(struct bt_conn *conn);
int bt_smp_auth_cancel(struct bt_conn *conn);

int bt_smp_le_oob_set_tk(struct bt_conn *conn, const uint8_t *tk);
int bt_smp_le_oob_generate_sc_data(struct bt_le_oob_sc_data *le_sc_oob);
int bt_smp_le_oob_set_sc_data(struct bt_conn *conn,
			      const struct bt_le_oob_sc_data *oobd_local,
			      const struct bt_le_oob_sc_data *oobd_remote);
int bt_smp_le_oob_get_sc_data(struct bt_conn *conn,
			      const struct bt_le_oob_sc_data **oobd_local,
			      const struct bt_le_oob_sc_data **oobd_remote);

/** brief Verify signed message
 *
 *  @param conn Bluetooth connection
 *  @param buf received packet buffer with message and signature
 *
 *  @return 0 in success, error code otherwise
 */
int bt_smp_sign_verify(struct bt_conn *conn, struct net_buf *buf);

/** brief Sign message
 *
 *  @param conn Bluetooth connection
 *  @param buf message buffer
 *
 *  @return 0 in success, error code otherwise
 */
int bt_smp_sign(struct bt_conn *conn, struct net_buf *buf);

/** Generate IRK from Identity Root (IR) */
int bt_smp_irk_get(uint8_t *ir, uint8_t *irk);
