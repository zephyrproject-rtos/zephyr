/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2020 Lingao Meng
 * Copyright (c) 2021 Manulytica Limited
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
#include "prov.h"
#include "settings.h"
#include "provisioner.h"

/* Timeout for receiving the link open response */
#define LINK_ESTABLISHMENT_TIMEOUT 60

#define LOG_LEVEL CONFIG_BT_MESH_PROVISIONER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mesh_provisioner);

static struct {
	struct bt_mesh_cdb_node *node;
	uint16_t net_idx;
	uint8_t elem_count;
	uint8_t attention_duration;
	uint8_t uuid[16];
	uint8_t new_dev_key[16];
} provisionee;

static void send_pub_key(void);
static void prov_dh_key_gen(void);

static int reset_state(void)
{
	if (!atomic_test_bit(bt_mesh_prov_link.flags, REPROVISION) &&
	    provisionee.node != NULL) {
		bt_mesh_cdb_node_del(provisionee.node, false);
	}

	return bt_mesh_prov_reset_state();
}

static void prov_link_close(enum prov_bearer_link_status status)
{
	LOG_DBG("%u", status);
	bt_mesh_prov_link.expect = PROV_NO_PDU;

	bt_mesh_prov_link.bearer->link_close(status);
}

static void prov_fail(uint8_t reason)
{
	/* According to MshPRTv1.1: 5.4.4, the
	 * provisioner just closes the link when something fails, while the
	 * provisionee sends the fail message, and waits for the provisioner to
	 * close the link.
	 */
	prov_link_close(PROV_BEARER_LINK_STATUS_FAIL);
}

static void send_invite(void)
{
	PROV_BUF(inv, PDU_LEN_INVITE);

	LOG_DBG("");

	bt_mesh_prov_buf_init(&inv, PROV_INVITE);
	net_buf_simple_add_u8(&inv, provisionee.attention_duration);

	memcpy(bt_mesh_prov_link.conf_inputs.invite, &provisionee.attention_duration,
	       PDU_LEN_INVITE);

	if (bt_mesh_prov_send(&inv, NULL)) {
		LOG_ERR("Failed to send invite");
		return;
	}

	bt_mesh_prov_link.expect = PROV_CAPABILITIES;
}

static void start_sent(int err, void *cb_data)
{
	send_pub_key();
}

static void send_start(void)
{
	LOG_DBG("");

	PROV_BUF(start, PDU_LEN_START);

	bool oob_pub_key = bt_mesh_prov_link.conf_inputs.capabilities[3] == PUB_KEY_OOB;

	bt_mesh_prov_buf_init(&start, PROV_START);
	net_buf_simple_add_u8(&start, bt_mesh_prov_link.algorithm);

	if (atomic_test_bit(bt_mesh_prov_link.flags, REMOTE_PUB_KEY) && oob_pub_key) {
		net_buf_simple_add_u8(&start, PUB_KEY_OOB);
		atomic_set_bit(bt_mesh_prov_link.flags, OOB_PUB_KEY);
	} else {
		net_buf_simple_add_u8(&start, PUB_KEY_NO_OOB);
	}

	net_buf_simple_add_u8(&start, bt_mesh_prov_link.oob_method);

	net_buf_simple_add_u8(&start, bt_mesh_prov_link.oob_action);

	net_buf_simple_add_u8(&start, bt_mesh_prov_link.oob_size);

	memcpy(bt_mesh_prov_link.conf_inputs.start, &start.data[1], PDU_LEN_START);

	if (bt_mesh_prov_auth(true, bt_mesh_prov_link.oob_method,
		bt_mesh_prov_link.oob_action, bt_mesh_prov_link.oob_size) < 0) {
		LOG_ERR("Invalid authentication method: 0x%02x; "
		       "action: 0x%02x; size: 0x%02x", bt_mesh_prov_link.oob_method,
		       bt_mesh_prov_link.oob_action, bt_mesh_prov_link.oob_size);
		return;
	}

	if (bt_mesh_prov_send(&start, start_sent)) {
		LOG_ERR("Failed to send Provisioning Start");
		return;
	}
}

