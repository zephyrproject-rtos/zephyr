/* test_ccm_mode.c - TinyCrypt AES-CCM tests (RFC 3610 tests) */

/*
 *  Copyright (C) 2015 by Intel Corporation, All Rights Reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *    - Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *    - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 *    - Neither the name of Intel Corporation nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  DESCRIPTION
 * This module tests the following AES-CCM Mode routines:
 *
 *  Scenarios tested include:
 *  - AES128 CCM mode encryption RFC 3610 test vector #1
 *  - AES128 CCM mode encryption RFC 3610 test vector #2
 *  - AES128 CCM mode encryption RFC 3610 test vector #3
 *  - AES128 CCM mode encryption RFC 3610 test vector #7
 *  - AES128 CCM mode encryption RFC 3610 test vector #8
 *  - AES128 CCM mode encryption RFC 3610 test vector #9
 *  - AES128 CCM mode encryption No associated data
 * - AES128 CCM mode encryption No payhoad data
 */

#include <tinycrypt/ccm_mode.h>
#include <tinycrypt/constants.h>
#include <test_utils.h>
#include <ztest.h>

#include <string.h>

#define CIPHERTEXT_LEN 50
#define DECRYPTED_LEN 25
#define NUM_NIST_KEYS 16
#define NONCE_LEN 13
#define HEADER_LEN 8
#define M_LEN8 8
#define M_LEN10 10
#define DATA_BUF_LEN23 23
#define DATA_BUF_LEN24 24
#define DATA_BUF_LEN25 25
#define EXPECTED_BUF_LEN31 31
#define EXPECTED_BUF_LEN32 32
#define EXPECTED_BUF_LEN33 33
#define EXPECTED_BUF_LEN34 34
#define EXPECTED_BUF_LEN35 35

uint32_t do_test(const uint8_t *key,
		 uint8_t *nonce, size_t nlen,
		 const uint8_t *hdr, size_t hlen,
		 const uint8_t *data, size_t dlen,
		 const uint8_t *expected, size_t elen,
		 const int mlen)
{
	uint32_t result = TC_PASS;
	uint8_t ciphertext[CIPHERTEXT_LEN];
	uint8_t decrypted[DECRYPTED_LEN];
	struct tc_ccm_mode_struct c;
	struct tc_aes_key_sched_struct sched;

	tc_aes128_set_encrypt_key(&sched, key);

	result = tc_ccm_config(&c, &sched, nonce, nlen, mlen);

	/**TESTPOINT: Check CCM config*/
	zassert_true(result, "CCM config failed");

	result = tc_ccm_generation_encryption(ciphertext, sizeof(ciphertext),
					      hdr, hlen, data, dlen, &c);

	/**TESTPOINT: Check CCM encrypt*/
	zassert_true(result, "ccm_encrypt failed");

	/**TESTPOINT: Verify ciphertext*/
	if (memcmp(expected, ciphertext, elen) != 0) {
		show_str("\t\tExpected", expected, elen);
		show_str("\t\tComputed", ciphertext, elen);

		/**ASSERTION: Signal wrong output and assert*/
		zassert_true(0, "ccm_encrypt produced wrong ciphertext");
	}

	result = tc_ccm_decryption_verification(decrypted, sizeof(decrypted),
						hdr, hlen, ciphertext,
						dlen + mlen, &c);

	/**TESTPOINT: Check decryption*/
	if (result == 0U) {
		show_str("\t\tExpected", data, dlen);
		show_str("\t\tComputed", decrypted, sizeof(decrypted));

		/**ASSERTION: Decrypt failed, so exit by assert*/
		zassert_true(0, "ccm_decrypt failed");
	}

	result = TC_PASS;
	return result;
}

