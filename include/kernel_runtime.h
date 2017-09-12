/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file kernel_runtime.h
 * @brief Kernel to runtime library interfaces
 *
 * These APIs are only intended to be used by the Zephyr runtime library
 * or internally to the kernel.
 */

#ifndef _ZEPHYR_RUNTIME_H_
#define _ZEPHYR_RUNTIME_H_

#ifndef _ASMLANGUAGE
#include <zephyr.h>

#ifdef __cplusplus
extern "C" {
#endif


#ifdef CONFIG_PRINTK
extern int (*_char_out)(int);

/**
 * @brief Emit one character to the console device
 *
 * This is used internally by printk() and related functions.
 *
 * @param c Character value to print
 * @return The printed character, or 0 if nothing was printed
 */
static inline int _k_char_out(int c)
{
	return _char_out(c);
}

/**
 * @brief Emit a character buffer to the console device
 *
 * @param c String of characters to print
 * @param c The length of the string
 */
__syscall void _k_str_out(char *c, size_t n);
#endif /* CONFIG_PRINTK */

#ifdef __cplusplus
}
#endif

#include <syscalls/kernel_runtime.h>

#endif /* _ASMLANGUAGE */
#endif /* _ZEPHYR_RUNTIME_H_ */
