/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Sample to illustrate the usage of crypto APIs. The sample plaintext
 * and ciphertexts used for crosschecking are from TinyCrypt.
 */

#include <device.h>
#include <zephyr.h>
#include <string.h>
#include <crypto/cipher.h>

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_CRYPTO_LEVEL
#include <logging/sys_log.h>

#ifdef CONFIG_CRYPTO_TINYCRYPT_SHIM
#define CRYPTO_DRV_NAME CONFIG_CRYPTO_TINYCRYPT_SHIM_DRV_NAME
#elif CONFIG_CRYPTO_MBEDTLS_SHIM
#define CRYPTO_DRV_NAME CONFIG_CRYPTO_MBEDTLS_SHIM_DRV_NAME
#else
#error "You need to enable one crypto device"
#endif

u8_t key[16] = {
	0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88,
	0x09, 0xcf, 0x4f, 0x3c
};
u8_t iv[16] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
	0x0c, 0x0d, 0x0e, 0x0f
};
u8_t plaintext[64] = {
	0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96, 0xe9, 0x3d, 0x7e, 0x11,
	0x73, 0x93, 0x17, 0x2a, 0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c,
	0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51, 0x30, 0xc8, 0x1c, 0x46,
	0xa3, 0x5c, 0xe4, 0x11, 0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef,
	0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17, 0xad, 0x2b, 0x41, 0x7b,
	0xe6, 0x6c, 0x37, 0x10
};
u8_t ciphertext[80] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
	0x0c, 0x0d, 0x0e, 0x0f, 0x76, 0x49, 0xab, 0xac, 0x81, 0x19, 0xb2, 0x46,
	0xce, 0xe9, 0x8e, 0x9b, 0x12, 0xe9, 0x19, 0x7d, 0x50, 0x86, 0xcb, 0x9b,
	0x50, 0x72, 0x19, 0xee, 0x95, 0xdb, 0x11, 0x3a, 0x91, 0x76, 0x78, 0xb2,
	0x73, 0xbe, 0xd6, 0xb8, 0xe3, 0xc1, 0x74, 0x3b, 0x71, 0x16, 0xe6, 0x9e,
	0x22, 0x22, 0x95, 0x16, 0x3f, 0xf1, 0xca, 0xa1, 0x68, 0x1f, 0xac, 0x09,
	0x12, 0x0e, 0xca, 0x30, 0x75, 0x86, 0xe1, 0xa7
};

u32_t cap_flags;

static void print_buffer_comparison(u8_t *wanted_result,
				    u8_t *result, size_t length)
{
	int i, j;

	printk("Was waiting for: \n");

	for (i = 0, j = 1; i < length; i++, j++) {
		printk("0x%02x ", wanted_result[i]);

		if (j == 10) {
			printk("\n");
			j = 0;
		}
	}

	printk("\n But got:\n");

	for (i = 0, j = 1; i < length; i++, j++) {
		printk("0x%02x ", result[i]);

		if (j == 10) {
			printk("\n");
			j = 0;
		}
	}

	printk("\n");
}

int validate_hw_compatibility(struct device *dev)
{
	u32_t flags = 0;

	flags = cipher_query_hwcaps(dev);
	if ((flags & CAP_RAW_KEY) == 0) {
		SYS_LOG_INF(" Please provision the key separately "
			"as the module doesnt support a raw key");
		return -1;
	}

	if ((flags & CAP_SYNC_OPS) == 0) {
		SYS_LOG_ERR("The app assumes sync semantics. "
		  "Please rewrite the app accordingly before proceeding");
		return -1;
	}

	if ((flags & CAP_SEPARATE_IO_BUFS) == 0) {
		SYS_LOG_ERR("The app assumes distinct IO buffers. "
		"Please rewrite the app accordingly before proceeding");
		return -1;
	}

	cap_flags = CAP_RAW_KEY | CAP_SYNC_OPS | CAP_SEPARATE_IO_BUFS;

	return 0;

}

