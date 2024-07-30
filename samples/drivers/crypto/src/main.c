/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Sample to illustrate the usage of crypto APIs. The sample plaintext
 * and ciphertexts used for crosschecking are from TinyCrypt.
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/crypto/crypto.h>

#define LOG_LEVEL CONFIG_CRYPTO_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

#ifdef CONFIG_CRYPTO_TINYCRYPT_SHIM
#define CRYPTO_DRV_NAME CONFIG_CRYPTO_TINYCRYPT_SHIM_DRV_NAME
#elif CONFIG_CRYPTO_MBEDTLS_SHIM
#define CRYPTO_DRV_NAME CONFIG_CRYPTO_MBEDTLS_SHIM_DRV_NAME
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_cryp)
#define CRYPTO_DEV_COMPAT st_stm32_cryp
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_aes)
#define CRYPTO_DEV_COMPAT st_stm32_aes
#elif DT_HAS_COMPAT_STATUS_OKAY(nxp_mcux_dcp)
#define CRYPTO_DEV_COMPAT nxp_mcux_dcp
#elif CONFIG_CRYPTO_NRF_ECB
#define CRYPTO_DEV_COMPAT nordic_nrf_ecb
#elif DT_HAS_COMPAT_STATUS_OKAY(renesas_smartbond_crypto)
#define CRYPTO_DEV_COMPAT renesas_smartbond_crypto
#else
#error "You need to enable one crypto device"
#endif

const static uint8_t key[16] __aligned(32) = {
	0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88,
	0x09, 0xcf, 0x4f, 0x3c
};

static uint8_t plaintext[64] = {
	0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96, 0xe9, 0x3d, 0x7e, 0x11,
	0x73, 0x93, 0x17, 0x2a, 0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c,
	0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51, 0x30, 0xc8, 0x1c, 0x46,
	0xa3, 0x5c, 0xe4, 0x11, 0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef,
	0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17, 0xad, 0x2b, 0x41, 0x7b,
	0xe6, 0x6c, 0x37, 0x10
};

uint32_t cap_flags;

static void print_buffer_comparison(const uint8_t *wanted_result,
				    uint8_t *result, size_t length)
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

	printk("\nBut got:\n");

	for (i = 0, j = 1; i < length; i++, j++) {
		printk("0x%02x ", result[i]);

		if (j == 10) {
			printk("\n");
			j = 0;
		}
	}

	printk("\n");
}

int validate_hw_compatibility(const struct device *dev)
{
	uint32_t flags = 0U;

	flags = crypto_query_hwcaps(dev);
	if ((flags & CAP_RAW_KEY) == 0U) {
		LOG_INF("Please provision the key separately "
			"as the module doesnt support a raw key");
		return -1;
	}

	if ((flags & CAP_SYNC_OPS) == 0U) {
		LOG_ERR("The app assumes sync semantics. "
		  "Please rewrite the app accordingly before proceeding");
		return -1;
	}

	if ((flags & CAP_SEPARATE_IO_BUFS) == 0U) {
		LOG_ERR("The app assumes distinct IO buffers. "
		"Please rewrite the app accordingly before proceeding");
		return -1;
	}

	cap_flags = CAP_RAW_KEY | CAP_SYNC_OPS | CAP_SEPARATE_IO_BUFS;

	return 0;

}

