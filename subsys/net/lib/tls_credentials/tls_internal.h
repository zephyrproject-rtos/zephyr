/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Internal API for fetching TLS credentials
 */

#ifndef __TLS_INTERNAL_H
#define __TLS_INTERNAL_H

#include <zephyr/net/tls_credentials.h>

/* Internal structure representing TLS credential. */
struct tls_credential {
	/* TLS credential type. */
	enum tls_credential_type type;

	/* Secure tag that credential can be referenced with. */
	sec_tag_t tag;

	/* A pointer to the credential buffer. */
	const void *buf;

	/* Credential length. */
	size_t len;
};

/*
 * Special sec_tag value indicating none or invalid sec_tag. For internal use only for now.
 */
#define TLS_SEC_TAG_NONE -1

/* Lock TLS credential access. */
void credentials_lock(void);

/* Unlock TLS credential access. */
void credentials_unlock(void);

/* Function for getting credential by tag and type.
 *
 * Note, that to assure thread safety, credential access should be locked with
 * credentials_lock before calling this function.
 */
struct tls_credential *credential_get(sec_tag_t tag,
				      enum tls_credential_type type);

/* Function for iterating over credentials by tag.
 *
 * Note, that to assure thread safety, credential access should be locked with
 * credentials_lock before calling this function.
 */
struct tls_credential *credential_next_get(sec_tag_t tag,
					   struct tls_credential *iter);

/* Function for iterating over occupied sec tags.
 *
 * Returns the next occupied sec tag after the one provided, or TLS_SEC_TAG_NONE if there are no
 * more.
 *
 * Provide TLS_SEC_TAG_NONE to start from the first available sec tag.
 */
sec_tag_t credential_next_tag_get(sec_tag_t iter);

/* Writes a (NULL-terminated, printable) string digest of the contents of the provided credential
 * to the provided destination buffer.
 *
 * Digest format/type is up to the tls_credentials backend in use.
 *
 * len pointer should be set to the amount of space available in the destination buffer prior to
 * calling, and will be set to the amount written to the destination buffer after calling
 * (excluding the NULL terminator).
 *
 * Note, that to assure thread safety, credential access should be locked with
 * credentials_lock before calling this function.
 */
int credential_digest(struct tls_credential *credential, void *dest, size_t *len);

#endif /* __TLS_INTERNAL_H */
