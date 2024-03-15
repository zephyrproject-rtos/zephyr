/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/mesh.h>
#include <zephyr/sys/byteorder.h>
#include <stdlib.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include "access.h"
#include "cfg.h"
#include "crypto.h"
#include "mesh.h"
#include "net.h"
#include "proxy.h"
#include "settings.h"

#include "common/bt_str.h"

#include "host/hci_core.h"

#define LOG_LEVEL CONFIG_BT_MESH_MODEL_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mesh_solicitation);

#if CONFIG_BT_MESH_OD_PRIV_PROXY_SRV
static struct srpl_entry {
	uint32_t sseq;
	uint16_t ssrc;
} sol_pdu_rpl[CONFIG_BT_MESH_PROXY_SRPL_SIZE];

static ATOMIC_DEFINE(store, CONFIG_BT_MESH_PROXY_SRPL_SIZE);
static atomic_t clear;
#endif
#if CONFIG_BT_MESH_PROXY_SOLICITATION
static uint32_t sseq_out;
#endif

#if CONFIG_BT_MESH_OD_PRIV_PROXY_SRV
static struct srpl_entry *srpl_find_by_addr(uint16_t ssrc)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sol_pdu_rpl); i++) {
		if (sol_pdu_rpl[i].ssrc == ssrc) {
			return &sol_pdu_rpl[i];
		}
	}

	return NULL;
}

static int srpl_entry_save(struct bt_mesh_subnet *sub, uint32_t sseq, uint16_t ssrc)
{
	struct srpl_entry *entry;

	if (!BT_MESH_ADDR_IS_UNICAST(ssrc)) {
		LOG_DBG("Addr not in unicast range");
		return -EINVAL;
	}

	entry = srpl_find_by_addr(ssrc);
	if (entry) {
		if (entry->sseq >= sseq) {
			LOG_WRN("Higher or equal SSEQ already saved for this SSRC");
			return -EALREADY;
		}

	} else {
		entry = srpl_find_by_addr(BT_MESH_ADDR_UNASSIGNED);
		if (!entry) {
			/* No space to save new PDU in RPL for this SSRC
			 * and this PDU is first for this SSRC
			 */
			return -ENOMEM;
		}
	}

	entry->sseq = sseq;
	entry->ssrc = ssrc;

	LOG_DBG("Added: SSRC %d SSEQ %d to SRPL", entry->ssrc, entry->sseq);

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		atomic_set_bit(store, entry - &sol_pdu_rpl[0]);
		bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_SRPL_PENDING);
	}

	return 0;
}
#endif

void bt_mesh_sseq_pending_store(void)
{
#if CONFIG_BT_MESH_PROXY_SOLICITATION
	char *path = "bt/mesh/SSeq";
	int err;

	if (sseq_out) {
		err = settings_save_one(path, &sseq_out, sizeof(sseq_out));
	} else {
		err = settings_delete(path);
	}

	if (err) {
		LOG_ERR("Failed to %s SSeq %s value", (sseq_out == 0 ? "delete" : "store"), path);
	} else {
		LOG_DBG("%s %s value", (sseq_out == 0 ? "Deleted" : "Stored"), path);
	}
#endif
}

#if CONFIG_BT_MESH_PROXY_SOLICITATION
static int sseq_set(const char *name, size_t len_rd,
		    settings_read_cb read_cb, void *cb_arg)
{
	int err;

	err = bt_mesh_settings_set(read_cb, cb_arg, &sseq_out, sizeof(sseq_out));
	if (err) {
		LOG_ERR("Failed to set \'sseq\'");
		return err;
	}

	LOG_DBG("Restored SSeq value 0x%06x", sseq_out);

	return 0;
}

BT_MESH_SETTINGS_DEFINE(sseq, "SSeq", sseq_set);
#endif

