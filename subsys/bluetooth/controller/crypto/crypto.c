/*
 * Copyright (c) 2016-2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_crypto
#include "common/log.h"

#include "util/memq.h"

#include "hal/ecb.h"
#include "lll.h"

int bt_rand(void *buf, size_t len)
{
	return lll_csrand_get(buf, len);
}

int bt_encrypt_le(const uint8_t key[16], const uint8_t plaintext[16],
		  uint8_t enc_data[16])
{
	BT_DBG("key %s", bt_hex(key, 16));
	BT_DBG("plaintext %s", bt_hex(plaintext, 16));

	ecb_encrypt(key, plaintext, enc_data, NULL);

	BT_DBG("enc_data %s", bt_hex(enc_data, 16));

	return 0;
}

int bt_encrypt_be(const uint8_t key[16], const uint8_t plaintext[16],
		  uint8_t enc_data[16])
{
	BT_DBG("key %s", bt_hex(key, 16));
	BT_DBG("plaintext %s", bt_hex(plaintext, 16));

	ecb_encrypt_be(key, plaintext, enc_data);

	BT_DBG("enc_data %s", bt_hex(enc_data, 16));

	return 0;
}
