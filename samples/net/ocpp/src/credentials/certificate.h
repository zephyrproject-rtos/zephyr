/*
 * Copyright (c) 2025 Linumiz GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __CERTIFICATE_H__
#define __CERTIFICATE_H__

#define CA_CERTIFICATE_TAG 1
#define TLS_PEER_HOSTNAME "localhost"

#ifdef USE_DUMMY_CREDS
static const unsigned char ca_certificate[] = { };
#else
static const unsigned char ca_certificate[] = {
/* Include your PEM/DER certificates */
'\0', /* Null-terminate in the case of PEM certificate for proper MBEDTLS parsing*/
};
#endif

#endif