#if CONFIG_BT_MESH_OD_PRIV_PROXY_SRV
static bool sol_pdu_decrypt(struct bt_mesh_subnet *sub, void *data)
{
	struct net_buf_simple *in = data;
	struct net_buf_simple *out = NET_BUF_SIMPLE(17);
	int err, i;
	uint32_t sseq;
	uint16_t ssrc;

	for (i = 0; i < ARRAY_SIZE(sub->keys); i++) {
		if (!sub->keys[i].valid) {
			LOG_ERR("invalid keys %d", i);
			continue;
		}

		net_buf_simple_init(out, 0);
		net_buf_simple_add_mem(out, in->data, in->len);

		err = bt_mesh_net_obfuscate(out->data, 0, &sub->keys[i].msg.privacy);
		if (err) {
			LOG_DBG("obfuscation err %d", err);
			continue;
		}
		err = bt_mesh_net_decrypt(&sub->keys[i].msg.enc, out,
					  0, BT_MESH_NONCE_SOLICITATION);
		if (!err) {
			LOG_DBG("Decrypted PDU %s", bt_hex(out->data, out->len));
			memcpy(&sseq, &out->data[2], 3);
			memcpy(&ssrc, &out->data[5], 2);
			err = srpl_entry_save(sub,
					     sys_be24_to_cpu(sseq),
					     sys_be16_to_cpu(ssrc));
			return err ? false : true;
		}
		LOG_DBG("decrypt err %d", err);
	}

	return false;
}
#endif

void bt_mesh_sol_recv(struct net_buf_simple *buf, uint8_t uuid_list_len)
{
#if CONFIG_BT_MESH_OD_PRIV_PROXY_SRV
	uint8_t type;
	struct bt_mesh_subnet *sub;
	uint16_t uuid;
	uint8_t reported_len;
	uint8_t svc_data_type;
	bool sol_uuid_found = false;
	bool svc_data_found = false;

	if (bt_mesh_gatt_proxy_get() == BT_MESH_GATT_PROXY_ENABLED ||
	    bt_mesh_priv_gatt_proxy_get() == BT_MESH_GATT_PROXY_ENABLED ||
	    bt_mesh_od_priv_proxy_get() == 0) {
		LOG_DBG("Not soliciting");
		return;
	}

	/* Get rid of ad_type that was checked in bt_mesh_scan_cb */
	type = net_buf_simple_pull_u8(buf);
	if (type != BT_DATA_UUID16_SOME && type != BT_DATA_UUID16_ALL) {
		LOG_DBG("Invalid type 0x%x, expected 0x%x or 0x%x",
			type, BT_DATA_UUID16_SOME, BT_DATA_UUID16_ALL);
		return;
	}

	if (buf->len < 24) {
		LOG_DBG("Invalid length (%u) Solicitation PDU", buf->len);
		return;
	}

	while (uuid_list_len >= 2) {
		uuid = net_buf_simple_pull_le16(buf);
		if (uuid == BT_UUID_MESH_PROXY_SOLICITATION_VAL) {
			sol_uuid_found = true;
		}
		uuid_list_len -= 2;
	}

	if (!sol_uuid_found) {
		LOG_DBG("No solicitation UUID found");
		return;
	}

	while (buf->len >= 22) {
		reported_len = net_buf_simple_pull_u8(buf);
		svc_data_type = net_buf_simple_pull_u8(buf);
		uuid = net_buf_simple_pull_le16(buf);

		if (reported_len == 21 && svc_data_type == BT_DATA_SVC_DATA16 &&
		    uuid == BT_UUID_MESH_PROXY_SOLICITATION_VAL) {
			svc_data_found = true;
			break;
		}

		if (buf->len <= reported_len - 3) {
			LOG_DBG("Invalid length (%u) Solicitation PDU", buf->len);
			return;
		}

		net_buf_simple_pull_mem(buf, reported_len - 3);
	}

	if (!svc_data_found) {
		LOG_DBG("No solicitation service data found");
		return;
	}

	type = net_buf_simple_pull_u8(buf);
	if (type != 0) {
		LOG_DBG("Invalid type %d, expected 0x00", type);
		return;
	}

	sub = bt_mesh_subnet_find(sol_pdu_decrypt, (void *)buf);
	if (!sub) {
		LOG_DBG("Unable to find subnetwork for received solicitation PDU");
		return;
	}

	LOG_DBG("Decrypted solicitation PDU for existing subnet");

	sub->solicited = true;
	bt_mesh_adv_gatt_update();
#endif
}


