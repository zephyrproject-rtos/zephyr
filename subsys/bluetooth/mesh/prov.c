/*  Bluetooth Mesh */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <errno.h>
#include <sys/atomic.h>
#include <sys/util.h>
#include <sys/byteorder.h>

#include <net/buf.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/mesh.h>
#include <bluetooth/uuid.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_MESH_DEBUG_PROV)
#define LOG_MODULE_NAME bt_mesh_prov
#include "common/log.h"

#include "host/ecc.h"
#include "host/testing.h"

#include "crypto.h"
#include "adv.h"
#include "mesh.h"
#include "net.h"
#include "beacon.h"
#include "access.h"
#include "foundation.h"
#include "proxy.h"
#include "prov.h"
#include "settings.h"
#include "prov_bearer.h"


#define AUTH_METHOD_NO_OOB     0x00
#define AUTH_METHOD_STATIC     0x01
#define AUTH_METHOD_OUTPUT     0x02
#define AUTH_METHOD_INPUT      0x03

#define OUTPUT_OOB_BLINK       0x00
#define OUTPUT_OOB_BEEP        0x01
#define OUTPUT_OOB_VIBRATE     0x02
#define OUTPUT_OOB_NUMBER      0x03
#define OUTPUT_OOB_STRING      0x04

#define INPUT_OOB_PUSH         0x00
#define INPUT_OOB_TWIST        0x01
#define INPUT_OOB_NUMBER       0x02
#define INPUT_OOB_STRING       0x03

#define PUB_KEY_NO_OOB         0x00
#define PUB_KEY_OOB            0x01

#define PROV_INVITE            0x00
#define PROV_CAPABILITIES      0x01
#define PROV_START             0x02
#define PROV_PUB_KEY           0x03
#define PROV_INPUT_COMPLETE    0x04
#define PROV_CONFIRM           0x05
#define PROV_RANDOM            0x06
#define PROV_DATA              0x07
#define PROV_COMPLETE          0x08
#define PROV_FAILED            0x09

#define PROV_NO_PDU            0xff

#define PROV_ALG_P256          0x00

enum {
	WAIT_PUB_KEY,          /* Waiting for local PubKey to be generated */
	LINK_ACTIVE,           /* Link has been opened */
	WAIT_NUMBER,           /* Waiting for number input from user */
	WAIT_STRING,           /* Waiting for string input from user */
	NOTIFY_INPUT_COMPLETE, /* Notify that input has been completed. */
	PROVISIONER,           /* The link was opened as provisioner */

	NUM_FLAGS,
};

struct provisioner_link {
	struct bt_mesh_cdb_node *node;
	uint16_t addr;
	uint16_t net_idx;
	uint8_t  attention_duration;
	uint8_t  uuid[16];
};

struct prov_link {
	ATOMIC_DEFINE(flags, NUM_FLAGS);
#if defined(CONFIG_BT_MESH_PROVISIONER)
	struct provisioner_link provisioner;
#endif

	uint8_t  dhkey[32];         /* Calculated DHKey */
	uint8_t  expect;            /* Next expected PDU */

	uint8_t  oob_method;
	uint8_t  oob_action;
	uint8_t  oob_size;

	uint8_t  conf[16];          /* Remote Confirmation */
	uint8_t  rand[16];          /* Local Random */
	uint8_t  auth[16];          /* Authentication Value */

	uint8_t  conf_salt[16];     /* ConfirmationSalt */
	uint8_t  conf_key[16];      /* ConfirmationKey */
	uint8_t  conf_inputs[145];  /* ConfirmationInputs */
	uint8_t  prov_salt[16];     /* Provisioning Salt */

	const struct prov_bearer *bearer;
};

#define PROV_BUF(name, len) \
	NET_BUF_SIMPLE_DEFINE(name, PROV_BEARER_BUF_HEADROOM + len)

static struct prov_link link;

static const struct bt_mesh_prov *prov;

static void send_pub_key(void);
static void pub_key_ready(const uint8_t *pkey);

static int reset_state(void)
{
	static struct bt_pub_key_cb pub_key_cb = {
		.func = pub_key_ready,
	};
	int err;

	/* Disable Attention Timer if it was set */
	if (link.conf_inputs[0]) {
		bt_mesh_attention(NULL, 0);
	}

#if defined(CONFIG_BT_MESH_PROVISIONER)
	if (link.provisioner.node != NULL) {
		bt_mesh_cdb_node_del(link.provisioner.node, false);
	}
#endif

	memset(&link, 0, sizeof(link));

	err = bt_pub_key_gen(&pub_key_cb);
	if (err) {
		BT_ERR("Failed to generate public key (%d)", err);
		return err;
	}

	return 0;
}

static inline int prov_send(struct net_buf_simple *buf,
			    prov_bearer_send_complete_t cb)
{
	return link.bearer->send(buf, cb, NULL);
}

