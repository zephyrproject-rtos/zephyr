/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <openthread/platform/crypto.h>

#include <psa/crypto.h>

#if defined(CONFIG_OPENTHREAD_ECDSA)
#include <string.h>
#include <zephyr/sys/__assert.h>
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
	default:
		/*
		 * There is currently no constant like PSA_ALG_NONE, but 0 is used
		 * to indicate an unknown algorithm.
		 */
		return (psa_algorithm_t) 0;
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

	return usage;
}

static bool checkKeyUsage(int aUsage)
{
	/* Check if only supported flags have been passed */
	int supported_flags = OT_CRYPTO_KEY_USAGE_EXPORT |
			      OT_CRYPTO_KEY_USAGE_ENCRYPT |
			      OT_CRYPTO_KEY_USAGE_DECRYPT |
			      OT_CRYPTO_KEY_USAGE_SIGN_HASH;

	return (aUsage & ~supported_flags) == 0;
}

static bool checkContext(otCryptoContext *aContext, size_t aMinSize)
{
	/* Verify that the passed context is initialized and points to a big enough buffer */
	return aContext != NULL && aContext->mContext != NULL && aContext->mContextSize >= aMinSize;
}

static void ensureKeyIsLoaded(otCryptoKeyRef aKeyRef)
{
	/*
	 * The workaround below will no longer be need after updating TF-M version used in Zephyr
	 * to 1.5.0 (see upstream commit 42e77b561fcfe19819ff1e63cb7c0b672ee8ba41).
	 * In the recent versions of TF-M the concept of key handles and psa_open_key()/
	 * psa_close_key() APIs have been being deprecated, but the version currently used in Zephyr
	 * is in the middle of that transition. Consequently, psa_destroy_key() and lots of other
	 * functions will fail when a key ID that they take as a parameter is not loaded from the
	 * persistent storage. That may occur when a given persistent key is created via
	 * psa_generate_key() or psa_import_key(), and then the device reboots.
	 *
	 * Use psa_open_key() when the key has not been loaded yet to work around the issue.
	 */
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status = psa_get_key_attributes(aKeyRef, &attributes);
	psa_key_id_t key_handle;

	if (status == PSA_ERROR_INVALID_HANDLE) {
		psa_open_key(aKeyRef, &key_handle);
	}

	psa_reset_key_attributes(&attributes);
}

void otPlatCryptoInit(void)
{
	psa_crypto_init();
}

otError otPlatCryptoImportKey(otCryptoKeyRef *aKeyRef,
			      otCryptoKeyType aKeyType,
			      otCryptoKeyAlgorithm aKeyAlgorithm,
			      int aKeyUsage,
			      otCryptoKeyStorage aKeyPersistence,
			      const uint8_t *aKey,
			      size_t aKeyLen)
{
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status;

	if (aKeyRef == NULL || aKey == NULL || !checkKeyUsage(aKeyUsage)) {
		return OT_ERROR_INVALID_ARGS;
	}

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

otError otPlatCryptoExportKey(otCryptoKeyRef aKeyRef,
			      uint8_t *aBuffer,
			      size_t aBufferLen,
			      size_t *aKeyLen)
{
	if (aBuffer == NULL) {
		return OT_ERROR_INVALID_ARGS;
	}

	ensureKeyIsLoaded(aKeyRef);

	return psaToOtError(psa_export_key(aKeyRef, aBuffer, aBufferLen, aKeyLen));
}

otError otPlatCryptoDestroyKey(otCryptoKeyRef aKeyRef)
{
	ensureKeyIsLoaded(aKeyRef);

	return psaToOtError(psa_destroy_key(aKeyRef));
}

bool otPlatCryptoHasKey(otCryptoKeyRef aKeyRef)
{
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status;

	ensureKeyIsLoaded(aKeyRef);
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
	*operation = psa_mac_operation_init();

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

	ensureKeyIsLoaded(aKey->mKeyRef);
	operation = aContext->mContext;
	status = psa_mac_sign_setup(operation, aKey->mKeyRef, PSA_ALG_HMAC(PSA_ALG_SHA_256));

	return psaToOtError(status);
}

otError otPlatCryptoHmacSha256Update(otCryptoContext *aContext,
				     const void *aBuf,
				     uint16_t aBufLength)
{
	psa_mac_operation_t *operation;

	if (aBuf == NULL || !checkContext(aContext, sizeof(psa_mac_operation_t))) {
		return OT_ERROR_INVALID_ARGS;
	}

	operation = aContext->mContext;

	return psaToOtError(psa_mac_update(operation, (const uint8_t *) aBuf, aBufLength));
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
	*key_ref = (psa_key_id_t) 0; /* In TF-M 1.5.0 this can be replaced with PSA_KEY_ID_NULL */

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
	psa_cipher_operation_t operation = PSA_CIPHER_OPERATION_INIT;
	psa_key_id_t *key_ref;
	size_t cipher_length;

	if (aInput == NULL || aOutput == NULL || !checkContext(aContext, sizeof(psa_key_id_t))) {
		return OT_ERROR_INVALID_ARGS;
	}

	/*
	 * The code below can be simplified after updating TF-M version used in Zephyr to 1.5.0
	 * (see upstream commit: 045ec4abfc73152a0116684ba9127d0a97cc8d34), using
	 * psa_cipher_encrypt() function which will replace the setup-update-finish sequence below.
	 */
	key_ref = aContext->mContext;
	ensureKeyIsLoaded(*key_ref);
	status = psa_cipher_encrypt_setup(&operation, *key_ref, PSA_ALG_ECB_NO_PADDING);

	if (status != PSA_SUCCESS) {
		goto out;
	}

	status = psa_cipher_update(&operation,
				   aInput,
				   block_size,
				   aOutput,
				   block_size,
				   &cipher_length);

	if (status != PSA_SUCCESS) {
		goto out;
	}

	status = psa_cipher_finish(&operation,
				   aOutput + cipher_length,
				   block_size - cipher_length,
				   &cipher_length);

out:
	psa_cipher_abort(&operation);
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
	*operation = psa_hash_operation_init();

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

	return psaToOtError(psa_hash_update(operation, (const uint8_t *) aBuf, aBufLength));
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

otError otPlatCryptoEcdsaGetPublicKey(const otPlatCryptoEcdsaKeyPair *aKeyPair,
				      otPlatCryptoEcdsaPublicKey *aPublicKey)
{
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key_id = 0;
	psa_status_t status;
	size_t exported_length;
	uint8_t buffer[1 + OT_CRYPTO_ECDSA_PUBLIC_KEY_SIZE];

	psa_set_key_algorithm(&attributes, PSA_ALG_DETERMINISTIC_ECDSA(PSA_ALG_SHA_256));
	psa_set_key_type(&attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
	psa_set_key_bits(&attributes, 256);

	status = psa_import_key(&attributes, aKeyPair->mDerBytes, aKeyPair->mDerLength, &key_id);
	if (status != PSA_SUCCESS) {
		goto out;
	}

	status = psa_export_public_key(key_id, buffer, sizeof(buffer), &exported_length);
	if (status != PSA_SUCCESS) {
		goto out;
	}
	__ASSERT_NO_MSG(exported_length == sizeof(buffer));
	memcpy(aPublicKey->m8, buffer + 1, OT_CRYPTO_ECDSA_PUBLIC_KEY_SIZE);

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

#endif /* #if CONFIG_OPENTHREAD_ECDSA */
