/*
 * Copyright (c) 2026 Takayuki Goto
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Definitions for the ARMv5 boot vector table
 */

#ifndef _ARM9_VECTOR_TABLE__H_
#define _ARM9_VECTOR_TABLE__H_

#ifdef _ASMLANGUAGE

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/sys/util.h>

GTEXT(__start)
GDATA(_vector_table)

GTEXT(z_arm_reset)
GTEXT(z_arm_undef_instruction)
GTEXT(z_arm_svc)
GTEXT(z_arm_prefetch_abort)
GTEXT(z_arm_data_abort)
GTEXT(z_arm_fiq)
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

#endif /* _ARM9_VECTOR_TABLE__H_ */
