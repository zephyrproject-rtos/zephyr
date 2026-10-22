/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define CA_CERTIFICATE_TAG 1

#define TLS_PEER_HOSTNAME "localhost"

/* This is the same cert as what is found in net-tools/https-cert.pem file
 */
static const unsigned char ca_certificate[] = {
#include "https-cert.der.inc"
};
