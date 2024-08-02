/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2020 Lingao Meng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <errno.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/net/buf.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/mesh.h>
#include <zephyr/bluetooth/uuid.h>

#include "common/bt_str.h"

#include "crypto.h"
#include "mesh.h"
#include "net.h"
#include "rpl.h"
#include "beacon.h"
#include "access.h"
#include "foundation.h"
#include "proxy.h"
#include "pb_gatt_srv.h"
#include "prov.h"
#include "settings.h"
#include "rpr.h"

#define LOG_LEVEL CONFIG_BT_MESH_PROVISIONEE_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mesh_provisionee);

static void reprovision_fail(void);

static void prov_send_fail_msg(uint8_t err)
{
	PROV_BUF(buf, PDU_LEN_FAILED);

	LOG_DBG("%u", err);

	bt_mesh_prov_link.expect = PROV_NO_PDU;

	bt_mesh_prov_buf_init(&buf, PROV_FAILED);
	net_buf_simple_add_u8(&buf, err);

	if (bt_mesh_prov_send(&buf, NULL)) {
		LOG_ERR("Failed to send Provisioning Failed message");
	}
}

static void prov_fail(uint8_t reason)
{
	/* According to MshPRTv1.1: 5.4.4, the provisioner just closes the link when something
	 * fails, while the provisionee sends the fail message, and waits for the provisioner to
	 * close the link.
	 */
	prov_send_fail_msg(reason);

	if (IS_ENABLED(CONFIG_BT_MESH_RPR_SRV) &&
	    atomic_test_bit(bt_mesh_prov_link.flags, REPROVISION)) {
		reprovision_fail();
	}
}

static void prov_invite(const uint8_t *data)
{
	PROV_BUF(buf, PDU_LEN_CAPABILITIES);

	LOG_DBG("Attention Duration: %u seconds", data[0]);

	if (data[0]) {
		bt_mesh_attention(NULL, data[0]);
	}

	memcpy(bt_mesh_prov_link.conf_inputs.invite, data, PDU_LEN_INVITE);

	bt_mesh_prov_buf_init(&buf, PROV_CAPABILITIES);

	/* Number of Elements supported */
	net_buf_simple_add_u8(&buf, bt_mesh_elem_count());

	uint16_t algorithm_bm = 0;
	uint8_t oob_type = bt_mesh_prov->static_val ?
			BT_MESH_STATIC_OOB_AVAILABLE : 0;
	bool oob_availability = bt_mesh_prov->output_size > 0 ||
			bt_mesh_prov->input_size > 0 || bt_mesh_prov->static_val;

	if (IS_ENABLED(CONFIG_BT_MESH_ECDH_P256_HMAC_SHA256_AES_CCM)) {
		WRITE_BIT(algorithm_bm, BT_MESH_PROV_AUTH_HMAC_SHA256_AES_CCM, 1);
	}

	if (IS_ENABLED(CONFIG_BT_MESH_ECDH_P256_CMAC_AES128_AES_CCM)) {
		WRITE_BIT(algorithm_bm, BT_MESH_PROV_AUTH_CMAC_AES128_AES_CCM, 1);
	}

	if (oob_availability && IS_ENABLED(CONFIG_BT_MESH_OOB_AUTH_REQUIRED)) {
		oob_type |= BT_MESH_OOB_AUTH_REQUIRED;
		WRITE_BIT(algorithm_bm, BT_MESH_PROV_AUTH_CMAC_AES128_AES_CCM, 0);
	}

	/* Supported algorithms */
	net_buf_simple_add_be16(&buf, algorithm_bm);

	/* Public Key Type */
	net_buf_simple_add_u8(&buf,
			      bt_mesh_prov->public_key_be == NULL ? PUB_KEY_NO_OOB : PUB_KEY_OOB);

	/* Static OOB Type */
	net_buf_simple_add_u8(&buf, oob_type);

	/* Output OOB Size */
	net_buf_simple_add_u8(&buf, bt_mesh_prov->output_size);

	/* Output OOB Action */
	net_buf_simple_add_be16(&buf, bt_mesh_prov->output_actions);

	/* Input OOB Size */
	net_buf_simple_add_u8(&buf, bt_mesh_prov->input_size);

	/* Input OOB Action */
	net_buf_simple_add_be16(&buf, bt_mesh_prov->input_actions);

	memcpy(bt_mesh_prov_link.conf_inputs.capabilities, &buf.data[1], PDU_LEN_CAPABILITIES);

	if (bt_mesh_prov_send(&buf, NULL)) {
		LOG_ERR("Failed to send capabilities");
		return;
	}

	bt_mesh_prov_link.expect = PROV_START;
}

