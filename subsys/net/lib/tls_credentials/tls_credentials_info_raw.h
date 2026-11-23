/*
 * Copyright (c) 2026 Joakim Andersson
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __TLS_INFO_RAW_H
#define __TLS_INFO_RAW_H

#include <zephyr/net/tls_credentials.h>
#include "tls_internal.h"

/* Common version of credential expiry retrieval that raw credentials backends can use.*/
int credential_info_expiry_get(struct tls_credential *credential, time_t *expiry_out);

#endif /* __TLS_INFO_RAW_H */
