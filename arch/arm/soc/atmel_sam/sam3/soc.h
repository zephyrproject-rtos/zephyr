/*
 * Copyright (c) 2016 Intel Corporation.
 * Copyright (c) 2013-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the Atmel SAM3 family processors.
 */

#ifndef _ATMEL_SAM3_SOC_H_
#define _ATMEL_SAM3_SOC_H_

/* IRQ numbers (from section 9.1, Peripheral Identifiers). */
#define IRQ_SUPC	0	/* Supply Controller                    */
#define IRQ_RSTC	1	/* Reset Controller                     */
#define IRQ_RTC		2	/* Real-time Clock                      */
#define IRQ_RTT		3	/* Real-time Timer                      */
#define IRQ_WDG		4	/* Watchdog Timer                       */
#define IRQ_PMC		5	/* Power Management Controller          */
#define IRQ_EEFC0	6	/* Enhanced Embedded Flash Controller 0 */
#define IRQ_EEFC1	7	/* Enhanced Embedded Flash Controller 1 */
#define IRQ_UART	8	/* UART                                 */
#define IRQ_PIOA	11	/* Parallel IO Controller A             */
#define IRQ_PIOB	12	/* Parallel IO Controller B             */
#define IRQ_PIOC	13	/* Parallel IO Controller C             */
#define IRQ_PIOD	14	/* Parallel IO Controller D             */
#define IRQ_PIOE	15	/* Parallel IO Controller E             */
#define IRQ_PIOF	16	/* Parallel IO Controller F             */
#define IRQ_USART0	17	/* USART #0                             */
#define IRQ_USART1	18	/* USART #1                             */
#define IRQ_USART2	19	/* USART #2                             */
#define IRQ_USART3	20	/* USART #3                             */
#define IRQ_HSMCI	21	/* High Speed Multimedia Card Interface */
#define IRQ_TWI0	22	/* Two-wire Interface #0                */
#define IRQ_TWI1	23	/* Two-wire Interface #1                */
#define IRQ_SPI0	24	/* SPI #0                               */
#define IRQ_SPI1	25	/* SPI #1                               */
#define IRQ_SSC		26	/* Synchronous Serial Controller        */
#define IRQ_TC0		27	/* Timer Counter Channel #0             */
#define IRQ_TC1		28	/* Timer Counter Channel #1             */
#define IRQ_TC2		29	/* Timer Counter Channel #2             */
#define IRQ_TC3		30	/* Timer Counter Channel #3             */
#define IRQ_TC4		31	/* Timer Counter Channel #4             */
#define IRQ_TC5		32	/* Timer Counter Channel #5             */
#define IRQ_TC6		33	/* Timer Counter Channel #6             */
#define IRQ_TC7		34	/* Timer Counter Channel #7             */
#define IRQ_TC8		35	/* Timer Counter Channel #8             */
#define IRQ_PWM		36	/* PWM Controller                       */
#define IRQ_ADC		37	/* ADC Controller                       */
#define IRQ_DACC	38	/* DAC Controller                       */
#define IRQ_DMAC	39	/* DMA Controller                       */
#define IRQ_UOTGHS	40	/* USB OTG High Speed                   */
#define IRQ_TRNG	41	/* True Random Number Generator         */
#define IRQ_EMAC	42	/* Ehternet MAC                         */
#define IRQ_CAN0	43	/* CAN Controller #0                    */
#define IRQ_CAN1	44	/* CAN Controller #1                    */

/* PID: Peripheral IDs (from section 9.1, Peripheral Identifiers).
 * PMC uses PIDs to enable clock for peripherals.
 */
