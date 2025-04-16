/*
 * Copyright (c) 2025 ispace, inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __TMS570_SOC_H__
#define __TMS570_SOC_H__

/* Do not let CMSIS to handle GIC and Timer */
#include <stdint.h>
#define __GIC_PRESENT 0

/* these are the default "Interrupt request assignments", see table in datasheet with same name */
#define IRQ_NUM_ESM_HIGH	0 /* cannot be remapped, is NMI (FIQ) 		*/
#define IRQ_RTI_COMPARE_0	2 /* RTI1 compare interrupt 0 			*/
#define IRQ_RTI_COMPARE_1	3 /* RTI1 compare interrupt 1 			*/
#define IRQ_RTI_COMPARE_2	4 /* RTI1 compare interrupt 2 			*/
#define IRQ_RTI_COMPARE_3	5 /* RTI1 compare interrupt 3 			*/
#define IRQ_RTI_OVERFLOW_0	6 /* RTI1 overflow interrupt 1			*/
#define IRQ_RTI_OVERFLOW_1	7 /* RTI1 overflow interrupt 1			*/
#define IRQ_RTI_TIME_BASE	8 /* RTI1 time-base				*/
#define IRQ_GIO_HIGH_LEVEL	9 /* GIO high level interrupt			*/
#define IRQ_NUM_ESM_LOW		20 /* ESM low level interrupt	 		*/

#endif /* __TMS570_SOC_H__ */
