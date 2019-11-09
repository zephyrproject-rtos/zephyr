/*
 * Copyright (c) 2018 Lexmark International, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CORTEX_R_CPU_H
#define _CORTEX_R_CPU_H

#define MODE_USR	0x10
#define MODE_FIQ	0x11
#define MODE_IRQ	0x12
#define MODE_SVC	0x13
#define MODE_ABT	0x17
#define MODE_UDF	0x1b
#define MODE_SYS	0x1f
#define MODE_MASK	0x1f

#define A_BIT	(1 << 8)
#define I_BIT	(1 << 7)
#define F_BIT	(1 << 6)
#define T_BIT	(1 << 5)

#define HIVECS	(1 << 13)

#define RET_FROM_SVC	0
#define RET_FROM_IRQ	1

#define __ISB()	__asm__ volatile ("isb sy" : : : "memory")
#define __DMB()	__asm__ volatile ("dmb sy" : : : "memory")

#endif
