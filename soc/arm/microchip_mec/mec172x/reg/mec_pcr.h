/*
 * Copyright (c) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stddef.h>

#include "mec_defs.h"
#include "mec_regaccess.h"

#ifndef _MEC_PCR_H
#define _MEC_PCR_H

#define MCHP_PCR_BASE_ADDR		0x40080100ul

#define MCHP_PCR_SYS_SLP_CTRL_OFS	0x00ul
#define MCHP_PCR_SYS_CLK_CTRL_OFS	0x04ul
#define MCHP_PCR_SLOW_CLK_CTRL_OFS	0x08ul
#define MCHP_PCR_OSC_ID_OFS		0x0Cul
#define MCHP_PCR_PRS_OFS		0x10ul
#define MCHP_PCR_PR_CTRL_OFS		0x14ul
#define MCHP_PCR_SYS_RESET_OFS		0x18ul
#define MCHP_PCR_PKE_CLK_CTRL_OFS	0x1Cul
#define MCHP_PCR_SLP_EN0_OFS		0x30ul
#define MCHP_PCR_SLP_EN1_OFS		0x34ul
#define MCHP_PCR_SLP_EN2_OFS		0x38ul
#define MCHP_PCR_SLP_EN3_OFS		0x3Cul
#define MCHP_PCR_SLP_EN4_OFS		0x40ul
#define MCHP_PCR_CLK_REQ0_OFS		0x50ul
#define MCHP_PCR_CLK_REQ1_OFS		0x54ul
#define MCHP_PCR_CLK_REQ2_OFS		0x58ul
#define MCHP_PCR_CLK_REQ3_OFS		0x5Cul
#define MCHP_PCR_CLK_REQ4_OFS		0x60ul
#define MCHP_PCR_PERIPH_RST0_OFS	0x70ul
#define MCHP_PCR_PERIPH_RST1_OFS	0x74ul
#define MCHP_PCR_PERIPH_RST2_OFS	0x78ul
#define MCHP_PCR_PERIPH_RST3_OFS	0x7Cul
#define MCHP_PCR_PERIPH_RST4_OFS	0x80ul
#define MCHP_PCR_PERIPH_RST_LCK_OFS	0x84ul
#define MCHP_PCR_VBAT_SRST_OFS		0x88ul
#define MCHP_PCR_CLK32K_SRC_VTR_OFS	0x8Cul
#define MCHP_PCR_CNT32K_PER_OFS		0xC0ul
#define MCHP_PCR_CNT32K_PULSE_HI_OFS	0xC4ul
#define MCHP_PCR_CNT32K_PER_MIN_OFS	0xC8ul
#define MCHP_PCR_CNT32K_PER_MAX_OFS	0xCCul
#define MCHP_PCR_CNT32K_DV_OFS		0xD0ul
#define MCHP_PCR_CNT32K_DV_MAX_OFS	0xD4ul
#define MCHP_PCR_CNT32K_VALID_OFS	0xD8ul
#define MCHP_PCR_CNT32K_VALID_MIN_OFS	0xDCul
#define MCHP_PCR_CNT32K_CTRL_OFS	0xE0ul
#define MCHP_PCR_CLK32K_MON_ISTS_OFS	0xE4ul
#define MCHP_PCR_CLK32K_MON_IEN_OFS	0xE8ul

#define MCHP_PCR_SYS_SLP_CTRL_ADDR	(MCHP_PCR_BASE_ADDR)
#define MCHP_PCR_SYS_CLK_CTRL_ADDR	(MCHP_PCR_BASE_ADDR + 0x04ul)
#define MCHP_PCR_SLOW_CLK_CTRL_ADDR	(MCHP_PCR_BASE_ADDR + 0x08ul)
#define MCHP_PCR_OSC_ID_ADDR		(MCHP_PCR_BASE_ADDR + 0x0Cul)
#define MCHP_PCR_PRS_ADDR		(MCHP_PCR_BASE_ADDR + 0x10ul)
#define MCHP_PCR_PR_CTRL_ADDR		(MCHP_PCR_BASE_ADDR + 0x14ul)
#define MCHP_PCR_SYS_RESET_ADDR		(MCHP_PCR_BASE_ADDR + 0x18ul)
#define MCHP_PCR_PKE_CLK_CTRL_ADDR	(MCHP_PCR_BASE_ADDR + 0x1Cul)
#define MCHP_PCR_SLP_EN0_ADDR		(MCHP_PCR_BASE_ADDR + 0x30ul)
#define MCHP_PCR_SLP_EN1_ADDR		(MCHP_PCR_BASE_ADDR + 0x34ul)
#define MCHP_PCR_SLP_EN2_ADDR		(MCHP_PCR_BASE_ADDR + 0x38ul)
#define MCHP_PCR_SLP_EN3_ADDR		(MCHP_PCR_BASE_ADDR + 0x3Cul)
#define MCHP_PCR_SLP_EN4_ADDR		(MCHP_PCR_BASE_ADDR + 0x40ul)
#define MCHP_PCR_CLK_REQ0_ADDR		(MCHP_PCR_BASE_ADDR + 0x50ul)
#define MCHP_PCR_CLK_REQ1_ADDR		(MCHP_PCR_BASE_ADDR + 0x54ul)
#define MCHP_PCR_CLK_REQ2_ADDR		(MCHP_PCR_BASE_ADDR + 0x58ul)
#define MCHP_PCR_CLK_REQ3_ADDR		(MCHP_PCR_BASE_ADDR + 0x5Cul)
#define MCHP_PCR_CLK_REQ4_ADDR		(MCHP_PCR_BASE_ADDR + 0x60ul)
#define MCHP_PCR_PERIPH_RST0_ADDR	(MCHP_PCR_BASE_ADDR + 0x70ul)
#define MCHP_PCR_PERIPH_RST1_ADDR	(MCHP_PCR_BASE_ADDR + 0x74ul)
#define MCHP_PCR_PERIPH_RST2_ADDR	(MCHP_PCR_BASE_ADDR + 0x78ul)
#define MCHP_PCR_PERIPH_RST3_ADDR	(MCHP_PCR_BASE_ADDR + 0x7Cul)
#define MCHP_PCR_PERIPH_RST4_ADDR	(MCHP_PCR_BASE_ADDR + 0x80ul)
#define MCHP_PCR_PERIPH_RESET_LOCK_ADDR (MCHP_PCR_BASE_ADDR + 0x84ul)
#define MCHP_PCR_VBAT_SRST_ADDR		(MCHP_PCR_BASE_ADDR + 0x88ul)
#define MCHP_PCR_CLK32K_SRC_VTR_ADDR	(MCHP_PCR_BASE_ADDR + 0x8Cul)
#define MCHP_PCR_CNT32K_PER_ADDR	(MCHP_PCR_BASE_ADDR + 0xC0ul)
#define MCHP_PCR_CNT32K_PULSE_HI_ADDR	(MCHP_PCR_BASE_ADDR + 0xC4ul)
#define MCHP_PCR_CNT32K_PER_MIN_ADDR	(MCHP_PCR_BASE_ADDR + 0xC8ul)
#define MCHP_PCR_CNT32K_PER_MAX_ADDR	(MCHP_PCR_BASE_ADDR + 0xCCul)
#define MCHP_PCR_CNT32K_DV_ADDR		(MCHP_PCR_BASE_ADDR + 0xD0ul)
#define MCHP_PCR_CNT32K_DV_MAX_ADDR	(MCHP_PCR_BASE_ADDR + 0xD4ul)
#define MCHP_PCR_CNT32K_VALID_ADDR	(MCHP_PCR_BASE_ADDR + 0xD8ul)
#define MCHP_PCR_CNT32K_VALID_MIN_ADDR	(MCHP_PCR_BASE_ADDR + 0xDCul)
#define MCHP_PCR_CNT32K_CTRL_ADDR	(MCHP_PCR_BASE_ADDR + 0xE0ul)
#define MCHP_PCR_CLK32K_MON_ISTS_ADDR	(MCHP_PCR_BASE_ADDR + 0xE4ul)
#define MCHP_PCR_CLK32K_MON_IEN_ADDR	(MCHP_PCR_BASE_ADDR + 0xE8ul)

#define MCHP_PCR_SLP_EN_ADDR(n)	\
	(MCHP_PCR_BASE_ADDR + 0x30ul + ((uint32_t)(n) * 4U))
#define MCHP_PCR_CLK_REQ_ADDR(n) \
	(MCHP_PCR_BASE_ADDR + 0x50ul + ((uint32_t)(n) * 4U))
#define MCHP_PCR_PERIPH_RESET_ADDR(n) \
	(MCHP_PCR_BASE_ADDR + 0x70ul + ((uint32_t)(n) * 4U))

#define MCHP_PCR_SLEEP_EN	(1u)
#define MCHP_PCR_SLEEP_DIS	(0u)

/*
 * MEC172x PCR implements multiple SLP_EN, CLR_REQ, and RST_EN registers.
 * CLK_REQ bits are read-only. The peripheral sets its CLK_REQ if it requires
 * clocks. CLK_REQ bits must all be zero for the PCR block to put the MEC17xx
 * into light or heavy sleep.
 * SLP_EN bit = 1 instructs HW to gate off clock tree to peripheral only if
 * peripherals PCR CLK_REQ bit is 0.
 * RST_EN bit = 1 will reset the peripheral at any time. The RST_EN registers
 * must be unlocked by writing the unlock code to PCR Peripheral Reset Lock
 * register.
 * SLP_EN usage is:
 * Initialization set all PCR SLP_EN bits = 0 except for crypto blocks as
 * these IP do not implement internal clock gating.
 * When firmware wants to enter light or heavy sleep.
 * Configure wake up source(s)
 * Write MCHP_PCR_SYS_SLP_CTR register to value based on light/heavy with
 *	SLEEP_ALL bit = 1.
 * Execute Cortex-M4 WFI sequence. DSB(), ISB(), WFI(), NOP()
 * Cortex-M4 will assert sleep signal to PCR block.
 * PCR HW will spin until all CLK_REQ==0
 * PCR will then turn off clocks based on light/heavy sleep.
 *
 * RST_EN usage is:
 * Save and disable maskable interrupts
 * Write unlock code to PCR Peripheral Reset Lock
 * Write bit patterns to one or more of PCR RST_EN[0, 4] registers
 *  Selected peripherals will be reset.
 * Write lock code to PCR Peripheral Reset Lock.
 * Restore interrupts.
 */
