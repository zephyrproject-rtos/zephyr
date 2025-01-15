/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <zephyr/bluetooth/ead.h>
#include <zephyr/bluetooth/bluetooth.h>

#include <zephyr/logging/log.h>

#include "common/bt_str.h"

#include "bs_tracing.h"
#include "bstests.h"

/**
 * @brief Encrypt and authenticate the given advertising data.
 *
 * This is the same function as @ref bt_ead_encrypt except this one adds the @p
 * randomizer parameter to let the user set the randomizer value.
 *
 * @note This function should only be used for testing purposes, it is only
 * available when @kconfig{CONFIG_BT_TESTING} is enabled.
 *
 * @param[in]  session_key Key of @ref BT_EAD_KEY_SIZE bytes used for the
 *             encryption.
 * @param[in]  iv Initialisation Vector used to generate the nonce. It must be
 *             changed each time the Session Key changes.
 * @param[in]  randomizer Randomizer value used to generate the nonce. The value
 *             is also placed in front of the encrypted advertising data.
 * @param[in]  payload Advertising Data to encrypt. Can be multiple advertising
 *             structures that are concatenated.
 * @param[in]  payload_size Size of the Advertising Data to encrypt.
 * @param[out] encrypted_payload Encrypted Ad Data including the Randomizer and
 *             the MIC. Size must be at least @ref BT_EAD_RANDOMIZER_SIZE + @p
 *             payload_size + @ref BT_EAD_MIC_SIZE. Use @ref
 *             BT_EAD_ENCRYPTED_PAYLOAD_SIZE to get the right size.
 *
 * @retval 0 Data have been correctly encrypted and authenticated.
 * @retval -EIO Error occurred during the encryption or the authentication.
 * @retval -EINVAL One of the argument is a NULL pointer.
 */
int bt_test_ead_encrypt(const uint8_t session_key[BT_EAD_KEY_SIZE],
			const uint8_t iv[BT_EAD_IV_SIZE],
			const uint8_t randomizer[BT_EAD_RANDOMIZER_SIZE], const uint8_t *payload,
			size_t payload_size, uint8_t *encrypted_payload);

LOG_MODULE_DECLARE(bt_bsim_ead_sample_data, CONFIG_BT_EAD_LOG_LEVEL);

struct test_sample_data {
	const uint8_t session_key[BT_EAD_KEY_SIZE];
	const uint8_t iv[BT_EAD_IV_SIZE];
	const uint8_t randomizer_little_endian[BT_EAD_RANDOMIZER_SIZE];
	const uint8_t *ad_data;
	const size_t size_ad_data;
	const uint8_t *ead;
	const size_t size_ead;
};

/* Encrypted Advertising Data Set 1 (ref: Supplement to the Bluetooth Core
 * Specification v11, Part A, 2.3.1)
 */

#define SIZE_SAMPLE_AD_DATA_1 20
static const uint8_t sample_ad_data_1[] = {0x0F, 0x09, 0x53, 0x68, 0x6F, 0x72, 0x74,
					   0x20, 0x4D, 0x69, 0x6E, 0x69, 0x2D, 0x42,
					   0x75, 0x73, 0x03, 0x19, 0x0A, 0x8C};
BUILD_ASSERT(sizeof(sample_ad_data_1) == SIZE_SAMPLE_AD_DATA_1);

#define SIZE_SAMPLE_EAD_1 29
static const uint8_t sample_ead_1[] = {
	0x18, 0xE1, 0x57, 0xCA, 0xDE, 0x74, 0xE4, 0xDC, 0xAF, 0xDC, 0x51, 0xC7, 0x28, 0x28, 0x10,
	0xC2, 0x21, 0x7F, 0x0E, 0x4C, 0xEF, 0x43, 0x43, 0x18, 0x1F, 0xBA, 0x00, 0x69, 0xCC,
};
BUILD_ASSERT(sizeof(sample_ead_1) == SIZE_SAMPLE_EAD_1);

static const struct test_sample_data sample_data_1 = {
	.session_key = {0x57, 0xA9, 0xDA, 0x12, 0xD1, 0x2E, 0x6E, 0x13, 0x1E, 0x20, 0x61, 0x2A,
			0xD1, 0x0A, 0x6A, 0x19},
	.iv = {0x9E, 0x7A, 0x00, 0xEF, 0xB1, 0x7A, 0xE7, 0x46},
	.randomizer_little_endian = {0x18, 0xE1, 0x57, 0xCA, 0xDE},
	.ad_data = sample_ad_data_1,
	.size_ad_data = SIZE_SAMPLE_AD_DATA_1,
	.ead = sample_ead_1,
	.size_ead = SIZE_SAMPLE_EAD_1,
};

/* Encrypted Advertising Data Set 2 (ref: Supplement to the Bluetooth Core
 * Specification v11, Part A, 2.3.2)
 */

#define SIZE_SAMPLE_AD_DATA_2 20
static const uint8_t sample_ad_data_2[] = {0x0F, 0x09, 0x53, 0x68, 0x6F, 0x72, 0x74,
					   0x20, 0x4D, 0x69, 0x6E, 0x69, 0x2D, 0x42,
					   0x75, 0x73, 0x03, 0x19, 0x0A, 0x8C};
BUILD_ASSERT(sizeof(sample_ad_data_2) == SIZE_SAMPLE_AD_DATA_2);

#define SIZE_SAMPLE_EAD_2 29
static const uint8_t sample_ead_2[] = {0x8d, 0x1c, 0x97, 0x6e, 0x7a, 0x35, 0x44, 0x40, 0x76, 0x12,
				       0x57, 0x88, 0xc2, 0x38, 0xa5, 0x8e, 0x8b, 0xd9, 0xcf, 0xf0,
				       0xde, 0xfe, 0x25, 0x1a, 0x8e, 0x72, 0x75, 0x45, 0x4c};
BUILD_ASSERT(sizeof(sample_ead_2) == SIZE_SAMPLE_EAD_2);

static const struct test_sample_data sample_data_2 = {
	.session_key = {0x57, 0xA9, 0xDA, 0x12, 0xD1, 0x2E, 0x6E, 0x13, 0x1E, 0x20, 0x61, 0x2A,
			0xD1, 0x0A, 0x6A, 0x19},
	.iv = {0x9E, 0x7A, 0x00, 0xEF, 0xB1, 0x7A, 0xE7, 0x46},
	.randomizer_little_endian = {0x8D, 0x1C, 0x97, 0x6E, 0x7A},
	.ad_data = sample_ad_data_2,
	.size_ad_data = SIZE_SAMPLE_AD_DATA_2,
	.ead = sample_ead_2,
	.size_ead = SIZE_SAMPLE_EAD_2,
};