#define PID_RTC		2	/* Real-time Clock                      */
#define PID_RTT		3	/* Real-time Timer                      */
#define PID_WDG		4	/* Watchdog Timer                       */
#define PID_PMC		5	/* Power Management Controller          */
#define PID_EEFC0	6	/* Enhanced Embedded Flash Controller 0 */
#define PID_EEFC1	7	/* Enhanced Embedded Flash Controller 1 */
#define PID_UART	8	/* UART                                 */
#define PID_PIOA	11	/* Parallel IO Controller A             */
#define PID_PIOB	12	/* Parallel IO Controller B             */
#define PID_PIOC	13	/* Parallel IO Controller C             */
#define PID_PIOD	14	/* Parallel IO Controller D             */
#define PID_PIOE	15	/* Parallel IO Controller E             */
#define PID_PIOF	16	/* Parallel IO Controller F             */
#define PID_USART0	17	/* USART #0                             */
#define PID_USART1	18	/* USART #1                             */
#define PID_USART2	19	/* USART #2                             */
#define PID_USART3	20	/* USART #3                             */
#define PID_HSMCI	21	/* High Speed Multimedia Card Interface */
#define PID_TWI0	22	/* Two-wire Interface #0                */
#define PID_TWI1	23	/* Two-wire Interface #1                */
#define PID_SPI0	24	/* SPI #0                               */
#define PID_SPI1	25	/* SPI #1                               */
#define PID_SSC		26	/* Synchronous Serial Controller        */
#define PID_TC0		27	/* Timer Counter Channel #0             */
#define PID_TC1		28	/* Timer Counter Channel #1             */
#define PID_TC2		29	/* Timer Counter Channel #2             */
#define PID_TC3		30	/* Timer Counter Channel #3             */
#define PID_TC4		31	/* Timer Counter Channel #4             */
#define PID_TC5		32	/* Timer Counter Channel #5             */
#define PID_TC6		33	/* Timer Counter Channel #6             */
#define PID_TC7		34	/* Timer Counter Channel #7             */
#define PID_TC8		35	/* Timer Counter Channel #8             */
#define PID_PWM		36	/* PWM Controller                       */
#define PID_ADC		37	/* ADC Controller                       */
#define PID_DACC	38	/* DAC Controller                       */
#define PID_DMAC	39	/* DMA Controller                       */
#define PID_UOTGHS	40	/* USB OTG High Speed                   */
#define PID_TRNG	41	/* True Random Number Generator         */
#define PID_EMAC	42	/* Ehternet MAC                         */
#define PID_CAN0	43	/* CAN Controller #0                    */
#define PID_CAN1	44	/* CAN Controller #1                    */

/* Power Manager Controller */
#define	PMC_ADDR	0x400E0600

#define PMC_CKGR_UCKR_UPLLEN		(1 << 16)
#define PMC_CKGR_UCKR_UPLLCOUNT		(3 << 20)

#define PMC_CKGR_MOR_KEY		(0x37 << 16)
#define PMC_CKGR_MOR_MOSCXTST		(0xFF << 8)
#define PMC_CKGR_MOR_MOSCXTEN		(1 << 0)
#define PMC_CKGR_MOR_MOSCRCEN		(1 << 3)
#define PMC_CKGR_MOR_MOSCRCF_4MHZ	(0 << 4)
#define PMC_CKGR_MOR_MOSCRCF_8MHZ	(1 << 4)
#define PMC_CKGR_MOR_MOSCRCF_12MHZ	(2 << 4)
#define PMC_CKGR_MOR_MOSCSEL		(1 << 24)

#define PMC_CKGR_PLLAR_PLLACOUNT	(0x3F << 8)
#define PMC_CKGR_PLLAR_ONE		(1 << 29)

/*
 * PLL clock = Main * (MULA + 1) / DIVA
 *
 * By default, MULA == 6, DIVA == 1.
 * With main crystal running at 12 MHz,
 * PLL = 12 * (6 + 1) / 1 = 84 MHz
 *
 * With processor clock prescaler at 1,
 * the processor clock is at 84 MHz.
 */
#define PMC_CKGR_PLLAR_MULA	\
	((CONFIG_SOC_ATMEL_SAM3_PLLA_MULA) << 16)
#define PMC_CKGR_PLLAR_DIVA	\
	((CONFIG_SOC_ATMEL_SAM3_PLLA_DIVA) << 0)

#define PMC_MCKR_CSS_MASK		(0x3)
#define PMC_MCKR_CSS_SLOW		(0 << 0)
#define PMC_MCKR_CSS_MAIN		(1 << 0)
#define PMC_MCKR_CSS_PLLA		(2 << 0)
#define PMC_MCKR_CSS_UPLL		(3 << 0)
#define PMC_MCKR_PRES_MASK		(0x70)
#define PMC_MCKR_PRES_CLK		(0 << 4)
#define PMC_MCKR_PRES_DIV2		(1 << 4)
#define PMC_MCKR_PRES_DIV4		(2 << 4)
#define PMC_MCKR_PRES_DIV8		(3 << 4)
#define PMC_MCKR_PRES_DIV16		(4 << 4)
#define PMC_MCKR_PRES_DIV32		(5 << 4)
#define PMC_MCKR_PRES_DIV64		(6 << 4)
#define PMC_MCKR_PRES_DIV3		(7 << 4)
#define PMC_MCKR_PLLADIV2		(1 << 12)
#define PMC_MCKR_UPLLDIV2		(1 << 13)

