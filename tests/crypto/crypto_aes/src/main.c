/*
 * Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/crypto/crypto.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <string.h>

#ifdef CONFIG_CRYPTO_MBEDTLS_SHIM
#define CRYPTO_DRV_NAME CONFIG_CRYPTO_MBEDTLS_SHIM_DRV_NAME
#elif CONFIG_CRYPTO_ESP32_AES
#define CRYPTO_DEV_COMPAT espressif_esp32_aes
#else
#error "You need to enable one crypto device"
#endif

/* Some crypto drivers require IO buffers to be aligned */
#define IO_ALIGNMENT_BYTES 4

/* Test vectors from FIPS-197 and NIST SP 800-38A */

/* ECB Mode Test Vectors - FIPS-197 */
static uint8_t ecb_key[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
			      0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};

static uint8_t ecb_plaintext[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
				    0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

static uint8_t ecb_ciphertext[16] = {0x69, 0xC4, 0xE0, 0xD8, 0x6A, 0x7B, 0x04, 0x30,
				     0xD8, 0xCD, 0xB7, 0x80, 0x70, 0xB4, 0xC5, 0x5A};

/* CBC Mode Test Vectors - Single block (16 bytes, no padding) */
static uint8_t cbc_key[16] = {0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
			      0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c};

static uint8_t cbc_iv[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
			     0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

static uint8_t cbc_plaintext[16] = {0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
				    0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a};

static uint8_t cbc_ciphertext[16] = {0x76, 0x49, 0xab, 0xac, 0x81, 0x19, 0xb2, 0x46,
				     0xce, 0xe9, 0x8e, 0x9b, 0x12, 0xe9, 0x19, 0x7d};

/* CTR Mode Test Vectors */
static uint8_t ctr_key[16] = {0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
			      0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c};

static uint8_t ctr_iv[12] = {0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5,
			     0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb};

static uint8_t ctr_plaintext[64] = {
	0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96, 0xe9, 0x3d, 0x7e, 0x11, 0x73,
	0x93, 0x17, 0x2a, 0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c, 0x9e, 0xb7,
	0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51, 0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4,
	0x11, 0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef, 0xf6, 0x9f, 0x24, 0x45,
	0xdf, 0x4f, 0x9b, 0x17, 0xad, 0x2b, 0x41, 0x7b, 0xe6, 0x6c, 0x37, 0x10};

static uint8_t ctr_ciphertext[64] = {
	0x22, 0xe5, 0x2f, 0xb1, 0x77, 0xd8, 0x65, 0xb2, 0xf7, 0xc6, 0xb5, 0x12, 0x69,
	0x2d, 0x11, 0x4d, 0xed, 0x6c, 0x1c, 0x72, 0x25, 0xda, 0xf6, 0xa2, 0xaa, 0xd9,
	0xd3, 0xda, 0x2d, 0xba, 0x21, 0x68, 0x35, 0xc0, 0xaf, 0x6b, 0x6f, 0x40, 0xc3,
	0xc6, 0xef, 0xc5, 0x85, 0xd0, 0x90, 0x2c, 0xc2, 0x63, 0x12, 0x2b, 0xc5, 0x8e,
	0x72, 0xde, 0x5c, 0xa2, 0xa3, 0x5c, 0x85, 0x3a, 0xb9, 0x2c, 0x06, 0xbb};

/* CCM Mode Test Vectors - RFC 3610 test vector #1 */
static uint8_t ccm_key[16] = {0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
			      0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf};

static uint8_t ccm_nonce[13] = {0x00, 0x00, 0x00, 0x03, 0x02, 0x01, 0x00,
				0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5};

static uint8_t ccm_hdr[8] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};

static uint8_t ccm_plaintext[23] = {0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
				    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
				    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e};

static uint8_t ccm_ciphertext[31] = {0x58, 0x8c, 0x97, 0x9a, 0x61, 0xc6, 0x63, 0xd2,
				     0xf0, 0x66, 0xd0, 0xc2, 0xc0, 0xf9, 0x89, 0x80,
				     0x6d, 0x5f, 0x6b, 0x61, 0xda, 0xc3, 0x84, 0x17,
				     0xe8, 0xd1, 0x2c, 0xfd, 0xf9, 0x26, 0xe0};

/* GCM Mode Test Vectors - MACsec GCM-AES test vector 2.4.1 */
static uint8_t gcm_key[16] = {0x07, 0x1b, 0x11, 0x3b, 0x0c, 0xa7, 0x43, 0xfe,
			      0xcc, 0xcf, 0x3d, 0x05, 0x1f, 0x73, 0x73, 0x82};

static uint8_t gcm_nonce[12] = {0xf0, 0x76, 0x1e, 0x8d, 0xcd, 0x3d,
				0x00, 0x01, 0x76, 0xd4, 0x57, 0xed};

static uint8_t gcm_hdr[20] = {0xe2, 0x01, 0x06, 0xd7, 0xcd, 0x0d, 0xf0, 0x76, 0x1e, 0x8d,
			      0xcd, 0x3d, 0x88, 0xe5, 0x4c, 0x2a, 0x76, 0xd4, 0x57, 0xed};

static uint8_t gcm_plaintext[42] = {
	0x08, 0x00, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a,
	0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
	0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x00, 0x04};

static uint8_t gcm_ciphertext[58] = {
	0x13, 0xb4, 0xc7, 0x2b, 0x38, 0x9d, 0xc5, 0x01, 0x8e, 0x72, 0xa1, 0x71, 0xdd, 0x85, 0xa5,
	0xd3, 0x75, 0x22, 0x74, 0xd3, 0xa0, 0x19, 0xfb, 0xca, 0xed, 0x09, 0xa4, 0x25, 0xcd, 0x9b,
	0x2e, 0x1c, 0x9b, 0x72, 0xee, 0xe7, 0xc9, 0xde, 0x7d, 0x52, 0xb3, 0xf3, 0xd6, 0xa5, 0x28,
	0x4f, 0x4a, 0x6d, 0x3f, 0xe2, 0x2a, 0x5d, 0x6c, 0x2b, 0x96, 0x04, 0x94, 0xc3};

static inline const struct device *get_crypto_dev(void)
{
#ifdef CRYPTO_DRV_NAME
	const struct device *dev = device_get_binding(CRYPTO_DRV_NAME);
#else
	const struct device *dev = DEVICE_DT_GET_ONE(CRYPTO_DEV_COMPAT);
#endif
	return dev;
}

static const struct device *crypto_dev;

static void *crypto_aes_setup(void)
{
	crypto_dev = get_crypto_dev();
	zassert_true(crypto_dev && device_is_ready(crypto_dev), "Crypto device is not ready");
	return NULL;
}

static void crypto_aes_before(void *fixture)
{
	ARG_UNUSED(fixture);

	/* Add delay between tests to ensure cleanup */
	k_msleep(10);
}

/* ECB Mode Tests */
ZTEST(crypto_aes, test_ecb_encrypt)
{
	uint8_t encrypted[16] __aligned(IO_ALIGNMENT_BYTES) = {0};
	struct cipher_ctx ctx = {
		.keylen = sizeof(ecb_key),
		.key.bit_stream = ecb_key,
		.flags = CAP_RAW_KEY | CAP_SYNC_OPS | CAP_SEPARATE_IO_BUFS,
	};

	struct cipher_pkt pkt = {
		.in_buf = (uint8_t *)ecb_plaintext,
		.in_len = sizeof(ecb_plaintext),
		.out_buf_max = sizeof(encrypted),
		.out_buf = encrypted,
	};

	int rc = cipher_begin_session(crypto_dev, &ctx, CRYPTO_CIPHER_ALGO_AES,
				      CRYPTO_CIPHER_MODE_ECB, CRYPTO_CIPHER_OP_ENCRYPT);

	if (rc == -ENOTSUP) {
		ztest_test_skip();
		return;
	}

	rc = cipher_block_op(&ctx, &pkt);
	if (rc != 0) {
		cipher_free_session(crypto_dev, &ctx);
		zassert_equal(rc, 0, "ECB encrypt failed (rc=%d)", rc);
		return;
	}

	rc = memcmp(encrypted, ecb_ciphertext, sizeof(ecb_ciphertext));
	cipher_free_session(crypto_dev, &ctx);
	zassert_equal(rc, 0, "ECB encrypt output mismatch");
}

ZTEST(crypto_aes, test_ecb_decrypt)
{
	uint8_t decrypted[16] __aligned(IO_ALIGNMENT_BYTES) = {0};
	struct cipher_ctx ctx = {
		.keylen = sizeof(ecb_key),
		.key.bit_stream = ecb_key,
		.flags = CAP_RAW_KEY | CAP_SYNC_OPS | CAP_SEPARATE_IO_BUFS,
	};

	struct cipher_pkt pkt = {
		.in_buf = (uint8_t *)ecb_ciphertext,
		.in_len = sizeof(ecb_ciphertext),
		.out_buf_max = sizeof(decrypted),
		.out_buf = decrypted,
	};

	int rc = cipher_begin_session(crypto_dev, &ctx, CRYPTO_CIPHER_ALGO_AES,
				      CRYPTO_CIPHER_MODE_ECB, CRYPTO_CIPHER_OP_DECRYPT);

	if (rc == -ENOTSUP) {
		ztest_test_skip();
		return;
	}

	rc = cipher_block_op(&ctx, &pkt);
	if (rc != 0) {
		cipher_free_session(crypto_dev, &ctx);
		zassert_equal(rc, 0, "ECB decrypt failed (rc=%d)", rc);
		return;
	}

	rc = memcmp(decrypted, ecb_plaintext, sizeof(ecb_plaintext));
	cipher_free_session(crypto_dev, &ctx);
	zassert_equal(rc, 0, "ECB decrypt output mismatch");
}

/* CBC Mode Tests */
ZTEST(crypto_aes, test_cbc_encrypt)
{
	uint8_t encrypted[32] __aligned(IO_ALIGNMENT_BYTES) = {0};
	uint8_t iv_copy[16];

	memcpy(iv_copy, cbc_iv, sizeof(cbc_iv));

	struct cipher_ctx ctx = {
		.keylen = sizeof(cbc_key),
		.key.bit_stream = cbc_key,
		.flags = CAP_RAW_KEY | CAP_SYNC_OPS | CAP_SEPARATE_IO_BUFS,
	};

	struct cipher_pkt pkt = {
		.in_buf = (uint8_t *)cbc_plaintext,
		.in_len = sizeof(cbc_plaintext),
		.out_buf_max = sizeof(encrypted),
		.out_buf = encrypted,
	};

	int rc = cipher_begin_session(crypto_dev, &ctx, CRYPTO_CIPHER_ALGO_AES,
				      CRYPTO_CIPHER_MODE_CBC, CRYPTO_CIPHER_OP_ENCRYPT);

	if (rc == -ENOTSUP) {
		ztest_test_skip();
		return;
	}

	rc = cipher_cbc_op(&ctx, &pkt, iv_copy);
	if (rc != 0) {
		cipher_free_session(crypto_dev, &ctx);
		zassert_equal(rc, 0, "CBC encrypt failed (rc=%d)", rc);
		return;
	}

	/* CBC prepends IV to output, so ciphertext starts at offset 16 */
	rc = memcmp(encrypted + 16, cbc_ciphertext, sizeof(cbc_ciphertext));
	cipher_free_session(crypto_dev, &ctx);
	zassert_equal(rc, 0, "CBC encrypt output mismatch");
}

ZTEST(crypto_aes, test_cbc_decrypt)
{
	/* For decrypt, need to prepend IV to ciphertext input */
	uint8_t input[32] __aligned(IO_ALIGNMENT_BYTES);
	uint8_t decrypted[16] __aligned(IO_ALIGNMENT_BYTES) = {0};

	/* Prepend IV to ciphertext */
	memcpy(input, cbc_iv, sizeof(cbc_iv));
	memcpy(input + 16, cbc_ciphertext, sizeof(cbc_ciphertext));

	struct cipher_ctx ctx = {
		.keylen = sizeof(cbc_key),
		.key.bit_stream = cbc_key,
		.flags = CAP_RAW_KEY | CAP_SYNC_OPS | CAP_SEPARATE_IO_BUFS,
	};
	struct cipher_pkt pkt = {
		.in_buf = (uint8_t *)input,
		.in_len = sizeof(input),
		.out_buf_max = sizeof(decrypted),
		.out_buf = decrypted,
	};

	int rc = cipher_begin_session(crypto_dev, &ctx, CRYPTO_CIPHER_ALGO_AES,
				      CRYPTO_CIPHER_MODE_CBC, CRYPTO_CIPHER_OP_DECRYPT);

	if (rc == -ENOTSUP) {
		ztest_test_skip();
		return;
	}

	rc = cipher_cbc_op(&ctx, &pkt, input);
	if (rc != 0) {
		cipher_free_session(crypto_dev, &ctx);
		zassert_equal(rc, 0, "CBC decrypt failed (rc=%d)", rc);
		return;
	}

	rc = memcmp(decrypted, cbc_plaintext, sizeof(cbc_plaintext));
	cipher_free_session(crypto_dev, &ctx);
	zassert_equal(rc, 0, "CBC decrypt output mismatch");
}

/* CTR Mode Tests */
ZTEST(crypto_aes, test_ctr_encrypt)
{
	uint8_t encrypted[64] __aligned(IO_ALIGNMENT_BYTES) = {0};
	uint8_t iv_copy[12];

	memcpy(iv_copy, ctr_iv, sizeof(ctr_iv));

	struct cipher_ctx ctx = {
		.keylen = sizeof(ctr_key),
		.key.bit_stream = ctr_key,
		.flags = CAP_RAW_KEY | CAP_SYNC_OPS | CAP_SEPARATE_IO_BUFS,
		.mode_params.ctr_info.ctr_len = 32,
	};

	struct cipher_pkt pkt = {
		.in_buf = (uint8_t *)ctr_plaintext,
		.in_len = sizeof(ctr_plaintext),
		.out_buf_max = sizeof(encrypted),
		.out_buf = encrypted,
	};

	int rc = cipher_begin_session(crypto_dev, &ctx, CRYPTO_CIPHER_ALGO_AES,
				      CRYPTO_CIPHER_MODE_CTR, CRYPTO_CIPHER_OP_ENCRYPT);

	if (rc == -ENOTSUP) {
		ztest_test_skip();
		return;
	}

	rc = cipher_ctr_op(&ctx, &pkt, iv_copy);
	if (rc != 0) {
		cipher_free_session(crypto_dev, &ctx);
		zassert_equal(rc, 0, "CTR encrypt failed (rc=%d)", rc);
		return;
	}

	rc = memcmp(encrypted, ctr_ciphertext, sizeof(ctr_ciphertext));
	cipher_free_session(crypto_dev, &ctx);
	zassert_equal(rc, 0, "CTR encrypt output mismatch");
}

ZTEST(crypto_aes, test_ctr_decrypt)
{
	uint8_t decrypted[64] __aligned(IO_ALIGNMENT_BYTES) = {0};
	uint8_t iv_copy[12];

	memcpy(iv_copy, ctr_iv, sizeof(ctr_iv));

	struct cipher_ctx ctx = {
		.keylen = sizeof(ctr_key),
		.key.bit_stream = ctr_key,
		.flags = CAP_RAW_KEY | CAP_SYNC_OPS | CAP_SEPARATE_IO_BUFS,
		.mode_params.ctr_info.ctr_len = 32,
	};

	struct cipher_pkt pkt = {
		.in_buf = (uint8_t *)ctr_ciphertext,
		.in_len = sizeof(ctr_ciphertext),
		.out_buf_max = sizeof(decrypted),
		.out_buf = decrypted,
	};

	int rc = cipher_begin_session(crypto_dev, &ctx, CRYPTO_CIPHER_ALGO_AES,
				      CRYPTO_CIPHER_MODE_CTR, CRYPTO_CIPHER_OP_DECRYPT);

	if (rc == -ENOTSUP) {
		ztest_test_skip();
		return;
	}

	rc = cipher_ctr_op(&ctx, &pkt, iv_copy);
	if (rc != 0) {
		cipher_free_session(crypto_dev, &ctx);
		zassert_equal(rc, 0, "CTR decrypt failed (rc=%d)", rc);
		return;
	}

	rc = memcmp(decrypted, ctr_plaintext, sizeof(ctr_plaintext));
	cipher_free_session(crypto_dev, &ctx);
	zassert_equal(rc, 0, "CTR decrypt output mismatch");
}

/* CCM Mode Tests */
ZTEST(crypto_aes, test_ccm_encrypt)
{
	uint8_t encrypted[50] __aligned(IO_ALIGNMENT_BYTES) = {0};
	uint8_t nonce_copy[13];

	memcpy(nonce_copy, ccm_nonce, sizeof(ccm_nonce));

	struct cipher_ctx ctx = {
		.keylen = sizeof(ccm_key),
		.key.bit_stream = ccm_key,
		.mode_params.ccm_info = {
			.nonce_len = sizeof(ccm_nonce),
			.tag_len = 8,
		},
		.flags = CAP_RAW_KEY | CAP_SYNC_OPS | CAP_SEPARATE_IO_BUFS,
	};

	struct cipher_pkt pkt = {
		.in_buf = (uint8_t *)ccm_plaintext,
		.in_len = sizeof(ccm_plaintext),
		.out_buf_max = sizeof(encrypted),
		.out_buf = encrypted,
	};

	struct cipher_aead_pkt aead_pkt = {
		.ad = (uint8_t *)ccm_hdr,
		.ad_len = sizeof(ccm_hdr),
		.pkt = &pkt,
		.tag = encrypted + sizeof(ccm_plaintext),
	};

	int rc = cipher_begin_session(crypto_dev, &ctx, CRYPTO_CIPHER_ALGO_AES,
				      CRYPTO_CIPHER_MODE_CCM, CRYPTO_CIPHER_OP_ENCRYPT);

	if (rc == -ENOTSUP) {
		ztest_test_skip();
		return;
	}

	rc = cipher_ccm_op(&ctx, &aead_pkt, nonce_copy);
	if (rc != 0) {
		cipher_free_session(crypto_dev, &ctx);
		zassert_equal(rc, 0, "CCM encrypt failed (rc=%d)", rc);
		return;
	}

	rc = memcmp(encrypted, ccm_ciphertext, sizeof(ccm_ciphertext));
	cipher_free_session(crypto_dev, &ctx);
	zassert_equal(rc, 0, "CCM encrypt output mismatch");
}

ZTEST(crypto_aes, test_ccm_decrypt)
{
	uint8_t decrypted[32] __aligned(IO_ALIGNMENT_BYTES) = {0};
	uint8_t nonce_copy[13];
	uint8_t ciphertext_copy[31];

	memcpy(nonce_copy, ccm_nonce, sizeof(ccm_nonce));
	memcpy(ciphertext_copy, ccm_ciphertext, sizeof(ccm_ciphertext));

	struct cipher_ctx ctx = {
		.keylen = sizeof(ccm_key),
		.key.bit_stream = ccm_key,
		.mode_params.ccm_info = {
			.nonce_len = sizeof(ccm_nonce),
			.tag_len = 8,
		},
		.flags = CAP_RAW_KEY | CAP_SYNC_OPS | CAP_SEPARATE_IO_BUFS,
	};

	struct cipher_pkt pkt = {
		.in_buf = (uint8_t *)ciphertext_copy,
		.in_len = sizeof(ccm_plaintext),
		.out_buf_max = sizeof(decrypted),
		.out_buf = decrypted,
	};

	struct cipher_aead_pkt aead_pkt = {
		.ad = (uint8_t *)ccm_hdr,
		.ad_len = sizeof(ccm_hdr),
		.pkt = &pkt,
		.tag = ciphertext_copy + sizeof(ccm_plaintext),
	};

	int rc = cipher_begin_session(crypto_dev, &ctx, CRYPTO_CIPHER_ALGO_AES,
				      CRYPTO_CIPHER_MODE_CCM, CRYPTO_CIPHER_OP_DECRYPT);

	if (rc == -ENOTSUP) {
		ztest_test_skip();
		return;
	}

	rc = cipher_ccm_op(&ctx, &aead_pkt, nonce_copy);
	if (rc != 0) {
		cipher_free_session(crypto_dev, &ctx);
		zassert_equal(rc, 0, "CCM decrypt failed (rc=%d)", rc);
		return;
	}

	rc = memcmp(decrypted, ccm_plaintext, sizeof(ccm_plaintext));
	cipher_free_session(crypto_dev, &ctx);
	zassert_equal(rc, 0, "CCM decrypt output mismatch");
}

/* GCM Mode Tests */
ZTEST(crypto_aes, test_gcm_encrypt)
{
	uint8_t encrypted[60] __aligned(IO_ALIGNMENT_BYTES) = {0};
	uint8_t nonce_copy[12];

	memcpy(nonce_copy, gcm_nonce, sizeof(gcm_nonce));

	struct cipher_ctx ctx = {
		.keylen = sizeof(gcm_key),
		.key.bit_stream = gcm_key,
		.mode_params.gcm_info = {
			.nonce_len = sizeof(gcm_nonce),
			.tag_len = 16,
		},
		.flags = CAP_RAW_KEY | CAP_SYNC_OPS | CAP_SEPARATE_IO_BUFS,
	};

	struct cipher_pkt pkt = {
		.in_buf = (uint8_t *)gcm_plaintext,
		.in_len = sizeof(gcm_plaintext),
		.out_buf_max = sizeof(encrypted),
		.out_buf = encrypted,
	};

	struct cipher_aead_pkt aead_pkt = {
		.ad = (uint8_t *)gcm_hdr,
		.ad_len = sizeof(gcm_hdr),
		.pkt = &pkt,
		.tag = encrypted + sizeof(gcm_plaintext),
	};

	int rc = cipher_begin_session(crypto_dev, &ctx, CRYPTO_CIPHER_ALGO_AES,
				      CRYPTO_CIPHER_MODE_GCM, CRYPTO_CIPHER_OP_ENCRYPT);

	if (rc == -ENOTSUP) {
		ztest_test_skip();
		return;
	}

	rc = cipher_gcm_op(&ctx, &aead_pkt, nonce_copy);
	if (rc != 0) {
		cipher_free_session(crypto_dev, &ctx);
		zassert_equal(rc, 0, "GCM encrypt failed (rc=%d)", rc);
		return;
	}

	rc = memcmp(encrypted, gcm_ciphertext, sizeof(gcm_ciphertext));
	cipher_free_session(crypto_dev, &ctx);
	zassert_equal(rc, 0, "GCM encrypt output mismatch");
}

ZTEST(crypto_aes, test_gcm_decrypt)
{
	uint8_t decrypted[44] __aligned(IO_ALIGNMENT_BYTES) = {0};
	uint8_t nonce_copy[12];
	uint8_t ciphertext_copy[58];

	memcpy(nonce_copy, gcm_nonce, sizeof(gcm_nonce));
	memcpy(ciphertext_copy, gcm_ciphertext, sizeof(gcm_ciphertext));

	struct cipher_ctx ctx = {
		.keylen = sizeof(gcm_key),
		.key.bit_stream = gcm_key,
		.mode_params.gcm_info = {
			.nonce_len = sizeof(gcm_nonce),
			.tag_len = 16,
		},
		.flags = CAP_RAW_KEY | CAP_SYNC_OPS | CAP_SEPARATE_IO_BUFS,
	};

	struct cipher_pkt pkt = {
		.in_buf = (uint8_t *)ciphertext_copy,
		.in_len = sizeof(gcm_plaintext),
		.out_buf_max = sizeof(decrypted),
		.out_buf = decrypted,
	};
	struct cipher_aead_pkt aead_pkt = {
		.ad = (uint8_t *)gcm_hdr,
		.ad_len = sizeof(gcm_hdr),
		.pkt = &pkt,
		.tag = ciphertext_copy + sizeof(gcm_plaintext),
	};

	int rc = cipher_begin_session(crypto_dev, &ctx, CRYPTO_CIPHER_ALGO_AES,
				      CRYPTO_CIPHER_MODE_GCM, CRYPTO_CIPHER_OP_DECRYPT);

	if (rc == -ENOTSUP) {
		ztest_test_skip();
		return;
	}

	rc = cipher_gcm_op(&ctx, &aead_pkt, nonce_copy);
	if (rc != 0) {
		cipher_free_session(crypto_dev, &ctx);
		zassert_equal(rc, 0, "GCM decrypt failed (rc=%d)", rc);
		return;
	}

	rc = memcmp(decrypted, gcm_plaintext, sizeof(gcm_plaintext));
	cipher_free_session(crypto_dev, &ctx);
	zassert_equal(rc, 0, "GCM decrypt output mismatch");
}

ZTEST_SUITE(crypto_aes, NULL, crypto_aes_setup, crypto_aes_before, NULL, NULL);
