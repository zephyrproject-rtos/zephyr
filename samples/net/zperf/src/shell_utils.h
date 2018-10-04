/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __SHELL_UTILS_H
#define __SHELL_UTILS_H

#include <shell/shell.h>

#define IPV4_STR_LEN_MAX 15
#define IPV4_STR_LEN_MIN 7

extern const u32_t TIME_US[];
extern const char *TIME_US_UNIT[];
extern const u32_t KBPS[];
extern const char *KBPS_UNIT[];
extern const u32_t K[];
extern const char *K_UNIT[];

extern void print_number(const struct shell *shell, u32_t value,
			 const u32_t *divisor, const char **units);
extern long parse_number(const char *string, const u32_t *divisor,
			 const char **units);
#endif /* __SHELL_UTILS_H */
