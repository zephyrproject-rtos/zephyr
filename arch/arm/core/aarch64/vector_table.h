/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Definitions for the boot vector table
 *
 *
 * Definitions for the boot vector table.
 *
 * System exception handler names all have the same format:
 *
 *   __<exception name with underscores>
 *
 * No other symbol has the same format, so they are easy to spot.
 */

#ifndef _VECTOR_TABLE_H_
#define _VECTOR_TABLE_H_

#ifdef _ASMLANGUAGE

#include <toolchain.h>
#include <linker/sections.h>

GTEXT(__start)
GDATA(_vector_table)
GTEXT(_isr_wrapper)

#else /* _ASMLANGUAGE */

#ifdef __cplusplus
extern "C" {
#endif

extern void *_vector_table[];

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* _VECTOR_TABLE_H_ */
