/*
 * Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_MICROCHIP_SAM_PMC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_MICROCHIP_SAM_PMC_H_

#define PMC_TYPE_CORE		0
#define PMC_TYPE_SYSTEM		1
#define PMC_TYPE_PERIPHERAL	2
#define PMC_TYPE_GCK		3
#define PMC_TYPE_PROGRAMMABLE	4

#define PMC_SLOW		0
#define PMC_MCK			1
#define PMC_UTMI		2
#define PMC_MAIN		3

/* SAMA7G5 */
#define PMC_CPUPLL		(PMC_MAIN + 1)
#define PMC_SYSPLL		(PMC_MAIN + 2)
#define PMC_DDRPLL		(PMC_MAIN + 3)
#define PMC_IMGPLL		(PMC_MAIN + 4)
#define PMC_BAUDPLL		(PMC_MAIN + 5)
#define PMC_AUDIOPMCPLL		(PMC_MAIN + 6)
#define PMC_AUDIOIOPLL		(PMC_MAIN + 7)
#define PMC_ETHPLL		(PMC_MAIN + 8)
#define PMC_CPU			(PMC_MAIN + 9)
#define PMC_MCK1		(PMC_MAIN + 10)
#define UTMI1			0
#define UTMI2			1
#define UTMI3			2

/* SAMA7D65 */
#define PMC_LVDSPLL		(PMC_MAIN + 12)
#define PMC_MCK3		(PMC_MAIN + 20)
#define PMC_MCK5		(PMC_MAIN + 21)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_MICROCHIP_SAM_PMC_H_ */