static void prov_start(const uint8_t *data)
{
	LOG_DBG("Algorithm:   0x%02x", data[0]);
	LOG_DBG("Public Key:  0x%02x", data[1]);
	LOG_DBG("Auth Method: 0x%02x", data[2]);
	LOG_DBG("Auth Action: 0x%02x", data[3]);
	LOG_DBG("Auth Size:   0x%02x", data[4]);

	if (IS_ENABLED(CONFIG_BT_MESH_ECDH_P256_HMAC_SHA256_AES_CCM) &&
		data[0] == BT_MESH_PROV_AUTH_HMAC_SHA256_AES_CCM) {
		bt_mesh_prov_link.algorithm = data[0];
	} else if (IS_ENABLED(CONFIG_BT_MESH_ECDH_P256_CMAC_AES128_AES_CCM) &&
		data[0] == BT_MESH_PROV_AUTH_CMAC_AES128_AES_CCM) {
		bt_mesh_prov_link.algorithm = data[0];
	} else {
		LOG_ERR("Unknown algorithm 0x%02x", data[0]);
		prov_fail(PROV_ERR_NVAL_FMT);
		return;
	}

	uint8_t auth_size = bt_mesh_prov_auth_size_get();

	if (data[1] > PUB_KEY_OOB ||
	    (data[1] == PUB_KEY_OOB &&
	     (!IS_ENABLED(CONFIG_BT_MESH_PROV_OOB_PUBLIC_KEY) || !bt_mesh_prov->public_key_be))) {
		LOG_ERR("Invalid public key type: 0x%02x", data[1]);
		prov_fail(PROV_ERR_NVAL_FMT);
		return;
	}

	atomic_set_bit_to(bt_mesh_prov_link.flags, OOB_PUB_KEY, data[1] == PUB_KEY_OOB);

	memcpy(bt_mesh_prov_link.conf_inputs.start, data, PDU_LEN_START);

	bt_mesh_prov_link.expect = PROV_PUB_KEY;
	bt_mesh_prov_link.oob_method = data[2];
	bt_mesh_prov_link.oob_action = data[3];
	bt_mesh_prov_link.oob_size = data[4];

	if (IS_ENABLED(CONFIG_BT_MESH_OOB_AUTH_REQUIRED) &&
	    (bt_mesh_prov_link.oob_method == AUTH_METHOD_NO_OOB ||
	    bt_mesh_prov_link.algorithm == BT_MESH_PROV_AUTH_CMAC_AES128_AES_CCM)) {
		prov_fail(PROV_ERR_NVAL_FMT);
		return;
	}

	if (bt_mesh_prov_auth(false, data[2], data[3], data[4]) < 0) {
		LOG_ERR("Invalid authentication method: 0x%02x; "
			"action: 0x%02x; size: 0x%02x", data[2], data[3], data[4]);
		prov_fail(PROV_ERR_NVAL_FMT);
		return;
	}

	if (atomic_test_bit(bt_mesh_prov_link.flags, OOB_STATIC_KEY)) {
		/* Trim the Auth if it is longer than required length */
		memcpy(bt_mesh_prov_link.auth, bt_mesh_prov->static_val,
		       bt_mesh_prov->static_val_len > auth_size ? auth_size
								: bt_mesh_prov->static_val_len);

		/* Pad with zeros if the Auth is shorter the required length */
		if (bt_mesh_prov->static_val_len < auth_size) {
			memset(bt_mesh_prov_link.auth + bt_mesh_prov->static_val_len, 0,
			       auth_size - bt_mesh_prov->static_val_len);
		}
	}
}