static void prov_buf_init(struct net_buf_simple *buf, uint8_t type)
{
	net_buf_simple_reserve(buf, PROV_BEARER_BUF_HEADROOM);
	net_buf_simple_add_u8(buf, type);
}

static void prov_link_close(enum prov_bearer_link_status status)
{
	BT_DBG("%u", status);
	link.expect = PROV_NO_PDU;

	link.bearer->link_close(status);
}

static void prov_send_fail_msg(uint8_t err)
{
	PROV_BUF(buf, 2);

	BT_DBG("%u", err);

	link.expect = PROV_NO_PDU;

	prov_buf_init(&buf, PROV_FAILED);
	net_buf_simple_add_u8(&buf, err);

	if (prov_send(&buf, NULL)) {
		BT_ERR("Failed to send Provisioning Failed message");
	}
}

static void prov_fail(uint8_t reason)
{
	/* According to Bluetooth Mesh Specification v1.0.1, Section 5.4.4, the
	 * provisioner just closes the link when something fails, while the
	 * provisionee sends the fail message, and waits for the provisioner to
	 * close the link.
	 */
	if (atomic_test_bit(link.flags, PROVISIONER)) {
		prov_link_close(PROV_BEARER_LINK_STATUS_FAIL);
	} else {
		prov_send_fail_msg(reason);
	}
}

static void prov_invite(const uint8_t *data)
{
	PROV_BUF(buf, 12);

	BT_DBG("Attention Duration: %u seconds", data[0]);

	if (data[0]) {
		bt_mesh_attention(NULL, data[0]);
	}

	link.conf_inputs[0] = data[0];

	prov_buf_init(&buf, PROV_CAPABILITIES);

	/* Number of Elements supported */
	net_buf_simple_add_u8(&buf, bt_mesh_elem_count());

	/* Supported algorithms - FIPS P-256 Eliptic Curve */
	net_buf_simple_add_be16(&buf, BIT(PROV_ALG_P256));

	/* Public Key Type, Only "No OOB" Public Key is supported */
	net_buf_simple_add_u8(&buf, PUB_KEY_NO_OOB);

	/* Static OOB Type */
	net_buf_simple_add_u8(&buf, prov->static_val ? BIT(0) : 0x00);

	/* Output OOB Size */
	net_buf_simple_add_u8(&buf, prov->output_size);

	/* Output OOB Action */
	net_buf_simple_add_be16(&buf, prov->output_actions);

	/* Input OOB Size */
	net_buf_simple_add_u8(&buf, prov->input_size);

	/* Input OOB Action */
	net_buf_simple_add_be16(&buf, prov->input_actions);

	memcpy(&link.conf_inputs[1], &buf.data[1], 11);

	if (prov_send(&buf, NULL)) {
		BT_ERR("Failed to send capabilities");
		return;
	}

	link.expect = PROV_START;
}

#if defined(CONFIG_BT_MESH_PROVISIONER)
static void start_sent(int err, void *cb_data);

static void send_invite(void)
{
	PROV_BUF(inv, 2);

	BT_DBG("");

	prov_buf_init(&inv, PROV_INVITE);
	net_buf_simple_add_u8(&inv, link.provisioner.attention_duration);

	link.conf_inputs[0] = link.provisioner.attention_duration;

	if (prov_send(&inv, NULL)) {
		BT_ERR("Failed to send invite");
		return;
	}

	link.expect = PROV_CAPABILITIES;
}

static void send_start(void)
{
	PROV_BUF(start, 6);

	BT_DBG("");

	prov_buf_init(&start, PROV_START);

	net_buf_simple_add_u8(&start, PROV_ALG_P256);
	net_buf_simple_add_u8(&start, PUB_KEY_NO_OOB);
	net_buf_simple_add_u8(&start, AUTH_METHOD_NO_OOB);
	memset(link.auth, 0, sizeof(link.auth));

	net_buf_simple_add_u8(&start, 0); /* Auth Action */
	net_buf_simple_add_u8(&start, 0); /* Auth Size */

	memcpy(&link.conf_inputs[12], &start.data[1], 5);

	if (prov_send(&start, start_sent)) {
		BT_ERR("Failed to send start");
	}
}

static void start_sent(int err, void *cb_data)
{
	if (!bt_pub_key_get()) {
		atomic_set_bit(link.flags, WAIT_PUB_KEY);
		BT_WARN("Waiting for local public key");
	} else {
		send_pub_key();
	}
}
#endif

