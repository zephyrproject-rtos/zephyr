/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <openthread/platform/crypto.h>

#include <psa/crypto.h>

#include <zephyr/sys/__assert.h>

#if !defined(CONFIG_BUILD_WITH_TFM) && defined(CONFIG_OPENTHREAD_CRYPTO_PSA)
#include <zephyr/settings/settings.h>
#endif

#if defined(CONFIG_OPENTHREAD_ECDSA)
#include <string.h>
#include <mbedtls/asn1.h>
#endif

static otError psaToOtError(psa_status_t aStatus)
{
	switch (aStatus) {
	case PSA_SUCCESS:
		return OT_ERROR_NONE;
	case PSA_ERROR_INVALID_ARGUMENT:
		return OT_ERROR_INVALID_ARGS;
	case PSA_ERROR_BUFFER_TOO_SMALL:
		return OT_ERROR_NO_BUFS;
	default:
		return OT_ERROR_FAILED;
	}
}

static psa_key_type_t toPsaKeyType(otCryptoKeyType aType)
{
	switch (aType) {
	case OT_CRYPTO_KEY_TYPE_RAW:
		return PSA_KEY_TYPE_RAW_DATA;
	case OT_CRYPTO_KEY_TYPE_AES:
		return PSA_KEY_TYPE_AES;
	case OT_CRYPTO_KEY_TYPE_HMAC:
		return PSA_KEY_TYPE_HMAC;
	case OT_CRYPTO_KEY_TYPE_ECDSA:
		return PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1);
	default:
		return PSA_KEY_TYPE_NONE;
	}
}

static psa_algorithm_t toPsaAlgorithm(otCryptoKeyAlgorithm aAlgorithm)
{
	switch (aAlgorithm) {
	case OT_CRYPTO_KEY_ALG_AES_ECB:
		return PSA_ALG_ECB_NO_PADDING;
	case OT_CRYPTO_KEY_ALG_HMAC_SHA_256:
		return PSA_ALG_HMAC(PSA_ALG_SHA_256);
	case OT_CRYPTO_KEY_ALG_ECDSA:
		return PSA_ALG_DETERMINISTIC_ECDSA(PSA_ALG_SHA_256);
	default:
		/*
		 * There is currently no constant like PSA_ALG_NONE, but 0 is used
		 * to indicate an unknown algorithm.
		 */
		return (psa_algorithm_t)0;
	}
}

static psa_key_usage_t toPsaKeyUsage(int aUsage)
{
	psa_key_usage_t usage = 0;

	if (aUsage & OT_CRYPTO_KEY_USAGE_EXPORT) {
		usage |= PSA_KEY_USAGE_EXPORT;
	}

	if (aUsage & OT_CRYPTO_KEY_USAGE_ENCRYPT) {
		usage |= PSA_KEY_USAGE_ENCRYPT;
	}

	if (aUsage & OT_CRYPTO_KEY_USAGE_DECRYPT) {
		usage |= PSA_KEY_USAGE_DECRYPT;
	}

	if (aUsage & OT_CRYPTO_KEY_USAGE_SIGN_HASH) {
		usage |= PSA_KEY_USAGE_SIGN_HASH;
	}

	if (aUsage & OT_CRYPTO_KEY_USAGE_VERIFY_HASH) {
		usage |= PSA_KEY_USAGE_VERIFY_HASH;
	}

	return usage;
}

static bool checkKeyUsage(int aUsage)
{
	/* Check if only supported flags have been passed */
	int supported_flags = OT_CRYPTO_KEY_USAGE_EXPORT | OT_CRYPTO_KEY_USAGE_ENCRYPT |
			      OT_CRYPTO_KEY_USAGE_DECRYPT | OT_CRYPTO_KEY_USAGE_SIGN_HASH |
			      OT_CRYPTO_KEY_USAGE_VERIFY_HASH;

	return (aUsage & ~supported_flags) == 0;
}

static bool checkContext(otCryptoContext *aContext, size_t aMinSize)
{
	/* Verify that the passed context is initialized and points to a big enough buffer */
	return aContext != NULL && aContext->mContext != NULL && aContext->mContextSize >= aMinSize;
}

