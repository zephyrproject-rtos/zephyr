/*
 * Copyright (c) 2026 ITE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define MBEDTLS_DECLARE_PRIVATE_IDENTIFIERS

#include "it51xxx_crypto.h"
#include "it51xxx_crypto_rsa.h"
#include "it51xxx_psa_mbedtls_origin.h"

#include "psa_crypto_rsa.h"
#include "psa_crypto_core.h"
#include "mbedtls/platform.h"
#include "mbedtls/private/bignum.h"
#include "mbedtls/psa_util.h"
#include "mbedtls/oid.h"

#include <soc_common.h>
#include <zephyr/logging/log.h>
#define LOG_LEVEL CONFIG_IT51XXX_PSA_CRYPTO_LOG_LEVEL
LOG_MODULE_REGISTER(it51xxx_psa_crypto_rsa);

#define DT_DRV_COMPAT    ite_it51xxx_psa_rsa
#define IT51XXX_RSA_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(DT_DRV_COMPAT)
BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "it51xxx: supported one psa-crypto-rsa node only");

#define IT51XXX_RSA_IRQ DT_IRQN(DT_PARENT(IT51XXX_RSA_NODE))

#define rsa_dlm (hwcrypto_dlm.rsa)

#define RSA40_CTRL             0x40
#define RSA_ENABLE             BIT(7)
#define RSA_EXPONENT_MASK      GENMASK(5, 4)
#define RSA_EXPONENT_SELECT(n) FIELD_PREP(GENMASK(5, 4), (n))
#define RSA_INTERRUPT_ENABLE   BIT(1)
#define RSA_CLOCK_ENABLE       BIT(0)

#define RSA41_CTRL_2                      0x41
#define RSA_PUBLIC_MODULUS_LENGTH_DIVISOR 32U
#define PUBLIC_MODULUS_LENGTH(n)          ((n) / RSA_PUBLIC_MODULUS_LENGTH_DIVISOR)

#define RSA42_STATUS          0x42
#define RSA_CALCULATION_START BIT(7)
#define RSA_CALCULATION_DONE  BIT(1)

#define RSA44_BASE_ADDR_BYTE_1 0x44
#define RSA45_BASE_ADDR_BYTE_2 0x45

enum it51xxx_rsa_key_select {
	KEY_PUBLIC_EXP_65537 = 0,
	KEY_PUBLIC_EXP_3,
	KEY_PRIVATE,
	KEY_RESERVED,
};

static const uint32_t it51xxx_rsa_supported_exponents[] = {3U, 65537U};

struct k_sem rsa_cal_done;

static void it51xxx_rsa_isr(const struct device *dev)
{
	const mm_reg_t base = it51xxx_crypto_get_base();
	uint8_t rsa_sts = sys_read8(base + RSA42_STATUS);

	if (rsa_sts & RSA_CALCULATION_DONE) {
		k_sem_give(&rsa_cal_done);
		sys_write8(rsa_sts | RSA_CALCULATION_DONE, base + RSA42_STATUS);
	}
}

static inline int it51xxx_crypto_rsa_exp_valid(const mbedtls_rsa_context *rsa)
{
	for (size_t i = 0; i < ARRAY_SIZE(it51xxx_rsa_supported_exponents); i++) {
		if (mbedtls_mpi_cmp_int(&rsa->MBEDTLS_PRIVATE(E),
					it51xxx_rsa_supported_exponents[i]) == 0) {
			return 0;
		}
	}

	return -ENOTSUP;
}

static inline int it51xxx_crypto_rsa_add_padding(psa_algorithm_t alg,
						 const mbedtls_rsa_context *rsa,
						 unsigned int hashlen, const uint8_t *hash,
						 uint8_t *encode)
{
	size_t sig_len = mbedtls_rsa_get_len(rsa);
	int ret = -ENOTSUP;

	switch (rsa->MBEDTLS_PRIVATE(padding)) {
	case MBEDTLS_RSA_PKCS_V15:
		if (!PSA_ALG_IS_RSA_PKCS1V15_SIGN(alg)) {
			break;
		}

		ret = rsa_rsassa_pkcs1_v15_encode(rsa->MBEDTLS_PRIVATE(hash_id), hashlen, hash,
						  sig_len, encode);
		if (!ret) {
			LOG_HEXDUMP_DBG(encode, sig_len, "pkcs1_v15 padding data:");
		}
		break;
	case MBEDTLS_RSA_PKCS_V21:
		if (PSA_ALG_IS_RSA_PSS(alg)) {
			LOG_ERR("rsa-pss padding has not implemented yet");
		} else if (PSA_ALG_IS_RSA_OAEP(alg)) {
			LOG_ERR("rsa-oaep padding has not implemented yet");
		} else {
			LOG_ERR("unsupported rsa-v21 algorithm %#lx", (unsigned long)alg);
		}
		break;
	default:
		LOG_ERR("unsupported padding type %#x", rsa->MBEDTLS_PRIVATE(padding));
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

static void it51xxx_rsa_dlm_init(void)
{
	memset(rsa_dlm.messages, 0, sizeof(rsa_dlm.messages));
	memset(rsa_dlm.key_private, 0, sizeof(rsa_dlm.key_private));
	memset(rsa_dlm.key_public, 0, sizeof(rsa_dlm.key_public));
}

static int it51xxx_set_rsa_keys(mbedtls_rsa_context *rsa)
{
	const mm_reg_t base = it51xxx_crypto_get_base();
	int ret;
	struct {
		uint8_t public[IT51XXX_HWCRYPTO_RSA_MAX_BYTE_LEN];
		uint8_t private[IT51XXX_HWCRYPTO_RSA_MAX_BYTE_LEN];
		size_t size;
	} keys;

	ret = it51xxx_crypto_rsa_exp_valid(rsa);
	if (ret) {
		return ret;
	}

	keys.size = mbedtls_rsa_get_len(rsa);
	if (keys.size > IT51XXX_HWCRYPTO_RSA_MAX_BYTE_LEN ||
	    keys.size < IT51XXX_HWCRYPTO_RSA_MIN_BYTE_LEN) {
		LOG_ERR("unsupported rsa key size %zu (bytes)", keys.size);
		return -ENOTSUP;
	}

	if (((keys.size * BITS_PER_BYTE) % RSA_PUBLIC_MODULUS_LENGTH_DIVISOR) != 0U) {
		LOG_ERR("rsa modulus length %zu bits must be a multiple of %u bits",
			keys.size * BITS_PER_BYTE, RSA_PUBLIC_MODULUS_LENGTH_DIVISOR);
		return -ENOTSUP;
	}

	ret = mbedtls_mpi_write_binary(&rsa->MBEDTLS_PRIVATE(N), (unsigned char *)keys.public,
				       keys.size);
	if (ret) {
		return ret;
	}

	ret = mbedtls_mpi_write_binary(&rsa->MBEDTLS_PRIVATE(D), (unsigned char *)keys.private,
				       keys.size);
	if (ret) {
		goto cleanup;
	}

	for (size_t i = 0; i < keys.size; i++) {
		rsa_dlm.key_private[i] = keys.private[keys.size - i - 1];
	}

	for (size_t i = 0; i < keys.size; i++) {
		rsa_dlm.key_public[i] = keys.public[keys.size - i - 1];
	}

	sys_write8(PUBLIC_MODULUS_LENGTH(keys.size * BITS_PER_BYTE), base + RSA41_CTRL_2);

cleanup:
	/* clear keys to prevent leakage */
	mbedtls_platform_zeroize(&keys, sizeof(keys));

	return ret;
}

