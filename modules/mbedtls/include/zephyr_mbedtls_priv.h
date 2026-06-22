/*
 * Copyright (C) 2022 Marcin Niestroj
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_MODULES_MBEDTLS_PRIV_H_
#define ZEPHYR_MODULES_MBEDTLS_PRIV_H_

void zephyr_mbedtls_debug(void *ctx, int level, const char *file, int line, const char *str);

#endif /* ZEPHYR_MODULES_MBEDTLS_PRIV_H_ */