#define MCHP_MAX_PCR_SCR_REGS	5ul

/* VTR Powered PCR registers */
#define MCHP_PCR_SLP(bitpos)	BIT(bitpos)

/* PCR System Sleep Control */
#define MCHP_PCR_SYS_SLP_CTRL_MASK		0x0109ul
#define MCHP_PCR_SYS_SLP_CTRL_SLP_HEAVY		BIT(0)
#define MCHP_PCR_SYS_SLP_CTRL_SLP_ALL		BIT(3)
/*
 * bit[8] can be used to prevent entry to heavy sleep unless the
 * PLL is locked.
 * bit[8]==0 (POR default) system will allow entry to light or heavy
 * sleep if and only if PLL is locked.
 * bit[8]==1 system will allow entry to heavy sleep before PLL is locked.
 */
#define MCHP_PCR_SYS_SLP_CTRL_ALLOW_SLP_NO_PLL_LOCK	BIT(8)

/* Assert all peripheral sleep enables once CPU asserts its sleep signal */
#define MCHP_PCR_SYS_SLP_LIGHT	0x08ul
#define MCHP_PCR_SYS_SLP_HEAVY	0x09ul

/*
 * PCR Process Clock Control
 * Divides 96MHz clock to ARM Cortex-M4 core including
 * SysTick and NVIC.
 */