static void prov_capabilities(const uint8_t *data)
{
#if defined(CONFIG_BT_MESH_PROVISIONER)
	uint16_t algorithms, output_action, input_action;

	BT_DBG("Elements: %u", data[0]);

	algorithms = sys_get_be16(&data[1]);
	BT_DBG("Algorithms:        %u", algorithms);

	BT_DBG("Public Key Type:   0x%02x", data[3]);
	BT_DBG("Static OOB Type:   0x%02x", data[4]);
	BT_DBG("Output OOB Size:   %u", data[5]);

	output_action = sys_get_be16(&data[6]);
	BT_DBG("Output OOB Action: 0x%04x", output_action);

	BT_DBG("Input OOB Size:    %u", data[8]);

	input_action = sys_get_be16(&data[9]);
	BT_DBG("Input OOB Action:  0x%04x", input_action);

	if (data[0] == 0) {
		BT_ERR("Invalid number of elements");
		prov_fail(PROV_ERR_NVAL_FMT);
		return;
	}

	link.provisioner.node =
		bt_mesh_cdb_node_alloc(link.provisioner.uuid,
				       link.provisioner.addr, data[0],
				       link.provisioner.net_idx);
	if (link.provisioner.node == NULL) {
		BT_ERR("Failed allocating node 0x%04x", link.provisioner.addr);
		prov_fail(PROV_ERR_RESOURCES);
		return;
	}

	memcpy(&link.conf_inputs[1], data, 11);

	send_start();
#endif
}

static bt_mesh_output_action_t output_action(uint8_t action)
{
	switch (action) {
	case OUTPUT_OOB_BLINK:
		return BT_MESH_BLINK;
	case OUTPUT_OOB_BEEP:
		return BT_MESH_BEEP;
	case OUTPUT_OOB_VIBRATE:
		return BT_MESH_VIBRATE;
	case OUTPUT_OOB_NUMBER:
		return BT_MESH_DISPLAY_NUMBER;
	case OUTPUT_OOB_STRING:
		return BT_MESH_DISPLAY_STRING;
	default:
		return BT_MESH_NO_OUTPUT;
	}
}

static bt_mesh_input_action_t input_action(uint8_t action)
{
	switch (action) {
	case INPUT_OOB_PUSH:
		return BT_MESH_PUSH;
	case INPUT_OOB_TWIST:
		return BT_MESH_TWIST;
	case INPUT_OOB_NUMBER:
		return BT_MESH_ENTER_NUMBER;
	case INPUT_OOB_STRING:
		return BT_MESH_ENTER_STRING;
	default:
		return BT_MESH_NO_INPUT;
	}
}

static int prov_auth(uint8_t method, uint8_t action, uint8_t size)
{
	bt_mesh_output_action_t output;
	bt_mesh_input_action_t input;

	switch (method) {
	case AUTH_METHOD_NO_OOB:
		if (action || size) {
			return -EINVAL;
		}

		(void)memset(link.auth, 0, sizeof(link.auth));
		return 0;
	case AUTH_METHOD_STATIC:
		if (action || size) {
			return -EINVAL;
		}

		memcpy(link.auth + 16 - prov->static_val_len,
		       prov->static_val, prov->static_val_len);
		(void)memset(link.auth, 0,
			     sizeof(link.auth) - prov->static_val_len);
		return 0;

	case AUTH_METHOD_OUTPUT:
		output = output_action(action);
		if (!output) {
			return -EINVAL;
		}

		if (!(prov->output_actions & output)) {
			return -EINVAL;
		}

		if (size > prov->output_size) {
			return -EINVAL;
		}

		atomic_set_bit(link.flags, NOTIFY_INPUT_COMPLETE);

		if (output == BT_MESH_DISPLAY_STRING) {
			unsigned char str[9];
			uint8_t i;

			bt_rand(str, size);

			/* Normalize to '0' .. '9' & 'A' .. 'Z' */
			for (i = 0U; i < size; i++) {
				str[i] %= 36;
				if (str[i] < 10) {
					str[i] += '0';
				} else {
					str[i] += 'A' - 10;
				}
			}
			str[size] = '\0';

			memcpy(link.auth, str, size);
			(void)memset(link.auth + size, 0,
				     sizeof(link.auth) - size);

			return prov->output_string((char *)str);
		} else {
			uint32_t div[8] = { 10, 100, 1000, 10000, 100000,
					    1000000, 10000000, 100000000 };
			uint32_t num;

			bt_rand(&num, sizeof(num));
			num %= div[size - 1];

			sys_put_be32(num, &link.auth[12]);
			(void)memset(link.auth, 0, 12);

			return prov->output_number(output, num);
		}

	case AUTH_METHOD_INPUT:
		input = input_action(action);
		if (!input) {
			return -EINVAL;
		}

		if (!(prov->input_actions & input)) {
			return -EINVAL;
		}

		if (size > prov->input_size) {
			return -EINVAL;
		}

		if (input == BT_MESH_ENTER_STRING) {
			atomic_set_bit(link.flags, WAIT_STRING);
		} else {
			atomic_set_bit(link.flags, WAIT_NUMBER);
		}

		return prov->input(input, size);

	default:
		return -EINVAL;
	}
}

