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
SYS_INIT(credentials_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

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


/* Get the following credential filtered by a TAG valud */
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

int tls_credential_add(sec_tag_t tag, enum tls_credential_type type,
		       const void *cred, size_t credlen)
{
	psa_storage_uid_t uid = tls_credential_get_uid(tag, type);
	psa_storage_create_flags_t create_flags = 0;
	psa_status_t status;
	unsigned int slot;
	int ret = 0;

	/* tag 0xffffffff type 0xffff are reserved */
	if (tag == 0xffffffff && type == 0xffff) {
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

	/* tag 0xffffffff type 0xffff are reserved */
	if (tag == 0xffffffff && type == 0xffff) {
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

	/* tag 0xffffffff type 0xffff are reserved */
	if (tag == 0xffffffff && type == 0xffff) {
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