static bool prov_check_method(struct bt_mesh_dev_capabilities *caps)
{
	if (bt_mesh_prov_link.oob_method == AUTH_METHOD_STATIC) {
		if (!caps->oob_type) {
			LOG_WRN("Device not support OOB static authentication provisioning");
			return false;
		}
	} else if (bt_mesh_prov_link.oob_method == AUTH_METHOD_INPUT) {
		if (bt_mesh_prov_link.oob_size > caps->input_size) {
			LOG_WRN("The required input length (0x%02x) "
				"exceeds the device capacity (0x%02x)",
				bt_mesh_prov_link.oob_size, caps->input_size);
			return false;
		}

		if (!(BIT(bt_mesh_prov_link.oob_action) & caps->input_actions)) {
			LOG_WRN("The required input action (0x%04x) "
				"not supported by the device (0x%02x)",
				(uint16_t)BIT(bt_mesh_prov_link.oob_action), caps->input_actions);
			return false;
		}

		if (bt_mesh_prov_link.oob_action == INPUT_OOB_STRING) {
			if (!bt_mesh_prov->output_string) {
				LOG_WRN("Not support output string");
				return false;
			}
		} else {
			if (!bt_mesh_prov->output_number) {
				LOG_WRN("Not support output number");
				return false;
			}
		}
	} else if (bt_mesh_prov_link.oob_method == AUTH_METHOD_OUTPUT) {
		if (bt_mesh_prov_link.oob_size > caps->output_size) {
			LOG_WRN("The required output length (0x%02x) "
				"exceeds the device capacity (0x%02x)",
				bt_mesh_prov_link.oob_size, caps->output_size);
			return false;
		}

		if (!(BIT(bt_mesh_prov_link.oob_action) & caps->output_actions)) {
			LOG_WRN("The required output action (0x%04x) "
				"not supported by the device (0x%02x)",
				(uint16_t)BIT(bt_mesh_prov_link.oob_action), caps->output_actions);
			return false;
		}

		if (!bt_mesh_prov->input) {
			LOG_WRN("Not support input");
			return false;
		}
	}

	return true;
}