void ecb_mode(const struct device *dev)
{
	/* from FIPS-197 test vectors */
	const uint8_t ecb_key[16] = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
	};
	uint8_t ecb_plaintext[16] = {
		0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
		0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
	};
	uint8_t ecb_ciphertext[16] = {
		0x69, 0xC4, 0xE0, 0xD8, 0x6A, 0x7B, 0x04, 0x30,
		0xD8, 0xCD, 0xB7, 0x80, 0x70, 0xB4, 0xC5, 0x5A
	};

	uint8_t encrypted[16] = {0};
	uint8_t decrypted[16] = {0};
	struct cipher_ctx ini = {
		.keylen = sizeof(ecb_key),
		.key.bit_stream = ecb_key,
		.flags = cap_flags,
	};
	struct cipher_pkt encrypt = {
		.in_buf = ecb_plaintext,
		.in_len = sizeof(ecb_plaintext),
		.out_buf_max = sizeof(encrypted),
		.out_buf = encrypted,
	};
	struct cipher_pkt decrypt = {
		.in_buf = encrypt.out_buf,
		.in_len = sizeof(encrypted),
		.out_buf = decrypted,
		.out_buf_max = sizeof(decrypted),
	};

	if (cipher_begin_session(dev, &ini, CRYPTO_CIPHER_ALGO_AES,
				 CRYPTO_CIPHER_MODE_ECB,
				 CRYPTO_CIPHER_OP_ENCRYPT)) {
		return;
	}

	if (cipher_block_op(&ini, &encrypt)) {
		LOG_ERR("ECB mode ENCRYPT - Failed");
		goto out;
	}

	LOG_INF("Output length (encryption): %d", encrypt.out_len);

	if (memcmp(encrypt.out_buf, ecb_ciphertext, sizeof(ecb_ciphertext))) {
		LOG_ERR("ECB mode ENCRYPT - Mismatch between expected and "
			    "returned cipher text");
		print_buffer_comparison(ecb_ciphertext, encrypt.out_buf,
					sizeof(ecb_ciphertext));
		goto out;
	}

	LOG_INF("ECB mode ENCRYPT - Match");
	cipher_free_session(dev, &ini);

	if (cipher_begin_session(dev, &ini, CRYPTO_CIPHER_ALGO_AES,
				 CRYPTO_CIPHER_MODE_ECB,
				 CRYPTO_CIPHER_OP_DECRYPT)) {
		return;
	}

	if (cipher_block_op(&ini, &decrypt)) {
		LOG_ERR("ECB mode DECRYPT - Failed");
		goto out;
	}

	LOG_INF("Output length (decryption): %d", decrypt.out_len);

	if (memcmp(decrypt.out_buf, ecb_plaintext, sizeof(ecb_plaintext))) {
		LOG_ERR("ECB mode DECRYPT - Mismatch between plaintext and "
			    "decrypted cipher text");
		print_buffer_comparison(ecb_plaintext, decrypt.out_buf,
					sizeof(ecb_plaintext));
		goto out;
	}

	LOG_INF("ECB mode DECRYPT - Match");
out:
	cipher_free_session(dev, &ini);
}

static const uint8_t cbc_ciphertext[80] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
	0x0c, 0x0d, 0x0e, 0x0f, 0x76, 0x49, 0xab, 0xac, 0x81, 0x19, 0xb2, 0x46,
	0xce, 0xe9, 0x8e, 0x9b, 0x12, 0xe9, 0x19, 0x7d, 0x50, 0x86, 0xcb, 0x9b,
	0x50, 0x72, 0x19, 0xee, 0x95, 0xdb, 0x11, 0x3a, 0x91, 0x76, 0x78, 0xb2,
	0x73, 0xbe, 0xd6, 0xb8, 0xe3, 0xc1, 0x74, 0x3b, 0x71, 0x16, 0xe6, 0x9e,
	0x22, 0x22, 0x95, 0x16, 0x3f, 0xf1, 0xca, 0xa1, 0x68, 0x1f, 0xac, 0x09,
	0x12, 0x0e, 0xca, 0x30, 0x75, 0x86, 0xe1, 0xa7
};

