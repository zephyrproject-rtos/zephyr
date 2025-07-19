/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/llext/symbol.h>

EXPORT_SYMBOL(strcpy);
EXPORT_SYMBOL(strncpy);
EXPORT_SYMBOL(strlen);
EXPORT_SYMBOL(strcmp);
EXPORT_SYMBOL(strncmp);
EXPORT_SYMBOL(memcmp);
EXPORT_SYMBOL(memcpy);
EXPORT_SYMBOL(memset);

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