static void prov_capabilities(const uint8_t *data)
{
	struct bt_mesh_dev_capabilities caps;

	caps.elem_count = data[0];
	LOG_DBG("Elements:          %u", caps.elem_count);

	caps.algorithms = sys_get_be16(&data[1]);
	LOG_DBG("Algorithms:        0x%02x", caps.algorithms);

	bool is_aes128 = false;
	bool is_sha256 = false;

	if ((caps.algorithms & BIT(BT_MESH_PROV_AUTH_CMAC_AES128_AES_CCM)) &&
		IS_ENABLED(CONFIG_BT_MESH_ECDH_P256_CMAC_AES128_AES_CCM)) {
		is_aes128 = true;
	}

	if ((caps.algorithms & BIT(BT_MESH_PROV_AUTH_HMAC_SHA256_AES_CCM)) &&
		IS_ENABLED(CONFIG_BT_MESH_ECDH_P256_HMAC_SHA256_AES_CCM)) {
		is_sha256 = true;
	}

	if (!(is_sha256 || is_aes128)) {
		LOG_ERR("Invalid encryption algorithm");
		prov_fail(PROV_ERR_NVAL_FMT);
		return;
	}

	caps.pub_key_type = data[3];
	caps.oob_type = data[4];
	caps.output_size = data[5];
	LOG_DBG("Public Key Type:   0x%02x", caps.pub_key_type);
	LOG_DBG("Static OOB Type:   0x%02x", caps.oob_type);
	LOG_DBG("Output OOB Size:   %u", caps.output_size);

	caps.output_actions = (bt_mesh_output_action_t)
					(sys_get_be16(&data[6]));
	caps.input_size = data[8];
	caps.input_actions = (bt_mesh_input_action_t)
					(sys_get_be16(&data[9]));
	LOG_DBG("Output OOB Action: 0x%04x", caps.output_actions);
	LOG_DBG("Input OOB Size:    %u", caps.input_size);
	LOG_DBG("Input OOB Action:  0x%04x", caps.input_actions);

	provisionee.elem_count = caps.elem_count;
	if (provisionee.elem_count == 0) {
		LOG_ERR("Invalid number of elements");
		prov_fail(PROV_ERR_NVAL_FMT);
		return;
	}

	if (caps.oob_type & BT_MESH_OOB_AUTH_REQUIRED) {

		bool oob_availability = caps.output_size > 0 || caps.input_size > 0 ||
			(caps.oob_type & BT_MESH_STATIC_OOB_AVAILABLE);

		if (!oob_availability && !is_sha256) {
			LOG_ERR("Invalid capabilities for OOB authentication");
			prov_fail(PROV_ERR_NVAL_FMT);
			return;
		}
	}

	bt_mesh_prov_link.algorithm = is_sha256 ? BT_MESH_PROV_AUTH_HMAC_SHA256_AES_CCM :
			BT_MESH_PROV_AUTH_CMAC_AES128_AES_CCM;

	if (atomic_test_bit(bt_mesh_prov_link.flags, REPROVISION)) {
		if (!bt_mesh_prov_link.addr) {
			bt_mesh_prov_link.addr = bt_mesh_cdb_free_addr_get(
				provisionee.elem_count);
			if (!bt_mesh_prov_link.addr) {
				LOG_ERR("Failed allocating address for node");
				prov_fail(PROV_ERR_ADDR);
				return;
			}
		}
	} else {
		provisionee.node =
			bt_mesh_cdb_node_alloc(provisionee.uuid,
					       bt_mesh_prov_link.addr,
					       provisionee.elem_count,
					       provisionee.net_idx);
		if (provisionee.node == NULL) {
			LOG_ERR("Failed allocating node 0x%04x", bt_mesh_prov_link.addr);
			prov_fail(PROV_ERR_RESOURCES);
			return;
		}

		/* Address might change in the alloc call */
		bt_mesh_prov_link.addr = provisionee.node->addr;
	}

	memcpy(bt_mesh_prov_link.conf_inputs.capabilities, data, PDU_LEN_CAPABILITIES);

	if (bt_mesh_prov->capabilities) {
		bt_mesh_prov->capabilities(&caps);
	}

	if (!prov_check_method(&caps)) {
		prov_fail(PROV_ERR_UNEXP_ERR);
		return;
	}

	send_start();
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

	if (bt_mesh_prov_conf_salt(bt_mesh_prov_link.algorithm,
				   inputs,
				   bt_mesh_prov_link.conf_salt)) {
		LOG_ERR("Unable to generate confirmation salt");
		prov_fail(PROV_ERR_UNEXP_ERR);
		return;
	}

	LOG_DBG("ConfirmationSalt: %s", bt_hex(bt_mesh_prov_link.conf_salt, auth_size));

	memcpy(conf_key_input, bt_mesh_prov_link.dhkey, 32);

	if (bt_mesh_prov_link.algorithm == BT_MESH_PROV_AUTH_HMAC_SHA256_AES_CCM &&
	    IS_ENABLED(CONFIG_BT_MESH_ECDH_P256_HMAC_SHA256_AES_CCM)) {
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
			      bt_mesh_prov_link.conf)) {
		LOG_ERR("Unable to generate confirmation value");
		prov_fail(PROV_ERR_UNEXP_ERR);
		return;
	}

	net_buf_simple_add_mem(&cfm, bt_mesh_prov_link.conf, auth_size);

	if (bt_mesh_prov_send(&cfm, NULL)) {
		LOG_ERR("Failed to send Provisioning Confirm");
		return;
	}

	bt_mesh_prov_link.expect = PROV_CONFIRM;
}