void cbc_mode(const struct device *dev)
{
	uint8_t encrypted[80] = {0};
	uint8_t decrypted[64] = {0};
	struct cipher_ctx ini = {
		.keylen = sizeof(key),
		.key.bit_stream = key,
		.flags = cap_flags,
	};
	struct cipher_pkt encrypt = {
		.in_buf = plaintext,
		.in_len = sizeof(plaintext),
		.out_buf_max = sizeof(encrypted),
		.out_buf = encrypted,
	};
	struct cipher_pkt decrypt = {
		.in_buf = encrypt.out_buf,
		.in_len = sizeof(encrypted),
		.out_buf = decrypted,
		.out_buf_max = sizeof(decrypted),
	};

	static uint8_t iv[16] = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
	};

	if (cipher_begin_session(dev, &ini, CRYPTO_CIPHER_ALGO_AES,
				 CRYPTO_CIPHER_MODE_CBC,
				 CRYPTO_CIPHER_OP_ENCRYPT)) {
		return;
	}

	if (cipher_cbc_op(&ini, &encrypt, iv)) {
		LOG_ERR("CBC mode ENCRYPT - Failed");
		goto out;
	}

	LOG_INF("Output length (encryption): %d", encrypt.out_len);

	if (memcmp(encrypt.out_buf, cbc_ciphertext, sizeof(cbc_ciphertext))) {
		LOG_ERR("CBC mode ENCRYPT - Mismatch between expected and "
			    "returned cipher text");
		print_buffer_comparison(cbc_ciphertext, encrypt.out_buf,
					sizeof(cbc_ciphertext));
		goto out;
	}

	LOG_INF("CBC mode ENCRYPT - Match");
	cipher_free_session(dev, &ini);

	if (cipher_begin_session(dev, &ini, CRYPTO_CIPHER_ALGO_AES,
				 CRYPTO_CIPHER_MODE_CBC,
				 CRYPTO_CIPHER_OP_DECRYPT)) {
		return;
	}

	/* TinyCrypt keeps IV at the start of encrypted buffer */
	if (cipher_cbc_op(&ini, &decrypt, encrypted)) {
		LOG_ERR("CBC mode DECRYPT - Failed");
		goto out;
	}

	LOG_INF("Output length (decryption): %d", decrypt.out_len);

	if (memcmp(decrypt.out_buf, plaintext, sizeof(plaintext))) {
		LOG_ERR("CBC mode DECRYPT - Mismatch between plaintext and "
			    "decrypted cipher text");
		print_buffer_comparison(plaintext, decrypt.out_buf,
					sizeof(plaintext));
		goto out;
	}

	LOG_INF("CBC mode DECRYPT - Match");
out:
	cipher_free_session(dev, &ini);
}

static const uint8_t ctr_ciphertext[64] = {
	0x22, 0xe5, 0x2f, 0xb1, 0x77, 0xd8, 0x65, 0xb2,
	0xf7, 0xc6, 0xb5, 0x12, 0x69, 0x2d, 0x11, 0x4d,
	0xed, 0x6c, 0x1c, 0x72, 0x25, 0xda, 0xf6, 0xa2,
	0xaa, 0xd9, 0xd3, 0xda, 0x2d, 0xba, 0x21, 0x68,
	0x35, 0xc0, 0xaf, 0x6b, 0x6f, 0x40, 0xc3, 0xc6,
	0xef, 0xc5, 0x85, 0xd0, 0x90, 0x2c, 0xc2, 0x63,
	0x12, 0x2b, 0xc5, 0x8e, 0x72, 0xde, 0x5c, 0xa2,
	0xa3, 0x5c, 0x85, 0x3a, 0xb9, 0x2c, 0x6, 0xbb
};

