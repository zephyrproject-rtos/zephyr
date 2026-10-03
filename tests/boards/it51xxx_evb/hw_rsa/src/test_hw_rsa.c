/*
 * Copyright (c) 2026 ITE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/misc/it51xxx_hw_rsa/it51xxx_hw_rsa.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "rsa_2048_keys.h"
#include "rsa_4096_keys.h"

ZTEST(it51xxx_hw_rsa, test_2048_key_bits)
{
	const bool exponent_3 = false;
	struct device *rsa_device = (struct device *)DEVICE_DT_GET(DT_NODELABEL(hw_rsa));
	static struct it51xxx_rsa_messages msgs;
	static uint8_t output[sizeof(ciphertext_2048)];
	struct it51xxx_rsa_keys keys = {
		.private_key = private_key_2048,
		.private_key_sz = sizeof(private_key_2048),
		.public_key = public_key_2048,
		.public_key_sz = sizeof(public_key_2048),
	};
	int ret;

	ret = it51xxx_hw_rsa_configure(rsa_device, keys);
	zassert_equal(ret, 0, "failed to configure keys");
	msgs.plain = plaintext_2048;
	msgs.plain_size = sizeof(plaintext_2048);
	msgs.cipher = output;
	msgs.cipher_size = sizeof(output);
	memset(output, 0, sizeof(output));
	ret = it51xxx_hw_rsa_encrypt(rsa_device, &msgs);
	zassert_equal(ret, 0, "failed to perform encryption");
	ret = memcmp(msgs.cipher, ciphertext_2048, sizeof(ciphertext_2048));
	zassert_equal(ret, 0, "encrypt error");

	ret = it51xxx_hw_rsa_configure(rsa_device, keys);
	zassert_equal(ret, 0, "failed to configure keys");
	msgs.cipher = ciphertext_2048;
	msgs.cipher_size = sizeof(ciphertext_2048);
	msgs.plain = output;
	msgs.plain_size = sizeof(output);
	memset(output, 0, sizeof(output));
	ret = it51xxx_hw_rsa_decrypt(rsa_device, exponent_3, &msgs);
	zassert_equal(ret, 0, "failed to perform decryption");

	ret = memcmp(msgs.plain, plaintext_2048, sizeof(plaintext_2048));
	zassert_equal(ret, 0, "decrypt error");
}

ZTEST(it51xxx_hw_rsa, test_4096_key_bits)
{
	const bool exponent_3 = false;
	struct device *rsa_device = (struct device *)DEVICE_DT_GET(DT_NODELABEL(hw_rsa));
	static struct it51xxx_rsa_messages msgs;
	static uint8_t output[sizeof(ciphertext_4096)];
	struct it51xxx_rsa_keys keys = {
		.private_key = private_key_4096,
		.private_key_sz = sizeof(private_key_4096),
		.public_key = public_key_4096,
		.public_key_sz = sizeof(public_key_4096),
	};
	int ret;

	ret = it51xxx_hw_rsa_configure(rsa_device, keys);
	zassert_equal(ret, 0, "failed to configure keys");
	msgs.plain = plaintext_4096;
	msgs.plain_size = sizeof(plaintext_4096);
	msgs.cipher = output;
	msgs.cipher_size = sizeof(output);
	memset(output, 0, sizeof(output));
	ret = it51xxx_hw_rsa_encrypt(rsa_device, &msgs);
	zassert_equal(ret, 0, "failed to perform encryption");
	ret = memcmp(msgs.cipher, ciphertext_4096, sizeof(ciphertext_4096));
	zassert_equal(ret, 0, "encrypt error");

	ret = it51xxx_hw_rsa_configure(rsa_device, keys);
	zassert_equal(ret, 0, "failed to configure keys");
	msgs.cipher = ciphertext_4096;
	msgs.cipher_size = sizeof(ciphertext_4096);
	msgs.plain = output;
	msgs.plain_size = sizeof(output);
	memset(output, 0, sizeof(output));
	ret = it51xxx_hw_rsa_decrypt(rsa_device, exponent_3, &msgs);
	zassert_equal(ret, 0, "failed to perform encryption");
	ret = memcmp(msgs.plain, plaintext_4096, sizeof(plaintext_4096));
	zassert_equal(ret, 0, "decrypt error");
}

ZTEST_SUITE(it51xxx_hw_rsa, NULL, NULL, NULL, NULL, NULL);