static void public_key_sent(int err, void *cb_data)
{
	atomic_set_bit(bt_mesh_prov_link.flags, PUB_KEY_SENT);

	if (atomic_test_bit(bt_mesh_prov_link.flags, OOB_PUB_KEY) &&
	    atomic_test_bit(bt_mesh_prov_link.flags, REMOTE_PUB_KEY)) {
		prov_dh_key_gen();
		return;
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

	/* PublicKeyProvisioner */
	memcpy(bt_mesh_prov_link.conf_inputs.pub_key_provisioner, &buf.data[1], PDU_LEN_PUB_KEY);

	if (bt_mesh_prov_send(&buf, public_key_sent)) {
		LOG_ERR("Failed to send Public Key");
		return;
	}

	bt_mesh_prov_link.expect = PROV_PUB_KEY;
}

static void prov_dh_key_gen(void)
{
	const uint8_t *remote_pk;
	const uint8_t *local_pk;

	local_pk = bt_mesh_prov_link.conf_inputs.pub_key_provisioner;
	remote_pk = bt_mesh_prov_link.conf_inputs.pub_key_device;

	if (!memcmp(local_pk, remote_pk, PUB_KEY_SIZE)) {
		LOG_ERR("Public keys are identical");
		prov_fail(PROV_ERR_NVAL_FMT);
		return;
	}

	if (bt_mesh_dhkey_gen(remote_pk, NULL, bt_mesh_prov_link.dhkey)) {
		LOG_ERR("Failed to generate DHKey");
		prov_fail(PROV_ERR_UNEXP_ERR);
	}

	LOG_DBG("DHkey: %s", bt_hex(bt_mesh_prov_link.dhkey, DH_KEY_SIZE));

	if (atomic_test_bit(bt_mesh_prov_link.flags, NOTIFY_INPUT_COMPLETE)) {
		bt_mesh_prov_link.expect = PROV_INPUT_COMPLETE;
	}

	if (atomic_test_bit(bt_mesh_prov_link.flags, WAIT_STRING) ||
	    atomic_test_bit(bt_mesh_prov_link.flags, WAIT_NUMBER) ||
	    atomic_test_bit(bt_mesh_prov_link.flags, NOTIFY_INPUT_COMPLETE)) {
		atomic_set_bit(bt_mesh_prov_link.flags, WAIT_CONFIRM);
		return;
	}

	send_confirm();
}

static void prov_dh_key_gen_handler(struct k_work *work)
{
	prov_dh_key_gen();
}

static K_WORK_DEFINE(dh_gen_work, prov_dh_key_gen_handler);

static void prov_pub_key(const uint8_t *data)
{
	LOG_DBG("Remote Public Key: %s", bt_hex(data, PUB_KEY_SIZE));

	atomic_set_bit(bt_mesh_prov_link.flags, REMOTE_PUB_KEY);

	/* PublicKeyDevice */
	memcpy(bt_mesh_prov_link.conf_inputs.pub_key_device, data, PUB_KEY_SIZE);
	bt_mesh_prov_link.bearer->clear_tx();

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

static void prov_input_complete(const uint8_t *data)
{
	LOG_DBG("");

	notify_input_complete();

	if (atomic_test_and_clear_bit(bt_mesh_prov_link.flags, WAIT_CONFIRM)) {
		send_confirm();
	}
}

static void send_prov_data(void)
{
	PROV_BUF(pdu, PDU_LEN_DATA);
	struct bt_mesh_cdb_subnet *sub;
	uint8_t net_key[16];
	struct bt_mesh_key session_key;
	uint8_t nonce[13];
	int err;

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

	err = bt_mesh_dev_key(bt_mesh_prov_link.dhkey,
			      bt_mesh_prov_link.prov_salt, provisionee.new_dev_key);
	if (err) {
		LOG_ERR("Unable to generate device key");
		prov_fail(PROV_ERR_UNEXP_ERR);
		goto session_key_destructor;
	}

	sub = bt_mesh_cdb_subnet_get(provisionee.node->net_idx);
	if (sub == NULL) {
		LOG_ERR("No subnet with net_idx %u", provisionee.node->net_idx);
		prov_fail(PROV_ERR_UNEXP_ERR);
		goto session_key_destructor;
	}

	err = bt_mesh_key_export(net_key, &sub->keys[SUBNET_KEY_TX_IDX(sub)].net_key);
	if (err) {
		LOG_ERR("Unable to export network key");
		prov_fail(PROV_ERR_UNEXP_ERR);
		goto session_key_destructor;
	}

	bt_mesh_prov_buf_init(&pdu, PROV_DATA);
	net_buf_simple_add_mem(&pdu, net_key, sizeof(net_key));
	net_buf_simple_add_be16(&pdu, provisionee.node->net_idx);
	net_buf_simple_add_u8(&pdu, bt_mesh_cdb_subnet_flags(sub));
	net_buf_simple_add_be32(&pdu, bt_mesh_cdb.iv_index);
	net_buf_simple_add_be16(&pdu, bt_mesh_prov_link.addr);
	net_buf_simple_add(&pdu, 8); /* For MIC */

	LOG_DBG("net_idx %u, iv_index 0x%08x, addr 0x%04x",
		provisionee.node->net_idx, bt_mesh.iv_index,
		bt_mesh_prov_link.addr);

	err = bt_mesh_prov_encrypt(&session_key, nonce, &pdu.data[1],
				   &pdu.data[1]);
	if (err) {
		LOG_ERR("Unable to encrypt provisioning data");
		prov_fail(PROV_ERR_DECRYPT);
		goto session_key_destructor;
	}

	if (bt_mesh_prov_send(&pdu, NULL)) {
		LOG_ERR("Failed to send Provisioning Data");
		goto session_key_destructor;
	}

	bt_mesh_prov_link.expect = PROV_COMPLETE;

session_key_destructor:
	bt_mesh_key_destroy(&session_key);
}

static void prov_complete(const uint8_t *data)
{
	struct bt_mesh_cdb_node *node = provisionee.node;

	LOG_DBG("key %s, net_idx %u, num_elem %u, addr 0x%04x",
		bt_hex(&provisionee.new_dev_key, 16), node->net_idx,
		node->num_elem, node->addr);

	bt_mesh_prov_link.expect = PROV_NO_PDU;
	atomic_set_bit(bt_mesh_prov_link.flags, COMPLETE);

	bt_mesh_prov_link.bearer->link_close(PROV_BEARER_LINK_STATUS_SUCCESS);
}

static void prov_node_add(void)
{
	LOG_DBG("");
	struct bt_mesh_cdb_node *node = provisionee.node;
	int err;

	if (atomic_test_bit(bt_mesh_prov_link.flags, REPROVISION)) {
		bt_mesh_cdb_node_update(node, bt_mesh_prov_link.addr,
					provisionee.elem_count);
	}

	err = bt_mesh_cdb_node_key_import(node, provisionee.new_dev_key);
	if (err) {
		LOG_ERR("Failed to import node device key");
		return;
	}

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		bt_mesh_cdb_node_store(node);
	}

	provisionee.node = NULL;

	if (bt_mesh_prov->node_added) {
		bt_mesh_prov->node_added(node->net_idx, node->uuid, node->addr,
					 node->num_elem);
	}
}

static void send_random(void)
{
	PROV_BUF(rnd, PDU_LEN_RANDOM);
	uint8_t rand_size = bt_mesh_prov_auth_size_get();

	bt_mesh_prov_buf_init(&rnd, PROV_RANDOM);
	net_buf_simple_add_mem(&rnd, bt_mesh_prov_link.rand, rand_size);

	if (bt_mesh_prov_send(&rnd, NULL)) {
		LOG_ERR("Failed to send Provisioning Random");
		return;
	}

	bt_mesh_prov_link.expect = PROV_RANDOM;
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
		bt_mesh_prov_link.rand, data, bt_mesh_prov_link.prov_salt)) {
		LOG_ERR("Failed to generate provisioning salt");
		prov_fail(PROV_ERR_UNEXP_ERR);
		return;
	}

	LOG_DBG("ProvisioningSalt: %s", bt_hex(bt_mesh_prov_link.prov_salt, 16));

	send_prov_data();
}

