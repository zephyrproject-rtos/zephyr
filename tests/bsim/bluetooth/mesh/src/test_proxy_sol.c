/*
 * Copyright (c) 2024 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Proxy Solicitation test
 */

#include "mesh_test.h"
#include "mesh/access.h"
#include "mesh/settings.h"
#include "mesh/net.h"
#include "mesh/crypto.h"
#include "mesh/proxy.h"
#include <string.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/uuid.h>

LOG_MODULE_REGISTER(test_proxy_sol, LOG_LEVEL_INF);

#define WAIT_TIME                  60 /*seconds*/
#define SEM_TIMEOUT                6  /*seconds*/
#define BEACON_TYPE_NET_ID         0
#define BEACON_TYPE_PRIVATE_NET_ID 2
#define BEACON_NET_ID_LEN          20
#define OTHER_ADV_TYPES_LEN        28

static struct bt_mesh_prov prov;
static struct bt_mesh_cfg_cli cfg_cli;
static struct bt_mesh_priv_beacon_cli priv_beacon_cli;
static struct bt_mesh_od_priv_proxy_cli od_priv_proxy_cli;
static struct k_sem beacon_sem;

static const struct bt_mesh_test_cfg tester_cfg = {
	.addr = 0x0001,
	.dev_key = {0x01},
};
static const struct bt_mesh_test_cfg iut_cfg = {
	.addr = 0x0002,
	.dev_key = {0x02},
};

static struct bt_mesh_model models[] = {
	BT_MESH_MODEL_CFG_SRV,
	BT_MESH_MODEL_CFG_CLI(&cfg_cli),
	BT_MESH_MODEL_PRIV_BEACON_SRV,
	BT_MESH_MODEL_PRIV_BEACON_CLI(&priv_beacon_cli),
	BT_MESH_MODEL_OD_PRIV_PROXY_SRV,
	BT_MESH_MODEL_OD_PRIV_PROXY_CLI(&od_priv_proxy_cli)
};

static struct bt_mesh_elem elems[] = {
	BT_MESH_ELEM(0, models, BT_MESH_MODEL_NONE),
};

static const struct bt_mesh_comp comp = {
	.cid = TEST_VND_COMPANY_ID,
	.vid = 0xaaaa,
	.pid = 0xbbbb,
	.elem = elems,
	.elem_count = ARRAY_SIZE(elems),
};

static bool is_tester_address(void)
{
	return bt_mesh_primary_addr() == tester_cfg.addr;
}

static uint8_t proxy_adv_type_get(uint8_t adv_type, struct net_buf_simple *buf)
{
	uint8_t type;
	uint8_t len = buf->len;

	if (adv_type != BT_GAP_ADV_TYPE_ADV_IND || len < 12) {
		return 0xFF;
	}

	(void)net_buf_simple_pull_mem(buf, 11);
	type = net_buf_simple_pull_u8(buf);

	if (len != ((type == BEACON_TYPE_NET_ID) ? BEACON_NET_ID_LEN : OTHER_ADV_TYPES_LEN)) {
		return 0xFF;
	}

	return type;
}

static void scan_recv(const struct bt_le_scan_recv_info *info, struct net_buf_simple *ad)
{
	uint8_t type = proxy_adv_type_get(info->adv_type, ad);

	if (is_tester_address() && type == BEACON_TYPE_PRIVATE_NET_ID) {
		k_sem_give(&beacon_sem);
	}
}

static struct bt_le_scan_cb scan_cb = {
	.recv = scan_recv,
};

static void tester_configure(void)
{
	uint8_t err, status;

	k_sem_init(&beacon_sem, 0, 1);
	bt_le_scan_cb_register(&scan_cb);

	bt_mesh_test_cfg_set(NULL, WAIT_TIME);
	bt_mesh_device_setup(&prov, &comp);

	ASSERT_OK_MSG(bt_mesh_provision(test_net_key, 0, 0, 0, tester_cfg.addr, tester_cfg.dev_key),
		      "Node failed to provision");

	err = bt_mesh_cfg_cli_app_key_add(0, tester_cfg.addr, 0, 0, test_app_key, &status);
	if (err || status) {
		FAIL("AppKey add failed (err %d, status %u)", err, status);
	}
}