static void prov_start(const uint8_t *data)
{
	BT_DBG("Algorithm:   0x%02x", data[0]);
	BT_DBG("Public Key:  0x%02x", data[1]);
	BT_DBG("Auth Method: 0x%02x", data[2]);
	BT_DBG("Auth Action: 0x%02x", data[3]);
	BT_DBG("Auth Size:   0x%02x", data[4]);

	if (data[0] != PROV_ALG_P256) {
		BT_ERR("Unknown algorithm 0x%02x", data[0]);
		prov_fail(PROV_ERR_NVAL_FMT);
		return;
	}

	if (data[1] != PUB_KEY_NO_OOB) {
		BT_ERR("Invalid public key type: 0x%02x", data[1]);
		prov_fail(PROV_ERR_NVAL_FMT);
		return;
	}

	memcpy(&link.conf_inputs[12], data, 5);

	link.expect = PROV_PUB_KEY;

	if (prov_auth(data[2], data[3], data[4]) < 0) {
		BT_ERR("Invalid authentication method: 0x%02x; "
			"action: 0x%02x; size: 0x%02x", data[2], data[3],
			data[4]);
		prov_fail(PROV_ERR_NVAL_FMT);
	}
}

static void send_confirm(void)
{
	PROV_BUF(cfm, 17);

	BT_DBG("ConfInputs[0]   %s", bt_hex(link.conf_inputs, 64));
	BT_DBG("ConfInputs[64]  %s", bt_hex(&link.conf_inputs[64], 64));
	BT_DBG("ConfInputs[128] %s", bt_hex(&link.conf_inputs[128], 17));

	if (bt_mesh_prov_conf_salt(link.conf_inputs, link.conf_salt)) {
		BT_ERR("Unable to generate confirmation salt");
		prov_fail(PROV_ERR_UNEXP_ERR);
		return;
	}

	BT_DBG("ConfirmationSalt: %s", bt_hex(link.conf_salt, 16));

	if (bt_mesh_prov_conf_key(link.dhkey, link.conf_salt, link.conf_key)) {
		BT_ERR("Unable to generate confirmation key");
		prov_fail(PROV_ERR_UNEXP_ERR);
		return;
	}

	BT_DBG("ConfirmationKey: %s", bt_hex(link.conf_key, 16));

	if (bt_rand(link.rand, 16)) {
		BT_ERR("Unable to generate random number");
		prov_fail(PROV_ERR_UNEXP_ERR);
		return;
	}

	BT_DBG("LocalRandom: %s", bt_hex(link.rand, 16));

	prov_buf_init(&cfm, PROV_CONFIRM);

	if (bt_mesh_prov_conf(link.conf_key, link.rand, link.auth,
			      net_buf_simple_add(&cfm, 16))) {
		BT_ERR("Unable to generate confirmation value");
		prov_fail(PROV_ERR_UNEXP_ERR);
		return;
	}

	if (prov_send(&cfm, NULL)) {
		BT_ERR("Failed to send Provisioning Confirm");
		return;
	}

	if (atomic_test_bit(link.flags, PROVISIONER)) {
		link.expect = PROV_CONFIRM;
	} else {
		link.expect = PROV_RANDOM;
	}
}

static void send_input_complete(void)
{
	PROV_BUF(buf, 1);

	prov_buf_init(&buf, PROV_INPUT_COMPLETE);
	if (prov_send(&buf, NULL)) {
		BT_ERR("Failed to send Provisioning Input Complete");
	}
	link.expect = PROV_CONFIRM;
}

int bt_mesh_input_number(uint32_t num)
{
	BT_DBG("%u", num);

	if (!atomic_test_and_clear_bit(link.flags, WAIT_NUMBER)) {
		return -EINVAL;
	}

	sys_put_be32(num, &link.auth[12]);

	send_input_complete();

	return 0;
}

int bt_mesh_input_string(const char *str)
{
	BT_DBG("%s", str);

	if (!atomic_test_and_clear_bit(link.flags, WAIT_STRING)) {
		return -EINVAL;
	}

	strncpy((char *)link.auth, str, prov->input_size);

	send_input_complete();

	return 0;
}

static void send_pub_key(void)
{
	PROV_BUF(buf, 65);
	const uint8_t *key;

	key = bt_pub_key_get();
	if (!key) {
		BT_ERR("No public key available");
		prov_fail(PROV_ERR_UNEXP_ERR);
		return;
	}

	BT_DBG("Local Public Key: %s", bt_hex(key, 64));

	prov_buf_init(&buf, PROV_PUB_KEY);

	/* Swap X and Y halves independently to big-endian */
	sys_memcpy_swap(net_buf_simple_add(&buf, 32), key, 32);
	sys_memcpy_swap(net_buf_simple_add(&buf, 32), &key[32], 32);

	if (atomic_test_bit(link.flags, PROVISIONER)) {
		/* PublicKeyProvisioner */
		memcpy(&link.conf_inputs[17], &buf.data[1], 64);
	} else {
		/* PublicKeyRemote */
		memcpy(&link.conf_inputs[81], &buf.data[1], 64);
	}

	if (prov_send(&buf, NULL)) {
		BT_ERR("Failed to send Public Key");
		return;
	}

	if (atomic_test_bit(link.flags, PROVISIONER)) {
		link.expect = PROV_PUB_KEY;
	} else {
		if (atomic_test_bit(link.flags, WAIT_NUMBER) ||
		    atomic_test_bit(link.flags, WAIT_STRING)) {
			link.expect = PROV_NO_PDU; /* Wait for input */
		} else {
			link.expect = PROV_CONFIRM;
		}
	}
}

