/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <string.h>

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <psa/protected_storage.h>

#include "tls_internal.h"
#include "tls_credentials_digest_raw.h"

LOG_MODULE_REGISTER(tls_credentials_trusted,
		    CONFIG_TLS_CREDENTIALS_LOG_LEVEL);

/* This implementation uses the PSA Protected Storage API to store:
 * - credentials with an UID constructed as:
 *	[ C2E0 ] | [ type as uint16_t ] | [ tag as uint32_t ]
 * - credential ToC with an UID constructed as:
 *	[ C2E0 ] | [ ffff as uint16_t ] | [ ffffffff as uint32_t ]
 *
 * The ToC contains a list of CONFIG_TLS_MAX_CREDENTIALS_NUMBER UIDs
 * of credentials, can be 0 if slot is free.
 */

#define PSA_PS_CRED_ID		0xC2E0ULL

#define CRED_MAX_SLOTS	CONFIG_TLS_MAX_CREDENTIALS_NUMBER

/* Global temporary pool of credentials to be used by TLS contexts. */
static struct tls_credential credentials[CRED_MAX_SLOTS];

/* Credentials Table Of Content copy of the one stored in Protected Storage */
static psa_storage_uid_t credentials_toc[CRED_MAX_SLOTS];

/* A mutex for protecting access to the credentials array. */
static struct k_mutex credential_lock;

/* Construct PSA PS uid from tag & type */
static inline psa_storage_uid_t tls_credential_get_uid(uint32_t tag,
						       uint16_t type)
{
	return PSA_PS_CRED_ID << 48 |
	       (type & 0xffffULL) << 32 |
	       (tag & 0xffffffff);
}

#define PSA_PS_CRED_TOC_ID tls_credential_get_uid(0xffffffff, 0xffff)

/* Get the TAG from an UID */
static inline sec_tag_t tls_credential_uid_to_tag(psa_storage_uid_t uid)
{
	return (uid & 0xffffffff);
}

/* Get the TYPE from an UID */
static inline int tls_credential_uid_to_type(psa_storage_uid_t uid)
{
	return ((uid >> 32) & 0xffff);
}

