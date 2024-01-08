/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Internal helper function for generating digests for raw credentials.
 */

#ifndef __TLS_DIGEST_RAW_H
#define __TLS_DIGEST_RAW_H

#include <zephyr/net/tls_credentials.h>
#include "tls_internal.h"

/* Common version of credential_digest that raw credentials backends can use. */
int credential_digest_raw(struct tls_credential *credential, void *dest, size_t *len);

#endif /* __TLS_DIGEST_RAW_H */