int bt_mesh_proxy_solicit(uint16_t net_idx)
{
#if CONFIG_BT_MESH_PROXY_SOLICITATION
	struct bt_mesh_subnet *sub;

	sub = bt_mesh_subnet_get(net_idx);
	if (!sub) {
		LOG_ERR("No subnet with net_idx %d", net_idx);
		return -EINVAL;
	}

	if (sub->sol_tx == true) {
		LOG_ERR("Solicitation already scheduled for this subnet");
		return -EALREADY;
	}

	/* SSeq reached its maximum value */
	if (sseq_out > 0xFFFFFF) {
		LOG_ERR("SSeq out of range");
		return -EOVERFLOW;
	}

	sub->sol_tx = true;

	bt_mesh_adv_gatt_update();
	return 0;
#else
	return -ENOTSUP;
#endif
}

#if CONFIG_BT_MESH_PROXY_SOLICITATION
static int sol_pdu_create(struct bt_mesh_subnet *sub, struct net_buf_simple *pdu)
{
	int err;

	net_buf_simple_add_u8(pdu, sub->keys[SUBNET_KEY_TX_IDX(sub)].msg.nid);
	/* CTL = 1, TTL = 0 */
	net_buf_simple_add_u8(pdu, 0x80);
	net_buf_simple_add_le24(pdu, sys_cpu_to_be24(sseq_out));
	net_buf_simple_add_le16(pdu, sys_cpu_to_be16(bt_mesh_primary_addr()));
	/* DST = 0x0000 */
	net_buf_simple_add_le16(pdu, 0x0000);

	err = bt_mesh_net_encrypt(&sub->keys[SUBNET_KEY_TX_IDX(sub)].msg.enc,
				  pdu, 0, BT_MESH_NONCE_SOLICITATION);

	if (err) {
		LOG_ERR("Encryption failed, err=%d", err);
		return err;
	}

	err = bt_mesh_net_obfuscate(pdu->data, 0,
				    &sub->keys[SUBNET_KEY_TX_IDX(sub)].msg.privacy);
	if (err) {
		LOG_ERR("Obfuscation failed, err=%d", err);
		return err;
	}

	net_buf_simple_push_u8(pdu, 0);
	net_buf_simple_push_le16(pdu, BT_UUID_MESH_PROXY_SOLICITATION_VAL);

	return 0;
}
#endif

#if CONFIG_BT_MESH_OD_PRIV_PROXY_SRV
static int srpl_set(const char *name, size_t len_rd,
		   settings_read_cb read_cb, void *cb_arg)
{
	struct srpl_entry *entry;
	int err;
	uint16_t ssrc;
	uint32_t sseq;

	if (!name) {
		LOG_ERR("Insufficient number of arguments");
		return -ENOENT;
	}

	ssrc = strtol(name, NULL, 16);
	entry = srpl_find_by_addr(ssrc);

	if (len_rd == 0) {
		LOG_DBG("val (null)");
		if (entry) {
			(void)memset(entry, 0, sizeof(*entry));
		} else {
			LOG_WRN("Unable to find RPL entry for 0x%04x", ssrc);
		}

		return 0;
	}

	if (!entry) {
		entry = srpl_find_by_addr(BT_MESH_ADDR_UNASSIGNED);
		if (!entry) {
			LOG_ERR("Unable to allocate SRPL entry for 0x%04x", ssrc);
			return -ENOMEM;
		}
	}

	err = bt_mesh_settings_set(read_cb, cb_arg, &sseq, sizeof(sseq));
	if (err) {
		LOG_ERR("Failed to set \'sseq\'");
		return err;
	}

	entry->ssrc = ssrc;
	entry->sseq = sseq;

	LOG_DBG("SRPL entry for 0x%04x: Seq 0x%06x", entry->ssrc,
	       entry->sseq);

	return 0;
}

BT_MESH_SETTINGS_DEFINE(srpl, "SRPL", srpl_set);
#endif

#if CONFIG_BT_MESH_OD_PRIV_PROXY_SRV
static void srpl_entry_clear(int i)
{
	uint16_t addr = sol_pdu_rpl[i].ssrc;

	LOG_DBG("Removing entry SSRC: %d, SSEQ: %d from RPL",
		sol_pdu_rpl[i].ssrc,
		sol_pdu_rpl[i].sseq);
	sol_pdu_rpl[i].ssrc = 0;
	sol_pdu_rpl[i].sseq = 0;

	atomic_clear_bit(store, i);

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		char path[18];

		snprintk(path, sizeof(path), "bt/mesh/SRPL/%x", addr);

		settings_delete(path);
	}
}