#define MCHP_PCR_PROC_CLK_CTRL_MASK	0xFFul
#define MCHP_PCR_PROC_CLK_CTRL_96MHZ	0x01ul
#define MCHP_PCR_PROC_CLK_CTRL_48MHZ	0x02ul
#define MCHP_PCR_PROC_CLK_CTRL_24MHZ	0x04ul
#define MCHP_PCR_PROC_CLK_CTRL_12MHZ	0x08ul
#define MCHP_PCR_PROC_CLK_CTRL_6MHZ	0x10ul
#define MCHP_PCR_PROC_CLK_CTRL_2MHZ	0x30ul
#define MCHP_PCR_PROC_CLK_CTRL_DFLT	MCHP_PCR_PROC_CLK_CTRL_24MHZ

/* PCR Slow Clock Control. Clock divicder for 100KHz clock domain */
#define MCHP_PCR_SLOW_CLK_CTRL_MASK	0x3FFul
#define MCHP_PCR_SLOW_CLK_CTRL_100KHZ	0x1E0ul

/*  PCR Oscillator ID register (Read-Only) */
#define MCHP_PCR_OSC_ID_MASK		0x1FFul
#define MCHP_PCR_OSC_ID_PLL_LOCK	BIT(8)

/* PCR Power Reset Status Register */
#define MCHP_PCR_PRS_MASK			0x0DFCul
#define MCHP_PCR_PRS_VCC_PWRGD_STATE_RO		BIT(2)
#define MCHP_PCR_PRS_HOST_RESET_STATE_RO	BIT(3)
#define MCHP_PCR_PRS_VTR_RST_RWC		BIT(4)
#define MCHP_PCR_PRS_VBAT_RST_RWC		BIT(5)
#define MCHP_PCR_PRS_RST_SYS_RWC		BIT(6)
#define MCHP_PCR_PRS_JTAG_RST_RO		BIT(7)
#define MCHP_PCR_PRS_WDT_EVENT_RWC		BIT(8)
#define MCHP_PCR_PRS_32K_ACTIVE_RO		BIT(10)
#define MCHP_PCR_PRS_LPC_ESPI_CLK_ACTIVE_RO	BIT(11)