void cbc_mode(void)
{
	struct device *dev;
	struct cipher_ctx ini;
	struct cipher_pkt encrypt;
	struct cipher_pkt decrypt;
	u8_t encrypted[80];
	u8_t decrypted[64];

	SYS_LOG_INF("CBC Mode");

	dev = device_get_binding(CRYPTO_DRV_NAME);
	if (!dev) {
		SYS_LOG_ERR("%s pseudo device not found", CRYPTO_DRV_NAME);
		return;
	}

	if (validate_hw_compatibility(dev)) {
		SYS_LOG_ERR("Incompatible h/w");
		return;
	}

	ini.keylen = 16;
	ini.key.bit_stream = key;
	ini.flags =  cap_flags;

	if (cipher_begin_session(dev, &ini, CRYPTO_CIPHER_ALGO_AES,
				 CRYPTO_CIPHER_MODE_CBC,
				 CRYPTO_CIPHER_OP_ENCRYPT)) {
		return;
	}

	encrypt.in_buf = plaintext;
	encrypt.in_len = sizeof(plaintext);
	encrypt.out_buf_max = sizeof(encrypted);
	encrypt.out_buf = encrypted;

	if (cipher_cbc_op(&ini, &encrypt, iv)) {
		SYS_LOG_ERR("CBC mode ENCRYPT - Failed");
		goto out;
	}

	if (memcmp(encrypt.out_buf, ciphertext, sizeof(ciphertext))) {
		SYS_LOG_ERR("CBC mode ENCRYPT - Mismatch between expected and "
			    "returned cipher text");
		print_buffer_comparison(ciphertext,
					encrypt.out_buf, sizeof(ciphertext));
		goto out;
	}

	SYS_LOG_INF("CBC mode ENCRYPT - Match");
	cipher_free_session(dev, &ini);

	if (cipher_begin_session(dev, &ini, CRYPTO_CIPHER_ALGO_AES,
				 CRYPTO_CIPHER_MODE_CBC,
				 CRYPTO_CIPHER_OP_DECRYPT)) {
		return;
	}

	decrypt.in_buf = encrypt.out_buf;	/* encrypted */
	decrypt.in_len = sizeof(encrypted);
	decrypt.out_buf = decrypted;
	decrypt.out_buf_max = sizeof(decrypted);

	/* TinyCrypt keeps IV at the start of encrypted buffer */
	if (cipher_cbc_op(&ini, &decrypt, encrypted)) {
		SYS_LOG_ERR("CBC mode DECRYPT - Failed");
		goto out;
	}

	if (memcmp(decrypt.out_buf, plaintext, sizeof(plaintext))) {
		SYS_LOG_ERR("CBC mode DECRYPT - Mismatch between plaintext and "
			    "decrypted cipher text");
		print_buffer_comparison(plaintext,
					decrypt.out_buf, sizeof(plaintext));
		goto out;
	}

	SYS_LOG_INF("CBC mode DECRYPT - Match");
out:
	cipher_free_session(dev, &ini);
}

u8_t ctr_ciphertext[64] = {
	0x22, 0xe5, 0x2f, 0xb1, 0x77, 0xd8, 0x65, 0xb2,
	0xf7, 0xc6, 0xb5, 0x12, 0x69, 0x2d, 0x11, 0x4d,
	0xed, 0x6c, 0x1c, 0x72, 0x25, 0xda, 0xf6, 0xa2,
	0xaa, 0xd9, 0xd3, 0xda, 0x2d, 0xba, 0x21, 0x68,
	0x35, 0xc0, 0xaf, 0x6b, 0x6f, 0x40, 0xc3, 0xc6,
	0xef, 0xc5, 0x85, 0xd0, 0x90, 0x2c, 0xc2, 0x63,
	0x12, 0x2b, 0xc5, 0x8e, 0x72, 0xde, 0x5c, 0xa2,
	0xa3, 0x5c, 0x85, 0x3a, 0xb9, 0x2c, 0x6, 0xbb
};