static void prov_dh_key_cb(const uint8_t dhkey[32])
{
	BT_DBG("%p", dhkey);

	if (!dhkey) {
		BT_ERR("DHKey generation failed");
		prov_fail(PROV_ERR_UNEXP_ERR);
		return;
	}

	sys_memcpy_swap(link.dhkey, dhkey, 32);

	BT_DBG("DHkey: %s", bt_hex(link.dhkey, 32));

	if (atomic_test_bit(link.flags, PROVISIONER)) {
		send_confirm();
	} else {
		send_pub_key();
	}
}

static void prov_dh_key_gen(void)
{
	uint8_t remote_pk_le[64], *remote_pk;

	if (atomic_test_bit(link.flags, PROVISIONER)) {
		remote_pk = &link.conf_inputs[81];
	} else {
		remote_pk = &link.conf_inputs[17];
	}

	/* Copy remote key in little-endian for bt_dh_key_gen().
	 * X and Y halves are swapped independently. The bt_dh_key_gen()
	 * will also take care of validating the remote public key.
	 */
	sys_memcpy_swap(remote_pk_le, remote_pk, 32);
	sys_memcpy_swap(&remote_pk_le[32], &remote_pk[32], 32);

	if (bt_dh_key_gen(remote_pk_le, prov_dh_key_cb)) {
		BT_ERR("Failed to generate DHKey");
		prov_fail(PROV_ERR_UNEXP_ERR);
	}
}

static void prov_pub_key(const uint8_t *data)
{
	BT_DBG("Remote Public Key: %s", bt_hex(data, 64));

	if (atomic_test_bit(link.flags, PROVISIONER)) {
		/* PublicKeyDevice */
		memcpy(&link.conf_inputs[81], data, 64);
		link.bearer->clear_tx();
	} else {
		/* PublicKeyProvisioner */
		memcpy(&link.conf_inputs[17], data, 64);

		if (!bt_pub_key_get()) {
			/* Clear retransmit timer */
			link.bearer->clear_tx();
			atomic_set_bit(link.flags, WAIT_PUB_KEY);
			BT_WARN("Waiting for local public key");
			return;
		}
	}

	prov_dh_key_gen();
}

static void pub_key_ready(const uint8_t *pkey)
{
	if (!pkey) {
		BT_WARN("Public key not available");
		return;
	}

	BT_DBG("Local public key ready");

	if (atomic_test_and_clear_bit(link.flags, WAIT_PUB_KEY)) {
		if (atomic_test_bit(link.flags, PROVISIONER)) {
			send_pub_key();
		} else {
			prov_dh_key_gen();
		}
	}
}

static void notify_input_complete(void)
{
	if (atomic_test_and_clear_bit(link.flags, NOTIFY_INPUT_COMPLETE) &&
	    prov->input_complete) {
		prov->input_complete();
	}
}

static void prov_input_complete(const uint8_t *data)
{
	BT_DBG("");
	notify_input_complete();
}