static void send_confirm(void)
{
	PROV_BUF(cfm, PDU_LEN_CONFIRM);
	uint8_t auth_size = bt_mesh_prov_auth_size_get();
	uint8_t *inputs = (uint8_t *)&bt_mesh_prov_link.conf_inputs;
	uint8_t conf_key_input[64];

	LOG_DBG("ConfInputs[0]   %s", bt_hex(inputs, 32));
	LOG_DBG("ConfInputs[32]  %s", bt_hex(&inputs[32], 32));
	LOG_DBG("ConfInputs[64]  %s", bt_hex(&inputs[64], 32));
	LOG_DBG("ConfInputs[96]  %s", bt_hex(&inputs[96], 32));
	LOG_DBG("ConfInputs[128] %s", bt_hex(&inputs[128], 17));

	if (bt_mesh_prov_conf_salt(bt_mesh_prov_link.algorithm, inputs,
				   bt_mesh_prov_link.conf_salt)) {
		LOG_ERR("Unable to generate confirmation salt");
		prov_fail(PROV_ERR_UNEXP_ERR);
		return;
	}

	LOG_DBG("ConfirmationSalt: %s", bt_hex(bt_mesh_prov_link.conf_salt, auth_size));

	memcpy(conf_key_input, bt_mesh_prov_link.dhkey, 32);

	if (IS_ENABLED(CONFIG_BT_MESH_ECDH_P256_HMAC_SHA256_AES_CCM) &&
		bt_mesh_prov_link.algorithm == BT_MESH_PROV_AUTH_HMAC_SHA256_AES_CCM) {
		memcpy(&conf_key_input[32], bt_mesh_prov_link.auth, 32);
		LOG_DBG("AuthValue  %s", bt_hex(bt_mesh_prov_link.auth, 32));
	}

	if (bt_mesh_prov_conf_key(bt_mesh_prov_link.algorithm, conf_key_input,
			bt_mesh_prov_link.conf_salt, bt_mesh_prov_link.conf_key)) {
		LOG_ERR("Unable to generate confirmation key");
		prov_fail(PROV_ERR_UNEXP_ERR);
		return;
	}

	LOG_DBG("ConfirmationKey: %s", bt_hex(bt_mesh_prov_link.conf_key, auth_size));

	if (bt_rand(bt_mesh_prov_link.rand, auth_size)) {
		LOG_ERR("Unable to generate random number");
		prov_fail(PROV_ERR_UNEXP_ERR);
		return;
	}

	LOG_DBG("LocalRandom: %s", bt_hex(bt_mesh_prov_link.rand, auth_size));

	bt_mesh_prov_buf_init(&cfm, PROV_CONFIRM);

	if (bt_mesh_prov_conf(bt_mesh_prov_link.algorithm, bt_mesh_prov_link.conf_key,
			bt_mesh_prov_link.rand, bt_mesh_prov_link.auth,
			net_buf_simple_add(&cfm, auth_size))) {
		LOG_ERR("Unable to generate confirmation value");
		prov_fail(PROV_ERR_UNEXP_ERR);
		return;
	}

	if (bt_mesh_prov_send(&cfm, NULL)) {
		LOG_ERR("Failed to send Provisioning Confirm");
		return;
	}

	bt_mesh_prov_link.expect = PROV_RANDOM;

}

static void send_input_complete(void)
{
	PROV_BUF(buf, PDU_LEN_INPUT_COMPLETE);

	bt_mesh_prov_buf_init(&buf, PROV_INPUT_COMPLETE);
	if (bt_mesh_prov_send(&buf, NULL)) {
		LOG_ERR("Failed to send Provisioning Input Complete");
	}
	bt_mesh_prov_link.expect = PROV_CONFIRM;
}

static void public_key_sent(int err, void *cb_data)
{
	atomic_set_bit(bt_mesh_prov_link.flags, PUB_KEY_SENT);

	if (atomic_test_bit(bt_mesh_prov_link.flags, INPUT_COMPLETE)) {
		send_input_complete();
		return;
	}
}

static void start_auth(void)
{
	if (atomic_test_bit(bt_mesh_prov_link.flags, WAIT_NUMBER) ||
	    atomic_test_bit(bt_mesh_prov_link.flags, WAIT_STRING)) {
		bt_mesh_prov_link.expect = PROV_NO_PDU; /* Wait for input */
	} else {
		bt_mesh_prov_link.expect = PROV_CONFIRM;
	}
}