void ctr_mode(void)
{
	struct device *dev;
	struct cipher_ctx ini;
	struct cipher_pkt  encrypt;
	struct cipher_pkt decrypt;
	u8_t encrypted[64] = {0};
	u8_t decrypted[64] = {0};
	u8_t iv[12] = {
		0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
		0xf8, 0xf9, 0xfa, 0xfb
	};

	SYS_LOG_INF("CTR Mode");

	dev = device_get_binding(CRYPTO_DRV_NAME);
	if (!dev) {
		SYS_LOG_ERR("%s crypto device not found", CRYPTO_DRV_NAME);
		return;
	}

	if (validate_hw_compatibility(dev)) {
		SYS_LOG_ERR("Incompatible h/w");
		return;
	}

	ini.keylen = 16;
	ini.key.bit_stream = key;
	ini.flags = cap_flags;
	/*  ivlen + ctrlen = keylen , so ctrlen is 128 - 96 = 32 bits */
	ini.mode_params.ctr_info.ctr_len =  32;

	if (cipher_begin_session(dev, &ini, CRYPTO_CIPHER_ALGO_AES,
				 CRYPTO_CIPHER_MODE_CTR,
				 CRYPTO_CIPHER_OP_ENCRYPT)) {
		return;
	}

	encrypt.in_buf = plaintext;

	encrypt.in_len = sizeof(plaintext);
	encrypt.out_buf_max = sizeof(encrypted);
	encrypt.out_buf = encrypted;

	if (cipher_ctr_op(&ini, &encrypt, iv)) {
		SYS_LOG_ERR("CTR mode ENCRYPT - Failed");
		goto out;
	}

	if (memcmp(encrypt.out_buf, ctr_ciphertext, sizeof(ctr_ciphertext))) {
		SYS_LOG_ERR("CTR mode ENCRYPT - Mismatch between expected "
			    "and returned cipher text");
		print_buffer_comparison(ctr_ciphertext, encrypt.out_buf,
					sizeof(ctr_ciphertext));
		goto out;
	}

	SYS_LOG_INF("CTR mode ENCRYPT - Match");
	cipher_free_session(dev, &ini);

	if (cipher_begin_session(dev, &ini, CRYPTO_CIPHER_ALGO_AES,
				 CRYPTO_CIPHER_MODE_CTR,
				 CRYPTO_CIPHER_OP_DECRYPT)) {
		return;
	}

	decrypt.in_buf = encrypted;
	decrypt.in_len = sizeof(encrypted);
	decrypt.out_buf = decrypted;
	decrypt.out_buf_max = sizeof(decrypted);

	if (cipher_ctr_op(&ini, &decrypt, iv)) {
		SYS_LOG_ERR("CTR mode DECRYPT - Failed");
		goto out;
	}

	if (memcmp(decrypt.out_buf, plaintext, sizeof(plaintext))) {
		SYS_LOG_ERR("CTR mode DECRYPT - Mismatch between plaintext "
			    "and decypted cipher text");
		print_buffer_comparison(plaintext,
					encrypt.out_buf, sizeof(plaintext));
		goto out;
	}

	SYS_LOG_INF("CTR mode DECRYPT - Match");
out:
	cipher_free_session(dev, &ini);
}

/* RFC 3610 test vector #1 */
u8_t ccm_key[16] = {
	0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb,
	0xcc, 0xcd, 0xce, 0xcf
};
u8_t ccm_nonce[13] = {
	0x00, 0x00, 0x00, 0x03, 0x02, 0x01, 0x00, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4,
	0xa5
};
u8_t ccm_hdr[8] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07
};
u8_t ccm_data[23] = {
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13,
	0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e
};
u8_t ccm_expected[31] = {
	0x58, 0x8c, 0x97, 0x9a, 0x61, 0xc6, 0x63, 0xd2, 0xf0, 0x66, 0xd0, 0xc2,
	0xc0, 0xf9, 0x89, 0x80, 0x6d, 0x5f, 0x6b, 0x61, 0xda, 0xc3, 0x84, 0x17,
	0xe8, 0xd1, 0x2c, 0xfd, 0xf9, 0x26, 0xe0
};

