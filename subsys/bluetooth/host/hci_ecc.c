/**
 * @file hci_ecc.c
 * HCI ECC emulation
 */

/*
 * Copyright (c) 2016 Intel Corporation
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

#include <zephyr.h>
#include <atomic.h>
#include <misc/stack.h>
#include <misc/byteorder.h>
#include <tinycrypt/constants.h>
#include <tinycrypt/utils.h>
#include <tinycrypt/ecc.h>
#include <tinycrypt/ecc_dh.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/log.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_driver.h>
#include "hci_core.h"

#if !defined(CONFIG_BLUETOOTH_DEBUG_HCI_CORE)
#undef BT_DBG
#define BT_DBG(fmt, ...)
#endif

static BT_STACK_NOINIT(ecc_thread_stack, 1280);

/* based on Core Specification 4.2 Vol 3. Part H 2.3.5.6.1 */
static const uint32_t debug_private_key[8] = {
	0xcd3c1abd, 0x5899b8a6, 0xeb40b799, 0x4aff607b, 0xd2103f50, 0x74c9b3e3,
	0xa3c55f38, 0x3f49f6d4
};

#if defined(CONFIG_BLUETOOTH_USE_DEBUG_KEYS)
static const uint8_t debug_public_key[64] = {
	0xe6, 0x9d, 0x35, 0x0e, 0x48, 0x01, 0x03, 0xcc, 0xdb, 0xfd, 0xf4, 0xac,
	0x11, 0x91, 0xf4, 0xef, 0xb9, 0xa5, 0xf9, 0xe9, 0xa7, 0x83, 0x2c, 0x5e,
	0x2c, 0xbe, 0x97, 0xf2, 0xd2, 0x03, 0xb0, 0x20, 0x8b, 0xd2, 0x89, 0x15,
	0xd0, 0x8e, 0x1c, 0x74, 0x24, 0x30, 0xed, 0x8f, 0xc2, 0x45, 0x63, 0x76,
	0x5c, 0x15, 0x52, 0x5a, 0xbf, 0x9a, 0x32, 0x63, 0x6d, 0xeb, 0x2a, 0x65,
	0x49, 0x9c, 0x80, 0xdc
};
#endif

static struct k_fifo ecc_queue;
static int (*drv_send)(struct net_buf *buf);
static uint32_t private_key[8];

static void send_cmd_status(uint16_t opcode, uint8_t status)
{
	struct bt_hci_evt_cmd_status *evt;
	struct bt_hci_evt_hdr *hdr;
	struct net_buf *buf;

	BT_DBG("opcode %x status %x", opcode, status);

	buf = bt_buf_get_evt(BT_HCI_EVT_CMD_STATUS);
	if (!buf) {
		BT_ERR("No available event buffers!");
		return;
	}

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->evt = BT_HCI_EVT_CMD_STATUS;
	hdr->len = sizeof(*evt);

	evt = net_buf_add(buf, sizeof(*evt));
	evt->ncmd = 1;
	evt->opcode = sys_cpu_to_le16(opcode);
	evt->status = status;

	bt_recv(buf);
}

static uint8_t generate_keys(uint8_t public_key[64], uint32_t private_key[32])
{
#if !defined(CONFIG_BLUETOOTH_USE_DEBUG_KEYS)
	EccPoint pkey;

	do {
		uint32_t random[8];
		int rc;

		if (bt_rand((uint8_t *)random, sizeof(random))) {
			BT_ERR("Failed to get random bytes for ECC keys");
			return 0x1f; /* unspecified error */
		}

		rc = ecc_make_key(&pkey, private_key, random);
		if (rc == TC_CRYPTO_FAIL) {
			BT_ERR("Failed to create ECC public/private pair");
			return 0x1f; /* unspecified error */
		}

	/* make sure generated key isn't debug key */
	} while (memcmp(private_key, debug_private_key, 32) == 0);

	memcpy(public_key, pkey.x, 32);
	memcpy(&public_key[32], pkey.y, 32);
#else
	memcpy(public_key, debug_public_key, 64);
	memcpy(private_key, debug_private_key, 32);
#endif
	return 0;
}

static void emulate_le_p256_public_key_cmd(struct net_buf *buf)
{
	struct bt_hci_evt_le_p256_public_key_complete *evt;
	struct bt_hci_evt_le_meta_event *meta;
	struct bt_hci_evt_hdr *hdr;

	BT_DBG();

	net_buf_unref(buf);

	send_cmd_status(BT_HCI_OP_LE_P256_PUBLIC_KEY, 0);

	buf = bt_buf_get_evt(BT_HCI_EVT_LE_META_EVENT);
	if (!buf) {
		BT_ERR("No available event buffers!");
		return;
	}

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->evt = BT_HCI_EVT_LE_META_EVENT;
	hdr->len = sizeof(*meta) + sizeof(*evt);

	meta = net_buf_add(buf, sizeof(*meta));
	meta->subevent = BT_HCI_EVT_LE_P256_PUBLIC_KEY_COMPLETE;

	evt = net_buf_add(buf, sizeof(*evt));

	evt->status = generate_keys(evt->key, private_key);
	if (evt->status) {
		memset(evt->key, 0, sizeof(evt->key));
	}

	bt_recv(buf);
}

