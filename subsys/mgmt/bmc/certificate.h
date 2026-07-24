/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-FileCopyrightText: © 2025-2026 Tenstorrent USA, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __CERTIFICATE_H__
#define __CERTIFICATE_H__

#if defined(CONFIG_BMC_APP_HTTPS_PUBLIC_KEY)
static const unsigned char server_certificate[] = {
#include "server_cert.der.inc"
};

/* This is the private key in pkcs#8 format. */
static const unsigned char private_key[] = {
#include "server_privkey.der.inc"
};
#endif

#endif /* __CERTIFICATE_H__ */