void otPlatCryptoInit(void)
{
	psa_crypto_init();

#if !defined(CONFIG_BUILD_WITH_TFM) && defined(CONFIG_OPENTHREAD_CRYPTO_PSA)
	/*
	 * In OpenThread, Settings are initialized after KeyManager by default. If device uses
	 * PSA with emulated TFM, Settings have to be initialized at the end of otPlatCryptoInit(),
	 * to be available before storing Network Key.
	 */
	__ASSERT_EVAL((void)settings_subsys_init(), int err = settings_subsys_init(), !err,
		      "Failed to initialize settings");
#endif
}

otError otPlatCryptoImportKey(otCryptoKeyRef *aKeyRef, otCryptoKeyType aKeyType,
			      otCryptoKeyAlgorithm aKeyAlgorithm, int aKeyUsage,
			      otCryptoKeyStorage aKeyPersistence, const uint8_t *aKey,
			      size_t aKeyLen)
{
#if defined(CONFIG_OPENTHREAD_ECDSA)
	int version;
	size_t len;
	unsigned char *p = (unsigned char *)aKey;
	unsigned char *end;
#endif

	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status = 0;

	if (aKeyRef == NULL || aKey == NULL || !checkKeyUsage(aKeyUsage)) {
		return OT_ERROR_INVALID_ARGS;
	}

#if defined(CONFIG_OPENTHREAD_ECDSA)
	/* Check if key is ECDSA pair and extract private key from it since PSA expects it. */
	if (aKeyType == OT_CRYPTO_KEY_TYPE_ECDSA) {

		end = p + aKeyLen;
		status = mbedtls_asn1_get_tag(&p, end, &len,
					      MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE);
		if (status != 0) {
			return OT_ERROR_FAILED;
		}

		end = p + len;
		status = mbedtls_asn1_get_int(&p, end, &version);
		if (status != 0) {
			return OT_ERROR_FAILED;
		}

		status = mbedtls_asn1_get_tag(&p, end, &len, MBEDTLS_ASN1_OCTET_STRING);
		if (status != 0 || len != 32) {
			return OT_ERROR_FAILED;
		}

		aKey = p;
		aKeyLen = len;
	}
#endif

	psa_set_key_type(&attributes, toPsaKeyType(aKeyType));
	psa_set_key_algorithm(&attributes, toPsaAlgorithm(aKeyAlgorithm));
	psa_set_key_usage_flags(&attributes, toPsaKeyUsage(aKeyUsage));

	switch (aKeyPersistence) {
	case OT_CRYPTO_KEY_STORAGE_PERSISTENT:
		psa_set_key_lifetime(&attributes, PSA_KEY_LIFETIME_PERSISTENT);
		psa_set_key_id(&attributes, *aKeyRef);
		break;
	case OT_CRYPTO_KEY_STORAGE_VOLATILE:
		psa_set_key_lifetime(&attributes, PSA_KEY_LIFETIME_VOLATILE);
		break;
	}

	status = psa_import_key(&attributes, aKey, aKeyLen, aKeyRef);
	psa_reset_key_attributes(&attributes);

	return psaToOtError(status);
}

otError otPlatCryptoExportKey(otCryptoKeyRef aKeyRef, uint8_t *aBuffer, size_t aBufferLen,
			      size_t *aKeyLen)
{
	if (aBuffer == NULL) {
		return OT_ERROR_INVALID_ARGS;
	}

	return psaToOtError(psa_export_key(aKeyRef, aBuffer, aBufferLen, aKeyLen));
}

otError otPlatCryptoDestroyKey(otCryptoKeyRef aKeyRef)
{
	return psaToOtError(psa_destroy_key(aKeyRef));
}

bool otPlatCryptoHasKey(otCryptoKeyRef aKeyRef)
{
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status;

	status = psa_get_key_attributes(aKeyRef, &attributes);
	psa_reset_key_attributes(&attributes);

	return status == PSA_SUCCESS;
}

