/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/crypto.h>

#include <tinycrypt/constants.h>
#include <tinycrypt/hmac_prng.h>
#include <tinycrypt/aes.h>
#include <tinycrypt/utils.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_CORE)
#define LOG_MODULE_NAME bt_crypto
#include "common/log.h"

#include "hci_core.h"

static struct tc_hmac_prng_struct prng;

static int prng_reseed(struct tc_hmac_prng_struct *h)
{
	uint8_t seed[32];
	int64_t extra;
	int ret;

	ret = bt_hci_le_rand(seed, sizeof(seed));
	if (ret) {
		return ret;
	}

	extra = k_uptime_get();

	ret = tc_hmac_prng_reseed(h, seed, sizeof(seed), (uint8_t *)&extra,
				  sizeof(extra));
	if (ret == TC_CRYPTO_FAIL) {
		BT_ERR("Failed to re-seed PRNG");
		return -EIO;
	}

	return 0;
}

int prng_init(void)
{
	uint8_t perso[8];
	int ret;

	ret = bt_hci_le_rand(perso, sizeof(perso));
	if (ret) {
		return ret;
	}

	ret = tc_hmac_prng_init(&prng, perso, sizeof(perso));
	if (ret == TC_CRYPTO_FAIL) {
		BT_ERR("Failed to initialize PRNG");
		return -EIO;
	}

	/* re-seed is needed after init */
	return prng_reseed(&prng);
}

#if defined(CONFIG_BT_HOST_CRYPTO_PRNG)
int bt_rand(void *buf, size_t len)
{
	int ret;

	ret = tc_hmac_prng_generate(buf, len, &prng);
	if (ret == TC_HMAC_PRNG_RESEED_REQ) {
		ret = prng_reseed(&prng);
		if (ret) {
			return ret;
		}

		ret = tc_hmac_prng_generate(buf, len, &prng);
	}

	if (ret == TC_CRYPTO_SUCCESS) {
		return 0;
	}

	return -EIO;
}
#else /* !CONFIG_BT_HOST_CRYPTO_PRNG */
int bt_rand(void *buf, size_t len)
{
	return bt_hci_le_rand(buf, len);
}
#endif /* CONFIG_BT_HOST_CRYPTO_PRNG */

int bt_encrypt_le(const uint8_t key[16], const uint8_t plaintext[16],
		  uint8_t enc_data[16])
{
	struct tc_aes_key_sched_struct s;
	uint8_t tmp[16];

	BT_DBG("key %s", bt_hex(key, 16));
	BT_DBG("plaintext %s", bt_hex(plaintext, 16));

	sys_memcpy_swap(tmp, key, 16);

	if (tc_aes128_set_encrypt_key(&s, tmp) == TC_CRYPTO_FAIL) {
		return -EINVAL;
	}

	sys_memcpy_swap(tmp, plaintext, 16);

	if (tc_aes_encrypt(enc_data, tmp, &s) == TC_CRYPTO_FAIL) {
		return -EINVAL;
	}

	sys_mem_swap(enc_data, 16);

	BT_DBG("enc_data %s", bt_hex(enc_data, 16));

	return 0;
}

int bt_encrypt_be(const uint8_t key[16], const uint8_t plaintext[16],
		  uint8_t enc_data[16])
{
	struct tc_aes_key_sched_struct s;

	BT_DBG("key %s", bt_hex(key, 16));
	BT_DBG("plaintext %s", bt_hex(plaintext, 16));

	if (tc_aes128_set_encrypt_key(&s, key) == TC_CRYPTO_FAIL) {
		return -EINVAL;
	}

	if (tc_aes_encrypt(enc_data, plaintext, &s) == TC_CRYPTO_FAIL) {
		return -EINVAL;
	}

	BT_DBG("enc_data %s", bt_hex(enc_data, 16));

	return 0;
}
