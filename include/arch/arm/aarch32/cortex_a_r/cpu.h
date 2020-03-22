/*
 * Copyright (c) 2018 Lexmark International, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_CORTEX_A_R_CPU_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_CORTEX_A_R_CPU_H_

#define MODE_USR	0x10
#define MODE_FIQ	0x11
#define MODE_IRQ	0x12
#define MODE_SVC	0x13
#define MODE_ABT	0x17
#define MODE_UND	0x1b
#define MODE_SYS	0x1f
#define MODE_MASK	0x1f

#define A_BIT	(1 << 8)
#define I_BIT	(1 << 7)
#define F_BIT	(1 << 6)
#define T_BIT	(1 << 5)

#define HIVECS	(1 << 13)

#define CPACR_NA	(0U)
#define CPACR_FA	(3U)

#define CPACR_CP10(r)	(r << 20)
#define CPACR_CP11(r)	(r << 22)

#define FPEXC_EN	(1 << 30)

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_CORTEX_A_R_CPU_H_ */
