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

extern const unsigned char ca_crt[];
extern const unsigned int ca_crt_len;
extern const unsigned char client_crt[];
extern const unsigned int client_crt_len;
extern const unsigned char client_key[];
extern const unsigned int client_key_len;

#endif /* __CA_CERTIFICATE_H__ */
