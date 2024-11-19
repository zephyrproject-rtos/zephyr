/**
 * @file hci_ecc.c
 * HCI ECC emulation
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/debug/stack.h>
#include <zephyr/sys/byteorder.h>

#if defined(CONFIG_BT_USE_PSA_API)
#include <psa/crypto.h>
#else /* !CONFIG_BT_USE_PSA_API */
#include <tinycrypt/constants.h>
#include <tinycrypt/utils.h>
#include <tinycrypt/ecc.h>
#include <tinycrypt/ecc_dh.h>
#endif /* CONFIG_BT_USE_PSA_API*/

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/drivers/bluetooth.h>

#include "common/bt_str.h"

#include "hci_ecc.h"
#include "ecc.h"

#ifdef CONFIG_BT_HCI_RAW
#include <zephyr/bluetooth/hci_raw.h>
#include "hci_raw_internal.h"
#else
#include "hci_core.h"
#endif
#include "long_wq.h"

#define LOG_LEVEL CONFIG_BT_HCI_CORE_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_hci_ecc);

static void ecc_process(struct k_work *work);
K_WORK_DEFINE(ecc_work, ecc_process);

/* based on Core Specification 4.2 Vol 3. Part H 2.3.5.6.1 */
static const uint8_t debug_private_key_be[BT_PRIV_KEY_LEN] = {
	0x3f, 0x49, 0xf6, 0xd4, 0xa3, 0xc5, 0x5f, 0x38,
	0x74, 0xc9, 0xb3, 0xe3, 0xd2, 0x10, 0x3f, 0x50,
	0x4a, 0xff, 0x60, 0x7b, 0xeb, 0x40, 0xb7, 0x99,
	0x58, 0x99, 0xb8, 0xa6, 0xcd, 0x3c, 0x1a, 0xbd,
};

enum {
	PENDING_PUB_KEY,
	PENDING_DHKEY,

	USE_DEBUG_KEY,

	/* Total number of flags - must be at the end of the enum */
	NUM_FLAGS,
};

static ATOMIC_DEFINE(flags, NUM_FLAGS);

static struct {
	uint8_t private_key_be[BT_PRIV_KEY_LEN];

	union {
		uint8_t public_key_be[BT_PUB_KEY_LEN];
		uint8_t dhkey_be[BT_DH_KEY_LEN];
	};
} ecc;

static void send_cmd_status(uint16_t opcode, uint8_t status)
{
	struct bt_hci_evt_cmd_status *evt;
	struct bt_hci_evt_hdr *hdr;
	struct net_buf *buf;

	LOG_DBG("opcode %x status 0x%02x %s", opcode, status, bt_hci_err_to_str(status));

	buf = bt_buf_get_evt(BT_HCI_EVT_CMD_STATUS, false, K_FOREVER);
	bt_buf_set_type(buf, BT_BUF_EVT);

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->evt = BT_HCI_EVT_CMD_STATUS;
	hdr->len = sizeof(*evt);

	evt = net_buf_add(buf, sizeof(*evt));
	evt->ncmd = 1U;
	evt->opcode = sys_cpu_to_le16(opcode);
	evt->status = status;

	bt_hci_recv(bt_dev.hci, buf);
}