void ccm_mode(void)
{
	struct device *dev;
	struct cipher_ctx ini;
	struct cipher_pkt encrypt;
	struct cipher_aead_pkt ccm_op;
	struct cipher_pkt decrypt;
	u8_t encrypted[50];
	u8_t decrypted[25];

	SYS_LOG_INF("CCM Mode");

	dev = device_get_binding(CRYPTO_DRV_NAME);
	if (!dev) {
		SYS_LOG_ERR("%s crypto device not found", CRYPTO_DRV_NAME);
		return;
	}

	if (validate_hw_compatibility(dev)) {
		SYS_LOG_ERR("Incompatible h/w");
		return;
	}

	ini.keylen = sizeof(ccm_key);
	ini.key.bit_stream = ccm_key;
	ini.mode_params.ccm_info.nonce_len = sizeof(ccm_nonce);
	ini.mode_params.ccm_info.tag_len = 8;
	ini.flags = cap_flags;

	if (cipher_begin_session(dev, &ini, CRYPTO_CIPHER_ALGO_AES,
				 CRYPTO_CIPHER_MODE_CCM,
				 CRYPTO_CIPHER_OP_ENCRYPT)) {
		return;
	}

	encrypt.in_buf = ccm_data;
	encrypt.in_len = sizeof(ccm_data);
	encrypt.out_buf_max = sizeof(encrypted);
	encrypt.out_buf = encrypted;

	ccm_op.ad = ccm_hdr;
	ccm_op.ad_len = sizeof(ccm_hdr);
	ccm_op.pkt = &encrypt;

	/* TinyCrypt always puts the tag at the end of the ciphered text,
	 * but other library such as mbedtls might be more flexible and can
	 * take a different buffer for it. So to make sure test passes on
	 * all backends: enforcing the tag buffer to be after the ciphered
	 * text. */
	ccm_op.tag = encrypted + sizeof(ccm_data);

	if (cipher_ccm_op(&ini, &ccm_op, ccm_nonce)) {
		SYS_LOG_ERR("CCM mode ENCRYPT - Failed");
		goto out;
	}

	if (memcmp(encrypt.out_buf, ccm_expected, sizeof(ccm_expected))) {
		SYS_LOG_ERR("CCM mode ENCRYPT - Mismatch between expected "
			    "and returned cipher text");
		print_buffer_comparison(ccm_expected,
					encrypt.out_buf, sizeof(ccm_expected));
		goto out;
	}

	SYS_LOG_INF("CCM mode ENCRYPT - Match");
	cipher_free_session(dev, &ini);

	if (cipher_begin_session(dev, &ini, CRYPTO_CIPHER_ALGO_AES,
				 CRYPTO_CIPHER_MODE_CCM,
				 CRYPTO_CIPHER_OP_DECRYPT)) {
		return;
	}

	decrypt.in_buf = encrypted;
	decrypt.in_len = sizeof(ccm_data);
	decrypt.out_buf = decrypted;
	decrypt.out_buf_max = sizeof(decrypted);

	ccm_op.pkt = &decrypt;

	if (cipher_ccm_op(&ini, &ccm_op, ccm_nonce)) {
		SYS_LOG_ERR("CCM mode DECRYPT - Failed");
		goto out;
	}

	if (memcmp(decrypt.out_buf, ccm_data, sizeof(ccm_data))) {
		SYS_LOG_ERR("CCM mode DECRYPT - Mismatch between plaintext "
			"and decrypted cipher text");
		print_buffer_comparison(ccm_data,
					decrypt.out_buf, sizeof(ccm_data));
		goto out;
	}

	SYS_LOG_INF("CCM mode DECRYPT - Match");
out:
	cipher_free_session(dev, &ini);
}

void main(void)
{
	SYS_LOG_INF("Cipher Sample");
	cbc_mode();
	ctr_mode();
	ccm_mode();
}
