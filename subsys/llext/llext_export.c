/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <zephyr/llext/symbol.h>

EXPORT_GROUP_SYMBOL(LIBC, strcpy);
EXPORT_GROUP_SYMBOL(LIBC, strncpy);
EXPORT_GROUP_SYMBOL(LIBC, strlen);
EXPORT_GROUP_SYMBOL(LIBC, strcmp);
EXPORT_GROUP_SYMBOL(LIBC, strncmp);
EXPORT_GROUP_SYMBOL(LIBC, memcmp);
EXPORT_GROUP_SYMBOL(LIBC, memcpy);
EXPORT_GROUP_SYMBOL(LIBC, memset);
EXPORT_GROUP_SYMBOL(LIBC, memmove);
EXPORT_GROUP_SYMBOL(LIBC, strtoul);
EXPORT_GROUP_SYMBOL(LIBC, sprintf);

#if defined(CONFIG_ARM) && !defined(CONFIG_CPU_AARCH32_CORTEX_R)
/*
 * On AArch32, compilers may lower memcpy()/memmove()/memset()/zero-init to the
 * ARM EABI run-time helpers instead of the C library functions. These helpers
 * are provided by the compiler run-time (libgcc/compiler-rt); export them so
 * that extensions which reference them can be linked. The exact set emitted
 * depends on the compiler (e.g. Clang uses the aligned __aeabi_memset4 /
 * __aeabi_memclr4 variants where GCC may emit direct memset() calls).
 */
extern void __aeabi_memcpy(void);
extern void __aeabi_memcpy4(void);
extern void __aeabi_memcpy8(void);
extern void __aeabi_memmove(void);
extern void __aeabi_memmove4(void);
extern void __aeabi_memmove8(void);
extern void __aeabi_memset(void);
extern void __aeabi_memset4(void);
extern void __aeabi_memset8(void);
extern void __aeabi_memclr(void);
extern void __aeabi_memclr4(void);
extern void __aeabi_memclr8(void);

EXPORT_GROUP_SYMBOL(LIBC, __aeabi_memcpy);
EXPORT_GROUP_SYMBOL(LIBC, __aeabi_memcpy4);
EXPORT_GROUP_SYMBOL(LIBC, __aeabi_memcpy8);
EXPORT_GROUP_SYMBOL(LIBC, __aeabi_memmove);
EXPORT_GROUP_SYMBOL(LIBC, __aeabi_memmove4);
EXPORT_GROUP_SYMBOL(LIBC, __aeabi_memmove8);
EXPORT_GROUP_SYMBOL(LIBC, __aeabi_memset);
EXPORT_GROUP_SYMBOL(LIBC, __aeabi_memset4);
EXPORT_GROUP_SYMBOL(LIBC, __aeabi_memset8);
EXPORT_GROUP_SYMBOL(LIBC, __aeabi_memclr);
EXPORT_GROUP_SYMBOL(LIBC, __aeabi_memclr4);
EXPORT_GROUP_SYMBOL(LIBC, __aeabi_memclr8);
#endif /* CONFIG_ARM && !CONFIG_CPU_AARCH32_CORTEX_R */

/* These symbols are used if CCAC is given the flag -Os */
#ifdef __CCAC__
extern void __ac_mc_va(void);
extern void __ac_push_13_to_13(void);
extern void __ac_push_13_to_14(void);
extern void __ac_push_13_to_15(void);
extern void __ac_push_13_to_16(void);
extern void __ac_push_13_to_17(void);
extern void __ac_push_13_to_18(void);
extern void __ac_push_13_to_19(void);
extern void __ac_push_13_to_20(void);
extern void __ac_push_13_to_21(void);
extern void __ac_push_13_to_22(void);
extern void __ac_push_13_to_23(void);
extern void __ac_push_13_to_24(void);
extern void __ac_push_13_to_25(void);
extern void __ac_push_13_to_26(void);
extern void __ac_push_none(void);
extern void __ac_pop_13_to_13(void);
extern void __ac_pop_13_to_13v(void);
extern void __ac_pop_13_to_14(void);
extern void __ac_pop_13_to_14v(void);
extern void __ac_pop_13_to_15(void);
extern void __ac_pop_13_to_15v(void);
extern void __ac_pop_13_to_16(void);
extern void __ac_pop_13_to_16v(void);
extern void __ac_pop_13_to_17(void);
extern void __ac_pop_13_to_17v(void);
extern void __ac_pop_13_to_18(void);
extern void __ac_pop_13_to_18v(void);
extern void __ac_pop_13_to_19(void);
extern void __ac_pop_13_to_19v(void);
extern void __ac_pop_13_to_20(void);
extern void __ac_pop_13_to_20v(void);
extern void __ac_pop_13_to_21(void);
extern void __ac_pop_13_to_21v(void);
extern void __ac_pop_13_to_22(void);
extern void __ac_pop_13_to_22v(void);
extern void __ac_pop_13_to_23(void);
extern void __ac_pop_13_to_23v(void);
extern void __ac_pop_13_to_24(void);
extern void __ac_pop_13_to_24v(void);
extern void __ac_pop_13_to_25(void);
extern void __ac_pop_13_to_25v(void);
extern void __ac_pop_13_to_26(void);
extern void __ac_pop_13_to_26v(void);
extern void __ac_pop_none(void);
extern void __ac_pop_nonev(void);