static int it51xxx_start_rsa_calculation(void)
{
	const mm_reg_t base = it51xxx_crypto_get_base();

	sys_write8(RSA_CALCULATION_START, base + RSA42_STATUS);

	/* wait for completion */
	k_sem_take(&rsa_cal_done, K_FOREVER);

	sys_write8(sys_read8(base + RSA40_CTRL) & ~(RSA_ENABLE | RSA_CLOCK_ENABLE),
		   base + RSA40_CTRL);

	return 0;
}

static int it51xxx_crypto_rsa_sign(psa_algorithm_t alg, mbedtls_rsa_context *rsa,
				   unsigned int hashlen, const uint8_t *hash, unsigned char *sig)
{
	const mm_reg_t base = it51xxx_crypto_get_base();
	int ret;
	size_t sig_len = mbedtls_rsa_get_len(rsa);
	uint8_t rsa_ctrl_val;
	uint8_t hash_with_padding[sig_len];

	chip_block_idle();
	it51xxx_rsa_dlm_init();

	ret = it51xxx_set_rsa_keys(rsa);
	if (ret) {
		goto out;
	}

	ret = it51xxx_crypto_rsa_add_padding(alg, rsa, hashlen, hash, hash_with_padding);
	if (ret) {
		goto out;
	}

	for (size_t i = 0; i < sig_len; i++) {
		rsa_dlm.messages[i] = hash_with_padding[sig_len - i - 1];
	}

	rsa_ctrl_val = sys_read8(base + RSA40_CTRL) & ~RSA_EXPONENT_MASK;
	rsa_ctrl_val |= RSA_ENABLE | RSA_CLOCK_ENABLE | RSA_EXPONENT_SELECT(KEY_PRIVATE);
	sys_write8(rsa_ctrl_val, base + RSA40_CTRL);

	ret = it51xxx_start_rsa_calculation();
	if (ret) {
		goto out;
	}

	for (size_t i = 0; i < sig_len; i++) {
		sig[sig_len - i - 1] = rsa_dlm.messages[i];
	}

	LOG_HEXDUMP_DBG(sig, sig_len, "signature:");

out:
	/* clear keys to prevent leakage */
	mbedtls_platform_zeroize(rsa_dlm.key_private, sizeof(rsa_dlm.key_private));
	mbedtls_platform_zeroize(rsa_dlm.key_public, sizeof(rsa_dlm.key_public));

	chip_permit_idle();

	return ret;
}