#if defined(CONFIG_BT_USE_PSA_API)
static void set_key_attributes(psa_key_attributes_t *attr)
{
	psa_set_key_type(attr, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
	psa_set_key_bits(attr, 256);
	psa_set_key_usage_flags(attr, PSA_KEY_USAGE_EXPORT | PSA_KEY_USAGE_DERIVE);
	psa_set_key_algorithm(attr, PSA_ALG_ECDH);
}

static uint8_t generate_keys(void)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key_id;
	uint8_t tmp_pub_key_buf[BT_PUB_KEY_LEN + 1];
	size_t tmp_len;

	set_key_attributes(&attr);

	if (psa_generate_key(&attr, &key_id) != PSA_SUCCESS) {
		LOG_ERR("Failed to generate ECC key");
		return BT_HCI_ERR_UNSPECIFIED;
	}

	if (psa_export_public_key(key_id, tmp_pub_key_buf, sizeof(tmp_pub_key_buf),
				&tmp_len) != PSA_SUCCESS) {
		LOG_ERR("Failed to export ECC public key");
		return BT_HCI_ERR_UNSPECIFIED;
	}
	/* secp256r1 PSA exported public key has an extra 0x04 predefined byte at
	 * the beginning of the buffer which is not part of the coordinate so
	 * we remove that.
	 */
	memcpy(ecc.public_key_be, &tmp_pub_key_buf[1], BT_PUB_KEY_LEN);

	if (psa_export_key(key_id, ecc.private_key_be, BT_PRIV_KEY_LEN,
			&tmp_len) != PSA_SUCCESS) {
		LOG_ERR("Failed to export ECC private key");
		return BT_HCI_ERR_UNSPECIFIED;
	}

	if (psa_destroy_key(key_id) != PSA_SUCCESS) {
		LOG_ERR("Failed to destroy ECC key ID");
		return BT_HCI_ERR_UNSPECIFIED;
	}

	return 0;
}
#else
static uint8_t generate_keys(void)
{
	do {
		int rc;

		rc = uECC_make_key(ecc.public_key_be, ecc.private_key_be,
				   &curve_secp256r1);
		if (rc == TC_CRYPTO_FAIL) {
			LOG_ERR("Failed to create ECC public/private pair");
			return BT_HCI_ERR_UNSPECIFIED;
		}

	/* make sure generated key isn't debug key */
	} while (memcmp(ecc.private_key_be, debug_private_key_be, BT_PRIV_KEY_LEN) == 0);

	if (IS_ENABLED(CONFIG_BT_LOG_SNIFFER_INFO)) {
		LOG_INF("SC private key 0x%s", bt_hex(ecc.private_key_be, BT_PRIV_KEY_LEN));
	}

	return 0;
}
#endif /* CONFIG_BT_USE_PSA_API */

static void emulate_le_p256_public_key_cmd(void)
{
	struct bt_hci_evt_le_p256_public_key_complete *evt;
	struct bt_hci_evt_le_meta_event *meta;
	struct bt_hci_evt_hdr *hdr;
	struct net_buf *buf;
	uint8_t status;

	LOG_DBG("");

	status = generate_keys();

	buf = bt_buf_get_rx(BT_BUF_EVT, K_FOREVER);

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->evt = BT_HCI_EVT_LE_META_EVENT;
	hdr->len = sizeof(*meta) + sizeof(*evt);

	meta = net_buf_add(buf, sizeof(*meta));
	meta->subevent = BT_HCI_EVT_LE_P256_PUBLIC_KEY_COMPLETE;

	evt = net_buf_add(buf, sizeof(*evt));
	evt->status = status;

	if (status) {
		(void)memset(evt->key, 0, sizeof(evt->key));
	} else {
		/* Convert X and Y coordinates from big-endian (provided
		 * by crypto API) to little endian HCI.
		 */
		sys_memcpy_swap(evt->key, ecc.public_key_be, BT_PUB_KEY_COORD_LEN);
		sys_memcpy_swap(&evt->key[BT_PUB_KEY_COORD_LEN],
				&ecc.public_key_be[BT_PUB_KEY_COORD_LEN], BT_PUB_KEY_COORD_LEN);
	}

	atomic_clear_bit(flags, PENDING_PUB_KEY);

	bt_hci_recv(bt_dev.hci, buf);
}

