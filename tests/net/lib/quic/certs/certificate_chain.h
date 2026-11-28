/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __CERTIFICATE_CHAIN_H__
#define __CERTIFICATE_CHAIN_H__

/* A server certificate that the CA of certificate.h did not sign itself:
 * an intermediate CA signed it, and the CA signed the intermediate.
 */
#define CHAIN_SERVER_CERTIFICATE_TAG 4
#define INTERMEDIATE_CERTIFICATE_TAG 5

static const unsigned char intermediate_certificate[] = {
#include "intermediate.crt.inc"
	0x00, /* null terminator for PEM parsing */
};

static const unsigned char chain_server_certificate[] = {
#include "chain_server.crt.inc"
	0x00, /* null terminator for PEM parsing */
};

static const unsigned char chain_server_private_key[] = {
#include "chain_server.key.inc"
	0x00, /* null terminator for PEM parsing */
};

#endif /* __CERTIFICATE_CHAIN_H__ */
