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

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/sys/util.h>

GTEXT(__start)
GDATA(_vector_table)

GTEXT(z_arm_nmi)
GTEXT(z_arm_undef_instruction)
GTEXT(z_arm_svc)
GTEXT(z_arm_prefetch_abort)
GTEXT(z_arm_data_abort)

GTEXT(z_arm_pendsv)
GTEXT(z_arm_reserved)

GTEXT(z_prep_c)
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