static void send_pub_key(void)
{
	PROV_BUF(buf, PDU_LEN_PUB_KEY);
	const uint8_t *key;

	key = bt_mesh_pub_key_get();
	if (!key) {
		LOG_ERR("No public key available");
		prov_fail(PROV_ERR_UNEXP_ERR);
		return;
	}

	bt_mesh_prov_buf_init(&buf, PROV_PUB_KEY);
	net_buf_simple_add_mem(&buf, key, PUB_KEY_SIZE);
	LOG_DBG("Local Public Key: %s", bt_hex(buf.data + 1, PUB_KEY_SIZE));

	/* PublicKeyDevice */
	memcpy(bt_mesh_prov_link.conf_inputs.pub_key_device, &buf.data[1], PDU_LEN_PUB_KEY);

	if (bt_mesh_prov_send(&buf, public_key_sent)) {
		LOG_ERR("Failed to send Public Key");
		return;
	}

	start_auth();
}

static void prov_dh_key_gen(void)
{
	const uint8_t *remote_pub_key;
	const uint8_t *remote_priv_key;

	remote_pub_key = bt_mesh_prov_link.conf_inputs.pub_key_provisioner;

	if (IS_ENABLED(CONFIG_BT_MESH_PROV_OOB_PUBLIC_KEY) &&
	    atomic_test_bit(bt_mesh_prov_link.flags, OOB_PUB_KEY)) {
		remote_priv_key = bt_mesh_prov->private_key_be;
	} else {
		remote_priv_key = NULL;
	}

	if (bt_mesh_dhkey_gen(remote_pub_key, remote_priv_key, bt_mesh_prov_link.dhkey)) {
		LOG_ERR("Failed to generate DHKey");
		prov_fail(PROV_ERR_UNEXP_ERR);
		return;
	}

	LOG_DBG("DHkey: %s", bt_hex(bt_mesh_prov_link.dhkey, DH_KEY_SIZE));

	if (atomic_test_bit(bt_mesh_prov_link.flags, OOB_PUB_KEY)) {
		start_auth();
	} else {
		send_pub_key();
	}
}

static void prov_dh_key_gen_handler(struct k_work *work)
{
	prov_dh_key_gen();
}

static K_WORK_DEFINE(dh_gen_work, prov_dh_key_gen_handler);

static void prov_pub_key(const uint8_t *data)
{
	LOG_DBG("Remote Public Key: %s", bt_hex(data, PUB_KEY_SIZE));

	/* PublicKeyProvisioner */
	memcpy(bt_mesh_prov_link.conf_inputs.pub_key_provisioner, data, PDU_LEN_PUB_KEY);

	if (IS_ENABLED(CONFIG_BT_MESH_PROV_OOB_PUBLIC_KEY) &&
	    atomic_test_bit(bt_mesh_prov_link.flags, OOB_PUB_KEY)) {
		if (!bt_mesh_prov->public_key_be || !bt_mesh_prov->private_key_be) {
			LOG_ERR("Public or private key is not ready");
			prov_fail(PROV_ERR_UNEXP_ERR);
			return;
		}

		if (!memcmp(bt_mesh_prov->public_key_be,
			    bt_mesh_prov_link.conf_inputs.pub_key_provisioner, PDU_LEN_PUB_KEY)) {
			LOG_ERR("Public keys are identical");
			prov_fail(PROV_ERR_NVAL_FMT);
			return;
		}

		/* No swap needed since user provides public key in big-endian */
		memcpy(bt_mesh_prov_link.conf_inputs.pub_key_device, bt_mesh_prov->public_key_be,
		       PDU_LEN_PUB_KEY);
	}

	k_work_submit(&dh_gen_work);
}

static void notify_input_complete(void)
{
	if (atomic_test_and_clear_bit(bt_mesh_prov_link.flags,
				      NOTIFY_INPUT_COMPLETE) &&
	    bt_mesh_prov->input_complete) {
		bt_mesh_prov->input_complete();
	}
}

static void send_random(void)
{
	PROV_BUF(rnd, PDU_LEN_RANDOM);

	bt_mesh_prov_buf_init(&rnd, PROV_RANDOM);
	net_buf_simple_add_mem(&rnd, bt_mesh_prov_link.rand, bt_mesh_prov_auth_size_get());

	if (bt_mesh_prov_send(&rnd, NULL)) {
		LOG_ERR("Failed to send Provisioning Random");
		return;
	}

	bt_mesh_prov_link.expect = PROV_DATA;
}

