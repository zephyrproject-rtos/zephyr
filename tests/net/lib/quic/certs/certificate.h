/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __CERTIFICATE_H__
#define __CERTIFICATE_H__

#define SERVER_CERTIFICATE_TAG 1
#define CA_CERTIFICATE_TAG 2
#define CLIENT_CERTIFICATE_TAG 3

static const unsigned char ca_certificate[] = {
#include "ca.crt.inc"
	0x00, /* null terminator for PEM parsing */
};

static const unsigned char server_certificate[] = {
#include "server.crt.inc"
	0x00, /* null terminator for PEM parsing */
};

/* This is the private key in pkcs#8 format. */
static const unsigned char server_private_key[] = {
#include "server.key.inc"
	0x00, /* null terminator for PEM parsing */
};

static const unsigned char client_certificate[] = {
#include "client.crt.inc"
	0x00, /* null terminator for PEM parsing */
};

/* This is the private key in pkcs#8 format. */
static const unsigned char client_private_key[] = {
#include "client.key.inc"
	0x00, /* null terminator for PEM parsing */
};

#endif /* __CERTIFICATE_H__ */