void test_ccm_vector_1(void)
{
	uint32_t result = TC_PASS;
	/* RFC 3610 test vector #1 */
	const uint8_t key[NUM_NIST_KEYS] = {
		0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
		0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf
	};
	uint8_t nonce[NONCE_LEN] = {
		0x00, 0x00, 0x00, 0x03, 0x02, 0x01, 0x00, 0xa0,
		0xa1, 0xa2, 0xa3, 0xa4, 0xa5
	};
	const uint8_t hdr[HEADER_LEN] = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07
	};
	const uint8_t data[DATA_BUF_LEN23] = {
		0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
		0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e
	};
	const uint8_t expected[EXPECTED_BUF_LEN31] = {
		0x58, 0x8c, 0x97, 0x9a, 0x61, 0xc6, 0x63, 0xd2,
		0xf0, 0x66, 0xd0, 0xc2, 0xc0, 0xf9, 0x89, 0x80,
		0x6d, 0x5f, 0x6b, 0x61, 0xda, 0xc3, 0x84, 0x17,
		0xe8, 0xd1, 0x2c, 0xfd, 0xf9, 0x26, 0xe0
	};
	uint16_t mlen = M_LEN8;

	TC_PRINT("%s: Performing CCM test #1 (RFC 3610 test vector #1):\n",
		 __func__);

	result = do_test(key, nonce, sizeof(nonce), hdr, sizeof(hdr),
			 data, sizeof(data), expected, sizeof(expected), mlen);

	/**TESTPOINT: Check result*/
	zassert_false(result, "CCM test #1 (RFC 3610 test vector #1) failed.");
}

void test_ccm_vector_2(void)
{
	uint32_t result = TC_PASS;
	/* RFC 3610 test vector #2 */
	const uint8_t key[NUM_NIST_KEYS] = {
		0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
		0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf
	};
	uint8_t nonce[NONCE_LEN] = {
		0x00, 0x00, 0x00, 0x04, 0x03, 0x02, 0x01, 0xa0,
		0xa1, 0xa2, 0xa3, 0xa4, 0xa5
	};
	const uint8_t hdr[HEADER_LEN] = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07
	};
	const uint8_t data[DATA_BUF_LEN24] = {
		0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
		0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
	};
	const uint8_t expected[EXPECTED_BUF_LEN32] = {
		0x72, 0xc9, 0x1a, 0x36, 0xe1, 0x35, 0xf8, 0xcf,
		0x29, 0x1c, 0xa8, 0x94, 0x08, 0x5c, 0x87, 0xe3,
		0xcc, 0x15, 0xc4, 0x39, 0xc9, 0xe4, 0x3a, 0x3b,
		0xa0, 0x91, 0xd5, 0x6e, 0x10, 0x40, 0x09, 0x16
	};
	uint16_t mlen = M_LEN8;

	TC_PRINT("%s: Performing CCM test #2 (RFC 3610 test vector #2):\n",
		 __func__);

	result = do_test(key, nonce, sizeof(nonce), hdr, sizeof(hdr),
			 data, sizeof(data), expected, sizeof(expected), mlen);

	/**TESTPOINT: Check result*/
	zassert_false(result, "CCM test #2 failed.");
}

void test_ccm_vector_3(void)
{
	uint32_t result = TC_PASS;
	/* RFC 3610 test vector #3  */
	const uint8_t key[NUM_NIST_KEYS] = {
		0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
		0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf
	};
	uint8_t nonce[NONCE_LEN] = {
		0x00, 0x00, 0x00, 0x05, 0x04, 0x03, 0x02, 0xa0,
		0xa1, 0xa2, 0xa3, 0xa4, 0xa5
	};
	const uint8_t hdr[HEADER_LEN] = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07
	};
	const uint8_t data[DATA_BUF_LEN25] = {
		0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
		0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
		0x20
	};
	const uint8_t expected[EXPECTED_BUF_LEN33] = {
		0x51, 0xb1, 0xe5, 0xf4, 0x4a, 0x19, 0x7d, 0x1d,
		0xa4, 0x6b, 0x0f, 0x8e, 0x2d, 0x28, 0x2a, 0xe8,
		0x71, 0xe8, 0x38, 0xbb, 0x64, 0xda, 0x85, 0x96,
		0x57, 0x4a, 0xda, 0xa7, 0x6f, 0xbd, 0x9f, 0xb0,
		0xc5
	};
	uint16_t mlen = M_LEN8;

	TC_PRINT("%s: Performing CCM test #3 (RFC 3610 test vector #3):\n",
		 __func__);

	result = do_test(key, nonce, sizeof(nonce), hdr, sizeof(hdr), data,
			 sizeof(data), expected, sizeof(expected), mlen);

	/**TESTPOINT: Check result*/
	zassert_false(result, "CCM test #3 failed.");
}