#if defined(CONFIG_BT_MESH_PROVISIONER)
static void send_prov_data(void)
{
	PROV_BUF(pdu, 34);
	struct bt_mesh_cdb_subnet *sub;
	uint8_t session_key[16];
	uint8_t nonce[13];
	int err;

	err = bt_mesh_session_key(link.dhkey, link.prov_salt, session_key);
	if (err) {
		BT_ERR("Unable to generate session key");
		prov_fail(PROV_ERR_UNEXP_ERR);
		return;
	}

	BT_DBG("SessionKey: %s", bt_hex(session_key, 16));

	err = bt_mesh_prov_nonce(link.dhkey, link.prov_salt, nonce);
	if (err) {
		BT_ERR("Unable to generate session nonce");
		prov_fail(PROV_ERR_UNEXP_ERR);
		return;
	}

	BT_DBG("Nonce: %s", bt_hex(nonce, 13));

	err = bt_mesh_dev_key(link.dhkey, link.prov_salt,
			      link.provisioner.node->dev_key);
	if (err) {
		BT_ERR("Unable to generate device key");
		prov_fail(PROV_ERR_UNEXP_ERR);
		return;
	}

	BT_DBG("DevKey: %s", bt_hex(link.provisioner.node->dev_key, 16));

	sub = bt_mesh_cdb_subnet_get(link.provisioner.node->net_idx);
	if (sub == NULL) {
		BT_ERR("No subnet with net_idx %u",
		       link.provisioner.node->net_idx);
		prov_fail(PROV_ERR_UNEXP_ERR);
		return;
	}

	prov_buf_init(&pdu, PROV_DATA);
	net_buf_simple_add_mem(&pdu, sub->keys[sub->kr_flag].net_key, 16);
	net_buf_simple_add_be16(&pdu, link.provisioner.node->net_idx);
	net_buf_simple_add_u8(&pdu, bt_mesh_cdb_subnet_flags(sub));
	net_buf_simple_add_be32(&pdu, bt_mesh_cdb.iv_index);
	net_buf_simple_add_be16(&pdu, link.provisioner.node->addr);
	net_buf_simple_add(&pdu, 8); /* For MIC */

	BT_DBG("net_idx %u, iv_index 0x%08x, addr 0x%04x",
	       link.provisioner.node->net_idx, bt_mesh.iv_index,
	       link.provisioner.node->addr);

	err = bt_mesh_prov_encrypt(session_key, nonce, &pdu.data[1],
				   &pdu.data[1]);
	if (err) {
		BT_ERR("Unable to encrypt provisioning data");
		prov_fail(PROV_ERR_DECRYPT);
		return;
	}

	if (prov_send(&pdu, NULL)) {
		BT_ERR("Failed to send Provisioning Data");
		return;
	}

	link.expect = PROV_COMPLETE;
}
#endif

static void prov_complete(const uint8_t *data)
{
#if defined(CONFIG_BT_MESH_PROVISIONER)
	struct bt_mesh_cdb_node *node = link.provisioner.node;

	BT_DBG("key %s, net_idx %u, num_elem %u, addr 0x%04x",
	       bt_hex(node->dev_key, 16), node->net_idx, node->num_elem,
	       node->addr);

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		bt_mesh_store_cdb_node(node);
	}

	link.provisioner.node = NULL;
	prov_link_close(PROV_BEARER_LINK_STATUS_SUCCESS);

	if (prov->node_added) {
		prov->node_added(node->net_idx, node->uuid, node->addr,
				 node->num_elem);
	}
#endif
}

static void send_random(void)
{
	PROV_BUF(rnd, 17);

	prov_buf_init(&rnd, PROV_RANDOM);
	net_buf_simple_add_mem(&rnd, link.rand, 16);

	if (prov_send(&rnd, NULL)) {
		BT_ERR("Failed to send Provisioning Random");
		return;
	}

	if (atomic_test_bit(link.flags, PROVISIONER)) {
		link.expect = PROV_RANDOM;
	} else {
		link.expect = PROV_DATA;
	}
}

static void prov_random(const uint8_t *data)
{
	uint8_t conf_verify[16];
	const uint8_t *prov_rand, *dev_rand;

	BT_DBG("Remote Random: %s", bt_hex(data, 16));

	if (bt_mesh_prov_conf(link.conf_key, data, link.auth, conf_verify)) {
		BT_ERR("Unable to calculate confirmation verification");
		prov_fail(PROV_ERR_UNEXP_ERR);
		return;
	}

	if (memcmp(conf_verify, link.conf, 16)) {
		BT_ERR("Invalid confirmation value");
		BT_DBG("Received:   %s", bt_hex(link.conf, 16));
		BT_DBG("Calculated: %s",  bt_hex(conf_verify, 16));
		prov_fail(PROV_ERR_CFM_FAILED);
		return;
	}

	if (atomic_test_bit(link.flags, PROVISIONER)) {
		prov_rand = link.rand;
		dev_rand = data;
	} else {
		prov_rand = data;
		dev_rand = link.rand;
	}

	if (bt_mesh_prov_salt(link.conf_salt, prov_rand, dev_rand,
			      link.prov_salt)) {
		BT_ERR("Failed to generate provisioning salt");
		prov_fail(PROV_ERR_UNEXP_ERR);
		return;
	}

	BT_DBG("ProvisioningSalt: %s", bt_hex(link.prov_salt, 16));

#if defined(CONFIG_BT_MESH_PROVISIONER)
	if (atomic_test_bit(link.flags, PROVISIONER)) {
		send_prov_data();
		return;
	}
#endif

	send_random();
}

static void prov_confirm(const uint8_t *data)
{
	BT_DBG("Remote Confirm: %s", bt_hex(data, 16));

	memcpy(link.conf, data, 16);

	notify_input_complete();

	if (atomic_test_bit(link.flags, PROVISIONER)) {
		send_random();
	} else {
		send_confirm();
	}
}

static inline bool is_pb_gatt(void)
{
	return link.bearer && link.bearer->type == BT_MESH_PROV_GATT;
}

