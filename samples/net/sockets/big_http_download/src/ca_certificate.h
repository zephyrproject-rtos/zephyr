/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __CA_CERTIFICATE_H__
#define __CA_CERTIFICATE_H__

#define CA_CERTIFICATE_TAG 1

/* By default only certificates in DER format are supported. If you want to use
 * certificate in PEM format, you can enable support for it in Kconfig.
 */

/* Let's Encrypt Authority X3 for https://www.7-zip.org */
static const unsigned char ca_certificate[] = {
#include "lets_encrypt_x3.der.inc"
};

#endif /* __CA_CERTIFICATE_H__ */
