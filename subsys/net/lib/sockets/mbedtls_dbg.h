/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2021 Emil Lindqvist
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <mbedtls/ssl.h>

#ifndef _MBEDTLS_DBG_
#define _MBEDTLS_DBG_

void mbedtls_dbg_init(mbedtls_ssl_config *config);

#endif