void ctr_mode(const struct device *dev)
{
	uint8_t encrypted[64] = {0};
	uint8_t decrypted[64] = {0};
	struct cipher_ctx ini = {
		.keylen = sizeof(key),
		.key.bit_stream = key,
		.flags = cap_flags,
		/*  ivlen + ctrlen = keylen , so ctrlen is 128 - 96 = 32 bits */
		.mode_params.ctr_info.ctr_len = 32,
	};
	struct cipher_pkt encrypt = {
		.in_buf = plaintext,
		.in_len = sizeof(plaintext),
		.out_buf_max = sizeof(encrypted),
		.out_buf = encrypted,
	};
	struct cipher_pkt decrypt = {
		.in_buf = encrypted,
		.in_len = sizeof(encrypted),
		.out_buf = decrypted,
		.out_buf_max = sizeof(decrypted),
	};
	uint8_t iv[12] = {
		0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
		0xf8, 0xf9, 0xfa, 0xfb
	};

	if (cipher_begin_session(dev, &ini, CRYPTO_CIPHER_ALGO_AES,
				 CRYPTO_CIPHER_MODE_CTR,
				 CRYPTO_CIPHER_OP_ENCRYPT)) {
		return;
	}

	if (cipher_ctr_op(&ini, &encrypt, iv)) {
		LOG_ERR("CTR mode ENCRYPT - Failed");
		goto out;
	}

	LOG_INF("Output length (encryption): %d", encrypt.out_len);

	if (memcmp(encrypt.out_buf, ctr_ciphertext, sizeof(ctr_ciphertext))) {
		LOG_ERR("CTR mode ENCRYPT - Mismatch between expected "
			    "and returned cipher text");
		print_buffer_comparison(ctr_ciphertext, encrypt.out_buf,
					sizeof(ctr_ciphertext));
		goto out;
	}

	LOG_INF("CTR mode ENCRYPT - Match");
	cipher_free_session(dev, &ini);

	if (cipher_begin_session(dev, &ini, CRYPTO_CIPHER_ALGO_AES,
				 CRYPTO_CIPHER_MODE_CTR,
				 CRYPTO_CIPHER_OP_DECRYPT)) {
		return;
	}

	if (cipher_ctr_op(&ini, &decrypt, iv)) {
		LOG_ERR("CTR mode DECRYPT - Failed");
		goto out;
	}

	LOG_INF("Output length (decryption): %d", decrypt.out_len);

	if (memcmp(decrypt.out_buf, plaintext, sizeof(plaintext))) {
		LOG_ERR("CTR mode DECRYPT - Mismatch between plaintext "
			    "and decrypted cipher text");
		print_buffer_comparison(plaintext,
					decrypt.out_buf, sizeof(plaintext));
		goto out;
	}

	LOG_INF("CTR mode DECRYPT - Match");
out:
	cipher_free_session(dev, &ini);
}

/* RFC 3610 test vector #1 */
const static uint8_t ccm_key[16] = {
	0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb,
	0xcc, 0xcd, 0xce, 0xcf
};
static uint8_t ccm_nonce[13] = {
	0x00, 0x00, 0x00, 0x03, 0x02, 0x01, 0x00, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4,
	0xa5
};
static uint8_t ccm_hdr[8] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07
};
static uint8_t ccm_data[23] = {
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13,
	0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e
};
static const uint8_t ccm_expected[31] = {
	0x58, 0x8c, 0x97, 0x9a, 0x61, 0xc6, 0x63, 0xd2, 0xf0, 0x66, 0xd0, 0xc2,
	0xc0, 0xf9, 0x89, 0x80, 0x6d, 0x5f, 0x6b, 0x61, 0xda, 0xc3, 0x84, 0x17,
	0xe8, 0xd1, 0x2c, 0xfd, 0xf9, 0x26, 0xe0
};

void ccm_mode(const struct device *dev)
{
	uint8_t encrypted[50];
	uint8_t decrypted[25];
	struct cipher_ctx ini = {
		.keylen = sizeof(ccm_key),
		.key.bit_stream = ccm_key,
		.mode_params.ccm_info = {
			.nonce_len = sizeof(ccm_nonce),
			.tag_len = 8,
		},
		.flags = cap_flags,
	};
	struct cipher_pkt encrypt = {
		.in_buf = ccm_data,
		.in_len = sizeof(ccm_data),
		.out_buf_max = sizeof(encrypted),
		.out_buf = encrypted,
	};
	struct cipher_aead_pkt ccm_op = {
		.ad = ccm_hdr,
		.ad_len = sizeof(ccm_hdr),
		.pkt = &encrypt,
		/* TinyCrypt always puts the tag at the end of the ciphered
		 * text, but other library such as mbedtls might be more
		 * flexible and can take a different buffer for it.  So to
		 * make sure test passes on all backends: enforcing the tag
		 * buffer to be after the ciphered text.
		 */
		.tag = encrypted + sizeof(ccm_data),
	};
	struct cipher_pkt decrypt = {
		.in_buf = encrypted,
		.in_len = sizeof(ccm_data),
		.out_buf = decrypted,
		.out_buf_max = sizeof(decrypted),
	};

	if (cipher_begin_session(dev, &ini, CRYPTO_CIPHER_ALGO_AES,
				 CRYPTO_CIPHER_MODE_CCM,
				 CRYPTO_CIPHER_OP_ENCRYPT)) {
		return;
	}

	ccm_op.pkt = &encrypt;
	if (cipher_ccm_op(&ini, &ccm_op, ccm_nonce)) {
		LOG_ERR("CCM mode ENCRYPT - Failed");
		goto out;
	}

	LOG_INF("Output length (encryption): %d", encrypt.out_len);

	if (memcmp(encrypt.out_buf, ccm_expected, sizeof(ccm_expected))) {
		LOG_ERR("CCM mode ENCRYPT - Mismatch between expected "
			    "and returned cipher text");
		print_buffer_comparison(ccm_expected,
					encrypt.out_buf, sizeof(ccm_expected));
		goto out;
	}

	LOG_INF("CCM mode ENCRYPT - Match");
	cipher_free_session(dev, &ini);

	if (cipher_begin_session(dev, &ini, CRYPTO_CIPHER_ALGO_AES,
				 CRYPTO_CIPHER_MODE_CCM,
				 CRYPTO_CIPHER_OP_DECRYPT)) {
		return;
	}

	ccm_op.pkt = &decrypt;
	if (cipher_ccm_op(&ini, &ccm_op, ccm_nonce)) {
		LOG_ERR("CCM mode DECRYPT - Failed");
		goto out;
	}

	LOG_INF("Output length (decryption): %d", decrypt.out_len);

	if (memcmp(decrypt.out_buf, ccm_data, sizeof(ccm_data))) {
		LOG_ERR("CCM mode DECRYPT - Mismatch between plaintext "
			"and decrypted cipher text");
		print_buffer_comparison(ccm_data,
					decrypt.out_buf, sizeof(ccm_data));
		goto out;
	}

	LOG_INF("CCM mode DECRYPT - Match");
out:
	cipher_free_session(dev, &ini);
}