static void prov_confirm(const uint8_t *data)
{
	uint8_t conf_size = bt_mesh_prov_auth_size_get();

	LOG_DBG("Remote Confirm: %s", bt_hex(data, conf_size));

	if (!memcmp(data, bt_mesh_prov_link.conf, conf_size)) {
		LOG_ERR("Confirm value is identical to ours, rejecting.");
		prov_fail(PROV_ERR_CFM_FAILED);
		return;
	}

	memcpy(bt_mesh_prov_link.conf, data, conf_size);

	send_random();
}

static void prov_failed(const uint8_t *data)
{
	LOG_WRN("Error: 0x%02x", data[0]);
	reset_state();
}

static void local_input_complete(void)
{
	if (atomic_test_and_clear_bit(bt_mesh_prov_link.flags, WAIT_CONFIRM)) {
		send_confirm();
	}
}

static void prov_link_closed(enum prov_bearer_link_status status)
{
	LOG_DBG("");
	if (atomic_test_bit(bt_mesh_prov_link.flags, COMPLETE)) {
		prov_node_add();
	}

	reset_state();
}

static void prov_link_opened(void)
{
	send_invite();
}

static const struct bt_mesh_prov_role role_provisioner = {
	.input_complete = local_input_complete,
	.link_opened = prov_link_opened,
	.link_closed = prov_link_closed,
	.error = prov_fail,
	.op = {
		[PROV_CAPABILITIES] = prov_capabilities,
		[PROV_PUB_KEY] = prov_pub_key,
		[PROV_INPUT_COMPLETE] = prov_input_complete,
		[PROV_CONFIRM] = prov_confirm,
		[PROV_RANDOM] = prov_random,
		[PROV_COMPLETE] = prov_complete,
		[PROV_FAILED] = prov_failed,
	},
};

