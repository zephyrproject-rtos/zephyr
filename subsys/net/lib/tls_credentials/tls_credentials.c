/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <string.h>

#include <zephyr/init.h>
#include <zephyr/kernel.h>

#include "tls_internal.h"
#include "tls_credentials_digest_raw.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(tls_credentials, CONFIG_TLS_CREDENTIALS_LOG_LEVEL);

/* Global pool of credentials shared among TLS contexts. */
static struct tls_credential credentials[CONFIG_TLS_MAX_CREDENTIALS_NUMBER];

/* A mutex for protecting access to the credentials array. */
static struct k_mutex credential_lock;

static int credentials_init(void)
{
	(void)memset(credentials, 0, sizeof(credentials));

	k_mutex_init(&credential_lock);

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

struct tls_credential *credential_get(sec_tag_t tag,
				      enum tls_credential_type type)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(credentials); i++) {
		if (credentials[i].type == type && credentials[i].tag == tag) {
			return &credentials[i];
		}
	}

	return NULL;
}

struct tls_credential *credential_next_get(sec_tag_t tag,
					   struct tls_credential *iter)
{
	int i;

	if (!iter) {
		iter = credentials;
	} else {
		iter++;
	}

	for (i = iter - credentials; i < ARRAY_SIZE(credentials); i++) {
		if (credentials[i].type != TLS_CREDENTIAL_NONE &&
		    credentials[i].tag == tag) {
			return &credentials[i];
		}
	}

	return NULL;
}

sec_tag_t credential_next_tag_get(sec_tag_t iter)
{
	int i;
	sec_tag_t lowest = TLS_SEC_TAG_NONE;

	/* Scan all slots and find lowest sectag greater than iter */
	for (i = 0; i < ARRAY_SIZE(credentials); i++) {
		/* Skip empty slots. */
		if (credentials[i].type == TLS_CREDENTIAL_NONE) {
			continue;
		}

		/* Skip any slots containing sectags not greater than iter */
		if (credentials[i].tag <= iter && iter != TLS_SEC_TAG_NONE) {
			continue;
		}

		/* Find the lowest of such slots */
		if (lowest == TLS_SEC_TAG_NONE || credentials[i].tag < lowest) {
			lowest = credentials[i].tag;
		}
	}

	return lowest;
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
	k_mutex_unlock(&credential_lock);
}

int tls_credential_add(sec_tag_t tag, enum tls_credential_type type,
		       const void *cred, size_t credlen)
{
	struct tls_credential *credential;
	int ret = 0;

	credentials_lock();

	credential = credential_get(tag, type);
	if (credential != NULL) {
		ret = -EEXIST;
		goto exit;
	}

	credential = unused_credential_get();
	if (credential == NULL) {
		ret = -ENOMEM;
		goto exit;
	}

	credential->tag = tag;
	credential->type = type;
	credential->buf = cred;
	credential->len = credlen;

exit:
	credentials_unlock();

	return ret;
}

int tls_credential_get(sec_tag_t tag, enum tls_credential_type type,
		       void *cred, size_t *credlen)
{
	struct tls_credential *credential;
	int ret = 0;

	credentials_lock();

	credential = credential_get(tag, type);
	if (credential == NULL) {
		ret = -ENOENT;
		*credlen = 0;
		goto exit;
	}

	if (credential->len > *credlen) {
		ret = -EFBIG;
		LOG_DBG("Not enough room in the credential buffer to "
			"retrieve credential with sectag %d and type %d. "
			"Increase TLS_CREDENTIALS_SHELL_MAX_CRED_LEN "
			">= %d.\n",
			tag, (int)type, (int)credential->len);
		*credlen = credential->len;
		goto exit;
	}

	*credlen = credential->len;
	memcpy(cred, credential->buf, credential->len);

exit:
	credentials_unlock();

	return ret;
}

int tls_credential_delete(sec_tag_t tag, enum tls_credential_type type)
{
	struct tls_credential *credential;
	int ret = 0;

	credentials_lock();

	credential = credential_get(tag, type);

	if (!credential) {
		ret = -ENOENT;
		goto exit;
	}

	(void)memset(credential, 0, sizeof(struct tls_credential));
	credential->type = TLS_CREDENTIAL_NONE;

exit:
	credentials_unlock();

	return ret;
}