static int it51xxx_crypto_rsa_verify(psa_algorithm_t alg, mbedtls_rsa_context *rsa,
				     unsigned int hashlen, const uint8_t *hash,
				     const unsigned char *sig)
{
	bool exponent_3 = !mbedtls_mpi_cmp_int(&rsa->MBEDTLS_PRIVATE(E), 3);
	const mm_reg_t base = it51xxx_crypto_get_base();
	int ret;
	size_t sig_len = mbedtls_rsa_get_len(rsa);
	uint8_t rsa_ctrl_val;
	uint8_t encoded[sig_len];
	uint8_t encoded_expected[sig_len];

	chip_block_idle();
	it51xxx_rsa_dlm_init();

	ret = it51xxx_set_rsa_keys(rsa);
	if (ret) {
		goto out;
	}

	ret = it51xxx_crypto_rsa_add_padding(alg, rsa, hashlen, hash, encoded_expected);
	if (ret) {
		goto out;
	}

	for (size_t i = 0; i < sig_len; i++) {
		rsa_dlm.messages[i] = sig[sig_len - i - 1];
	}

	rsa_ctrl_val = sys_read8(base + RSA40_CTRL) & ~RSA_EXPONENT_MASK;
	rsa_ctrl_val |= RSA_ENABLE | RSA_CLOCK_ENABLE;
	rsa_ctrl_val |= exponent_3 ? RSA_EXPONENT_SELECT(KEY_PUBLIC_EXP_3)
				   : RSA_EXPONENT_SELECT(KEY_PUBLIC_EXP_65537);
	sys_write8(rsa_ctrl_val, base + RSA40_CTRL);

	ret = it51xxx_start_rsa_calculation();
	if (ret) {
		goto out;
	}

	for (size_t i = 0; i < sig_len; i++) {
		encoded[sig_len - i - 1] = rsa_dlm.messages[i];
	}

	LOG_HEXDUMP_DBG(encoded, sig_len, "verify:");

	if (memcmp(encoded, encoded_expected, sig_len)) {
		ret = PSA_ERROR_INVALID_SIGNATURE;
	}

out:
	/* clear keys to prevent leakage */
	mbedtls_platform_zeroize(rsa_dlm.key_private, sizeof(rsa_dlm.key_private));
	mbedtls_platform_zeroize(rsa_dlm.key_public, sizeof(rsa_dlm.key_public));

	chip_permit_idle();

	return ret;
}