static void prov_set_method(uint8_t method, uint8_t action, uint8_t size)
{
	bt_mesh_prov_link.oob_method = method;
	bt_mesh_prov_link.oob_action = action;
	bt_mesh_prov_link.oob_size = size;
}

int bt_mesh_auth_method_set_input(bt_mesh_input_action_t action, uint8_t size)
{
	if (!action || !size || size > PROV_IO_OOB_SIZE_MAX) {
		return -EINVAL;
	}

	prov_set_method(AUTH_METHOD_INPUT, find_msb_set(action) - 1, size);
	return 0;
}

int bt_mesh_auth_method_set_output(bt_mesh_output_action_t action, uint8_t size)
{
	if (!action || !size || size > PROV_IO_OOB_SIZE_MAX) {
		return -EINVAL;
	}

	prov_set_method(AUTH_METHOD_OUTPUT, find_msb_set(action) - 1, size);
	return 0;
}

int bt_mesh_auth_method_set_static(const uint8_t *static_val, uint8_t size)
{
	if (!size || !static_val) {
		return -EINVAL;
	}

	prov_set_method(AUTH_METHOD_STATIC, 0, 0);

	/* Trim the Auth if it is longer than required length */
	memcpy(bt_mesh_prov_link.auth, static_val,
	       size > PROV_AUTH_MAX_LEN ? PROV_AUTH_MAX_LEN : size);

	/* Pad with zeros if the Auth is shorter the required length */
	if (size < PROV_AUTH_MAX_LEN) {
		memset(bt_mesh_prov_link.auth + size, 0, PROV_AUTH_MAX_LEN - size);
	}

	return 0;
}

