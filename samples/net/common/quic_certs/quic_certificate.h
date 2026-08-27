/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __QUIC_CERTIFICATE_H__
#define __QUIC_CERTIFICATE_H__

#define QUIC_SERVER_CERTIFICATE_TAG 3
#define QUIC_PSK_TAG 4
#define QUIC_CA_CERTIFICATE_TAG 5

/* TLS credentials API requires that the PEM format certificates
 * and keys are stored as null-terminated strings, so add 0x00 at
 * the end.
 */
static const unsigned char quic_server_certificate[] = {
#include "quic_server_ec_cert.pem.inc"
0x00
};

/* This is the private key in pkcs#8 format. */
static const unsigned char quic_server_private_key[] = {
#include "quic_server_ec_key.pem.inc"
0x00
};

/* Used by the Quic echo-client */
static const unsigned char quic_ca_certificate[] = {
#include "quic_server_ec_cert.pem.inc"
0x00
};

#endif /* __QUIC_CERTIFICATE_H__ */
