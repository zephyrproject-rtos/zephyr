/*
 * Copyright (c) 2016-2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/bt_str.h"

#include "util/memq.h"

#include "hal/ecb.h"
#include "ll_sw/lll.h"

#define LOG_LEVEL CONFIG_BT_HCI_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_ctlr_crypto);

int bt_rand(void *buf, size_t len)
{
	return lll_csrand_get(buf, len);
}

int bt_encrypt_le(const uint8_t key[16], const uint8_t plaintext[16],
		  uint8_t enc_data[16])
{
	LOG_DBG("key %s", bt_hex(key, 16));
	LOG_DBG("plaintext %s", bt_hex(plaintext, 16));

	ecb_encrypt(key, plaintext, enc_data, NULL);

	LOG_DBG("enc_data %s", bt_hex(enc_data, 16));

	return 0;
}

int bt_encrypt_be(const uint8_t key[16], const uint8_t plaintext[16],
		  uint8_t enc_data[16])
{
	LOG_DBG("key %s", bt_hex(key, 16));
	LOG_DBG("plaintext %s", bt_hex(plaintext, 16));

	ecb_encrypt_be(key, plaintext, enc_data);

	LOG_DBG("enc_data %s", bt_hex(enc_data, 16));

	return 0;
}