static psa_status_t it51xxx_psa_rsa_sign_hash(const psa_key_attributes_t *attributes,
					      const uint8_t *key_buffer, size_t key_buffer_size,
					      psa_algorithm_t alg, const uint8_t *hash,
					      size_t hash_length, uint8_t *signature,
					      size_t signature_size, size_t *signature_length)
{
	psa_key_type_t key_type = psa_get_key_type(attributes);
	psa_status_t status;
	int ret;
	mbedtls_rsa_context *rsa = NULL;
	size_t rsa_modulus_bytes;
	uint8_t hash_size = PSA_HASH_LENGTH(PSA_ALG_SIGN_GET_HASH(alg));

	if (hash_size > 0u && hash_size != hash_length) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	status = mbedtls_psa_rsa_load_representation(key_type, key_buffer, key_buffer_size, &rsa);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	rsa_modulus_bytes = mbedtls_rsa_get_len(rsa);
	if (signature_size < rsa_modulus_bytes) {
		status = PSA_ERROR_BUFFER_TOO_SMALL;
		goto exit;
	}

	if (rsa_modulus_bytes > IT51XXX_HWCRYPTO_RSA_MAX_BYTE_LEN ||
	    rsa_modulus_bytes < IT51XXX_HWCRYPTO_RSA_MIN_BYTE_LEN) {
		LOG_ERR("unsupported rsa modulus length %zu (bytes)", rsa_modulus_bytes);
		status = PSA_ERROR_NOT_SUPPORTED;
		goto exit;
	}

	ret = -ENOTSUP;

	if (PSA_ALG_IS_RSA_PKCS1V15_SIGN(alg) && PSA_KEY_TYPE_IS_RSA(key_type)) {
		ret = mbedtls_rsa_set_padding(rsa, MBEDTLS_RSA_PKCS_V15,
					      mbedtls_md_type_from_psa_alg(alg));
		if (ret == 0) {
			(void)it51xxx_crypto_lock(K_FOREVER);
			ret = it51xxx_crypto_rsa_sign(alg, rsa, (unsigned int)hash_length, hash,
						      signature);
			it51xxx_crypto_unlock();
		}
	}

	if (ret == 0) {
		*signature_length = rsa_modulus_bytes;
	}
	status = mbedtls_to_psa_error(ret);

exit:
	mbedtls_rsa_free(rsa);
	mbedtls_free(rsa);

	return status;
}

