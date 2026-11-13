/*
 * Copyright (c) 2019 O.S.Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __CERTIFICATE_H__
#define __CERTIFICATE_H__

#if defined(CONFIG_UPDATEHUB_DTLS)
/* Mbed TLS only parses PEM, and so more than one certificate, from a
 * NUL-terminated buffer.
 */
static const unsigned char updatehub_root_ca[] = {
#include "updatehub-root-ca.pem.inc"
0x00
};
#endif

#endif /* __CERTIFICATE_H__ */