static void emulate_le_generate_dhkey(void)
{
	struct bt_hci_evt_le_generate_dhkey_complete *evt;
	struct bt_hci_evt_le_meta_event *meta;
	struct bt_hci_evt_hdr *hdr;
	struct net_buf *buf;
	int ret = 0;
	bool use_debug = atomic_test_bit(flags, USE_DEBUG_KEY);

#if defined(CONFIG_BT_USE_PSA_API)
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key_id;
	/* PSA expects secp256r1 public key to start with a predefined 0x04 byte
	 * at the beginning the buffer.
	 */
	uint8_t tmp_pub_key_buf[BT_PUB_KEY_LEN + 1] = { 0x04 };
	size_t tmp_len;

	set_key_attributes(&attr);

	if (psa_import_key(&attr, use_debug ? debug_private_key_be : ecc.private_key_be,
				BT_PRIV_KEY_LEN, &key_id) != PSA_SUCCESS) {
		ret = -EIO;
		LOG_ERR("Failed to import the private key for key agreement");
		goto exit;
	}

	memcpy(&tmp_pub_key_buf[1], ecc.public_key_be, BT_PUB_KEY_LEN);
	if (psa_raw_key_agreement(PSA_ALG_ECDH, key_id, tmp_pub_key_buf,
				sizeof(tmp_pub_key_buf), ecc.dhkey_be, BT_DH_KEY_LEN,
				&tmp_len) != PSA_SUCCESS) {
		ret = -EIO;
		LOG_ERR("Raw key agreement failed");
		goto exit;
	}

	if (psa_destroy_key(key_id) != PSA_SUCCESS) {
		LOG_ERR("Failed to destroy the key");
		ret = -EIO;
	}

#else /* !CONFIG_BT_USE_PSA_API */
	ret = uECC_valid_public_key(ecc.public_key_be, &curve_secp256r1);
	if (ret < 0) {
		LOG_ERR("public key is not valid (ret %d)", ret);
		ret = -EIO;
		goto exit;
	}
	ret = uECC_shared_secret(ecc.public_key_be,
				use_debug ? debug_private_key_be : ecc.private_key_be,
				ecc.dhkey_be, &curve_secp256r1);
	ret = (ret == TC_CRYPTO_FAIL) ? -EIO : 0;
#endif /* CONFIG_BT_USE_PSA_API */

exit:
	buf = bt_buf_get_rx(BT_BUF_EVT, K_FOREVER);

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->evt = BT_HCI_EVT_LE_META_EVENT;
	hdr->len = sizeof(*meta) + sizeof(*evt);

	meta = net_buf_add(buf, sizeof(*meta));
	meta->subevent = BT_HCI_EVT_LE_GENERATE_DHKEY_COMPLETE;

	evt = net_buf_add(buf, sizeof(*evt));

	if (ret != 0) {
		evt->status = BT_HCI_ERR_UNSPECIFIED;
		(void)memset(evt->dhkey, 0xff, sizeof(evt->dhkey));
	} else {
		evt->status = 0U;
		/* Convert from big-endian (provided by crypto API) to
		 * little-endian HCI.
		 */
		sys_memcpy_swap(evt->dhkey, ecc.dhkey_be, sizeof(ecc.dhkey_be));
	}

	atomic_clear_bit(flags, PENDING_DHKEY);

	bt_hci_recv(bt_dev.hci, buf);
}

static void ecc_process(struct k_work *work)
{
	if (atomic_test_bit(flags, PENDING_PUB_KEY)) {
		emulate_le_p256_public_key_cmd();
	} else if (atomic_test_bit(flags, PENDING_DHKEY)) {
		emulate_le_generate_dhkey();
	} else {
		__ASSERT(0, "Unhandled ECC command");
	}
}

static void clear_ecc_events(struct net_buf *buf)
{
	struct bt_hci_cp_le_set_event_mask *cmd;

	cmd = (void *)(buf->data + sizeof(struct bt_hci_cmd_hdr));

	/*
	 * don't enable controller ECC events as those will be generated from
	 * emulation code
	 */
	cmd->events[0] &= ~0x80; /* LE Read Local P-256 PKey Compl */
	cmd->events[1] &= ~0x01; /* LE Generate DHKey Compl Event */
}

