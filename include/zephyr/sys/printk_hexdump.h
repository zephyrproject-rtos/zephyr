/*
 * Copyright (c) 2024 GARDENA GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_PRINTK_HEXDUMP_H_
#define ZEPHYR_INCLUDE_SYS_PRINTK_HEXDUMP_H_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 *
 * @brief Print data buffer using printk().
 *
 * @param str Prefix for every line of data
 * @param data Pointer to data
 * @param data_length Number of bytes in @p data to be printed
 */
#ifdef CONFIG_PRINTK

void printk_hexdump(const char *str, const uint8_t *data, size_t data_length);

#else

static inline void printk_hexdump(const char *str, const uint8_t *data, size_t data_length);
{
	ARG_UNUSED(str);
	ARG_UNUSED(data);
	ARG_UNUSED(data_length);
}
#endif

#ifdef __cplusplus
}
#endif

#endif