#define PMC_FSMR_LPM			(1 << 20)

#define PMC_INT_MOSCXTS			(1 << 0)
#define PMC_INT_LOCKA			(1 << 1)
#define PMC_INT_MCKRDY			(1 << 3)
#define PMC_INT_LOCKU			(1 << 6)
#define PMC_INT_OSCSELS			(1 << 7)
#define PMC_INT_PCKRDY0			(1 << 8)
#define PMC_INT_PCKRDY1			(1 << 9)
#define PMC_INT_PCKRDY2			(1 << 10)
#define PMC_INT_MOSCSELS		(1 << 16)
#define PMC_INT_MOSCRCS			(1 << 17)
#define PMC_INT_CFDEV			(1 << 18)
#define PMC_INT_CFDS			(1 << 19)
#define PMC_INT_FOS			(1 << 20)

/* UART */
#define UART_ADDR	0x400E0800

/* EEFC */
#define EEFC_BANK0_ADDR	0x400E0A00
#define EEFC_BANK1_ADDR	0x400E0C00

/* Peripheral DMA Controller (PDC) */
#define PDC_PTCR_RXTEN	(1 << 0)
#define PDC_PTCR_RXTDIS	(1 << 1)
#define PDC_PTCR_TXTEN	(1 << 8)
#define PDC_PTCR_TXTDIS	(1 << 9)

/* PIO Controllers */
#define PIOA_ADDR	0x400E0E00
#define PIOB_ADDR	0x400E1000
#define PIOC_ADDR	0x400E1200
#define PIOD_ADDR	0x400E1400
#define PIOE_ADDR	0x400E1600
#define PIOF_ADDR	0x400E1800

/* Supply Controller (SUPC) */
#define SUPC_ADDR	0x400E1A10

#define SUPC_CR_KEY	(0xA5 << 24)
#define SUPC_CR_XTALSEL	(1 << 3)

#define SUPC_SR_OSCSEL	(1 << 7)

/* Two-wire Interface (TWI) */
#define TWI0_ADDR	0x4008C000
#define TWI1_ADDR	0x40090000

/* Watchdog timer (WDT) */
#define WDT_ADDR	0x400E1A50

#define WDT_DISABLE	(1 << 15)

#ifndef _ASMLANGUAGE

#include <device.h>
#include <misc/util.h>
#include <drivers/rand32.h>

#include "soc_registers.h"

/* uart configuration settings */
#define UART_IRQ_FLAGS 0

/* EEFC Register struct */
#define __EEFC0		((volatile struct __eefc *)EEFC_BANK0_ADDR)
#define __EEFC1		((volatile struct __eefc *)EEFC_BANK1_ADDR)

/* PMC Register struct */
#define __PMC		((volatile struct __pmc *)PMC_ADDR)

/* PIO Registers struct */
#define __PIOA		((volatile struct __pio *)PIOA_ADDR)
#define __PIOB		((volatile struct __pio *)PIOB_ADDR)
#define __PIOC		((volatile struct __pio *)PIOC_ADDR)
#define __PIOD		((volatile struct __pio *)PIOD_ADDR)
#define __PIOE		((volatile struct __pio *)PIOE_ADDR)
#define __PIOF		((volatile struct __pio *)PIOF_ADDR)

/* Supply Controller Register struct */
#define __SUPC		((volatile struct __supc *)SUPC_ADDR)

/* Two-wire Interface (TWI) */
#define __TWI0		((volatile struct __twi *)TWI0_ADDR)
#define __TWI1		((volatile struct __twi *)TWI1_ADDR)

/* Watchdog timer (WDT) */
#define __WDT		((volatile struct __wdt *)WDT_ADDR)

#endif /* !_ASMLANGUAGE */

#endif /* _ATMEL_SAM3_SOC_H_ */