static void iut_configure(void)
{
	uint8_t err, status;

	bt_mesh_test_cfg_set(NULL, WAIT_TIME);
	bt_mesh_device_setup(&prov, &comp);

	/* Configurations stored in flash during immediate_replay_attack */
	if (!bt_mesh_is_provisioned()) {
		ASSERT_OK_MSG(
			bt_mesh_provision(test_net_key, 0, 0, 0, iut_cfg.addr, iut_cfg.dev_key),
			"Node failed to provision");

		err = bt_mesh_cfg_cli_app_key_add(0, iut_cfg.addr, 0, 0, test_app_key, &status);
		if (err || status) {
			FAIL("AppKey add failed (err %d, status %u)", err, status);
		}
		err = bt_mesh_cfg_cli_gatt_proxy_set(0, iut_cfg.addr, BT_MESH_GATT_PROXY_DISABLED,
						     &status);
		if (err || status) {
			FAIL("Proxy state disable failed (err %d, status %u)", err, status);
		}
		err = bt_mesh_priv_beacon_cli_gatt_proxy_set(0, iut_cfg.addr,
							     BT_MESH_GATT_PROXY_DISABLED, &status);
		if (err || status) {
			FAIL("Private proxy state disable failed (err %d, status %u)", err, status);
		}
		err = bt_mesh_od_priv_proxy_cli_set(0, iut_cfg.addr, BT_MESH_FEATURE_ENABLED,
						    &status);
		if (err || !status) {
			FAIL("On-Demand Private Proxy enable failed (err %d, status %u)", err,
			     status);
		}
	}
}

static void sol_fixed_pdu_create(struct bt_mesh_subnet *sub, struct net_buf_simple *pdu)
{
	uint32_t fixed_seq_n = 2;

	net_buf_simple_add_u8(pdu, sub->keys[SUBNET_KEY_TX_IDX(sub)].msg.nid);
	net_buf_simple_add_u8(pdu, 0x80);
	net_buf_simple_add_le24(pdu, sys_cpu_to_be24(fixed_seq_n));
	net_buf_simple_add_le16(pdu, sys_cpu_to_be16(bt_mesh_primary_addr()));
	net_buf_simple_add_le16(pdu, 0x0000);

	ASSERT_OK(bt_mesh_net_encrypt(&sub->keys[SUBNET_KEY_TX_IDX(sub)].msg.enc, pdu, 0,
				      BT_MESH_NONCE_SOLICITATION));

	ASSERT_OK(bt_mesh_net_obfuscate(pdu->data, 0,
					&sub->keys[SUBNET_KEY_TX_IDX(sub)].msg.privacy));

	net_buf_simple_push_u8(pdu, 0);
	net_buf_simple_push_le16(pdu, BT_UUID_MESH_PROXY_SOLICITATION_VAL);
}

static int sol_fixed_pdu_send(void)
{
	NET_BUF_SIMPLE_DEFINE(pdu, 20);
	net_buf_simple_init(&pdu, 3);

	struct bt_mesh_subnet *sub = bt_mesh_subnet_find(NULL, NULL);

	uint16_t adv_int = BT_MESH_TRANSMIT_INT(CONFIG_BT_MESH_SOL_ADV_XMIT);

	sol_fixed_pdu_create(sub, &pdu);

	struct bt_data ad[] = {
		BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
		BT_DATA_BYTES(BT_DATA_UUID16_ALL,
			      BT_UUID_16_ENCODE(BT_UUID_MESH_PROXY_SOLICITATION_VAL)),
		BT_DATA(BT_DATA_SVC_DATA16, pdu.data, pdu.size),
	};

	return bt_mesh_adv_bt_data_send(CONFIG_BT_MESH_SOL_ADV_XMIT, adv_int, ad, 3);
}