static int credentials_toc_get(void)
{
	psa_status_t status;
	size_t len;

	status = psa_ps_get(PSA_PS_CRED_TOC_ID, 0, sizeof(credentials_toc),
			    credentials_toc, &len);
	if (status == PSA_ERROR_DOES_NOT_EXIST) {
		return -ENOENT;
	} else if (status != PSA_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static int credentials_toc_write(void)
{
	psa_status_t status;

	status = psa_ps_set(PSA_PS_CRED_TOC_ID, sizeof(credentials_toc),
			    credentials_toc, 0);
	if (status != PSA_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static int credentials_toc_update(unsigned int slot, psa_storage_uid_t uid)
{
	int ret;

	if (slot >= CRED_MAX_SLOTS) {
		return -EINVAL;
	}

	credentials_toc[slot] = uid;

	ret = credentials_toc_write();
	if (ret) {
		return ret;
	}

	return credentials_toc_get();
}

static unsigned int tls_credential_toc_find_slot(psa_storage_uid_t uid)
{
	unsigned int slot;

	for (slot = 0; slot < CRED_MAX_SLOTS; ++slot) {
		if (credentials_toc[slot] == uid) {
			return slot;
		}
	}

	return CRED_MAX_SLOTS;
}

static int credentials_init(void)
{
	struct psa_storage_info_t info;
	unsigned int sync = 0;
	psa_status_t status;
	unsigned int slot;
	int ret;

	/* Retrieve Table of Content from storage */
	ret = credentials_toc_get();
	if (ret == -ENOENT) {
		memset(credentials_toc, 0, sizeof(credentials_toc));
		return 0;
	} else if (ret != 0) {
		return -EIO;
	}

	/* Check validity of ToC */
	for (slot = 0; slot < CRED_MAX_SLOTS; ++slot) {
		if (credentials_toc[slot] == 0) {
			continue;
		}

		status = psa_ps_get_info(credentials_toc[slot], &info);
		if (status == PSA_ERROR_DOES_NOT_EXIST) {
			LOG_WRN("Credential %d doesn't exist in storage", slot);
			credentials_toc[slot] = 0;
			sync = 1;
		} else if (status != PSA_SUCCESS) {
			return -EIO;
		}
	}

	if (sync != 0) {
		ret = credentials_toc_write();
		if (ret != 0) {
			return -EIO;
		}
	}

	return 0;
}
SYS_INIT(credentials_init, POST_KERNEL, 0);

static struct tls_credential *unused_credential_get(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(credentials); i++) {
		if (credentials[i].type == TLS_CREDENTIAL_NONE) {
			return &credentials[i];
		}
	}

	return NULL;
}

/* Get a credential struct from an UID */
static struct tls_credential *credential_get_from_uid(psa_storage_uid_t uid)
{
	struct tls_credential *credential;
	struct psa_storage_info_t info;
	psa_status_t status;

	if (tls_credential_toc_find_slot(uid) == CRED_MAX_SLOTS) {
		return NULL;
	}

	credential = unused_credential_get();
	if (credential == NULL) {
		return NULL;
	}

	status = psa_ps_get_info(uid, &info);
	if (status != PSA_SUCCESS) {
		return NULL;
	}

	credential->buf = k_malloc(info.size);
	if (credential->buf == NULL) {
		return NULL;
	}

	status = psa_ps_get(uid, 0, info.size, (void *)credential->buf,
			    &credential->len);
	if (status != PSA_SUCCESS) {
		k_free((void *)credential->buf);
		credential->buf = NULL;
		return NULL;
	}

	credential->tag = tls_credential_uid_to_tag(uid);
	credential->type = tls_credential_uid_to_type(uid);

	return credential;
}

/* Get a credential struct from a TAG and TYPE values */
struct tls_credential *credential_get(sec_tag_t tag,
				      enum tls_credential_type type)
{
	return credential_get_from_uid(tls_credential_get_uid(tag, type));
}


/* Get the following credential filtered by a TAG value */
struct tls_credential *credential_next_get(sec_tag_t tag,
					   struct tls_credential *iter)
{
	psa_storage_uid_t uid;
	unsigned int slot;

	if (!iter) {
		slot = 0;
	} else {
		uid = tls_credential_get_uid(iter->tag, iter->type);

		slot = tls_credential_toc_find_slot(uid);
		if (slot == CRED_MAX_SLOTS) {
			return NULL;
		}

		slot++;
	}

	for (; slot < CRED_MAX_SLOTS; slot++) {
		uid = credentials_toc[slot];
		if (uid == 0) {
			continue;
		}

		if (tls_credential_uid_to_type(uid) != TLS_CREDENTIAL_NONE &&
		    tls_credential_uid_to_tag(uid) == tag) {
			return credential_get_from_uid(uid);
		}
	}

	return NULL;
}

sec_tag_t credential_next_tag_get(sec_tag_t iter)
{
	unsigned int slot;
	psa_storage_uid_t uid;
	sec_tag_t lowest_candidate = TLS_SEC_TAG_NONE;
	sec_tag_t candidate;

	/* Scan all slots and find lowest sectag greater than iter */
	for (slot = 0; slot < CRED_MAX_SLOTS; slot++) {
		uid = credentials_toc[slot];

		/* Skip empty slots. */
		if (uid == 0) {
			continue;
		}
		if (tls_credential_uid_to_type(uid) == TLS_CREDENTIAL_NONE) {
			continue;
		}

		candidate = tls_credential_uid_to_tag(uid);

		/* Skip any slots containing sectags not greater than iter */
		if (candidate <= iter && iter != TLS_SEC_TAG_NONE) {
			continue;
		}

		/* Find the lowest of such slots */
		if (lowest_candidate == TLS_SEC_TAG_NONE || candidate < lowest_candidate) {
			lowest_candidate = candidate;
		}
	}

	return lowest_candidate;
}

int credential_digest(struct tls_credential *credential, void *dest, size_t *len)
{
	return credential_digest_raw(credential, dest, len);
}

void credentials_lock(void)
{
	k_mutex_lock(&credential_lock, K_FOREVER);
}

void credentials_unlock(void)
{
	int i;

	/* Erase & free all retrieved credentials */
	for (i = 0; i < ARRAY_SIZE(credentials); i++) {
		if (credentials[i].buf) {
			k_free((void *)credentials[i].buf);
		}
		memset(&credentials[i], 0, sizeof(credentials[i]));
	}

	k_mutex_unlock(&credential_lock);
}

/* Double check that security tag and credential type are allowed */
static bool tag_type_valid(sec_tag_t tag, enum tls_credential_type type)
{
	/* tag 0xffffffff type 0xffff are reserved */
	return !(tag == 0xffffffff && type == 0xffff);
}

int tls_credential_add(sec_tag_t tag, enum tls_credential_type type,
		       const void *cred, size_t credlen)
{
	psa_storage_uid_t uid = tls_credential_get_uid(tag, type);
	psa_storage_create_flags_t create_flags = 0;
	psa_status_t status;
	unsigned int slot;
	int ret = 0;

	if (!tag_type_valid(tag, type)) {
		ret = -EINVAL;
		goto cleanup;
	}

	k_mutex_lock(&credential_lock, K_FOREVER);

	if (tls_credential_toc_find_slot(uid) != CRED_MAX_SLOTS) {
		ret = -EEXIST;
		goto cleanup;
	}

	slot = tls_credential_toc_find_slot(0);
	if (slot == CRED_MAX_SLOTS) {
		ret = -ENOMEM;
		goto cleanup;
	}

	/* TODO: Set create_flags depending on tag value ? */

	status = psa_ps_set(uid, credlen, cred, create_flags);
	if (status != PSA_SUCCESS) {
		ret = -EIO;
		goto cleanup;
	}

	ret = credentials_toc_update(slot, uid);

cleanup:
	k_mutex_unlock(&credential_lock);

	return ret;
}

int tls_credential_get(sec_tag_t tag, enum tls_credential_type type,
		       void *cred, size_t *credlen)
{
	struct psa_storage_info_t info;
	psa_storage_uid_t uid = tls_credential_get_uid(tag, type);
	psa_status_t status;
	unsigned int slot;
	int ret = 0;

	if (!tag_type_valid(tag, type)) {
		ret = -EINVAL;
		goto cleanup;
	}

	k_mutex_lock(&credential_lock, K_FOREVER);

	slot = tls_credential_toc_find_slot(uid);
	if (slot == CRED_MAX_SLOTS) {
		ret = -ENOENT;
		goto cleanup;
	}

	status = psa_ps_get_info(uid, &info);
	if (status == PSA_ERROR_DOES_NOT_EXIST) {
		ret = -ENOENT;
		goto cleanup;
	} else if (status != PSA_SUCCESS) {
		ret = -EIO;
		goto cleanup;
	}

	if (info.size > *credlen) {
		ret = -EFBIG;
		goto cleanup;
	}

	status = psa_ps_get(uid, 0, info.size, cred, credlen);
	if (status != PSA_SUCCESS) {
		ret = -EIO;
		goto cleanup;
	}

cleanup:
	k_mutex_unlock(&credential_lock);

	return ret;
}

int tls_credential_delete(sec_tag_t tag, enum tls_credential_type type)
{
	psa_storage_uid_t uid = tls_credential_get_uid(tag, type);
	psa_status_t status;
	unsigned int slot;
	int ret = 0;

	if (!tag_type_valid(tag, type)) {
		ret = -EINVAL;
		goto cleanup;
	}

	k_mutex_lock(&credential_lock, K_FOREVER);

	slot = tls_credential_toc_find_slot(uid);
	if (slot == CRED_MAX_SLOTS) {
		ret = -ENOENT;
		goto cleanup;
	}

	ret = credentials_toc_update(slot, 0);
	if (ret != 0) {
		goto cleanup;
	}

	status = psa_ps_remove(uid);
	if (status == PSA_ERROR_DOES_NOT_EXIST) {
		ret = -ENOENT;
		goto cleanup;
	} else if (status != PSA_SUCCESS) {
		ret = -EIO;
		goto cleanup;
	}

cleanup:
	k_mutex_unlock(&credential_lock);

	return ret;
}

#if defined(CONFIG_TLS_CREDENTIAL_KEYGEN)

#include <psa/crypto.h>
#include <mbedtls/pk.h>
#include <mbedtls/asn1.h>

/* For this backend, the private-key will be stored in DER format.
 * Use `cred get <tag> PK bin` to retrieve (the non-terminated base64 encoding of) the key using
 * the credential shell.
 */
int tls_credential_keygen(sec_tag_t tag, enum tls_credential_keygen_type type,
			  void *key_buf, size_t *key_len)
{
	int ret = 0;

	/* PSA keygen */
	psa_status_t psa_status;
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key_id = PSA_KEY_ID_NULL;
	psa_key_usage_t key_usage;

	/* MbedTLS key formatting */
	int mbed_status;
	mbedtls_pk_context pk_ctx;
	void* key_start = 0;
	size_t key_max;

	/* Permanent key storage */
	int cred_status;
	psa_storage_uid_t uid = tls_credential_get_uid(tag, TLS_CREDENTIAL_PRIVATE_KEY);
	bool key_stored = false;

	if (key_len == NULL) {
		return -EINVAL;
	}

	key_max = *key_len;

	k_mutex_lock(&credential_lock, K_FOREVER);
	mbedtls_pk_init(&pk_ctx);

	/* Ensure the storage destination is valid. */
	if (!tag_type_valid(tag, TLS_CREDENTIAL_PRIVATE_KEY)) {
		ret = -EINVAL;
		goto cleanup;
	}

	/* Presently, only SECP256R1 is supported by this backend for keygen */
	if (!tls_credential_can_keygen(type)) {
		ret = -ENOTSUP;
		goto cleanup;
	}

	/* Verify sectag not already taken */
	if (tls_credential_toc_find_slot(uid) != CRED_MAX_SLOTS) {
		ret = -EEXIST;
		goto cleanup;
	}

	/* Before attempting keygen, check that an empty slot is available */
	if (tls_credential_toc_find_slot(0) == CRED_MAX_SLOTS) {
		ret = -ENOMEM;
		goto cleanup;
	}

	/* Use PSA to generate a volatile SECP256R1 private/public key-pair
         *
	 * It is possible (and better) to mark the private key as persistent, and store it directly
	 * in Trusted Internal Storage. This would be more secure, but until we support opaque
	 * keys properly, it isn't an option.
	 *
	 * Accordingly, for now, we simply generate a volatile key and transfer it into
	 * protected storage.
	 */

	key_usage = PSA_KEY_USAGE_EXPORT;
	psa_set_key_usage_flags(&key_attributes, key_usage);

	psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_ECDSA(PSA_ALG_SHA_256));
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
	psa_set_key_bits(&key_attributes, 256);

	psa_status = psa_crypto_init();

	if (psa_status != PSA_SUCCESS) {
		LOG_ERR("Failed to initialize crypto. Status: %d", psa_status);
		ret = -EFAULT;
		goto cleanup;
	}

	psa_status = psa_generate_key(&key_attributes, &key_id);

	if (psa_status != PSA_SUCCESS) {
		LOG_ERR("Failed to generate private key. Status: %d", psa_status);
		ret = -EFAULT;
		goto cleanup;
	}

	/* Hand key off to MbedTLS for immediate use (CSR and formatted export). */
	mbed_status = mbedtls_pk_setup_opaque(&pk_ctx, key_id);

	if (mbed_status) {
		LOG_ERR("Failed to set up opaque private key. Status: %d", mbed_status);
		ret = -EFAULT;
		goto cleanup;
	}

	/* Reset key buffer before using it */
	memset(key_buf, 0, key_max);
	*key_len = 0;

	/* Export private key material in the same RFC5915/SEC1 DER format that
	 * MbedTLS will later expect when loading the private key material from storage.
	 *
	 * Temporarily use the public key output buffer for this.
	 */
	mbed_status = mbedtls_pk_write_key_der(&pk_ctx, key_buf, key_max);
	if (mbed_status == MBEDTLS_ERR_ASN1_BUF_TOO_SMALL) {
		LOG_ERR("Failed to format private key material. Provided buffer too small.");
		ret = -EFBIG;
		goto cleanup;
	}
	if (mbed_status < 0) {
		LOG_ERR("Failed to format private key material. Status: %d", mbed_status);
		ret = -EFAULT;
		goto cleanup;
	}

	/* On success, mbed_status contains the number of bytes written. */

	/* Place the formatted private key in credentials storage.
	 * Note that mbedtls_pk_write_key_der writes the key to the end of the key buffer,
	 * hence the need for key_start.
	 */
	key_start = (char*)key_buf + key_max - mbed_status;
	cred_status = tls_credential_add(tag, TLS_CREDENTIAL_PRIVATE_KEY, key_start, mbed_status);
	if (cred_status != 0) {
		LOG_ERR("Error storing private key: %d", cred_status);
		ret = -EFAULT;
		goto cleanup;
	}

	/* Clear key buffer afterwards to prevent accidental private key leak */
	memset(key_buf, 0, key_max);

	/* Finally, write the public key to the output buffer in ASN.1 DER format
	 * (X.509 SubjectPublicKeyInfo entry. See RFC5280)
	 */
	mbed_status = mbedtls_pk_write_pubkey_der(&pk_ctx, key_buf, key_max);
	if (mbed_status == MBEDTLS_ERR_ASN1_BUF_TOO_SMALL) {
		LOG_ERR("Failed to format public key. Provided buffer too small.");
		ret = -EFBIG;
		goto cleanup;
	}
	if (mbed_status < 0) {
		LOG_ERR("Failed to format public key. Status: %d", mbed_status);
		ret = -EFAULT;
		goto cleanup;
	}

	/* On success, mbed_status contains the number of bytes written. */
	*key_len = mbed_status;

	/* mbedtls_pk_write_pubkey_der writes its data to the end of the key buffer.
	 * Shift this data to the beginning of the key buffer for convenient use by the caller.
	 */
	key_start = (char*)key_buf + key_max - mbed_status;
	memmove(key_buf, key_start, mbed_status);

cleanup:
	k_mutex_unlock(&credential_lock);
	mbedtls_pk_free(&pk_ctx);

	/* Destroy temporary PSA key if it was created */
	if (key_id != PSA_KEY_ID_NULL) {
		psa_status = psa_destroy_key(key_id);
		if (psa_status != PSA_SUCCESS) {
			LOG_ERR("Failed to destroy keypair after storage: %d", psa_status);
		}
	}

	/* Unstore private key if an error occurred after it was stored */
	if (ret && key_stored) {
		cred_status = tls_credential_delete(tag, TLS_CREDENTIAL_PRIVATE_KEY);
		if (cred_status) {
			LOG_ERR("Failed to unstore generated private key: %d ", cred_status);
		}
	}

	/* If an error occurred, the key buffer is not valid, wipe it. */
	if (ret) {
		memset(key_buf, 0, key_max);
		*key_len = 0;
	}

	return ret;
}

bool tls_credential_can_keygen(enum tls_credential_keygen_type type)
{
	if (type == TLS_CREDENTIAL_KEYGEN_DEFAULT) {
		return true;
	}
	if (type == TLS_CREDENTIAL_KEYGEN_SECP256R1) {
		return true;
	}
	return false;
}

#endif /* defined(CONFIG_TLS_CREDENTIAL_KEYGEN)	*/

#if defined(CONFIG_TLS_CREDENTIAL_CSR)

/* This lets us check which MbedTLS features are enabled */
#include "tls_credentials_mbedtls_config.h"

#if defined(MBEDTLS_X509_CSR_WRITE_C)

#include <psa/crypto.h>
#include <mbedtls/pk.h>
#include <mbedtls/asn1.h>
#include <mbedtls/x509_csr.h>

/**
 * @brief PSA Random number generator wrapper for Mbed TLS
 */
static int psa_rng_for_mbedtls(void *p_rng, unsigned char *output, size_t output_len)
{
	ARG_UNUSED(p_rng);

	return psa_generate_random(output, output_len);
}

int tls_credential_csr(sec_tag_t tag, char *dn, enum tls_credential_keygen_type type,
		       void *csr, size_t *csr_len)
{
	int ret = 0;
	psa_status_t psa_status;
	int mbed_status;

	/* Keygen/Retrieval */
	int cred_status;
	bool key_stored = false;

	/* CSR */
	mbedtls_pk_context pk_ctx;
	mbedtls_x509write_csr writer;
	size_t csr_max;
	size_t key_len;
	char *cred_start;

	if (csr_len == NULL) {
		return -EINVAL;
	}

	csr_max = *csr_len;

	/* Reset CSR buffer before using it */
	memset(csr, 0, csr_max);
	*csr_len = 0;

	if (!tag_type_valid(tag, TLS_CREDENTIAL_PRIVATE_KEY)) {
		return -EINVAL;
	}

	/* PSA Crypto initialization is required by mbedtls_pk_parse_key.
	 * See its documentation for details.
	 */
	psa_status = psa_crypto_init();

	if (psa_status != PSA_SUCCESS) {
		LOG_ERR("Failed to initialisze crypto. Status: %d", psa_status);
		return -EFAULT;
	}

	/* Create temporary contexts */
	mbedtls_pk_init(&pk_ctx);
	mbedtls_x509write_csr_init(&writer);

	/* If the user has not requested to use an existing key, generate one.
	 * The csr buffer is temporarily used until the key is stored in the backend.
	 */
	if (type != TLS_CREDENTIAL_KEYGEN_EXISTING) {
		key_len = csr_max;
		cred_status = tls_credential_keygen(tag, type, csr, &key_len);
		if (cred_status) {
			if (cred_status == -ENOTSUP) {
				LOG_ERR("Keygen is not supported");
			} else {
				LOG_ERR("Keygen for CSR failed, error: %d ", cred_status);
			}
			ret = -EFAULT;
			goto cleanup;
		}

		/* If a key was stored, we'll need to unstore it if something goes wrong later. */
		key_stored = true;
	}

	/* Now load the private key at the specified tag.
	 * Once again, the csr buffer is temporarily used for this purpose.
	 */
	key_len = csr_max;
	cred_status = tls_credential_get(tag, TLS_CREDENTIAL_PRIVATE_KEY, csr, &key_len);
	if (cred_status) {
		LOG_ERR("Could not load private key with sectag %d, error: %d ", tag, cred_status);
		ret = -ENOENT;
		goto cleanup;
	}

	/* Parse and copy the key into pk_ctx for use with MbedTLS APIs. */
	mbed_status = mbedtls_pk_parse_key(&pk_ctx, csr, key_len, NULL, 0,
					   psa_rng_for_mbedtls, NULL);
	if (mbed_status) {
		LOG_ERR("Error parsing generated private key, %d", mbed_status);
		ret = -EFAULT;
		goto cleanup;
	}

	/* Now that it has been copied, wipe the private key from the csr buffer to prevent leak. */
	memset(csr, 0, csr_max);

	/* Configure CSR writer */
	mbedtls_x509write_csr_set_md_alg(&writer, MBEDTLS_MD_SHA256);
	mbedtls_x509write_csr_set_key(&writer, &pk_ctx);
	mbed_status = mbedtls_x509write_csr_set_subject_name(&writer, dn);
	if (mbed_status) {
		LOG_ERR("Could not set distinguished name \"%s\" for CSR, error %d",
			dn, mbed_status);
		ret = -EINVAL;
		goto cleanup;
	}

	/* Write CSR to output buffer. */
	mbed_status = mbedtls_x509write_csr_der(&writer, csr, csr_max, psa_rng_for_mbedtls, NULL);

	if (mbed_status == MBEDTLS_ERR_ASN1_BUF_TOO_SMALL) {
		LOG_ERR("Failed to write CSR. Provided buffer too small.");
		ret = -EFBIG;
		goto cleanup;
	}
	if (mbed_status < 0) {
		LOG_ERR("Failed to write CSR. Status: %d", mbed_status);
		ret = -EFAULT;
		goto cleanup;
	}

	/* On success, mbed_status contains the number of bytes written. */
	*csr_len = mbed_status;

	/* mbedtls_x509write_csr_der writes its data to the end of the csr buffer.
	 * Shift this data to the beginning of the csr buffer for convenient use by the caller.
	 */
	cred_start = (char*)csr + csr_max - mbed_status;
	memmove(csr, cred_start, mbed_status);

cleanup:
	/* Destroy temporary contexts */
	mbedtls_x509write_csr_free(&writer);
	mbedtls_pk_free(&pk_ctx);

	/* If an error occurred, the CSR buffer is not valid. Wipe it.*/
	if (ret) {
		memset(csr, 0, csr_max);
		*csr_len = 0;
	}

	/* Unstore private key if an error occurred after it was stored
	 * Note that key_stored cannot be true if type == TLS_CREDENTIAL_KEYGEN_EXISTING,
	 * so this will never trigger for a pre-existing key.
	 */
	if (ret && key_stored) {
		cred_status = tls_credential_delete(tag, TLS_CREDENTIAL_PRIVATE_KEY);
		if (cred_status) {
			LOG_ERR("Failed to unstore CSR private key: %d ", cred_status);
		}
	}

	k_mutex_unlock(&credential_lock);

	return ret;
}

#else /* defined(MBEDTLS_X509_CSR_WRITE_C) */

int tls_credential_csr(sec_tag_t tag, char *dn, enum tls_credential_keygen_type type,
		       void *csr, size_t *csr_len)
{
	return -ENOTSUP;
}

#endif /* defined(MBEDTLS_X509_CSR_WRITE_C) */

#endif /* defined(CONFIG_TLS_CREDENTIAL_CSR) */