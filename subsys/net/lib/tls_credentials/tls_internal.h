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

#endif /* __TLS_INTERNAL_H */