otError otPlatCryptoHmacSha256Init(otCryptoContext *aContext)
{
	psa_mac_operation_t *operation;

	if (!checkContext(aContext, sizeof(psa_mac_operation_t))) {
		return OT_ERROR_INVALID_ARGS;
	}

	operation = aContext->mContext;
	memset(operation, 0, sizeof(*operation));

	return OT_ERROR_NONE;
}

otError otPlatCryptoHmacSha256Deinit(otCryptoContext *aContext)
{
	psa_mac_operation_t *operation;

	if (!checkContext(aContext, sizeof(psa_mac_operation_t))) {
		return OT_ERROR_INVALID_ARGS;
	}

	operation = aContext->mContext;

	return psaToOtError(psa_mac_abort(operation));
}

otError otPlatCryptoHmacSha256Start(otCryptoContext *aContext, const otCryptoKey *aKey)
{
	psa_mac_operation_t *operation;
	psa_status_t status;

	if (aKey == NULL || !checkContext(aContext, sizeof(psa_mac_operation_t))) {
		return OT_ERROR_INVALID_ARGS;
	}

	operation = aContext->mContext;
	status = psa_mac_sign_setup(operation, aKey->mKeyRef, PSA_ALG_HMAC(PSA_ALG_SHA_256));

	return psaToOtError(status);
}

otError otPlatCryptoHmacSha256Update(otCryptoContext *aContext, const void *aBuf,
				     uint16_t aBufLength)
{
	psa_mac_operation_t *operation;

	if (aBuf == NULL || !checkContext(aContext, sizeof(psa_mac_operation_t))) {
		return OT_ERROR_INVALID_ARGS;
	}

	operation = aContext->mContext;

	return psaToOtError(psa_mac_update(operation, (const uint8_t *)aBuf, aBufLength));
}

otError otPlatCryptoHmacSha256Finish(otCryptoContext *aContext, uint8_t *aBuf, size_t aBufLength)
{
	psa_mac_operation_t *operation;
	size_t mac_length;

	if (aBuf == NULL || !checkContext(aContext, sizeof(psa_mac_operation_t))) {
		return OT_ERROR_INVALID_ARGS;
	}

	operation = aContext->mContext;

	return psaToOtError(psa_mac_sign_finish(operation, aBuf, aBufLength, &mac_length));
}

otError otPlatCryptoAesInit(otCryptoContext *aContext)
{
	psa_key_id_t *key_ref;

	if (!checkContext(aContext, sizeof(psa_key_id_t))) {
		return OT_ERROR_INVALID_ARGS;
	}

	key_ref = aContext->mContext;
	*key_ref = (psa_key_id_t)0; /* In TF-M 1.5.0 this can be replaced with PSA_KEY_ID_NULL */

	return OT_ERROR_NONE;
}

otError otPlatCryptoAesSetKey(otCryptoContext *aContext, const otCryptoKey *aKey)
{
	psa_key_id_t *key_ref;

	if (aKey == NULL || !checkContext(aContext, sizeof(psa_key_id_t))) {
		return OT_ERROR_INVALID_ARGS;
	}

	key_ref = aContext->mContext;
	*key_ref = aKey->mKeyRef;

	return OT_ERROR_NONE;
}

otError otPlatCryptoAesEncrypt(otCryptoContext *aContext, const uint8_t *aInput, uint8_t *aOutput)
{
	const size_t block_size = PSA_BLOCK_CIPHER_BLOCK_LENGTH(PSA_KEY_TYPE_AES);
	psa_status_t status = PSA_SUCCESS;
	psa_key_id_t *key_ref;
	size_t cipher_length;

	if (aInput == NULL || aOutput == NULL || !checkContext(aContext, sizeof(psa_key_id_t))) {
		return OT_ERROR_INVALID_ARGS;
	}

	key_ref = aContext->mContext;
	status = psa_cipher_encrypt(*key_ref, PSA_ALG_ECB_NO_PADDING, aInput, block_size, aOutput,
				    block_size, &cipher_length);

	return psaToOtError(status);
}