/*  PCR Power Reset Control Register */
#define MCHP_PCR_PR_CTRL_MASK			0x101ul
#define MCHP_PCR_PR_CTRL_PWR_INV		BIT(0)
#define MCHP_PCR_PR_CTRL_USE_ESPI_PLTRST	0ul
#define MCHP_PCR_PR_CTRL_USE_PCI_RST		BIT(8)

/* PCR System Reset Register */
#define MCHP_PCR_SYS_RESET_MASK		0x100ul
#define MCHP_PCR_SYS_RESET_NOW		BIT(8)

/* Turbo Clock Register */
#define MCHP_PCR_TURBO_CLK_MASK		0x04ul
#define MCHP_PCR_TURBO_CLK_96M		BIT(2)

/*
 * Sleep Enable Reg 0 (Offset +30h)
 * Clock Required Reg 0 (Offset +50h)
 * Reset Enable Reg 0 (Offset +70h)
 */
#define MCHP_PCR0_JTAG_STAP_POS		0u
#define MCHP_PCR0_OTP_POS		1u
#define MCHP_PCR0_ISPI_EMC_POS		2u

/*
 * Sleep Enable Reg 1 (Offset +34h)
 * Clock Required Reg 1 (Offset +54h)
 * Reset Enable Reg 1 (Offset +74h)
 */
#define MCHP_PCR1_ECIA_POS	0u
#define MCHP_PCR1_PECI_POS	1u
#define MCHP_PCR1_TACH0_POS	2u
#define MCHP_PCR1_PWM0_POS	4u
#define MCHP_PCR1_PMC_POS	5u
#define MCHP_PCR1_DMA_POS	6u
#define MCHP_PCR1_TFDP_POS	7u
#define MCHP_PCR1_CPU_POS	8u
#define MCHP_PCR1_WDT_POS	9u
#define MCHP_PCR1_SMB0_POS	10u
#define MCHP_PCR1_TACH1_POS	11u
#define MCHP_PCR1_TACH2_POS	12u
#define MCHP_PCR1_TACH3_POS	13u
#define MCHP_PCR1_PWM1_POS	20u
#define MCHP_PCR1_PWM2_POS	21u
#define MCHP_PCR1_PWM3_POS	22u
#define MCHP_PCR1_PWM4_POS	23u
#define MCHP_PCR1_PWM5_POS	24u
#define MCHP_PCR1_PWM6_POS	25u
#define MCHP_PCR1_PWM7_POS	26u
#define MCHP_PCR1_PWM8_POS	27u
#define MCHP_PCR1_ECS_POS	29u
#define MCHP_PCR1_B16TMR0_POS	30u
#define MCHP_PCR1_B16TMR1_POS	31u

/*
 * Sleep Enable Reg 2 (Offset +38h)
 * Clock Required Reg 2 (Offset +58h)
 * Reset Enable Reg 2 (Offset +78h)
 */
#define MCHP_PCR2_EMI0_POS		0u
#define MCHP_PCR2_UART0_POS		1u
#define MCHP_PCR2_UART1_POS		2u
#define MCHP_PCR2_GCFG_POS		12u
#define MCHP_PCR2_ACPI_EC0_POS		13u
#define MCHP_PCR2_ACPI_EC1_POS		14u
#define MCHP_PCR2_ACPI_PM1_POS		15u
#define MCHP_PCR2_KBC_POS		16u
#define MCHP_PCR2_MBOX_POS		17u
#define MCHP_PCR2_RTC_POS		18u
#define MCHP_PCR2_ESPI_POS		19u
#define MCHP_PCR2_SCR32_POS		20u
#define MCHP_PCR2_ACPI_EC2_POS		21u
#define MCHP_PCR2_ACPI_EC3_POS		22u
#define MCHP_PCR2_ACPI_EC4_POS		23u
#define MCHP_PCR2_P80BD_POS		25u
#define MCHP_PCR2_ESPI_SAF_POS		27u
#define MCHP_PCR2_GLUE_POS		29u