static void prov_data(const uint8_t *data)
{
	PROV_BUF(msg, 1);
	uint8_t session_key[16];
	uint8_t nonce[13];
	uint8_t dev_key[16];
	uint8_t pdu[25];
	uint8_t flags;
	uint32_t iv_index;
	uint16_t addr;
	uint16_t net_idx;
	int err;
	bool identity_enable;

	BT_DBG("");

	err = bt_mesh_session_key(link.dhkey, link.prov_salt, session_key);
	if (err) {
		BT_ERR("Unable to generate session key");
		prov_fail(PROV_ERR_UNEXP_ERR);
		return;
	}

	BT_DBG("SessionKey: %s", bt_hex(session_key, 16));

	err = bt_mesh_prov_nonce(link.dhkey, link.prov_salt, nonce);
	if (err) {
		BT_ERR("Unable to generate session nonce");
		prov_fail(PROV_ERR_UNEXP_ERR);
		return;
	}

	BT_DBG("Nonce: %s", bt_hex(nonce, 13));

	err = bt_mesh_prov_decrypt(session_key, nonce, data, pdu);
	if (err) {
		BT_ERR("Unable to decrypt provisioning data");
		prov_fail(PROV_ERR_DECRYPT);
		return;
	}

	err = bt_mesh_dev_key(link.dhkey, link.prov_salt, dev_key);
	if (err) {
		BT_ERR("Unable to generate device key");
		prov_fail(PROV_ERR_UNEXP_ERR);
		return;
	}

	BT_DBG("DevKey: %s", bt_hex(dev_key, 16));

	net_idx = sys_get_be16(&pdu[16]);
	flags = pdu[18];
	iv_index = sys_get_be32(&pdu[19]);
	addr = sys_get_be16(&pdu[23]);

	BT_DBG("net_idx %u iv_index 0x%08x, addr 0x%04x",
	       net_idx, iv_index, addr);

	prov_buf_init(&msg, PROV_COMPLETE);
	if (prov_send(&msg, NULL)) {
		BT_ERR("Failed to send Provisioning Complete");
		return;
	}

	/* Ignore any further PDUs on this link */
	link.expect = PROV_NO_PDU;

	/* Store info, since bt_mesh_provision() will end up clearing it */
	if (IS_ENABLED(CONFIG_BT_MESH_GATT_PROXY)) {
		identity_enable = is_pb_gatt();
	} else {
		identity_enable = false;
	}

	err = bt_mesh_provision(pdu, net_idx, flags, iv_index, addr, dev_key);
	if (err) {
		BT_ERR("Failed to provision (err %d)", err);
		return;
	}

	/* After PB-GATT provisioning we should start advertising
	 * using Node Identity.
	 */
	if (IS_ENABLED(CONFIG_BT_MESH_GATT_PROXY) && identity_enable) {
		bt_mesh_proxy_identity_enable();
	}
}

static void prov_failed(const uint8_t *data)
{
	BT_WARN("Error: 0x%02x", data[0]);
	reset_state();
}

static const struct {
	void (*func)(const uint8_t *data);
	uint16_t len;
} prov_handlers[] = {
	{ prov_invite, 1 },
	{ prov_capabilities, 11 },
	{ prov_start, 5, },
	{ prov_pub_key, 64 },
	{ prov_input_complete, 0 },
	{ prov_confirm, 16 },
	{ prov_random, 16 },
	{ prov_data, 33 },
	{ prov_complete, 0 },
	{ prov_failed, 1 },
};

static void prov_recv(const struct prov_bearer *bearer, void *cb_data,
		      struct net_buf_simple *buf)
{
	uint8_t type = buf->data[0];

	BT_DBG("type 0x%02x len %u", type, buf->len);

	if (type >= ARRAY_SIZE(prov_handlers)) {
		BT_ERR("Unknown provisioning PDU type 0x%02x", type);
		prov_fail(PROV_ERR_NVAL_FMT);
		return;
	}

	if (type != PROV_FAILED && type != link.expect) {
		BT_WARN("Unexpected msg 0x%02x != 0x%02x", type, link.expect);
		prov_fail(PROV_ERR_UNEXP_PDU);
		return;
	}

	if (1 + prov_handlers[type].len != buf->len) {
		BT_ERR("Invalid length %u for type 0x%02x", buf->len, type);
		prov_fail(PROV_ERR_NVAL_FMT);
		return;
	}

	prov_handlers[type].func(&buf->data[1]);
}

static void prov_link_opened(const struct prov_bearer *bearer, void *cb_data)
{
	atomic_set_bit(link.flags, LINK_ACTIVE);

	if (prov->link_open) {
		prov->link_open(bearer->type);
	}

	link.bearer = bearer;

#if defined(CONFIG_BT_MESH_PROVISIONER)
	if (atomic_test_bit(link.flags, PROVISIONER)) {
		send_invite();
		return;
	}
#endif

	link.expect = PROV_INVITE;
}

