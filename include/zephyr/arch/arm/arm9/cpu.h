/*
 * Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_ARM9_CPU_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_ARM9_CPU_H_

/*
 * ARM Architecture Reference Manual
 * ARM document ID DDI0100E, June 2000
 * The mode bits:
 * Chapter A2.5.2 Table 2-2
 */
#define MODE_USR	0x10
#define MODE_FIQ	0x11
#define MODE_IRQ	0x12
#define MODE_SVC	0x13
#define MODE_ABT	0x17
#define MODE_UND	0x1b
#define MODE_SYS	0x1f
#define MODE_MASK	0x1f

#define I_BIT	(1 << 7)
#define F_BIT	(1 << 6)
#define T_BIT	(1 << 5)

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_ARM9_CPU_H_ */