int bt_mesh_auth_method_set_none(void)
{
	prov_set_method(AUTH_METHOD_NO_OOB, 0, 0);
	return 0;
}

int bt_mesh_prov_remote_pub_key_set(const uint8_t public_key[PUB_KEY_SIZE])
{
	if (public_key == NULL) {
		return -EINVAL;
	}

	if (atomic_test_and_set_bit(bt_mesh_prov_link.flags, REMOTE_PUB_KEY)) {
		return -EALREADY;
	}

	memcpy(bt_mesh_prov_link.conf_inputs.pub_key_device, public_key, PDU_LEN_PUB_KEY);

	return 0;
}

static int link_open(const uint8_t *uuid, const struct prov_bearer *bearer,
		     uint16_t net_idx, uint16_t addr,
		     uint8_t attention_duration, void *bearer_cb_data, uint8_t timeout)
{
	int err;

	if (atomic_test_and_set_bit(bt_mesh_prov_link.flags, LINK_ACTIVE)) {
		return -EBUSY;
	}

	if (uuid) {
		memcpy(provisionee.uuid, uuid, 16);

		struct bt_uuid_128 uuid_repr = { .uuid = { BT_UUID_TYPE_128 } };

		memcpy(uuid_repr.val, uuid, 16);
		LOG_DBG("Provisioning %s", bt_uuid_str(&uuid_repr.uuid));

	} else {
		atomic_set_bit(bt_mesh_prov_link.flags, REPROVISION);
		LOG_DBG("Reprovisioning");
	}

	atomic_set_bit(bt_mesh_prov_link.flags, PROVISIONER);
	bt_mesh_prov_link.addr = addr;
	bt_mesh_prov_link.bearer = bearer;
	bt_mesh_prov_link.role = &role_provisioner;
	provisionee.net_idx = net_idx;
	provisionee.attention_duration = attention_duration;

	err = bt_mesh_prov_link.bearer->link_open(
		uuid, timeout, bt_mesh_prov_bearer_cb_get(), bearer_cb_data);
	if (err) {
		atomic_clear_bit(bt_mesh_prov_link.flags, LINK_ACTIVE);
	}

	return err;
}

#if defined(CONFIG_BT_MESH_PB_ADV)
int bt_mesh_pb_adv_open(const uint8_t uuid[16], uint16_t net_idx, uint16_t addr,
			uint8_t attention_duration)
{
	return link_open(uuid, &bt_mesh_pb_adv, net_idx, addr, attention_duration, NULL,
			 LINK_ESTABLISHMENT_TIMEOUT);
}
#endif

#if defined(CONFIG_BT_MESH_PB_GATT_CLIENT)
int bt_mesh_pb_gatt_open(const uint8_t uuid[16], uint16_t net_idx, uint16_t addr,
			 uint8_t attention_duration)
{
	return link_open(uuid, &bt_mesh_pb_gatt, net_idx, addr, attention_duration, NULL,
			 LINK_ESTABLISHMENT_TIMEOUT);
}
#endif

#if defined(CONFIG_BT_MESH_RPR_CLI)
int bt_mesh_pb_remote_open(struct bt_mesh_rpr_cli *cli,
			   const struct bt_mesh_rpr_node *srv, const uint8_t uuid[16],
			   uint16_t net_idx, uint16_t addr)
{
	struct pb_remote_ctx ctx = { cli, srv };

	return link_open(uuid, &pb_remote_cli, net_idx, addr, 0, &ctx, 0);
}

/* Remote Provision done where client and server is on same node, skip open link
 * and sending of reprovision message, just execute reprovisioning on it self.
 */