void test_ccm_vector_4(void)
{
	uint32_t result = TC_PASS;
	/* RFC 3610 test vector #7  */
	const uint8_t key[NUM_NIST_KEYS] = {
		0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
		0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf
	};
	uint8_t nonce[NONCE_LEN] = {
		0x00, 0x00, 0x00, 0x09, 0x08, 0x07, 0x06, 0xa0,
		0xa1, 0xa2, 0xa3, 0xa4, 0xa5
	};
	const uint8_t hdr[HEADER_LEN] = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07
	};
	const uint8_t data[DATA_BUF_LEN23] = {
		0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
		0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e
	};
	const uint8_t expected[EXPECTED_BUF_LEN33] = {
		0x01, 0x35, 0xD1, 0xB2,  0xC9, 0x5F, 0x41, 0xD5,
		0xD1, 0xD4, 0xFE, 0xC1,  0x85, 0xD1, 0x66, 0xB8,
		0x09, 0x4E, 0x99, 0x9D,  0xFE, 0xD9, 0x6C, 0x04,
		0x8C, 0x56, 0x60, 0x2C,  0x97, 0xAC, 0xBB, 0x74,
		0x90
	};
	uint16_t mlen = M_LEN10;

	TC_PRINT("%s: Performing CCM test #4 (RFC 3610 test vector #7):\n",
		 __func__);

	result = do_test(key, nonce, sizeof(nonce), hdr, sizeof(hdr),
			 data, sizeof(data), expected, sizeof(expected), mlen);

	/**TESTPOINT: Check result*/
	zassert_false(result, "CCM test #4 failed.");
}

void test_ccm_vector_5(void)
{
	uint32_t result = TC_PASS;
	/* RFC 3610 test vector #8  */
	const uint8_t key[NUM_NIST_KEYS] = {
		0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
		0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF
	};
	uint8_t nonce[NONCE_LEN] = {
		0x00, 0x00, 0x00, 0x0A, 0x09, 0x08, 0x07, 0xA0,
		0xA1, 0xA2, 0xA3, 0xA4, 0xA5
	};
	const uint8_t hdr[HEADER_LEN] = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07
	};
	const uint8_t data[DATA_BUF_LEN24] = {
		0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
		0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
	};
	const uint8_t expected[EXPECTED_BUF_LEN34] = {
		0x7B, 0x75, 0x39, 0x9A, 0xC0, 0x83, 0x1D, 0xD2,
		0xF0, 0xBB, 0xD7, 0x58, 0x79, 0xA2, 0xFD, 0x8F,
		0x6C, 0xAE, 0x6B, 0x6C, 0xD9, 0xB7, 0xDB, 0x24,
		0xC1, 0x7B, 0x44, 0x33, 0xF4, 0x34, 0x96, 0x3F,
		0x34, 0xB4
	};
	uint16_t mlen = M_LEN10;

	TC_PRINT("%s: Performing CCM test #5 (RFC 3610 test vector #8):\n",
		 __func__);

	result = do_test(key, nonce, sizeof(nonce), hdr, sizeof(hdr),
			 data, sizeof(data), expected, sizeof(expected), mlen);

	/**TESTPOINT: Check result*/
	zassert_false(result, "CCM test #5 failed.");
}

void test_ccm_vector_6(void)
{
	uint32_t result = TC_PASS;
	/* RFC 3610 test vector #9  */
	const uint8_t key[NUM_NIST_KEYS] = {
		0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
		0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF
	};
	uint8_t nonce[NONCE_LEN] = {
		0x00, 0x00, 0x00, 0x0B, 0x0A, 0x09, 0x08, 0xA0,
		0xA1, 0xA2, 0xA3, 0xA4, 0xA5
	};
	const uint8_t hdr[HEADER_LEN] = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07
	};
	const uint8_t data[DATA_BUF_LEN25] = {
		0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
		0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
		0x20
	};
	const uint8_t expected[EXPECTED_BUF_LEN35] = {
		0x82, 0x53, 0x1a, 0x60, 0xCC, 0x24, 0x94, 0x5a,
		0x4b, 0x82, 0x79, 0x18, 0x1a, 0xb5, 0xc8, 0x4d,
		0xf2, 0x1c, 0xe7, 0xf9, 0xb7, 0x3f, 0x42, 0xe1,
		0x97, 0xea, 0x9c, 0x07, 0xe5, 0x6b, 0x5e, 0xb1,
		0x7e, 0x5f, 0x4e
	};
	uint16_t mlen = M_LEN10;

	TC_PRINT("%s: Performing CCM test #6 (RFC 3610 test vector #9):\n",
		 __func__);

	result = do_test(key, nonce, sizeof(nonce), hdr, sizeof(hdr),
			 data, sizeof(data), expected, sizeof(expected), mlen);

	/**TESTPOINT: Check result*/
	zassert_false(result, "CCM test #6 failed.");
}