static void prov_random(const uint8_t *data)
{
	uint8_t rand_size = bt_mesh_prov_auth_size_get();
	uint8_t conf_verify[PROV_AUTH_MAX_LEN];

	LOG_DBG("Remote Random: %s", bt_hex(data, rand_size));
	if (!memcmp(data, bt_mesh_prov_link.rand, rand_size)) {
		LOG_ERR("Random value is identical to ours, rejecting.");
		prov_fail(PROV_ERR_CFM_FAILED);
		return;
	}

	if (bt_mesh_prov_conf(bt_mesh_prov_link.algorithm, bt_mesh_prov_link.conf_key,
		data, bt_mesh_prov_link.auth, conf_verify)) {
		LOG_ERR("Unable to calculate confirmation verification");
		prov_fail(PROV_ERR_UNEXP_ERR);
		return;
	}

	if (memcmp(conf_verify, bt_mesh_prov_link.conf, rand_size)) {
		LOG_ERR("Invalid confirmation value");
		LOG_DBG("Received:   %s", bt_hex(bt_mesh_prov_link.conf, rand_size));
		LOG_DBG("Calculated: %s",  bt_hex(conf_verify, rand_size));
		prov_fail(PROV_ERR_CFM_FAILED);
		return;
	}

	if (bt_mesh_prov_salt(bt_mesh_prov_link.algorithm, bt_mesh_prov_link.conf_salt,
		data, bt_mesh_prov_link.rand, bt_mesh_prov_link.prov_salt)) {
		LOG_ERR("Failed to generate provisioning salt");
		prov_fail(PROV_ERR_UNEXP_ERR);
		return;
	}

	LOG_DBG("ProvisioningSalt: %s", bt_hex(bt_mesh_prov_link.prov_salt, 16));

	send_random();
}

static void prov_confirm(const uint8_t *data)
{
	uint8_t conf_size = bt_mesh_prov_auth_size_get();

	LOG_DBG("Remote Confirm: %s", bt_hex(data, conf_size));

	memcpy(bt_mesh_prov_link.conf, data, conf_size);
	notify_input_complete();

	send_confirm();
}

static inline bool is_pb_gatt(void)
{
	return bt_mesh_prov_link.bearer &&
	       bt_mesh_prov_link.bearer->type == BT_MESH_PROV_GATT;
}

static bool refresh_is_valid(const uint8_t *netkey, uint16_t net_idx,
			     uint32_t iv_index)
{
	enum bt_mesh_rpr_node_refresh proc = bt_mesh_node_refresh_get();
	struct bt_mesh_subnet *sub = bt_mesh_subnet_get(net_idx);
	uint16_t old_addr = bt_mesh_primary_addr();
	bool valid_addr;

	if (iv_index != bt_mesh.iv_index) {
		LOG_ERR("Invalid IV index");
		return false;
	}

	if (!sub || bt_mesh_key_compare(netkey, &sub->keys[SUBNET_KEY_TX_IDX(sub)].net)) {
		LOG_ERR("Invalid netkey");
		return false;
	}

	if (proc == BT_MESH_RPR_NODE_REFRESH_ADDR) {
		valid_addr = bt_mesh_prov_link.addr < old_addr ||
		       bt_mesh_prov_link.addr >= old_addr + bt_mesh_comp_get()->elem_count;
	} else {
		valid_addr = bt_mesh_prov_link.addr == bt_mesh_primary_addr();
	}

	if (!valid_addr) {
		LOG_ERR("Invalid address");
	}

	return valid_addr;
}

