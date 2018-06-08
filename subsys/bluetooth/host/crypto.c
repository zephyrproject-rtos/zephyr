/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <errno.h>

#include <zephyr.h>
#include <misc/byteorder.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_CORE)
#include "common/log.h"

#include "common/cryptdev.h"

#include "hci_core.h"

static struct cipher_ctx prng_session;

static int prng_reseed(void)
{
	u8_t seed[32];
	int ret, i;

	for (i = 0; i < (sizeof(seed) / 8); i++) {
		struct bt_hci_rp_le_rand *rp;
		struct net_buf *rsp;

		/* FIXME: Write a bluetooth-based entropy driver instead,
		 * and get rid of bt_rand() and related functions, calling
		 * the crypto API directly?
		 */
		ret = bt_hci_cmd_send_sync(BT_HCI_OP_LE_RAND, NULL, &rsp);
		if (ret) {
			return ret;
		}

		rp = (void *)rsp->data;
		memcpy(&seed[i * 8], rp->rand, 8);

		explicit_bzero(rp->rand, 8);
		net_buf_unref(rsp);
	}

	ret = cipher_prng_op(&prng_session, &(struct cipher_prng_pkt) {
		.reseed = true,
		.additional_input = seed,
		.additional_input_len = sizeof(seed),
	});

	explicit_bzero(seed, sizeof(seed));

	if (ret < 0) {
		BT_ERR("Failed to re-seed PRNG");
		return -EIO;
	}

	return 0;
}

int prng_init(void)
{
	struct bt_hci_rp_le_rand *rp;
	struct net_buf *rsp;
	struct device *dev;
	int ret;

	/* Check first that HCI_LE_Rand is supported */
	if (!BT_CMD_TEST(bt_dev.supported_commands, 27, 7)) {
		return -ENOTSUP;
	}

	ret = bt_hci_cmd_send_sync(BT_HCI_OP_LE_RAND, NULL, &rsp);
	if (ret) {
		return ret;
	}

	rp = (void *)rsp->data;

	dev = bt_get_cryptdev();
	prng_session.key.personalization.data = rp->rand;
	prng_session.key.personalization.len = sizeof(rp->rand);
	prng_session.flags = cipher_query_hwcaps(dev);

	ret = cipher_begin_session(dev, &prng_session,
				   CRYPTO_CIPHER_ALGO_PRNG,
				   CRYPTO_CIPHER_MODE_PRNG_HMAC,
				   CRYPTO_CIPHER_OP_NONE);

	net_buf_unref(rsp);

	if (ret < 0) {
		BT_ERR("Failed to initialize PRNG");
		return -EIO;
	}

	/* re-seed is needed after init */
	return prng_reseed();
}

int bt_rand(void *buf, size_t len)
{
	int ret;

	ret = cipher_prng_op(&prng_session, &(struct cipher_prng_pkt) {
		.reseed = false,
		.data = buf,
		.data_len = len,
	});
	if (ret == -EAGAIN) {
		ret = prng_reseed();
		if (ret < 0) {
			return ret;
		}

		ret = cipher_prng_op(&prng_session, &(struct cipher_prng_pkt) {
			.reseed = false,
			.data = buf,
			.data_len = len,
		});
	}

	return ret;
}

int bt_encrypt_le(const u8_t key[16], const u8_t plaintext[16],
		  u8_t enc_data[16])
{
	u8_t le_key[16];
	u8_t le_plaintext[16];
	struct cipher_ctx ctx = {
		.key.bit_stream = le_key,
		.keylen = 128 / 8,
	};
	struct cipher_pkt pkt = {
		.in_buf = le_plaintext,
		.in_len = 16,
		.out_buf = enc_data,
		.out_buf_max = 16,
	};
	struct device *dev;
	int ret;

	BT_DBG("key %s plaintext %s", bt_hex(key, 16), bt_hex(plaintext, 16));

	dev = bt_get_cryptdev();
	ctx.flags = cipher_query_hwcaps(dev);
	ret = cipher_begin_session(dev, &ctx,
				   CRYPTO_CIPHER_ALGO_AES,
				   CRYPTO_CIPHER_MODE_ECB,
				   CRYPTO_CIPHER_OP_ENCRYPT);
	if (ret < 0) {
		BT_DBG("Could not begin crypto session");
		return ret;
	}

	sys_memcpy_swap(le_key, key, 16);
	sys_memcpy_swap(le_plaintext, plaintext, 16);

	ret = cipher_block_op(&ctx, &pkt);
	if (ret == 0) {
		sys_mem_swap(enc_data, 16);

		BT_DBG("enc_data %s", bt_hex(enc_data, 16));
	}

	cipher_free_session(dev, &ctx);

	explicit_bzero(le_key, sizeof(le_key));
	explicit_bzero(le_plaintext, sizeof(le_plaintext));

	return ret;
}

int bt_encrypt_be(const u8_t key[16], const u8_t plaintext[16],
		  u8_t enc_data[16])
{
	struct cipher_ctx ctx = {
		.key.bit_stream = (u8_t *)key,
		.keylen = 16,
	};
	struct cipher_pkt pkt = {
		.in_buf = (u8_t *)plaintext,
		.in_len = 16,
		.out_buf = enc_data,
		.out_buf_max = 16,
	};
	struct device *dev;
	int ret;

	BT_DBG("key %s plaintext %s", bt_hex(key, 16), bt_hex(plaintext, 16));

	dev = bt_get_cryptdev();
	ctx.flags = cipher_query_hwcaps(dev);
	ret = cipher_begin_session(dev, &ctx,
				   CRYPTO_CIPHER_ALGO_AES,
				   CRYPTO_CIPHER_MODE_ECB,
				   CRYPTO_CIPHER_OP_ENCRYPT);
	if (ret < 0) {
		BT_DBG("Could not begin crypto session");
		return ret;
	}

	ret = cipher_block_op(&ctx, &pkt);
	if (ret == 0) {
		BT_DBG("enc_data %s", bt_hex(enc_data, 16));
	}

	cipher_free_session(dev, &ctx);

	return ret;
}
