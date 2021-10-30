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

static const unsigned char ca_certificate[] = "../server/ca.crt";
static const unsigned char self_certificate[] = "../server/client.crt";
static const unsigned char self_key[] = "../server/client.key";

#endif /* __CA_CERTIFICATE_H__ */