static void srpl_store(struct srpl_entry *entry)
{
	char path[18];
	int err;

	LOG_DBG("src 0x%04x seq 0x%06x", entry->ssrc, entry->sseq);

	snprintk(path, sizeof(path), "bt/mesh/SRPL/%x", entry->ssrc);

	err = settings_save_one(path, &entry->sseq, sizeof(entry->sseq));
	if (err) {
		LOG_ERR("Failed to store RPL %s value", path);
	} else {
		LOG_DBG("Stored RPL %s value", path);
	}
}
#endif

void bt_mesh_srpl_pending_store(void)
{
#if CONFIG_BT_MESH_OD_PRIV_PROXY_SRV
	bool clr;

	clr = atomic_cas(&clear, 1, 0);

	for (int i = 0; i < ARRAY_SIZE(sol_pdu_rpl); i++) {
		LOG_DBG("src 0x%04x seq 0x%06x", sol_pdu_rpl[i].ssrc, sol_pdu_rpl[i].sseq);

		if (clr) {
			srpl_entry_clear(i);
		} else if (atomic_test_and_clear_bit(store, i)) {
			srpl_store(&sol_pdu_rpl[i]);
		}
	}
#endif
}

void bt_mesh_srpl_entry_clear(uint16_t addr)
{
#if CONFIG_BT_MESH_OD_PRIV_PROXY_SRV
	struct srpl_entry *entry;

	if (!BT_MESH_ADDR_IS_UNICAST(addr)) {
		LOG_DBG("Addr not in unicast range");
		return;
	}

	entry = srpl_find_by_addr(addr);
	if (!entry) {
		return;
	}

	srpl_entry_clear(entry - &sol_pdu_rpl[0]);
#endif
}

void bt_mesh_sol_reset(void)
{
#if CONFIG_BT_MESH_PROXY_SOLICITATION
	sseq_out = 0;

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_SSEQ_PENDING);
	}
#endif

#if CONFIG_BT_MESH_OD_PRIV_PROXY_SRV
	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		(void)atomic_cas(&clear, 0, 1);

		bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_SRPL_PENDING);
	}
#endif
}

#if CONFIG_BT_MESH_PROXY_SOLICITATION
static bool sol_subnet_find(struct bt_mesh_subnet *sub, void *cb_data)
{
	return sub->sol_tx;
}
#endif

int bt_mesh_sol_send(void)
{
#if CONFIG_BT_MESH_PROXY_SOLICITATION
	uint16_t adv_int;
	struct bt_mesh_subnet *sub;
	int err;

	NET_BUF_SIMPLE_DEFINE(pdu, 20);

	sub = bt_mesh_subnet_find(sol_subnet_find, NULL);
	if (!sub) {
		return -ENOENT;
	}

	/* SSeq reached its maximum value */
	if (sseq_out > 0xFFFFFF) {
		LOG_ERR("SSeq out of range");
		sub->sol_tx = false;
		return -EOVERFLOW;
	}

	net_buf_simple_init(&pdu, 3);

	adv_int = BT_MESH_TRANSMIT_INT(CONFIG_BT_MESH_SOL_ADV_XMIT);

	err = sol_pdu_create(sub, &pdu);
	if (err) {
		LOG_ERR("Failed to create Solicitation PDU, err=%d", err);
		return err;
	}

	struct bt_data ad[] = {
		BT_DATA_BYTES(BT_DATA_FLAGS,
			      (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
		BT_DATA_BYTES(BT_DATA_UUID16_ALL,
			      BT_UUID_16_ENCODE(
				      BT_UUID_MESH_PROXY_SOLICITATION_VAL)),
		BT_DATA(BT_DATA_SVC_DATA16, pdu.data, pdu.size),
	};

	err = bt_mesh_adv_bt_data_send(CONFIG_BT_MESH_SOL_ADV_XMIT,
				       adv_int, ad, 3);
	if (err) {
		LOG_ERR("Failed to advertise Solicitation PDU, err=%d", err);

		sub->sol_tx = false;

		return err;
	}
	sub->sol_tx = false;

	sseq_out++;

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		bt_mesh_settings_store_schedule(BT_MESH_SETTINGS_SSEQ_PENDING);
	}

	return 0;
#else
	return -ENOTSUP;
#endif
}