/*  MACsec GCM-AES test vector 2.4.1 */
const static uint8_t gcm_key[16] = {
	0x07, 0x1b, 0x11, 0x3b, 0x0c, 0xa7, 0x43, 0xfe, 0xcc, 0xcf, 0x3d, 0x05,
	0x1f, 0x73, 0x73, 0x82
};
static uint8_t gcm_nonce[12] = {
	0xf0, 0x76, 0x1e, 0x8d, 0xcd, 0x3d, 0x00, 0x01, 0x76, 0xd4, 0x57, 0xed
};
static uint8_t gcm_hdr[20] = {
	0xe2, 0x01, 0x06, 0xd7, 0xcd, 0x0d, 0xf0, 0x76, 0x1e, 0x8d, 0xcd, 0x3d,
	0x88, 0xe5, 0x4c, 0x2a, 0x76, 0xd4, 0x57, 0xed
};
static uint8_t gcm_data[42] = {
	0x08, 0x00, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
	0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24,
	0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30,
	0x31, 0x32, 0x33, 0x34, 0x00, 0x04
};
static const uint8_t gcm_expected[58] = {
	0x13, 0xb4, 0xc7, 0x2b, 0x38, 0x9d, 0xc5, 0x01, 0x8e, 0x72, 0xa1, 0x71,
	0xdd, 0x85, 0xa5, 0xd3, 0x75, 0x22, 0x74, 0xd3, 0xa0, 0x19, 0xfb, 0xca,
	0xed, 0x09, 0xa4, 0x25, 0xcd, 0x9b, 0x2e, 0x1c, 0x9b, 0x72, 0xee, 0xe7,
	0xc9, 0xde, 0x7d, 0x52, 0xb3, 0xf3,
	0xd6, 0xa5, 0x28, 0x4f, 0x4a, 0x6d, 0x3f, 0xe2, 0x2a, 0x5d, 0x6c, 0x2b,
	0x96, 0x04, 0x94, 0xc3
};

