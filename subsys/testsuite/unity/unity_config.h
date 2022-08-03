/* SPDX-License-Identifier: Apache-2.0 */
/**
 * Copyright 2022 Martin Schröder <info@swedishembedded.com>
 * Consulting: https://swedishembedded.com/go
 * Training: https://swedishembedded.com/tag/training
 **/

#ifndef UNITY_CONFIG_H__
#define UNITY_CONFIG_H__

#define UNITY_OUTPUT_COLOR
#define UNITY_USE_COMMAND_LINE_ARGS

#ifndef CONFIG_BOARD_NATIVE_POSIX
#include <stddef.h>
#include <stdio.h>
#define UNITY_EXCLUDE_SETJMP_H 1
#define UNITY_OUTPUT_CHAR(a) printf("%c", a)
#endif

extern int unity_main(int argc, char **argv);

#endif