otError otPlatCryptoAesFree(otCryptoContext *aContext)
{
	return OT_ERROR_NONE;
}

otError otPlatCryptoSha256Init(otCryptoContext *aContext)
{
	psa_hash_operation_t *operation;

	if (!checkContext(aContext, sizeof(psa_hash_operation_t))) {
		return OT_ERROR_INVALID_ARGS;
	}

	operation = aContext->mContext;
	memset(operation, 0, sizeof(*operation));

	return OT_ERROR_NONE;
}

otError otPlatCryptoSha256Deinit(otCryptoContext *aContext)
{
	psa_hash_operation_t *operation;

	if (!checkContext(aContext, sizeof(psa_hash_operation_t))) {
		return OT_ERROR_INVALID_ARGS;
	}

	operation = aContext->mContext;

	return psaToOtError(psa_hash_abort(operation));
}

otError otPlatCryptoSha256Start(otCryptoContext *aContext)
{
	psa_hash_operation_t *operation;

	if (!checkContext(aContext, sizeof(psa_hash_operation_t))) {
		return OT_ERROR_INVALID_ARGS;
	}

	operation = aContext->mContext;

	return psaToOtError(psa_hash_setup(operation, PSA_ALG_SHA_256));
}

otError otPlatCryptoSha256Update(otCryptoContext *aContext, const void *aBuf, uint16_t aBufLength)
{
	psa_hash_operation_t *operation;

	if (aBuf == NULL || !checkContext(aContext, sizeof(psa_hash_operation_t))) {
		return OT_ERROR_INVALID_ARGS;
	}

	operation = aContext->mContext;

	return psaToOtError(psa_hash_update(operation, (const uint8_t *)aBuf, aBufLength));
}

otError otPlatCryptoSha256Finish(otCryptoContext *aContext, uint8_t *aHash, uint16_t aHashSize)
{
	psa_hash_operation_t *operation;
	size_t hash_size;

	if (aHash == NULL || !checkContext(aContext, sizeof(psa_hash_operation_t))) {
		return OT_ERROR_INVALID_ARGS;
	}

	operation = aContext->mContext;

	return psaToOtError(psa_hash_finish(operation, aHash, aHashSize, &hash_size));
}

void otPlatCryptoRandomInit(void)
{
	psa_crypto_init();
}

void otPlatCryptoRandomDeinit(void)
{
}

otError otPlatCryptoRandomGet(uint8_t *aBuffer, uint16_t aSize)
{
	return psaToOtError(psa_generate_random(aBuffer, aSize));
}

#if defined(CONFIG_OPENTHREAD_ECDSA)

otError otPlatCryptoEcdsaGenerateKey(otPlatCryptoEcdsaKeyPair *aKeyPair)
{
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key_id = 0;
	psa_status_t status;
	size_t exported_length;

	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_EXPORT);
	psa_set_key_algorithm(&attributes, PSA_ALG_DETERMINISTIC_ECDSA(PSA_ALG_SHA_256));
	psa_set_key_type(&attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
	psa_set_key_bits(&attributes, 256);

	status = psa_generate_key(&attributes, &key_id);
	if (status != PSA_SUCCESS) {
		goto out;
	}

	status = psa_export_key(key_id, aKeyPair->mDerBytes, OT_CRYPTO_ECDSA_MAX_DER_SIZE,
				&exported_length);
	if (status != PSA_SUCCESS) {
		goto out;
	}
	aKeyPair->mDerLength = exported_length;

out:
	psa_reset_key_attributes(&attributes);
	psa_destroy_key(key_id);

	return psaToOtError(status);
}

