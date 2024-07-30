/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MBEDTLS_INIT_H
#define MBEDTLS_INIT_H

/* This should be called by platforms that do not wish to
 * have mbedtls initialised during kernel startup
 */
int mbedtls_init(void);

#endif /* MBEDTLS_INIT_H */