static void prov_data(const uint8_t *data)
{
	PROV_BUF(msg, PDU_LEN_COMPLETE);
	struct bt_mesh_key session_key;
	uint8_t nonce[13];
	uint8_t dev_key[16];
	uint8_t pdu[25];
	uint8_t flags;
	uint32_t iv_index;
	uint16_t net_idx;
	int err;
	bool identity_enable;

	LOG_DBG("");

	err = bt_mesh_session_key(bt_mesh_prov_link.dhkey,
				  bt_mesh_prov_link.prov_salt, &session_key);
	if (err) {
		LOG_ERR("Unable to generate session key");
		prov_fail(PROV_ERR_UNEXP_ERR);
		return;
	}

	err = bt_mesh_prov_nonce(bt_mesh_prov_link.dhkey,
				 bt_mesh_prov_link.prov_salt, nonce);
	if (err) {
		LOG_ERR("Unable to generate session nonce");
		prov_fail(PROV_ERR_UNEXP_ERR);
		goto session_key_destructor;
	}

	LOG_DBG("Nonce: %s", bt_hex(nonce, 13));

	err = bt_mesh_prov_decrypt(&session_key, nonce, data, pdu);
	if (err) {
		LOG_ERR("Unable to decrypt provisioning data");
		prov_fail(PROV_ERR_DECRYPT);
		goto session_key_destructor;
	}

	err = bt_mesh_dev_key(bt_mesh_prov_link.dhkey,
			      bt_mesh_prov_link.prov_salt, dev_key);
	if (err) {
		LOG_ERR("Unable to generate device key");
		prov_fail(PROV_ERR_UNEXP_ERR);
		goto session_key_destructor;
	}

	net_idx = sys_get_be16(&pdu[16]);
	flags = pdu[18];
	iv_index = sys_get_be32(&pdu[19]);
	bt_mesh_prov_link.addr = sys_get_be16(&pdu[23]);

	if (IS_ENABLED(CONFIG_BT_MESH_RPR_SRV) &&
	    atomic_test_bit(bt_mesh_prov_link.flags, REPROVISION) &&
	    !refresh_is_valid(pdu, net_idx, iv_index)) {
		prov_send_fail_msg(PROV_ERR_INVALID_DATA);
		goto session_key_destructor;
	}

	LOG_DBG("net_idx %u iv_index 0x%08x, addr 0x%04x",
		net_idx, iv_index, bt_mesh_prov_link.addr);

	bt_mesh_prov_buf_init(&msg, PROV_COMPLETE);
	if (bt_mesh_prov_send(&msg, NULL)) {
		LOG_ERR("Failed to send Provisioning Complete");
		goto session_key_destructor;
	}

	/* Ignore any further PDUs on this link */
	bt_mesh_prov_link.expect = PROV_NO_PDU;
	atomic_set_bit(bt_mesh_prov_link.flags, COMPLETE);

	if (IS_ENABLED(CONFIG_BT_MESH_RPR_SRV) &&
	    atomic_test_bit(bt_mesh_prov_link.flags, REPROVISION)) {
		bt_mesh_dev_key_cand(dev_key);
		goto session_key_destructor;
	}

	/* Store info, since bt_mesh_provision() will end up clearing it */
	if (IS_ENABLED(CONFIG_BT_MESH_GATT_PROXY)) {
		identity_enable = is_pb_gatt();
	} else {
		identity_enable = false;
	}

	err = bt_mesh_provision(pdu, net_idx, flags, iv_index, bt_mesh_prov_link.addr, dev_key);
	if (err) {
		LOG_ERR("Failed to provision (err %d)", err);
		goto session_key_destructor;
	}

	/* After PB-GATT provisioning we should start advertising
	 * using Node Identity.
	 */
	if (IS_ENABLED(CONFIG_BT_MESH_GATT_PROXY) && identity_enable) {
		bt_mesh_proxy_identity_enable();
	}

session_key_destructor:
	bt_mesh_key_destroy(&session_key);
}

static void reprovision_complete(void)
{
	bt_mesh_reprovision(bt_mesh_prov_link.addr);

	/* When performing the refresh composition procedure,
	 * the device key will be activated after the first
	 * successful decryption with the new key.
	 */
	if (bt_mesh_node_refresh_get() == BT_MESH_RPR_NODE_REFRESH_ADDR) {
		bt_mesh_dev_key_cand_activate();
	}

	if (bt_mesh_prov->reprovisioned) {
		bt_mesh_prov->reprovisioned(bt_mesh_primary_addr());
	}
}

static void reprovision_fail(void)
{
	bt_mesh_dev_key_cand_remove();
}

