/*
 * Copyright (c) 2024 Dawid Osuchowski
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_POSIX_STDIO_H_
#define ZEPHYR_INCLUDE_POSIX_STDIO_H_

#include "posix_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(__FILE_defined)
#define __FILE_defined
typedef int  FILE;
#endif

#define stdin  ((FILE *) 1)
#define stdout ((FILE *) 2)
#define stderr ((FILE *) 3)

int fgetc(FILE *stream);
#define getc(stream) fgetc(stream);
#define getchar(void) getc(stdin);


#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_STDIO_H_ */
