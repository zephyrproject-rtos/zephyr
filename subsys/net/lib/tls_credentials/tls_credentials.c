/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <string.h>

#include <zephyr/init.h>
#include <zephyr/kernel.h>

#include "tls_internal.h"

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
		goto exit;
	}

	if (credential->len > *credlen) {
		ret = -EFBIG;
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
