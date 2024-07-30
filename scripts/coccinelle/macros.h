/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* File to be specified by -macro-file in invocations of coccinelle
 * to avoid parse errors that prevent application of rules.
 *
 * This is not exhaustive: only defines that have been proven to
 * inhibit context recognition are listed.  The structure of the file
 * is expected to follow that of the Coccinelle standard.h macro file.
 */

/* Zephyr macros */

#define ZTEST(suite, fn) static void _##suite##_##fn##_wrapper(void)

/* Attributes */

/* Confirmed problematic */
#define __noinit
#define __syscall
