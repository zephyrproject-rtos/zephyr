/*
 * Copyright (c) 2025 Linumiz GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __CERTIFICATE_H__
#define __CERTIFICATE_H__

#define CA_CERTIFICATE_TAG 1
#define TLS_PEER_HOSTNAME "localhost"

static const unsigned char ca_certificate[] = {
#include "ec.crt.inc"
};

#endif