otError otPlatCryptoEcdsaSign(const otPlatCryptoEcdsaKeyPair *aKeyPair,
			      const otPlatCryptoSha256Hash *aHash,
			      otPlatCryptoEcdsaSignature *aSignature)
{
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key_id;
	psa_status_t status;
	size_t signature_length;

	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_SIGN_HASH);
	psa_set_key_algorithm(&attributes, PSA_ALG_DETERMINISTIC_ECDSA(PSA_ALG_SHA_256));
	psa_set_key_type(&attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
	psa_set_key_bits(&attributes, 256);

	status = psa_import_key(&attributes, aKeyPair->mDerBytes, aKeyPair->mDerLength, &key_id);
	if (status != PSA_SUCCESS) {
		goto out;
	}

	status = psa_sign_hash(key_id, PSA_ALG_DETERMINISTIC_ECDSA(PSA_ALG_SHA_256), aHash->m8,
			       OT_CRYPTO_SHA256_HASH_SIZE, aSignature->m8,
			       OT_CRYPTO_ECDSA_SIGNATURE_SIZE, &signature_length);
	if (status != PSA_SUCCESS) {
		goto out;
	}

out:
	psa_reset_key_attributes(&attributes);
	psa_destroy_key(key_id);

	return psaToOtError(status);
}

otError otPlatCryptoEcdsaVerify(const otPlatCryptoEcdsaPublicKey *aPublicKey,
				const otPlatCryptoSha256Hash *aHash,
				const otPlatCryptoEcdsaSignature *aSignature)
{
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key_id;
	psa_status_t status;
	uint8_t buffer[1 + OT_CRYPTO_ECDSA_PUBLIC_KEY_SIZE];

	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_VERIFY_HASH);
	psa_set_key_algorithm(&attributes, PSA_ALG_DETERMINISTIC_ECDSA(PSA_ALG_SHA_256));
	psa_set_key_type(&attributes, PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1));
	psa_set_key_bits(&attributes, 256);

	/*
	 * `psa_import_key` expects a key format as specified by SEC1 &sect;2.3.3 for the
	 * uncompressed representation of the ECPoint.
	 */
	buffer[0] = 0x04;
	memcpy(buffer + 1, aPublicKey->m8, OT_CRYPTO_ECDSA_PUBLIC_KEY_SIZE);
	status = psa_import_key(&attributes, buffer, sizeof(buffer), &key_id);
	if (status != PSA_SUCCESS) {
		goto out;
	}

	status = psa_verify_hash(key_id, PSA_ALG_DETERMINISTIC_ECDSA(PSA_ALG_SHA_256), aHash->m8,
				 OT_CRYPTO_SHA256_HASH_SIZE, aSignature->m8,
				 OT_CRYPTO_ECDSA_SIGNATURE_SIZE);
	if (status != PSA_SUCCESS) {
		goto out;
	}

out:
	psa_reset_key_attributes(&attributes);
	psa_destroy_key(key_id);

	return psaToOtError(status);
}

otError otPlatCryptoEcdsaSignUsingKeyRef(otCryptoKeyRef aKeyRef,
					 const otPlatCryptoSha256Hash *aHash,
					 otPlatCryptoEcdsaSignature *aSignature)
{
	psa_status_t status;
	size_t signature_length;

	status = psa_sign_hash(aKeyRef, PSA_ALG_DETERMINISTIC_ECDSA(PSA_ALG_SHA_256), aHash->m8,
			       OT_CRYPTO_SHA256_HASH_SIZE, aSignature->m8,
			       OT_CRYPTO_ECDSA_SIGNATURE_SIZE, &signature_length);
	if (status != PSA_SUCCESS) {
		goto out;
	}

	__ASSERT_NO_MSG(signature_length == OT_CRYPTO_ECDSA_SIGNATURE_SIZE);
out:
	return psaToOtError(status);
}

otError otPlatCryptoEcdsaVerifyUsingKeyRef(otCryptoKeyRef aKeyRef,
					   const otPlatCryptoSha256Hash *aHash,
					   const otPlatCryptoEcdsaSignature *aSignature)
{
	psa_status_t status;

	status = psa_verify_hash(aKeyRef, PSA_ALG_DETERMINISTIC_ECDSA(PSA_ALG_SHA_256), aHash->m8,
				 OT_CRYPTO_SHA256_HASH_SIZE, aSignature->m8,
				 OT_CRYPTO_ECDSA_SIGNATURE_SIZE);
	if (status != PSA_SUCCESS) {
		goto out;
	}

out:
	return psaToOtError(status);
}

