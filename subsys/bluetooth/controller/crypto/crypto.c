/*
 * Copyright (c) 2016-2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/entropy.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#define LOG_MODULE_NAME bt_ctlr_crypto
#include "common/log.h"

#include "hal/ecb.h"

static struct device *entropy_driver;

int bt_rand(void *buf, size_t len)
{
	struct device *dev = entropy_driver;

	if (unlikely(!dev)) {
		/* Only one entropy device exists, so this is safe even
		 * if the whole operation isn't atomic.
		 */
		dev = device_get_binding(CONFIG_ENTROPY_NAME);
		__ASSERT((dev != NULL),
			"Device driver for %s (CONFIG_ENTROPY_NAME) not found. "
			"Check your build configuration!",
			CONFIG_ENTROPY_NAME);
		entropy_driver = dev;
	}

	return entropy_get_entropy(dev, (u8_t *)buf, len);
}

int bt_encrypt_le(const u8_t key[16], const u8_t plaintext[16],
		  u8_t enc_data[16])
{
	BT_DBG("key %s", bt_hex(key, 16));
	BT_DBG("plaintext %s", bt_hex(plaintext, 16));

	ecb_encrypt(key, plaintext, enc_data, NULL);

	BT_DBG("enc_data %s", bt_hex(enc_data, 16));

	return 0;
}

int bt_encrypt_be(const u8_t key[16], const u8_t plaintext[16],
		  u8_t enc_data[16])
{
	BT_DBG("key %s", bt_hex(key, 16));
	BT_DBG("plaintext %s", bt_hex(plaintext, 16));

	ecb_encrypt_be(key, plaintext, enc_data);

	BT_DBG("enc_data %s", bt_hex(enc_data, 16));

	return 0;
}