void test_ccm_vector_7(void)
{
	uint32_t result = TC_PASS;
	/* Test based on RFC 3610 test vector #9 but with no associated data */
	const uint8_t key[NUM_NIST_KEYS] = {
		0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
		0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF
	};
	uint8_t nonce[NONCE_LEN] = {
		0x00, 0x00, 0x00, 0x0B, 0x0A, 0x09, 0x08, 0xA0,
		0xA1, 0xA2, 0xA3, 0xA4, 0xA5
	};
	uint8_t *hdr = NULL;

	uint8_t data[DATA_BUF_LEN25] = {
		0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
		0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
		0x20
	};
	struct tc_ccm_mode_struct c;
	struct tc_aes_key_sched_struct sched;
	uint8_t decrypted[DECRYPTED_LEN];
	uint8_t ciphertext[CIPHERTEXT_LEN];
	uint16_t mlen = M_LEN10;

	TC_PRINT("%s: Performing CCM test #7 (no associated data):\n",
		 __func__);

	tc_aes128_set_encrypt_key(&sched, key);

	/**TESTPOINT: Check CCM configuration*/
	zassert_true(tc_ccm_config(&c, &sched, nonce, sizeof(nonce), mlen),
		"ccm_config failed");

	result = tc_ccm_generation_encryption(ciphertext, sizeof(ciphertext),
					      hdr, 0, data, sizeof(data), &c);
	/**TESTPOINT: Check CCM encryption*/
	zassert_true(result, "ccm_encryption failed");

	result = tc_ccm_decryption_verification(decrypted, sizeof(decrypted),
						hdr, 0, ciphertext,
						sizeof(data) + mlen, &c);

	if (result == 0U) {
		TC_ERROR("ccm_decrypt failed in %s.\n", __func__);
		show_str("\t\tExpected", data, sizeof(data));
		show_str("\t\tComputed", decrypted, sizeof(decrypted));

		/**TESTPOINT: Check CCM decryption, and assert*/
		zassert_true(result, "ccm_decryption failed");

	}

	result = TC_PASS;

	/**TESTPOINT: Check result*/
	zassert_false(result, "CCM test #7 failed.");

}

void test_ccm_vector_8(void)
{
	uint32_t result = TC_PASS;
	/* Test based on RFC 3610 test vector #9 but with no payload data */
	const uint8_t key[NUM_NIST_KEYS] = {
		0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
		0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF
	};
	uint8_t nonce[NONCE_LEN] = {
		0x00, 0x00, 0x00, 0x0B, 0x0A, 0x09, 0x08, 0xA0,
		0xA1, 0xA2, 0xA3, 0xA4, 0xA5
	};
	const uint8_t hdr[8] = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07
	};
	uint8_t data[] = {};
	struct tc_ccm_mode_struct c;
	struct tc_aes_key_sched_struct sched;
	uint8_t decrypted[DECRYPTED_LEN];
	uint8_t ciphertext[CIPHERTEXT_LEN];
	uint16_t mlen = M_LEN10;

	TC_PRINT("%s: Performing CCM test #8 (no payload data):\n", __func__);

	tc_aes128_set_encrypt_key(&sched, key);

	/**TESTPOINT: Check CCM configuration*/
	zassert_true(tc_ccm_config(&c, &sched, nonce, sizeof(nonce), mlen),
	"CCM config failed");

	result = tc_ccm_generation_encryption(ciphertext, sizeof(ciphertext),
					      hdr, sizeof(hdr), data,
					      sizeof(data), &c);
	/**TESTPOINT: Check CCM encryption*/
	zassert_true(result, "ccm_encrypt failed");

	result = tc_ccm_decryption_verification(decrypted, sizeof(decrypted),
						hdr, sizeof(hdr),
						ciphertext, mlen, &c);
	/**TESTPOINT: Check CCM decryption*/
	if (result == 0U) {
		show_str("\t\tExpected", data, sizeof(data));
		show_str("\t\tComputed", decrypted, sizeof(decrypted));

		/**ASSERTION: Decrypt failed, so exit by assert*/
		zassert_true(0, "ccm_decrypt failed");
	}

	result = TC_PASS;
	TC_END_RESULT(result);

	/**TESTPOINT: Check result*/
	zassert_false(result, "CCM test #8 (no payload data) failed.");
}