static psa_status_t it51xxx_psa_rsa_verify_hash(const psa_key_attributes_t *attributes,
						const uint8_t *key_buffer, size_t key_buffer_size,
						psa_algorithm_t alg, const uint8_t *hash,
						size_t hash_length, const uint8_t *signature,
						size_t signature_length)
{
	psa_key_type_t key_type = psa_get_key_type(attributes);
	psa_status_t status;
	int ret;
	mbedtls_rsa_context *rsa = NULL;
	size_t rsa_modulus_bytes;
	uint8_t hash_size = PSA_HASH_LENGTH(PSA_ALG_SIGN_GET_HASH(alg));

	if (hash_size > 0u && hash_size != hash_length) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	status = mbedtls_psa_rsa_load_representation(key_type, key_buffer, key_buffer_size, &rsa);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	rsa_modulus_bytes = mbedtls_rsa_get_len(rsa);
	if (signature_length != rsa_modulus_bytes) {
		status = PSA_ERROR_INVALID_SIGNATURE;
		goto exit;
	}

	if (rsa_modulus_bytes > IT51XXX_HWCRYPTO_RSA_MAX_BYTE_LEN ||
	    rsa_modulus_bytes < IT51XXX_HWCRYPTO_RSA_MIN_BYTE_LEN) {
		LOG_ERR("unsupported rsa modulus length %zu (bytes)", rsa_modulus_bytes);
		status = PSA_ERROR_NOT_SUPPORTED;
		goto exit;
	}

	ret = -ENOTSUP;

	if (PSA_ALG_IS_RSA_PKCS1V15_SIGN(alg) && PSA_KEY_TYPE_IS_RSA(key_type)) {
		ret = mbedtls_rsa_set_padding(rsa, MBEDTLS_RSA_PKCS_V15,
					      mbedtls_md_type_from_psa_alg(alg));
		if (ret == 0) {
			(void)it51xxx_crypto_lock(K_FOREVER);
			ret = it51xxx_crypto_rsa_verify(alg, rsa, (unsigned int)hash_length, hash,
							signature);
			it51xxx_crypto_unlock();
		}
	}

	/* Mbed TLS distinguishes "invalid padding" from "valid padding but
	 * the rest of the signature is invalid". This has little use in
	 * practice and PSA doesn't report this distinction.
	 */
	status = (ret == MBEDTLS_ERR_RSA_INVALID_PADDING) ? PSA_ERROR_INVALID_SIGNATURE
							  : mbedtls_to_psa_error(ret);

exit:
	mbedtls_rsa_free(rsa);
	mbedtls_free(rsa);

	return status;
}

psa_status_t ite_it51xxx_sign_hash(const psa_key_attributes_t *attributes,
				   const uint8_t *key_buffer, size_t key_buffer_size,
				   psa_algorithm_t alg, const uint8_t *hash, size_t hash_length,
				   uint8_t *signature, size_t signature_size,
				   size_t *signature_length)
{
	if (psa_get_key_type(attributes) == PSA_KEY_TYPE_RSA_KEY_PAIR) {
		return it51xxx_psa_rsa_sign_hash(attributes, key_buffer, key_buffer_size, alg, hash,
						 hash_length, signature, signature_size,
						 signature_length);
	}

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t ite_it51xxx_verify_hash(const psa_key_attributes_t *attributes,
				     const uint8_t *key_buffer, size_t key_buffer_size,
				     psa_algorithm_t alg, const uint8_t *hash, size_t hash_length,
				     const uint8_t *signature, size_t signature_length)
{

	if (PSA_KEY_TYPE_IS_RSA(psa_get_key_type(attributes))) {
		return it51xxx_psa_rsa_verify_hash(attributes, key_buffer, key_buffer_size, alg,
						   hash, hash_length, signature, signature_length);
	}

	return PSA_ERROR_NOT_SUPPORTED;
}

int it51xxx_crypto_rsa_init(void)
{
	const mm_reg_t base = it51xxx_crypto_get_base();

	k_sem_init(&rsa_cal_done, 0, 1);

	LOG_INF("rsa dlm addr %#x(%zu)", (uint32_t)&rsa_dlm, sizeof(rsa_dlm));
	sys_write8(FIELD_GET(GENMASK(15, 8), (uint32_t)&rsa_dlm), base + RSA44_BASE_ADDR_BYTE_1);
	sys_write8(FIELD_GET(GENMASK(17, 16), (uint32_t)&rsa_dlm), base + RSA45_BASE_ADDR_BYTE_2);
	sys_write8(RSA_INTERRUPT_ENABLE, base + RSA40_CTRL);

	IRQ_CONNECT(IT51XXX_RSA_IRQ, 0, it51xxx_rsa_isr, NULL, 0);
	irq_enable(IT51XXX_RSA_IRQ);

	return 0;
}