static int reprovision_local_client_server(uint16_t addr)
{
	int err;
	const uint8_t *pub_key;
	const uint8_t *priv_key = NULL;

	if (atomic_test_and_set_bit(bt_mesh_prov_link.flags, LINK_ACTIVE)) {
		return -EBUSY;
	}

	LOG_DBG("net_idx %u iv_index 0x%08x, addr 0x%04x",
		provisionee.node->net_idx, bt_mesh_cdb.iv_index, addr);

	atomic_set_bit(bt_mesh_prov_link.flags, REPROVISION);
	atomic_set_bit(bt_mesh_prov_link.flags, PROVISIONER);
	bt_mesh_prov_link.addr = addr;
	bt_mesh_prov_link.bearer = &pb_remote_cli;
	bt_mesh_prov_link.role = &role_provisioner;
	provisionee.net_idx = provisionee.node->net_idx;
	provisionee.attention_duration = 0;

	if (IS_ENABLED(CONFIG_BT_MESH_PROV_OOB_PUBLIC_KEY) &&
		       bt_mesh_prov->public_key_be && bt_mesh_prov->private_key_be) {
		LOG_DBG("Use OOB Public and Private key");
		pub_key = bt_mesh_prov->public_key_be;
		priv_key = bt_mesh_prov->private_key_be;
	} else {
		pub_key = bt_mesh_pub_key_get();
	}

	if (!pub_key) {
		LOG_ERR("No public key available");
		return -ENOEXEC;
	}

	if (bt_mesh_dhkey_gen(pub_key, priv_key, bt_mesh_prov_link.dhkey)) {
		LOG_ERR("Failed to generate DHKey");
		return -ENOEXEC;
	}
	LOG_DBG("DHkey: %s", bt_hex(bt_mesh_prov_link.dhkey, DH_KEY_SIZE));

	err = bt_mesh_dev_key(bt_mesh_prov_link.dhkey,
			      bt_mesh_prov_link.prov_salt, provisionee.new_dev_key);
	if (err) {
		LOG_ERR("Unable to generate device key");
		return err;
	}

	bt_mesh_dev_key_cand(provisionee.new_dev_key);
	/* Mark the link that was never opened as closed. */
	atomic_set_bit(bt_mesh_prov_link.flags, COMPLETE);
	bt_mesh_reprovision(addr);
	bt_mesh_dev_key_cand_activate();

	if (bt_mesh_prov->reprovisioned) {
		LOG_DBG("Application reprovisioned callback 0x%04x", bt_mesh_primary_addr());
		bt_mesh_prov->reprovisioned(bt_mesh_primary_addr());
	}

	prov_link_closed(PROV_BEARER_LINK_STATUS_SUCCESS);
	return 0;
}

int bt_mesh_pb_remote_open_node(struct bt_mesh_rpr_cli *cli,
				struct bt_mesh_rpr_node *srv, uint16_t addr,
				bool composition_change)
{
	struct pb_remote_ctx ctx = { cli, srv };

	if (srv->addr != addr) {
		ctx.refresh = BT_MESH_RPR_NODE_REFRESH_ADDR;
	} else if (composition_change) {
		ctx.refresh = BT_MESH_RPR_NODE_REFRESH_COMPOSITION;
	} else {
		ctx.refresh = BT_MESH_RPR_NODE_REFRESH_DEVKEY;
	}

	provisionee.node = bt_mesh_cdb_node_get(srv->addr);
	if (!provisionee.node) {
		LOG_ERR("No CDB node for 0x%04x", srv->addr);
		return -ENOENT;
	}

	/* Check if server is on same device as client */
	if (IS_ENABLED(CONFIG_BT_MESH_RPR_SRV) && bt_mesh_has_addr(srv->addr)) {
		return reprovision_local_client_server(addr);
	}

	return link_open(NULL, &pb_remote_cli, provisionee.node->net_idx, addr,
			 0, &ctx, 0);
}
#endif
