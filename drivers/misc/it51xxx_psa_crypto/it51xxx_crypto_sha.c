/*
 * Copyright (c) 2026 ITE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "it51xxx_crypto.h"
#include "it51xxx_crypto_sha.h"

#include <soc_common.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>
#define LOG_LEVEL CONFIG_IT51XXX_PSA_CRYPTO_LOG_LEVEL
LOG_MODULE_REGISTER(it51xxx_psa_crypto_sha);

#define DT_DRV_COMPAT    ite_it51xxx_psa_sha
#define IT51XXX_SHA_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(DT_DRV_COMPAT)
BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "it51xxx: supported one psa-crypto-sha node only");

#define IT51XXX_SHA_IRQ DT_IRQN(DT_PARENT(IT51XXX_SHA_NODE))

#define SHA00_CTRL       0x00
#define SHA_WRITE_BACK   BIT(2)
#define SHA_INIT         BIT(1)
#define SHA_LOAD_AND_EXE BIT(0)

#define SHA01_STATUS 0x01
#define INT_ENABLE   BIT(3)
#define INT_STATUS   BIT(2)
#define SHA_BUSY     BIT(0)

#define SHA02_EXECUTION_COUNTER      0x02
#define SHA224_SHA256_EXE_ONCE_BYTES 64

#define SHA03_BASE_ADDR_0 0x03
#define BASE_ADDR_0_MASK  GENMASK(7, 6)
#define BASE_ADDR_0(x)    ((uint8_t)((x) & BASE_ADDR_0_MASK))

#define SHA04_BASE_ADDR_1 0x04
#define BASE_ADDR_1(x)    ((uint8_t)FIELD_GET(GENMASK(15, 8), (x)))

#define SHA05_SETTING    0x05
#define SELECT_SHA2_MASK GENMASK(2, 1)
#define SELECT_SHA2(x)   FIELD_PREP(SELECT_SHA2_MASK, x)

#define SHA06_BASE_ADDR_2 0x06
#define BASE_ADDR_2(x)    ((uint8_t)FIELD_GET(GENMASK(17, 16), (x)))

#define IT51XXX_SHA256_BLOCK_LEN 64

#define IT51XXX_SHA_INIT 1U

#define sha256_dlm hwcrypto_dlm.sha256

enum it51xxx_sha2_algorithm {
	IT51XXX_SHA2_SHA256 = 0,
	IT51XXX_SHA2_SHA512,
	IT51XXX_SHA2_SHA224,
	IT51XXX_SHA2_SHA384,
};

static const uint8_t sha224_sha256_supported_execute_times[] = {1U, 8U, 16U};

struct k_sem sha_cal_done;

static void it51xxx_sha_isr(const struct device *dev)
{
	const mm_reg_t base = it51xxx_crypto_get_base();
	uint8_t sha_sts = sys_read8(base + SHA01_STATUS);

	if (sha_sts & INT_STATUS) {
		k_sem_give(&sha_cal_done);
		sys_write8(sha_sts | INT_STATUS, base + SHA01_STATUS);
	}
}

static int it51xxx_sha_set_algorithm(psa_algorithm_t alg)
{
	const mm_reg_t base = it51xxx_crypto_get_base();

	/* it51xxx psa-crypto-sha driver currently supports only sha256 */
	switch (alg) {
	case PSA_ALG_SHA_256:
		sys_write8(SELECT_SHA2(IT51XXX_SHA2_SHA256), base + SHA05_SETTING);
		break;
	case PSA_ALG_SHA_224:
		__fallthrough;
	case PSA_ALG_SHA_384:
		__fallthrough;
	case PSA_ALG_SHA_512:
		__fallthrough;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static void it51xxx_sha256_init(void)
{
	const mm_reg_t base = it51xxx_crypto_get_base();

	sha256_dlm.sha_init = IT51XXX_SHA_INIT;
	sha256_dlm.total_len = 0;
	sha256_dlm.input_bytes_index = 0;

	sys_write8(BASE_ADDR_0((uintptr_t)&sha256_dlm), base + SHA03_BASE_ADDR_0);
	sys_write8(BASE_ADDR_1((uintptr_t)&sha256_dlm), base + SHA04_BASE_ADDR_1);
	sys_write8(BASE_ADDR_2((uintptr_t)&sha256_dlm), base + SHA06_BASE_ADDR_2);
}

static int it51xxx_sha256_module_calculation(const uint16_t calculation_bytes)
{
	const mm_reg_t base = it51xxx_crypto_get_base();
	uint8_t reg_val;

	for (size_t i = 0; i <= ARRAY_SIZE(sha224_sha256_supported_execute_times); i++) {
		if (i == ARRAY_SIZE(sha224_sha256_supported_execute_times)) {
			LOG_ERR("unsupported calculation bytes %d", calculation_bytes);
			return -ENOTSUP;
		}

		if (calculation_bytes ==
		    SHA224_SHA256_EXE_ONCE_BYTES * sha224_sha256_supported_execute_times[i]) {
			reg_val = sha224_sha256_supported_execute_times[i] - 1;
			break;
		}
	}

	/* load data from dlm */
	sys_write8(reg_val, base + SHA02_EXECUTION_COUNTER);
	while (sys_read8(base + SHA01_STATUS) & SHA_BUSY) {
		/* do nothing and wait until the sha status is idle */
	}

	reg_val = 0;

	if (sha256_dlm.sha_init) {
		sha256_dlm.sha_init = 0;
		reg_val |= SHA_INIT;
	}
	sys_write8(reg_val | SHA_LOAD_AND_EXE, base + SHA00_CTRL);
	k_sem_take(&sha_cal_done, K_FOREVER);

	sha256_dlm.input_bytes_index = 0;

	return 0;
}

static void it51xxx_write_back_hash_result(uint8_t *hash, size_t *hash_length)
{
	const mm_reg_t base = it51xxx_crypto_get_base();

	sys_write8(BASE_ADDR_0((uintptr_t)&sha256_dlm.hash), base + SHA03_BASE_ADDR_0);
	sys_write8(BASE_ADDR_1((uintptr_t)&sha256_dlm.hash), base + SHA04_BASE_ADDR_1);
	sys_write8(BASE_ADDR_2((uintptr_t)&sha256_dlm.hash), base + SHA06_BASE_ADDR_2);

	/* write back the hash result to dlm */
	sys_write8(SHA_WRITE_BACK, base + SHA00_CTRL);
	k_sem_take(&sha_cal_done, K_FOREVER);

	memcpy(hash, sha256_dlm.hash, sizeof(sha256_dlm.hash));
	*hash_length = sizeof(sha256_dlm.hash);
}

static void it51xxx_sha_calculate_hash(const uint8_t *input, size_t input_length, uint8_t *hash,
				       size_t *hash_length)
{
	size_t remaining = input_length;
	uint16_t calculation_bytes;
	uint64_t bit_length = (uint64_t)input_length * 8U;

	it51xxx_sha256_init();

	/* process complete blocks */
	while (remaining >= IT51XXX_SHA256_BLOCK_LEN) {
		if (remaining >= 1024U) {
			calculation_bytes = 1024U;
		} else if (remaining >= 512U) {
			calculation_bytes = 512U;
		} else {
			calculation_bytes = IT51XXX_SHA256_BLOCK_LEN;
		}

		memcpy(sha256_dlm.input_bytes, input, calculation_bytes);
		sha256_dlm.input_bytes_index = calculation_bytes;

		it51xxx_sha256_module_calculation(calculation_bytes);

		input += calculation_bytes;
		remaining -= calculation_bytes;
	}

	/* copy the remaining partial block */
	if (remaining > 0U) {
		memcpy(sha256_dlm.input_bytes, input, remaining);
	}

	sha256_dlm.input_bytes_index = remaining;
	sha256_dlm.total_len = input_length;

	/* append the padding bit and zero-fill the rest of the block. */
	memset(sha256_dlm.input_bytes + remaining, 0, IT51XXX_SHA256_BLOCK_LEN - remaining);
	sha256_dlm.input_bytes[remaining] = 0x80;

	/*
	 * If the length field does not fit, process the padded block
	 * and clear another block for the length field.
	 */
	if (remaining >= IT51XXX_SHA256_BLOCK_LEN - 8U) {
		it51xxx_sha256_module_calculation(IT51XXX_SHA256_BLOCK_LEN);
		memset(sha256_dlm.input_bytes, 0, IT51XXX_SHA256_BLOCK_LEN);
	}

	/* store the 64-bit message length in bits in big-endian order. */
	sys_put_be64(bit_length, sha256_dlm.input_bytes + IT51XXX_SHA256_BLOCK_LEN - 8U);

	it51xxx_sha256_module_calculation(IT51XXX_SHA256_BLOCK_LEN);

	it51xxx_write_back_hash_result(hash, hash_length);
}

psa_status_t ite_it51xxx_hash_compute(psa_algorithm_t alg, const uint8_t *input,
				      size_t input_length, uint8_t *hash, size_t hash_size,
				      size_t *hash_length)
{
	size_t actual_hash_length = PSA_HASH_LENGTH(alg);

	if (!hash || !hash_size) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	/* Fill the output buffer with something that isn't a valid hash
	 * (barring an attack on the hash and deliberately-crafted input),
	 * in case the caller doesn't check the return status properly.
	 */

	/* If hash_size is 0 then hash may be NULL and then the
	 * call to memset would have undefined behavior.
	 */
	if (hash_size != 0) {
		memset(hash, '!', hash_size);
	}

	if (hash_size < actual_hash_length) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	if (input_length > UINT32_MAX / 8U) {
		LOG_ERR("input length is too large: %zu/%zu bytes", input_length,
			(size_t)(UINT32_MAX / 8U));
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	(void)it51xxx_crypto_lock(K_FOREVER);

	*hash_length = 0;
	if (it51xxx_sha_set_algorithm(alg)) {
		it51xxx_crypto_unlock();
		return PSA_ERROR_NOT_SUPPORTED;
	}

	chip_block_idle();
	it51xxx_sha_calculate_hash(input, input_length, hash, hash_length);
	chip_permit_idle();

	LOG_HEXDUMP_DBG(hash, *hash_length, "hash:");

	it51xxx_crypto_unlock();

	return PSA_SUCCESS;
}

psa_status_t it51xxx_crypto_sha_init(void)
{
	const mm_reg_t base = it51xxx_crypto_get_base();

	k_sem_init(&sha_cal_done, 0, 1);

	/* enable sha interrupt */
	sys_write8(sys_read8(base + SHA01_STATUS) | INT_ENABLE, base + SHA01_STATUS);

	IRQ_CONNECT(IT51XXX_SHA_IRQ, 0, it51xxx_sha_isr, NULL, 0);
	irq_enable(IT51XXX_SHA_IRQ);

	return PSA_SUCCESS;
}