static void test_tester_beacon_rcvd(void)
{
	tester_configure();

	/* Check that no beacons are currently being picked up by the scanner */
	ASSERT_EQUAL(-EAGAIN, k_sem_take(&beacon_sem, K_SECONDS(SEM_TIMEOUT)));

	ASSERT_OK(bt_mesh_proxy_solicit(0));

	ASSERT_OK_MSG(k_sem_take(&beacon_sem, K_SECONDS(SEM_TIMEOUT)),
		      "No beacon (proxy advs) reveiced.");

	PASS();
}

static void test_tester_immediate_replay_attack(void)
{
	tester_configure();

	/* Check that no beacons are currently being picked up by the scanner */
	ASSERT_EQUAL(-EAGAIN, k_sem_take(&beacon_sem, K_SECONDS(SEM_TIMEOUT)));

	/* Send initial solicitation PDU with fixed sequence number */
	ASSERT_OK(sol_fixed_pdu_send());

	ASSERT_OK_MSG(k_sem_take(&beacon_sem, K_SECONDS(SEM_TIMEOUT)),
		      "No beacon (proxy advs) reveiced.");
	k_sem_reset(&beacon_sem);

	/* Wait for iut proxy advs to time out */
	k_sleep(K_MSEC(200));
	ASSERT_EQUAL(-EAGAIN, k_sem_take(&beacon_sem, K_SECONDS(SEM_TIMEOUT)));

	/* Replay attack */
	ASSERT_OK(sol_fixed_pdu_send());

	ASSERT_EQUAL(-EAGAIN, k_sem_take(&beacon_sem, K_SECONDS(SEM_TIMEOUT)));

	PASS();
}

static void test_tester_power_replay_attack(void)
{
	tester_configure();

	/* Check that no beacons are currently being picked up by the scanner */
	ASSERT_EQUAL(-EAGAIN, k_sem_take(&beacon_sem, K_SECONDS(SEM_TIMEOUT)));

	/* Replay attack, using standard api, starting with sseq = 0 < fixed sseq */
	for (size_t i = 0; i < 3; i++) {
		k_sleep(K_MSEC(100));
		ASSERT_OK(bt_mesh_proxy_solicit(0));
	}

	ASSERT_EQUAL(-EAGAIN, k_sem_take(&beacon_sem, K_SECONDS(SEM_TIMEOUT)));

	/* Send a sol pdu with sseq = 3 > fixed sseq (2) */
	ASSERT_OK(bt_mesh_proxy_solicit(0));
	ASSERT_OK_MSG(k_sem_take(&beacon_sem, K_SECONDS(SEM_TIMEOUT)),
		      "No beacon (proxy advs) reveiced.");

	PASS();
}

static void test_iut_beacon_send(void)
{
	iut_configure();
	k_sleep(K_SECONDS(3 * SEM_TIMEOUT));

	PASS();
}

static void test_iut_immediate_replay_attack(void)
{
	iut_configure();
	k_sleep(K_SECONDS(5 * SEM_TIMEOUT));

	PASS();
}

static void test_iut_power_replay_attack(void)
{
	iut_configure();
	k_sleep(K_SECONDS(4 * SEM_TIMEOUT));

	PASS();
}

#define TEST_CASE(role, name, description)                                             \
	{                                                                              \
		.test_id = "proxy_sol_" #role "_" #name,                               \
		.test_descr = description,                                             \
		.test_tick_f = bt_mesh_test_timeout,                                   \
		.test_main_f = test_##role##_##name,                                   \
	}

static const struct bst_test_instance test_proxy_sol[] = {

	TEST_CASE(tester, beacon_rcvd, "Check for beacon after solicitation"),
	TEST_CASE(tester, immediate_replay_attack, "Perform replay attack immediately"),
	TEST_CASE(tester, power_replay_attack, "Perform replay attack after power cycle of iut"),

	TEST_CASE(iut, beacon_send, "Respond with beacon after solicitation"),
	TEST_CASE(iut, immediate_replay_attack, "Device is under immediate replay attack"),
	TEST_CASE(iut, power_replay_attack, "Device is under power cycle replay attack"),

	BSTEST_END_MARKER};

struct bst_test_list *test_proxy_sol_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_proxy_sol);
	return tests;
}