EXPORT_SYMBOL(__ac_mc_va);
EXPORT_SYMBOL(__ac_push_13_to_13);
EXPORT_SYMBOL(__ac_push_13_to_14);
EXPORT_SYMBOL(__ac_push_13_to_15);
EXPORT_SYMBOL(__ac_push_13_to_16);
EXPORT_SYMBOL(__ac_push_13_to_17);
EXPORT_SYMBOL(__ac_push_13_to_18);
EXPORT_SYMBOL(__ac_push_13_to_19);
EXPORT_SYMBOL(__ac_push_13_to_20);
EXPORT_SYMBOL(__ac_push_13_to_21);
EXPORT_SYMBOL(__ac_push_13_to_22);
EXPORT_SYMBOL(__ac_push_13_to_23);
EXPORT_SYMBOL(__ac_push_13_to_24);
EXPORT_SYMBOL(__ac_push_13_to_25);
EXPORT_SYMBOL(__ac_push_13_to_26);
EXPORT_SYMBOL(__ac_push_none);
EXPORT_SYMBOL(__ac_pop_13_to_13);
EXPORT_SYMBOL(__ac_pop_13_to_13v);
EXPORT_SYMBOL(__ac_pop_13_to_14);
EXPORT_SYMBOL(__ac_pop_13_to_14v);
EXPORT_SYMBOL(__ac_pop_13_to_15);
EXPORT_SYMBOL(__ac_pop_13_to_15v);
EXPORT_SYMBOL(__ac_pop_13_to_16);
EXPORT_SYMBOL(__ac_pop_13_to_16v);
EXPORT_SYMBOL(__ac_pop_13_to_17);
EXPORT_SYMBOL(__ac_pop_13_to_17v);
EXPORT_SYMBOL(__ac_pop_13_to_18);
EXPORT_SYMBOL(__ac_pop_13_to_18v);
EXPORT_SYMBOL(__ac_pop_13_to_19);
EXPORT_SYMBOL(__ac_pop_13_to_19v);
EXPORT_SYMBOL(__ac_pop_13_to_20);
EXPORT_SYMBOL(__ac_pop_13_to_20v);
EXPORT_SYMBOL(__ac_pop_13_to_21);
EXPORT_SYMBOL(__ac_pop_13_to_21v);
EXPORT_SYMBOL(__ac_pop_13_to_22);
EXPORT_SYMBOL(__ac_pop_13_to_22v);
EXPORT_SYMBOL(__ac_pop_13_to_23);
EXPORT_SYMBOL(__ac_pop_13_to_23v);
EXPORT_SYMBOL(__ac_pop_13_to_24);
EXPORT_SYMBOL(__ac_pop_13_to_24v);
EXPORT_SYMBOL(__ac_pop_13_to_25);
EXPORT_SYMBOL(__ac_pop_13_to_25v);
EXPORT_SYMBOL(__ac_pop_13_to_26);
EXPORT_SYMBOL(__ac_pop_13_to_26v);
EXPORT_SYMBOL(__ac_pop_none);
EXPORT_SYMBOL(__ac_pop_nonev);
#endif

#include <zephyr/syscall_exports_llext.c>