static void emulate_le_generate_dhkey(struct net_buf *buf)
{
	struct bt_hci_evt_le_generate_dhkey_complete *evt;
	struct bt_hci_cp_le_generate_dhkey *cmd;
	struct bt_hci_evt_le_meta_event *meta;
	struct bt_hci_evt_hdr *hdr;
	EccPoint pk;

	cmd = (void *)buf->data  + sizeof(struct bt_hci_cmd_hdr);

	/* TODO verify cmd parameters? */
	send_cmd_status(BT_HCI_OP_LE_GENERATE_DHKEY, 0);

	memcpy(pk.x, cmd->key, 32);
	memcpy(pk.y, &cmd->key[32], 32);

	net_buf_unref(buf);

	buf = bt_buf_get_evt(BT_HCI_EVT_LE_META_EVENT);
	if (!buf) {
		BT_ERR("No available event buffers!");
		return;
	}

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->evt = BT_HCI_EVT_LE_META_EVENT;
	hdr->len = sizeof(*meta) + sizeof(*evt);

	meta = net_buf_add(buf, sizeof(*meta));
	meta->subevent = BT_HCI_EVT_LE_GENERATE_DHKEY_COMPLETE;

	evt = net_buf_add(buf, sizeof(*evt));
	evt->status = 0;

	if (ecc_valid_public_key(&pk) < 0) {
		evt->status = 0x1f; /* unspecified error */
		memset(evt->dhkey, 0, sizeof(evt->dhkey));
		bt_recv(buf);
		return;
	}

	if (ecdh_shared_secret((uint32_t *)evt->dhkey, &pk, private_key)
	    == TC_CRYPTO_FAIL) {
		evt->status = 0x1f; /* unspecified error */
		memset(evt->dhkey, 0, sizeof(evt->dhkey));
	}

	bt_recv(buf);
}

static void ecc_thread(void *p1, void *p2, void *p3)
{
	while (true) {
		struct net_buf *buf;

		buf = k_fifo_get(&ecc_queue, K_FOREVER);

		switch (bt_hci_get_cmd_opcode(buf)) {
		case BT_HCI_OP_LE_P256_PUBLIC_KEY:
			emulate_le_p256_public_key_cmd(buf);
			break;
		case BT_HCI_OP_LE_GENERATE_DHKEY:
			emulate_le_generate_dhkey(buf);
			break;
		default:
			BT_ERR("Unhandled command for ECC task (opcode %x)",
			       bt_hci_get_cmd_opcode(buf));
			net_buf_unref(buf);
			break;
		}

		stack_analyze("ecc stack", ecc_thread_stack,
			      sizeof(ecc_thread_stack));
	}
}

static void clear_ecc_events(struct net_buf *buf)
{
	struct bt_hci_cp_le_set_event_mask *cmd;

	cmd = (void *)buf->data  + sizeof(struct bt_hci_cmd_hdr);

	/*
	 * don't enable controller ECC events as those will be generated from
	 * emulation code
	 */
	cmd->events[0] &= ~0x80; /* LE Read Local P-256 PKey Compl */
	cmd->events[1] &= ~0x01; /* LE Generate DHKey Compl Event */
}

static int ecc_send(struct net_buf *buf)
{
	if (bt_buf_get_type(buf) == BT_BUF_CMD) {
		switch (bt_hci_get_cmd_opcode(buf)) {
		case BT_HCI_OP_LE_P256_PUBLIC_KEY:
		case BT_HCI_OP_LE_GENERATE_DHKEY:
			net_buf_put(&ecc_queue, buf);
			return 0;
		case BT_HCI_OP_LE_SET_EVENT_MASK:
			clear_ecc_events(buf);
			break;
		default:
			break;
		}
	}

	return drv_send(buf);
}

void bt_hci_ecc_init(void)
{
	k_fifo_init(&ecc_queue);

	k_thread_spawn(ecc_thread_stack, sizeof(ecc_thread_stack),
		       ecc_thread, NULL, NULL, NULL,
		       K_PRIO_PREEMPT(10), 0, K_NO_WAIT);

	/* set wrapper for driver send function */
	drv_send = bt_dev.drv->send;
	bt_dev.drv->send = ecc_send;
}
