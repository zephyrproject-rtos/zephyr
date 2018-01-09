/*
 * Copyright (c) 2016-2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#include "common/log.h"

#include "hal/cpu.h"
#include "hal/rand.h"
#include "hal/ecb.h"

K_MUTEX_DEFINE(mutex_rand);

int bt_rand(void *buf, size_t len)
{
	while (len) {
		k_mutex_lock(&mutex_rand, K_FOREVER);
		len = rand_get(len, buf);
		k_mutex_unlock(&mutex_rand);
		if (len) {
			cpu_sleep();
		}
	}

	return 0;
}

int bt_encrypt_le(const u8_t key[16], const u8_t plaintext[16],
		  u8_t enc_data[16])
{
	BT_DBG("key %s plaintext %s", bt_hex(key, 16), bt_hex(plaintext, 16));

	ecb_encrypt(key, plaintext, enc_data, NULL);

	BT_DBG("enc_data %s", bt_hex(enc_data, 16));

	return 0;
}

int bt_encrypt_be(const u8_t key[16], const u8_t plaintext[16],
		  u8_t enc_data[16])
{
	BT_DBG("key %s plaintext %s", bt_hex(key, 16), bt_hex(plaintext, 16));

	ecb_encrypt_be(key, plaintext, enc_data);

	BT_DBG("enc_data %s", bt_hex(enc_data, 16));

	return 0;
}