static void local_input_complete(void)
{
	if (atomic_test_bit(bt_mesh_prov_link.flags, PUB_KEY_SENT) ||
	    atomic_test_bit(bt_mesh_prov_link.flags, OOB_PUB_KEY)) {
		send_input_complete();
	} else {
		atomic_set_bit(bt_mesh_prov_link.flags, INPUT_COMPLETE);
	}
}

static void prov_link_closed(enum prov_bearer_link_status status)
{
	if (IS_ENABLED(CONFIG_BT_MESH_RPR_SRV) &&
	    atomic_test_bit(bt_mesh_prov_link.flags, REPROVISION)) {
		if (atomic_test_bit(bt_mesh_prov_link.flags, COMPLETE) &&
		    status == PROV_BEARER_LINK_STATUS_SUCCESS) {
			reprovision_complete();
		} else {
			reprovision_fail();
		}
	} else if (bt_mesh_prov_link.conf_inputs.invite[0]) {
		/* Disable Attention Timer if it was set */
		bt_mesh_attention(NULL, 0);
	}

	bt_mesh_prov_reset_state();
}

static void prov_link_opened(void)
{
	bt_mesh_prov_link.expect = PROV_INVITE;
	if (IS_ENABLED(CONFIG_BT_MESH_RPR_SRV) && bt_mesh_is_provisioned()) {
		atomic_set_bit(bt_mesh_prov_link.flags, REPROVISION);
	}
}

static const struct bt_mesh_prov_role role_device = {
	.input_complete = local_input_complete,
	.link_opened = prov_link_opened,
	.link_closed = prov_link_closed,
	.error = prov_fail,
	.op = {
		[PROV_INVITE] = prov_invite,
		[PROV_START] = prov_start,
		[PROV_PUB_KEY] = prov_pub_key,
		[PROV_CONFIRM] = prov_confirm,
		[PROV_RANDOM] = prov_random,
		[PROV_DATA] = prov_data,
	},
};

int bt_mesh_prov_enable(bt_mesh_prov_bearer_t bearers)
{
	if (IS_ENABLED(CONFIG_BT_MESH_RPR_SRV) &&
	    (bearers & BT_MESH_PROV_REMOTE)) {
		pb_remote_srv.link_accept(bt_mesh_prov_bearer_cb_get(), NULL);

		/* Only PB-Remote supports reprovisioning */
		if (bt_mesh_is_provisioned()) {
			goto role_init;
		}
	} else if (bt_mesh_is_provisioned()) {
		return -EALREADY;
	}

#if defined(CONFIG_BT_MESH_PROVISIONEE_LOG_LEVEL)
	if (CONFIG_BT_MESH_PROVISIONEE_LOG_LEVEL > 2) {
		struct bt_uuid_128 uuid = { .uuid = { BT_UUID_TYPE_128 } };

		sys_memcpy_swap(uuid.val, bt_mesh_prov->uuid, 16);
		LOG_INF("Device UUID: %s", bt_uuid_str(&uuid.uuid));
	}
#endif

	if (IS_ENABLED(CONFIG_BT_MESH_PB_ADV) &&
	    (bearers & BT_MESH_PROV_ADV)) {
		bt_mesh_pb_adv.link_accept(bt_mesh_prov_bearer_cb_get(), NULL);
	}

	if (IS_ENABLED(CONFIG_BT_MESH_PB_GATT) &&
	    (bearers & BT_MESH_PROV_GATT)) {
		bt_mesh_pb_gatt.link_accept(bt_mesh_prov_bearer_cb_get(), NULL);
	}

role_init:
	bt_mesh_prov_link.role = &role_device;

	return 0;
}

int bt_mesh_prov_disable(bt_mesh_prov_bearer_t bearers)
{
	if (bt_mesh_is_provisioned()) {
		return -EALREADY;
	}

	if (bt_mesh_prov_active()) {
		return -EBUSY;
	}

	if (IS_ENABLED(CONFIG_BT_MESH_PB_ADV) &&
	    (bearers & BT_MESH_PROV_ADV)) {
		bt_mesh_beacon_disable();
		bt_mesh_scan_disable();
	}

	if (IS_ENABLED(CONFIG_BT_MESH_PB_GATT) &&
	    (bearers & BT_MESH_PROV_GATT)) {
		(void)bt_mesh_pb_gatt_srv_disable();
	}

	return 0;
}