static uint8_t le_gen_dhkey(uint8_t *key, uint8_t key_type)
{
	if (atomic_test_bit(flags, PENDING_PUB_KEY)) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	if (key_type > BT_HCI_LE_KEY_TYPE_DEBUG) {
		return BT_HCI_ERR_INVALID_PARAM;
	}

	if (atomic_test_and_set_bit(flags, PENDING_DHKEY)) {
		return BT_HCI_ERR_CMD_DISALLOWED;
	}

	/* Convert X and Y coordinates from little-endian HCI to
	 * big-endian (expected by the crypto API).
	 */
	sys_memcpy_swap(ecc.public_key_be, key, BT_PUB_KEY_COORD_LEN);
	sys_memcpy_swap(&ecc.public_key_be[BT_PUB_KEY_COORD_LEN], &key[BT_PUB_KEY_COORD_LEN],
			BT_PUB_KEY_COORD_LEN);

	atomic_set_bit_to(flags, USE_DEBUG_KEY,
			  key_type == BT_HCI_LE_KEY_TYPE_DEBUG);

	bt_long_wq_submit(&ecc_work);

	return BT_HCI_ERR_SUCCESS;
}

static void le_gen_dhkey_v1(struct net_buf *buf)
{
	struct bt_hci_cp_le_generate_dhkey *cmd;
	uint8_t status;

	cmd = (void *)buf->data;
	status = le_gen_dhkey(cmd->key, BT_HCI_LE_KEY_TYPE_GENERATED);

	net_buf_unref(buf);
	send_cmd_status(BT_HCI_OP_LE_GENERATE_DHKEY, status);
}

static void le_gen_dhkey_v2(struct net_buf *buf)
{
	struct bt_hci_cp_le_generate_dhkey_v2 *cmd;
	uint8_t status;

	cmd = (void *)buf->data;
	status = le_gen_dhkey(cmd->key, cmd->key_type);

	net_buf_unref(buf);
	send_cmd_status(BT_HCI_OP_LE_GENERATE_DHKEY_V2, status);
}

static void le_p256_pub_key(struct net_buf *buf)
{
	uint8_t status;

	net_buf_unref(buf);

	if (atomic_test_bit(flags, PENDING_DHKEY)) {
		status = BT_HCI_ERR_CMD_DISALLOWED;
	} else if (atomic_test_and_set_bit(flags, PENDING_PUB_KEY)) {
		status = BT_HCI_ERR_CMD_DISALLOWED;
	} else {
		bt_long_wq_submit(&ecc_work);
		status = BT_HCI_ERR_SUCCESS;
	}

	send_cmd_status(BT_HCI_OP_LE_P256_PUBLIC_KEY, status);
}

int bt_hci_ecc_send(struct net_buf *buf)
{
	if (bt_buf_get_type(buf) == BT_BUF_CMD) {
		struct bt_hci_cmd_hdr *chdr = (void *)buf->data;

		switch (sys_le16_to_cpu(chdr->opcode)) {
		case BT_HCI_OP_LE_P256_PUBLIC_KEY:
			net_buf_pull(buf, sizeof(*chdr));
			le_p256_pub_key(buf);
			return 0;
		case BT_HCI_OP_LE_GENERATE_DHKEY:
			net_buf_pull(buf, sizeof(*chdr));
			le_gen_dhkey_v1(buf);
			return 0;
		case BT_HCI_OP_LE_GENERATE_DHKEY_V2:
			net_buf_pull(buf, sizeof(*chdr));
			le_gen_dhkey_v2(buf);
			return 0;
		case BT_HCI_OP_LE_SET_EVENT_MASK:
			clear_ecc_events(buf);
			break;
		default:
			break;
		}
	}

	return bt_hci_send(bt_dev.hci, buf);
}

void bt_hci_ecc_supported_commands(uint8_t *supported_commands)
{
	/* LE Read Local P-256 Public Key */
	supported_commands[34] |= BIT(1);
	/* LE Generate DH Key v1 */
	supported_commands[34] |= BIT(2);
	/* LE Generate DH Key v2 */
	supported_commands[41] |= BIT(2);
}

int default_CSPRNG(uint8_t *dst, unsigned int len)
{
	return !bt_rand(dst, len);
}