static void prov_link_closed(const struct prov_bearer *bearer, void *cb_data,
			     enum prov_bearer_link_status reason)
{
	/* Reset state before calling link_close, so a new provisioning
	 * procedure can be started from the callback.
	 */
	reset_state();

	BT_DBG("%u", reason);

	if (prov->link_close) {
		prov->link_close(bearer->type);
	}
}

static void prov_bearer_error(const struct prov_bearer *bearer, void *cb_data,
			      uint8_t err)
{
	prov_fail(err);
}

static const struct prov_bearer_cb prov_bearer_cb = {
	.link_opened = prov_link_opened,
	.link_closed = prov_link_closed,
	.error = prov_bearer_error,
	.recv = prov_recv,
};

#if defined(CONFIG_BT_MESH_PB_ADV) && defined(CONFIG_BT_MESH_PROVISIONER)
int bt_mesh_pb_adv_open(const uint8_t uuid[16], uint16_t net_idx, uint16_t addr,
			uint8_t attention_duration)
{
	int err;

	if (atomic_test_and_set_bit(link.flags, LINK_ACTIVE)) {
		return -EBUSY;
	}

	struct bt_uuid_128 uuid_repr = { .uuid = { BT_UUID_TYPE_128 } };

	memcpy(uuid_repr.val, uuid, 16);
	BT_DBG("Provisioning %s", bt_uuid_str(&uuid_repr.uuid));

	atomic_set_bit(link.flags, PROVISIONER);
	memcpy(link.provisioner.uuid, uuid, 16);
	link.provisioner.addr = addr;
	link.provisioner.net_idx = net_idx;
	link.provisioner.attention_duration = attention_duration;
	link.bearer = &pb_adv;

	err = link.bearer->link_open(link.provisioner.uuid, PROTOCOL_TIMEOUT,
				     &prov_bearer_cb, NULL);
	if (err) {
		atomic_clear_bit(link.flags, LINK_ACTIVE);
	}

	return err;
}
#endif

const struct bt_mesh_prov *bt_mesh_prov_get(void)
{
	return prov;
}

bool bt_prov_active(void)
{
	return atomic_test_bit(link.flags, LINK_ACTIVE);
}

int bt_mesh_prov_init(const struct bt_mesh_prov *prov_info)
{
	if (!prov_info) {
		BT_ERR("No provisioning context provided");
		return -EINVAL;
	}

	prov = prov_info;

	if (IS_ENABLED(CONFIG_BT_MESH_PB_ADV)) {
		pb_adv_init();
	}

	if (IS_ENABLED(CONFIG_BT_MESH_PB_GATT)) {
		pb_gatt_init();
	}

	return reset_state();
}

int bt_mesh_prov_enable(bt_mesh_prov_bearer_t bearers)
{
	if (bt_mesh_is_provisioned()) {
		return -EALREADY;
	}

	if (IS_ENABLED(CONFIG_BT_DEBUG)) {
		struct bt_uuid_128 uuid = { .uuid = { BT_UUID_TYPE_128 } };

		memcpy(uuid.val, prov->uuid, 16);
		BT_INFO("Device UUID: %s", bt_uuid_str(&uuid.uuid));
	}

	if (IS_ENABLED(CONFIG_BT_MESH_PB_ADV) &&
	    (bearers & BT_MESH_PROV_ADV)) {
		pb_adv.link_accept(&prov_bearer_cb, NULL);
	}

	if (IS_ENABLED(CONFIG_BT_MESH_PB_GATT) &&
	    (bearers & BT_MESH_PROV_GATT)) {
		pb_gatt.link_accept(&prov_bearer_cb, NULL);
	}

	return 0;
}

int bt_mesh_prov_disable(bt_mesh_prov_bearer_t bearers)
{
	if (bt_mesh_is_provisioned()) {
		return -EALREADY;
	}

	if (IS_ENABLED(CONFIG_BT_MESH_PB_ADV) &&
	    (bearers & BT_MESH_PROV_ADV)) {
		bt_mesh_beacon_disable();
		bt_mesh_scan_disable();
	}

	if (IS_ENABLED(CONFIG_BT_MESH_PB_GATT) &&
	    (bearers & BT_MESH_PROV_GATT)) {
		bt_mesh_proxy_prov_disable(true);
	}

	return 0;
}

void bt_mesh_prov_complete(uint16_t net_idx, uint16_t addr)
{
	if (prov->complete) {
		prov->complete(net_idx, addr);
	}
}

void bt_mesh_prov_reset(void)
{
	if (IS_ENABLED(CONFIG_BT_MESH_PB_ADV)) {
		pb_adv_reset();
	}

	if (IS_ENABLED(CONFIG_BT_MESH_PB_GATT)) {
		pb_gatt_reset();
	}

	reset_state();

	if (prov->reset) {
		prov->reset();
	}
}