otError otPlatCryptoEcdsaExportPublicKey(otCryptoKeyRef aKeyRef,
					 otPlatCryptoEcdsaPublicKey *aPublicKey)
{
	psa_status_t status;
	size_t exported_length;
	uint8_t buffer[1 + OT_CRYPTO_ECDSA_PUBLIC_KEY_SIZE];

	status = psa_export_public_key(aKeyRef, buffer, sizeof(buffer), &exported_length);
	if (status != PSA_SUCCESS) {
		goto out;
	}

	__ASSERT_NO_MSG(exported_length == sizeof(buffer));
	memcpy(aPublicKey->m8, buffer + 1, OT_CRYPTO_ECDSA_PUBLIC_KEY_SIZE);

out:
	return psaToOtError(status);
}

otError otPlatCryptoEcdsaGenerateAndImportKey(otCryptoKeyRef aKeyRef)
{
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status;
	psa_key_id_t key_id = (psa_key_id_t)aKeyRef;

	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_VERIFY_HASH | PSA_KEY_USAGE_SIGN_HASH);
	psa_set_key_algorithm(&attributes, PSA_ALG_DETERMINISTIC_ECDSA(PSA_ALG_SHA_256));
	psa_set_key_type(&attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
	psa_set_key_lifetime(&attributes, PSA_KEY_LIFETIME_PERSISTENT);
	psa_set_key_id(&attributes, key_id);
	psa_set_key_bits(&attributes, 256);

	status = psa_generate_key(&attributes, &key_id);
	if (status != PSA_SUCCESS) {
		goto out;
	}

out:
	psa_reset_key_attributes(&attributes);

	return psaToOtError(status);
}

otError otPlatCryptoPbkdf2GenerateKey(const uint8_t *aPassword,
				      uint16_t       aPasswordLen,
				      const uint8_t *aSalt,
				      uint16_t       aSaltLen,
				      uint32_t       aIterationCounter,
				      uint16_t       aKeyLen,
				      uint8_t       *aKey)
{
	psa_status_t status = PSA_SUCCESS;
	psa_key_id_t key_id = PSA_KEY_ID_NULL;
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_algorithm_t algorithm = PSA_ALG_PBKDF2_AES_CMAC_PRF_128;
	psa_key_derivation_operation_t operation = PSA_KEY_DERIVATION_OPERATION_INIT;

	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_DERIVE);
	psa_set_key_lifetime(&attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&attributes, algorithm);
	psa_set_key_type(&attributes, PSA_KEY_TYPE_PASSWORD);
	psa_set_key_bits(&attributes, PSA_BYTES_TO_BITS(aPasswordLen));

	status = psa_import_key(&attributes, aPassword, aPasswordLen, &key_id);
	if (status != PSA_SUCCESS) {
		goto out;
	}

	status = psa_key_derivation_setup(&operation, algorithm);
	if (status != PSA_SUCCESS) {
		goto out;
	}

	status = psa_key_derivation_input_integer(&operation, PSA_KEY_DERIVATION_INPUT_COST,
						  aIterationCounter);
	if (status != PSA_SUCCESS) {
		goto out;
	}

	status = psa_key_derivation_input_bytes(&operation, PSA_KEY_DERIVATION_INPUT_SALT,
						aSalt, aSaltLen);
	if (status != PSA_SUCCESS) {
		goto out;
	}

	status = psa_key_derivation_input_key(&operation, PSA_KEY_DERIVATION_INPUT_PASSWORD,
					      key_id);
	if (status != PSA_SUCCESS) {
		goto out;
	}

	status = psa_key_derivation_output_bytes(&operation, aKey, aKeyLen);
	if (status != PSA_SUCCESS) {
		goto out;
	}

out:
	psa_reset_key_attributes(&attributes);
	psa_key_derivation_abort(&operation);
	psa_destroy_key(key_id);

	__ASSERT_NO_MSG(status == PSA_SUCCESS);
	return psaToOtError(status);
}

#endif /* #if CONFIG_OPENTHREAD_ECDSA */