/*
 * Sleep Enable Reg 3 (Offset +3Ch)
 * Clock Required Reg 3 (Offset +5Ch)
 * Reset Enable Reg 3 (Offset +7Ch)
 */
#define MCHP_PCR3_ADC_POS	3u
#define MCHP_PCR3_PS2_0_POS	5u
#define MCHP_PCR3_GPSPI0_POS	9u
#define MCHP_PCR3_HTMR0_POS	10u
#define MCHP_PCR3_KEYSCAN_POS	11u
#define MCHP_PCR3_RPMFAN0_POS	12u
#define MCHP_PCR3_SMB1_POS	13u
#define MCHP_PCR3_SMB2_POS	14u
#define MCHP_PCR3_SMB3_POS	15u
#define MCHP_PCR3_LED0_POS	16u
#define MCHP_PCR3_LED1_POS	17u
#define MCHP_PCR3_LED2_POS	18u
#define MCHP_PCR3_BCL0_POS	19u
#define MCHP_PCR3_SMB4_POS	20u
#define MCHP_PCR3_B16TMR2_POS	21u
#define MCHP_PCR3_B16TMR3_POS	22u
#define MCHP_PCR3_B32TMR0_POS	23u
#define MCHP_PCR3_B32TMR1_POS	24u
#define MCHP_PCR3_LED3_POS	25u
#define MCHP_PCR3_CRYPTO_POS	26u
#define MCHP_PCR3_HTMR1_POS	29u
#define MCHP_PCR3_CCT_POS	30u
#define MCHP_PCR3_PWM9_POS	31u

#define MCHP_PCR3_CRYPTO_MASK	BIT(MCHP_PCR3_CRYPTO_POS)

/*
 * Sleep Enable Reg 4 (Offset +40h)
 * Clock Required Reg 4 (Offset +60h)
 * Reset Enable Reg 4 (Offset +80h)
 */
#define MCHP_PCR4_PWM10_POS	0u
#define MCHP_PCR4_PWM11_POS	1u
#define MCHP_CTMR0_POS		2u
#define MCHP_CTMR1_POS		3u
#define MCHP_CTMR2_POS		4u
#define MCHP_CTMR3_POS		5u
#define MCHP_PCR4_RTMR_POS	6u
#define MCHP_PCR4_RPMFAN1_POS	7u
#define MCHP_PCR4_QMSPI_POS	8u
#define MCHP_PCR4_RCID0_POS	10u
#define MCHP_PCR4_RCID1_POS	11u
#define MCHP_PCR4_RCID2_POS	12u
#define MCHP_PCR4_PHOT_POS	13u
#define MCHP_PCR4_EEPROM_POS	14u
#define MCHP_PCR4_SPIP_POS	16u
#define MCHP_PCR4_GPSPI1_POS	22u

/* Reset Enable Lock (Offset +84h) */
#define MCHP_PCR_RSTEN_UNLOCK	0xA6382D4Cul
#define MCHP_PCR_RSTEN_LOCK	0xA6382D4Dul

/* VBAT Soft Reset (Offset +88h) */
#define MCHP_PCR_VBSR_MASK	BIT(0)
#define MCHP_PCR_VBSR_EN	BIT(0) /* self clearing */

/* VTR Source 32 KHz Clock (Offset +8Ch) */
#define MCHP_PCR_VTR_32K_SRC_MASK	0x03U
#define MCHP_PCR_VTR_32K_SRC_SILOSC	0x00U
#define MCHP_PCR_VTR_32K_SRC_XTAL	0x01U
#define MCHP_PCR_VTR_32K_SRC_PIN	0x02U
#define MCHP_PCR_VTR_32K_SRC_NONE	0x03U

/*
 * Clock monitor 32KHz period counter (Offset +C0h, RO)
 * Clock monitor 32KHz high counter (Offset +C4h, RO)
 * Clock monitor 32KHz period counter minimum (Offset +C8h, RW)
 * Clock monitor 32KHz period counter maximum (Offset +CCh, RW)
 * Clock monitor 32KHz Duty Cycle variation counter (Offset +D0h, RO)
 * Clock monitor 32KHz Duty Cycle variation counter maximum (Offset +D4h, RW)
 */
