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
#define IRQ_NUM_ESM_HIGH	0 /* cannot be remapped, is NMI (FIQ)	*/
#define IRQ_RTI_COMPARE_0	2 /* RTI1 compare interrupt 0		*/
#define IRQ_RTI_COMPARE_1	3 /* RTI1 compare interrupt 1		*/
#define IRQ_RTI_COMPARE_2	4 /* RTI1 compare interrupt 2		*/
#define IRQ_RTI_COMPARE_3	5 /* RTI1 compare interrupt 3		*/
#define IRQ_RTI_OVERFLOW_0	6 /* RTI1 overflow interrupt 1		*/
#define IRQ_RTI_OVERFLOW_1	7 /* RTI1 overflow interrupt 1		*/
#define IRQ_RTI_TIME_BASE	8 /* RTI1 time-base			*/
#define IRQ_GIO_HIGH_LEVEL	9 /* GIO high level interrupt		*/
#define IRQ_NUM_ESM_LOW		20 /* ESM low level interrupt		*/

#define DRV_SYS				DT_NODELABEL(sys)

#define DRV_SYS1			DT_REG_ADDR_BY_IDX(DRV_SYS, 0)
#define DRV_SYS2			DT_REG_ADDR_BY_IDX(DRV_SYS, 1)
#define DRV_PCR1			DT_REG_ADDR_BY_IDX(DRV_SYS, 2)
#define DRV_PCR2			DT_REG_ADDR_BY_IDX(DRV_SYS, 3)
#define DRV_PCR3			DT_REG_ADDR_BY_IDX(DRV_SYS, 4)
#define DRV_FCR				DT_REG_ADDR_BY_IDX(DRV_SYS, 5)
#define DRV_ESM				DT_REG_ADDR_BY_IDX(DRV_SYS, 6)
#define DRV_SYSBASE			DT_REG_ADDR_BY_IDX(DRV_SYS, 7)
#define DRV_WATCHDOG_STATUS		DT_REG_ADDR_BY_IDX(DRV_SYS, 8)
#define DRV_DCC				DT_REG_ADDR_BY_IDX(DRV_SYS, 9)
#define DRV_POM_CONTROL			DT_REG_ADDR_BY_IDX(DRV_SYS, 10)

#define DRV_VIMRAM			DT_REG_ADDR_BY_IDX(DT_NODELABEL(vim), 2)
#define DRV_VIMRAM_SZ			DT_REG_SIZE_BY_IDX(DT_NODELABEL(vim), 2)

#endif /* __TMS570_SOC_H__ */
