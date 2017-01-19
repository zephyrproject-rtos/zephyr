/* stdlib.h */

/*
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_stdlib_h__
#define __INC_stdlib_h__

#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

unsigned long int strtoul(const char *str, char **endptr, int base);
long int strtol(const char *str, char **endptr, int base);
int atoi(const char *s);

#ifdef __cplusplus
}
#endif

#endif  /* __INC_stdlib_h__ */