#define MCHP_PCR_CLK32M_CNT_MASK	0xFFFFU

/*
 * Clock monitor 32KHz Valid Count (Offset +0xD8, RO)
 * Clock monitor 32KHz Valid Count minimum (Offset +0xDC, RW)
 */
#define MCHP_PCR_CLK32M_VALID_CNT_MASK	0xFFU

/* Clock monitor control register (Offset +0xE0, RW) */
#define MCHP_PCR_CLK32M_CTRL_MASK	0x01000017U
#define MCHP_PCR_CLK32M_CTRL_PER_EN	BIT(0)
#define MCHP_PCR_CLK32M_CTRL_DC_EN	BIT(1)
#define MCHP_PCR_CLK32M_CTRL_VAL_EN	BIT(2)
#define MCHP_PCR_CLK32M_CTRL_SRC_SO	BIT(4)
#define MCHP_PCR_CLK32M_CTRL_CLR_CNT	BIT(24)

/* Clock monitor interrupt status (Offset +0xE4, R/W1C) */
#define MCHP_PCR_CLK32M_ISTS_MASK	0x7FU
#define MCHP_PCR_CLK32M_ISTS_PULSE_RDY	BIT(0)
#define MCHP_PCR_CLK32M_ISTS_PASS_PER	BIT(1)
#define MCHP_PCR_CLK32M_ISTS_PASS_DC	BIT(2)
#define MCHP_PCR_CLK32M_ISTS_FAIL	BIT(3)
#define MCHP_PCR_CLK32M_ISTS_STALL	BIT(4)
#define MCHP_PCR_CLK32M_ISTS_VALID	BIT(5)
#define MCHP_PCR_CLK32M_ISTS_UNWELL	BIT(6)

/* Clock monitor interrupt enable (Offset +0xE8, RW) */
#define MCHP_PCR_CLK32M_IEN_MASK	0x7FU
#define MCHP_PCR_CLK32M_IEN_PULSE_RDY	BIT(0)
#define MCHP_PCR_CLK32M_IEN_PASS_PER	BIT(1)
#define MCHP_PCR_CLK32M_IEN_PASS_DC	BIT(2)
#define MCHP_PCR_CLK32M_IEN_FAIL	BIT(3)
#define MCHP_PCR_CLK32M_IEN_STALL	BIT(4)
#define MCHP_PCR_CLK32M_IEN_VALID	BIT(5)
#define MCHP_PCR_CLK32M_IEN_UNWELL	BIT(6)

/* PCR 32KHz clock monitor uses 48 MHz for all counters */
#define MCHP_PCR_CLK32M_CLOCK		48000000U

/* PCR register access */
#define MCHP_PCR_SLP_CTRL()		REG32(MCHP_PCR_SYS_SLP_CTRL_ADDR)
#define MCHP_PCR_PROC_CLK_DIV()		REG32(MCHP_PCR_SYS_CLK_CTRL_ADDR)
#define MCHP_PCR_SLOW_CLK_CTRL()	REG32(MCHP_PCR_SLOW_CLK_CTRL_ADDR)
#define MCHP_PCR_OSC_ID()		REG32(MCHP_PCR_OSC_ID_ADDR)
#define MCHP_PCR_PRS()			REG32(MCHP_PCR_PRS_ADDR)
#define MCHP_PCR_PR_CTRL()		REG32(MCHP_PCR_PR_CTRL_ADDR)
#define MCHP_PCR_SYS_RESET()		REG32(MCHP_PCR_SYS_RESET_ADDR)
#define MCHP_PCR_PERIPH_RST_LOCK()	REG32(MCHP_PCR_PERIPH_RESET_LOCK_ADDR)
#define MCHP_PCR_SLP_EN(n)		REG32(MCHP_PCR_SLP_EN_ADDR(n))
#define MCHP_PCR_CLK_REQ_RO(n)		REG32(MCHP_PCR_CLK_REQ_ADDR(n))
#define MCHP_PCR_PERIPH_RST(n)		REG32(MCHP_PCR_PERIPH_RESET_ADDR(n))

#define MCHP_PCR_DEV_SLP_EN_CLR(n, b) \
	REG32(MCHP_PCR_SLP_EN_ADDR(n)) &= ~BIT((b))