void gcm_mode(const struct device *dev)
{
	uint8_t encrypted[60] = {0};
	uint8_t decrypted[44] = {0};
	struct cipher_ctx ini = {
		.keylen = sizeof(gcm_key),
		.key.bit_stream = gcm_key,
		.mode_params.gcm_info = {
			.nonce_len = sizeof(gcm_nonce),
			.tag_len = 16,
		},
		.flags = cap_flags,
	};
	struct cipher_pkt encrypt = {
		.in_buf = gcm_data,
		.in_len = sizeof(gcm_data),
		.out_buf_max = sizeof(encrypted),
		.out_buf = encrypted,
	};
	struct cipher_aead_pkt gcm_op = {
		.ad = gcm_hdr,
		.ad_len = sizeof(gcm_hdr),
		.pkt = &encrypt,
		/* TinyCrypt always puts the tag at the end of the ciphered
		 * text, but other library such as mbedtls might be more
		 * flexible and can take a different buffer for it.  So to
		 * make sure test passes on all backends: enforcing the tag
		 * buffer to be after the ciphered text.
		 */
		.tag = encrypted + sizeof(gcm_data),
	};
	struct cipher_pkt decrypt = {
		.in_buf = encrypted,
		.in_len = sizeof(gcm_data),
		.out_buf = decrypted,
		.out_buf_max = sizeof(decrypted),
	};

	if (cipher_begin_session(dev, &ini, CRYPTO_CIPHER_ALGO_AES,
				 CRYPTO_CIPHER_MODE_GCM,
				 CRYPTO_CIPHER_OP_ENCRYPT)) {
		return;
	}

	gcm_op.pkt = &encrypt;
	if (cipher_gcm_op(&ini, &gcm_op, gcm_nonce)) {
		LOG_ERR("GCM mode ENCRYPT - Failed");
		goto out;
	}

	LOG_INF("Output length (encryption): %d", encrypt.out_len);

	if (memcmp(encrypt.out_buf, gcm_expected, sizeof(gcm_expected))) {
		LOG_ERR("GCM mode ENCRYPT - Mismatch between expected "
			    "and returned cipher text");
		print_buffer_comparison(gcm_expected,
					encrypt.out_buf, sizeof(gcm_expected));
		goto out;
	}

	LOG_INF("GCM mode ENCRYPT - Match");
	cipher_free_session(dev, &ini);

	if (cipher_begin_session(dev, &ini, CRYPTO_CIPHER_ALGO_AES,
				 CRYPTO_CIPHER_MODE_GCM,
				 CRYPTO_CIPHER_OP_DECRYPT)) {
		return;
	}

	gcm_op.pkt = &decrypt;
	if (cipher_gcm_op(&ini, &gcm_op, gcm_nonce)) {
		LOG_ERR("GCM mode DECRYPT - Failed");
		goto out;
	}

	LOG_INF("Output length (decryption): %d", decrypt.out_len);

	if (memcmp(decrypt.out_buf, gcm_data, sizeof(gcm_data))) {
		LOG_ERR("GCM mode DECRYPT - Mismatch between plaintext "
			"and decrypted cipher text");
		print_buffer_comparison(gcm_data,
					decrypt.out_buf, sizeof(gcm_data));
		goto out;
	}

	LOG_INF("GCM mode DECRYPT - Match");
out:
	cipher_free_session(dev, &ini);
}

struct mode_test {
	const char *mode;
	void (*mode_func)(const struct device *dev);
};

int main(void)
{
#ifdef CRYPTO_DRV_NAME
	const struct device *dev = device_get_binding(CRYPTO_DRV_NAME);

	if (!dev) {
		LOG_ERR("%s pseudo device not found", CRYPTO_DRV_NAME);
		return 0;
	}
#else
	const struct device *const dev = DEVICE_DT_GET_ONE(CRYPTO_DEV_COMPAT);

	if (!device_is_ready(dev)) {
		LOG_ERR("Crypto device is not ready\n");
		return 0;
	}
#endif
	const struct mode_test modes[] = {
		{ .mode = "ECB Mode", .mode_func = ecb_mode },
		{ .mode = "CBC Mode", .mode_func = cbc_mode },
		{ .mode = "CTR Mode", .mode_func = ctr_mode },
		{ .mode = "CCM Mode", .mode_func = ccm_mode },
		{ .mode = "GCM Mode", .mode_func = gcm_mode },
		{ },
	};
	int i;

	if (validate_hw_compatibility(dev)) {
		LOG_ERR("Incompatible h/w");
		return 0;
	}

	LOG_INF("Cipher Sample");

	for (i = 0; modes[i].mode; i++) {
		LOG_INF("%s", modes[i].mode);
		modes[i].mode_func(dev);
	}
	return 0;
}
