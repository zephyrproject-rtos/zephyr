/*
 * Copyright (c) 2018 Marvell
 * Copyright (c) 2018 Lexmark International, Inc.
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

#ifndef _VECTOR_TABLE__H_
#define _VECTOR_TABLE__H_

#ifdef _ASMLANGUAGE

#include <toolchain.h>
#include <linker/sections.h>
#include <misc/util.h>

GTEXT(__start)
GTEXT(_vector_table)

GTEXT(__nmi)
GTEXT(__undef_instruction)
GTEXT(__svc)
GTEXT(__prefetch_abort)
GTEXT(__data_abort)

GTEXT(__pendsv)
GTEXT(__reserved)

GTEXT(_PrepC)
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

#endif /* _VECTOR_TABLE__H_ */