#define MCHP_PCR_DEV_SLP_EN_SET(n, b) \
	REG32(MCHP_PCR_SLP_EN_ADDR(n)) |= BIT((b))

/*
 * Encode peripheral SLP_EN/CLK_REQ/RST_EN bit number and register index.
 * b[4:0] = bit number
 * b[10:8] = zero based register number
 */
#define MCHP_PCR_ERB(rnum, bnum) ((((rnum) & 0x07u) << 8) | ((bnum) & 0x1fu))
#define MCHP_PCR_ERB_RNUM(erb) (((uint32_t)(erb) >> 8) & 0x07u)
#define MCHP_PCR_ERB_BITPOS(erb) ((uint32_t)(erb) & 0x1fu)

#ifdef __cplusplus
extern "C" {
#endif

enum pcr_id {
	PCR_STAP	= MCHP_PCR_ERB(0, 0),
	PCR_OTP		= MCHP_PCR_ERB(0, 1),
	PCR_ECIA	= MCHP_PCR_ERB(1, 0),
	PCR_PECI	= MCHP_PCR_ERB(1, 1),
	PCR_TACH0	= MCHP_PCR_ERB(1, 2),
	PCR_PWM0	= MCHP_PCR_ERB(1, 4),
	PCR_PMC		= MCHP_PCR_ERB(1, 5),
	PCR_DMA		= MCHP_PCR_ERB(1, 6),
	PCR_TFDP	= MCHP_PCR_ERB(1, 7),
	PCR_CPU		= MCHP_PCR_ERB(1, 8),
	PCR_WDT		= MCHP_PCR_ERB(1, 9),
	PCR_SMB0	= MCHP_PCR_ERB(1, 10),
	PCR_TACH1	= MCHP_PCR_ERB(1, 11),
	PCR_TACH2	= MCHP_PCR_ERB(1, 12),
	PCR_TACH3	= MCHP_PCR_ERB(1, 13),
	PCR_PWM1	= MCHP_PCR_ERB(1, 20),
	PCR_PWM2	= MCHP_PCR_ERB(1, 21),
	PCR_PWM3	= MCHP_PCR_ERB(1, 22),
	PCR_PWM4	= MCHP_PCR_ERB(1, 23),
	PCR_PWM5	= MCHP_PCR_ERB(1, 24),
	PCR_PWM6	= MCHP_PCR_ERB(1, 25),
	PCR_PWM7	= MCHP_PCR_ERB(1, 26),
	PCR_PWM8	= MCHP_PCR_ERB(1, 27),
	PCR_ECS		= MCHP_PCR_ERB(1, 29),
	PCR_B16TMR0	= MCHP_PCR_ERB(1, 30),
	PCR_B16TMR1	= MCHP_PCR_ERB(1, 31),
	PCR_EMI0	= MCHP_PCR_ERB(2, 0),
	PCR_UART0	= MCHP_PCR_ERB(2, 1),
	PCR_UART1	= MCHP_PCR_ERB(2, 2),
	PCR_GCFG	= MCHP_PCR_ERB(2, 12),
	PCR_ACPI_EC0	= MCHP_PCR_ERB(2, 13),
	PCR_ACPI_EC1	= MCHP_PCR_ERB(2, 14),
	PCR_ACPI_PM1	= MCHP_PCR_ERB(2, 15),
	PCR_KBC		= MCHP_PCR_ERB(2, 16),
	PCR_MBOX	= MCHP_PCR_ERB(2, 17),
	PCR_RTC		= MCHP_PCR_ERB(2, 18),
	PCR_ESPI	= MCHP_PCR_ERB(2, 19),
	PCR_SCR32	= MCHP_PCR_ERB(2, 20),
	PCR_ACPI_EC2	= MCHP_PCR_ERB(2, 21),
	PCR_ACPI_EC3	= MCHP_PCR_ERB(2, 22),
	PCR_ACPI_EC4	= MCHP_PCR_ERB(2, 23),
	PCR_P80DP	= MCHP_PCR_ERB(2, 25),
	PCR_ESPI_SAF	= MCHP_PCR_ERB(2, 27),
	PCR_GLUE	= MCHP_PCR_ERB(2, 29),
	PCR_ADC		= MCHP_PCR_ERB(3, 3),
	PCR_PS2_0	= MCHP_PCR_ERB(3, 5),
	PCR_GPSPI_0	= MCHP_PCR_ERB(3, 9),
	PCR_HTMR0	= MCHP_PCR_ERB(3, 10),
	PCR_KEYSCAN	= MCHP_PCR_ERB(3, 11),
	PCR_RPMFAN_0	= MCHP_PCR_ERB(3, 12),
	PCR_SMB1	= MCHP_PCR_ERB(3, 13),
	PCR_SMB2	= MCHP_PCR_ERB(3, 14),
	PCR_SMB3	= MCHP_PCR_ERB(3, 15),
	PCR_LED0	= MCHP_PCR_ERB(3, 16),
	PCR_LED1	= MCHP_PCR_ERB(3, 17),
	PCR_LED2	= MCHP_PCR_ERB(3, 18),
	PCR_BCL_0	= MCHP_PCR_ERB(3, 19),
	PCR_SMB4	= MCHP_PCR_ERB(3, 20),
	PCR_B16TMR2	= MCHP_PCR_ERB(3, 21),
	PCR_B16MTR3	= MCHP_PCR_ERB(3, 22),
	PCR_B32TMR0	= MCHP_PCR_ERB(3, 23),
	PCR_B32TMR1	= MCHP_PCR_ERB(3, 24),
	PCR_LED3	= MCHP_PCR_ERB(3, 25),
	PCR_CRYPTO	= MCHP_PCR_ERB(3, 26),
	PCR_HTMR1	= MCHP_PCR_ERB(3, 29),
	PCR_CCT		= MCHP_PCR_ERB(3, 30),
	PCR_PWM9	= MCHP_PCR_ERB(3, 31),
	PCR_PWM10	= MCHP_PCR_ERB(4, 0),
	PCR_PWM11	= MCHP_PCR_ERB(4, 1),
	PCR_CTMR_0	= MCHP_PCR_ERB(4, 2),
	PCR_CTMR_1	= MCHP_PCR_ERB(4, 3),
	PCR_CTMR_2	= MCHP_PCR_ERB(4, 4),
	PCR_CTMR_3	= MCHP_PCR_ERB(4, 5),
	PCR_RTMR	= MCHP_PCR_ERB(4, 6),
	PCR_RPMFAN_1	= MCHP_PCR_ERB(4, 7),
	PCR_QMSPI	= MCHP_PCR_ERB(4, 8),
	PCR_RCID_0	= MCHP_PCR_ERB(4, 10),
	PCR_RCID_1	= MCHP_PCR_ERB(4, 11),
	PCR_RCID_2	= MCHP_PCR_ERB(4, 12),
	PCR_PHOT	= MCHP_PCR_ERB(4, 13),
	PCR_EEPROM	= MCHP_PCR_ERB(4, 14),
	PCR_SPIP	= MCHP_PCR_ERB(4, 16),
	PCR_GPSPI_1	= MCHP_PCR_ERB(4, 22),
	PCR_MAX_ID,
};

static __attribute__ ((always_inline)) inline void
mec_pcr_periph_slp_ctrl(enum pcr_id id, uint8_t enable)
{
	uint32_t bitpos = MCHP_PCR_ERB_BITPOS(id);
	uint32_t ofs = MCHP_PCR_ERB_RNUM(id);
	uintptr_t raddr = (uintptr_t)(MCHP_PCR_SLP_EN0_ADDR);

	raddr += (ofs * 4u);
	if (enable) {
		REG32(raddr) |= BIT(bitpos);
	} else {
		REG32(raddr) &= ~BIT(bitpos);
	}
}

#define mchp_pcr_periph_slp_ctrl(id, en) mec_pcr_periph_slp_ctrl((id), (en))

static __attribute__ ((always_inline)) inline void
mec_pcr_periph_reset(enum pcr_id id)
{
	uint32_t bitpos = MCHP_PCR_ERB_BITPOS(id);
	uint32_t ofs = MCHP_PCR_ERB_RNUM(id);
	uintptr_t raddr = (uintptr_t)(MCHP_PCR_SLP_EN0_ADDR);

	raddr += (ofs * 4u);
	REG32(MCHP_PCR_PERIPH_RESET_LOCK_ADDR) = MCHP_PCR_RSTEN_UNLOCK;
	REG32(raddr) = BIT(bitpos);
	REG32(MCHP_PCR_PERIPH_RESET_LOCK_ADDR) = MCHP_PCR_RSTEN_LOCK;
}

#define mchp_pcr_periph_reset(id) mec_pcr_periph_reset((id))

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _MEC_PCR_H */
