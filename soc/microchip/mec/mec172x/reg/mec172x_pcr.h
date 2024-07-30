/*
 * Copyright (c) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stddef.h>

#ifndef _MEC172X_PCR_H
#define _MEC172X_PCR_H

#define MCHP_PCR_SYS_SLP_CTRL_OFS	0x00u
#define MCHP_PCR_SYS_CLK_CTRL_OFS	0x04u
#define MCHP_PCR_SLOW_CLK_CTRL_OFS	0x08u
#define MCHP_PCR_OSC_ID_OFS		0x0cu
#define MCHP_PCR_PRS_OFS		0x10u
#define MCHP_PCR_PR_CTRL_OFS		0x14u
#define MCHP_PCR_SYS_RESET_OFS		0x18u
#define MCHP_PCR_PKE_CLK_CTRL_OFS	0x1cu
#define MCHP_PCR_SLP_EN0_OFS		0x30u
#define MCHP_PCR_SLP_EN1_OFS		0x34u
#define MCHP_PCR_SLP_EN2_OFS		0x38u
#define MCHP_PCR_SLP_EN3_OFS		0x3cu
#define MCHP_PCR_SLP_EN4_OFS		0x40u
#define MCHP_PCR_CLK_REQ0_OFS		0x50u
#define MCHP_PCR_CLK_REQ1_OFS		0x54u
#define MCHP_PCR_CLK_REQ2_OFS		0x58u
#define MCHP_PCR_CLK_REQ3_OFS		0x5cu
#define MCHP_PCR_CLK_REQ4_OFS		0x60u
#define MCHP_PCR_PERIPH_RST0_OFS	0x70u
#define MCHP_PCR_PERIPH_RST1_OFS	0x74u
#define MCHP_PCR_PERIPH_RST2_OFS	0x78u
#define MCHP_PCR_PERIPH_RST3_OFS	0x7cu
#define MCHP_PCR_PERIPH_RST4_OFS	0x80u
#define MCHP_PCR_PERIPH_RST_LCK_OFS	0x84u
#define MCHP_PCR_VBAT_SRST_OFS		0x88u
#define MCHP_PCR_CLK32K_SRC_VTR_OFS	0x8cu
#define MCHP_PCR_CNT32K_PER_OFS		0xc0u
#define MCHP_PCR_CNT32K_PULSE_HI_OFS	0xc4u
#define MCHP_PCR_CNT32K_PER_MIN_OFS	0xc8u
#define MCHP_PCR_CNT32K_PER_MAX_OFS	0xccu
#define MCHP_PCR_CNT32K_DV_OFS		0xd0u
#define MCHP_PCR_CNT32K_DV_MAX_OFS	0xd4u
#define MCHP_PCR_CNT32K_VALID_OFS	0xd8u
#define MCHP_PCR_CNT32K_VALID_MIN_OFS	0xdcu
#define MCHP_PCR_CNT32K_CTRL_OFS	0xe0u
#define MCHP_PCR_CLK32K_MON_ISTS_OFS	0xe4u
#define MCHP_PCR_CLK32K_MON_IEN_OFS	0xe8u

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
#define MCHP_MAX_PCR_SCR_REGS	5u

/* VTR Powered PCR registers */
#define MCHP_PCR_SLP(bitpos)	BIT(bitpos)

/* PCR System Sleep Control */
#define MCHP_PCR_SYS_SLP_CTRL_MASK		0x0109u
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
#define MCHP_PCR_SYS_SLP_LIGHT			BIT(3)
#define MCHP_PCR_SYS_SLP_HEAVY			(BIT(3) | BIT(0))

/*
 * PCR Process Clock Control
 * Divides 96MHz clock to ARM Cortex-M4 core including
 * SysTick and NVIC.
 */
#define MCHP_PCR_PROC_CLK_CTRL_MASK	GENMASK(7, 0)
#define MCHP_PCR_PROC_CLK_CTRL_96MHZ	1
#define MCHP_PCR_PROC_CLK_CTRL_48MHZ	2
#define MCHP_PCR_PROC_CLK_CTRL_24MHZ	4
#define MCHP_PCR_PROC_CLK_CTRL_12MHZ	8
#define MCHP_PCR_PROC_CLK_CTRL_6MHZ	16
#define MCHP_PCR_PROC_CLK_CTRL_2MHZ	48
#define MCHP_PCR_PROC_CLK_CTRL_DFLT	MCHP_PCR_PROC_CLK_CTRL_24MHZ

/* PCR Slow Clock Control. Clock divider for 100KHz clock domain */
#define MCHP_PCR_SLOW_CLK_CTRL_MASK	GENMASK(9, 0)
#define MCHP_PCR_SLOW_CLK_CTRL_100KHZ	0x1e0u

/*  PCR Oscillator ID register (Read-Only) */
#define MCHP_PCR_OSC_ID_MASK		GENMASK(8, 0)
#define MCHP_PCR_OSC_ID_PLL_LOCK	BIT(8)

/* PCR Power Reset Status Register */
#define MCHP_PCR_PRS_MASK			\
	(GENMASK(11, 10) | GENMASK(8, 2))
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
#define MCHP_PCR_PR_CTRL_MASK			(BIT(8) | BIT(0))
#define MCHP_PCR_PR_CTRL_PWR_INV		BIT(0)
#define MCHP_PCR_PR_CTRL_USE_ESPI_PLTRST	0u
#define MCHP_PCR_PR_CTRL_USE_PCI_RST		BIT(8)

/* PCR System Reset Register */
#define MCHP_PCR_SYS_RESET_MASK		BIT(8)
#define MCHP_PCR_SYS_RESET_NOW		BIT(8)

/* Turbo Clock Register */
#define MCHP_PCR_TURBO_CLK_MASK		BIT(2)
#define MCHP_PCR_TURBO_CLK_96M		BIT(2)

/*
 * Sleep Enable Reg 0 (Offset +30h)
 * Clock Required Reg 0 (Offset +50h)
 * Reset Enable Reg 0 (Offset +70h)
 */
#define MCHP_PCR0_JTAG_STAP_POS		0
#define MCHP_PCR0_OTP_POS		1
#define MCHP_PCR0_ISPI_EMC_POS		2

/*
 * Sleep Enable Reg 1 (Offset +34h)
 * Clock Required Reg 1 (Offset +54h)
 * Reset Enable Reg 1 (Offset +74h)
 */
#define MCHP_PCR1_ECIA_POS	0
#define MCHP_PCR1_PECI_POS	1
#define MCHP_PCR1_TACH0_POS	2
#define MCHP_PCR1_PWM0_POS	4
#define MCHP_PCR1_PMC_POS	5
#define MCHP_PCR1_DMA_POS	6
#define MCHP_PCR1_TFDP_POS	7
#define MCHP_PCR1_CPU_POS	8
#define MCHP_PCR1_WDT_POS	9
#define MCHP_PCR1_SMB0_POS	10
#define MCHP_PCR1_TACH1_POS	11
#define MCHP_PCR1_TACH2_POS	12
#define MCHP_PCR1_TACH3_POS	13
#define MCHP_PCR1_PWM1_POS	20
#define MCHP_PCR1_PWM2_POS	21
#define MCHP_PCR1_PWM3_POS	22
#define MCHP_PCR1_PWM4_POS	23
#define MCHP_PCR1_PWM5_POS	24
#define MCHP_PCR1_PWM6_POS	25
#define MCHP_PCR1_PWM7_POS	26
#define MCHP_PCR1_PWM8_POS	27
#define MCHP_PCR1_ECS_POS	29
#define MCHP_PCR1_B16TMR0_POS	30
#define MCHP_PCR1_B16TMR1_POS	31

/*
 * Sleep Enable Reg 2 (Offset +38h)
 * Clock Required Reg 2 (Offset +58h)
 * Reset Enable Reg 2 (Offset +78h)
 */
#define MCHP_PCR2_EMI0_POS		0
#define MCHP_PCR2_UART0_POS		1
#define MCHP_PCR2_UART1_POS		2
#define MCHP_PCR2_GCFG_POS		12
#define MCHP_PCR2_ACPI_EC0_POS		13
#define MCHP_PCR2_ACPI_EC1_POS		14
#define MCHP_PCR2_ACPI_PM1_POS		15
#define MCHP_PCR2_KBC_POS		16
#define MCHP_PCR2_MBOX_POS		17
#define MCHP_PCR2_RTC_POS		18
#define MCHP_PCR2_ESPI_POS		19
#define MCHP_PCR2_SCR32_POS		20
#define MCHP_PCR2_ACPI_EC2_POS		21
#define MCHP_PCR2_ACPI_EC3_POS		22
#define MCHP_PCR2_ACPI_EC4_POS		23
#define MCHP_PCR2_P80BD_POS		25
#define MCHP_PCR2_ESPI_SAF_POS		27
#define MCHP_PCR2_GLUE_POS		29

/*
 * Sleep Enable Reg 3 (Offset +3Ch)
 * Clock Required Reg 3 (Offset +5Ch)
 * Reset Enable Reg 3 (Offset +7Ch)
 */
#define MCHP_PCR3_ADC_POS	3
#define MCHP_PCR3_PS2_0_POS	5
#define MCHP_PCR3_GPSPI0_POS	9
#define MCHP_PCR3_HTMR0_POS	10
#define MCHP_PCR3_KEYSCAN_POS	11
#define MCHP_PCR3_RPMFAN0_POS	12
#define MCHP_PCR3_SMB1_POS	13
#define MCHP_PCR3_SMB2_POS	14
#define MCHP_PCR3_SMB3_POS	15
#define MCHP_PCR3_LED0_POS	16
#define MCHP_PCR3_LED1_POS	17
#define MCHP_PCR3_LED2_POS	18
#define MCHP_PCR3_BCL0_POS	19
#define MCHP_PCR3_SMB4_POS	20
#define MCHP_PCR3_B16TMR2_POS	21
#define MCHP_PCR3_B16TMR3_POS	22
#define MCHP_PCR3_B32TMR0_POS	23
#define MCHP_PCR3_B32TMR1_POS	24
#define MCHP_PCR3_LED3_POS	25
#define MCHP_PCR3_CRYPTO_POS	26
#define MCHP_PCR3_HTMR1_POS	29
#define MCHP_PCR3_CCT_POS	30
#define MCHP_PCR3_PWM9_POS	31

#define MCHP_PCR3_CRYPTO_MASK	BIT(MCHP_PCR3_CRYPTO_POS)

/*
 * Sleep Enable Reg 4 (Offset +40h)
 * Clock Required Reg 4 (Offset +60h)
 * Reset Enable Reg 4 (Offset +80h)
 */
#define MCHP_PCR4_PWM10_POS	0
#define MCHP_PCR4_PWM11_POS	1
#define MCHP_CTMR0_POS		2
#define MCHP_CTMR1_POS		3
#define MCHP_CTMR2_POS		4
#define MCHP_CTMR3_POS		5
#define MCHP_PCR4_RTMR_POS	6
#define MCHP_PCR4_RPMFAN1_POS	7
#define MCHP_PCR4_QMSPI_POS	8
#define MCHP_PCR4_RCID0_POS	10
#define MCHP_PCR4_RCID1_POS	11
#define MCHP_PCR4_RCID2_POS	12
#define MCHP_PCR4_PHOT_POS	13
#define MCHP_PCR4_EEPROM_POS	14
#define MCHP_PCR4_SPIP_POS	16
#define MCHP_PCR4_GPSPI1_POS	22

/* Reset Enable Lock (Offset +84h) */
#define MCHP_PCR_RSTEN_UNLOCK	0xa6382d4cu
#define MCHP_PCR_RSTEN_LOCK	0xa6382d4du

/* VBAT Soft Reset (Offset +88h) */
#define MCHP_PCR_VBSR_MASK	BIT(0)
#define MCHP_PCR_VBSR_EN	BIT(0) /* self clearing */

/* VTR Source 32 KHz Clock (Offset +8Ch) */
#define MCHP_PCR_VTR_32K_SRC_MASK	GENMASK(1, 0)
#define MCHP_PCR_VTR_32K_SRC_SILOSC	0u
#define MCHP_PCR_VTR_32K_SRC_XTAL	BIT(0)
#define MCHP_PCR_VTR_32K_SRC_PIN	BIT(1)
#define MCHP_PCR_VTR_32K_SRC_NONE	(BIT(0) | BIT(1))

/*
 * Clock monitor 32KHz period counter (Offset +C0h, RO)
 * Clock monitor 32KHz high counter (Offset +C4h, RO)
 * Clock monitor 32KHz period counter minimum (Offset +C8h, RW)
 * Clock monitor 32KHz period counter maximum (Offset +CCh, RW)
 * Clock monitor 32KHz Duty Cycle variation counter (Offset +D0h, RO)
 * Clock monitor 32KHz Duty Cycle variation counter maximum (Offset +D4h, RW)
 */
#define MCHP_PCR_CLK32M_CNT_MASK	GENMASK(15, 0)

/*
 * Clock monitor 32KHz Valid Count (Offset +0xD8, RO)
 * Clock monitor 32KHz Valid Count minimum (Offset +0xDC, RW)
 */
#define MCHP_PCR_CLK32M_VALID_CNT_MASK	GENMASK(7, 0)

/* Clock monitor control register (Offset +0xE0, RW) */
#define MCHP_PCR_CLK32M_CTRL_MASK	(BIT(24) | BIT(4) | GENMASK(2, 0))
#define MCHP_PCR_CLK32M_CTRL_PER_EN	BIT(0)
#define MCHP_PCR_CLK32M_CTRL_DC_EN	BIT(1)
#define MCHP_PCR_CLK32M_CTRL_VAL_EN	BIT(2)
#define MCHP_PCR_CLK32M_CTRL_SRC_SO	BIT(4)
#define MCHP_PCR_CLK32M_CTRL_CLR_CNT	BIT(24)

/* Clock monitor interrupt status (Offset +0xE4, R/W1C) */
#define MCHP_PCR_CLK32M_ISTS_MASK	GENMASK(6, 0)
#define MCHP_PCR_CLK32M_ISTS_PULSE_RDY	BIT(0)
#define MCHP_PCR_CLK32M_ISTS_PASS_PER	BIT(1)
#define MCHP_PCR_CLK32M_ISTS_PASS_DC	BIT(2)
#define MCHP_PCR_CLK32M_ISTS_FAIL	BIT(3)
#define MCHP_PCR_CLK32M_ISTS_STALL	BIT(4)
#define MCHP_PCR_CLK32M_ISTS_VALID	BIT(5)
#define MCHP_PCR_CLK32M_ISTS_UNWELL	BIT(6)

/* Clock monitor interrupt enable (Offset +0xE8, RW) */
#define MCHP_PCR_CLK32M_IEN_MASK	GENMASK(6, 0)
#define MCHP_PCR_CLK32M_IEN_PULSE_RDY	BIT(0)
#define MCHP_PCR_CLK32M_IEN_PASS_PER	BIT(1)
#define MCHP_PCR_CLK32M_IEN_PASS_DC	BIT(2)
#define MCHP_PCR_CLK32M_IEN_FAIL	BIT(3)
#define MCHP_PCR_CLK32M_IEN_STALL	BIT(4)
#define MCHP_PCR_CLK32M_IEN_VALID	BIT(5)
#define MCHP_PCR_CLK32M_IEN_UNWELL	BIT(6)

/* PCR 32KHz clock monitor uses 48 MHz for all counters */
#define MCHP_PCR_CLK32M_CLOCK		48000000u

struct pcr_regs {
	volatile uint32_t SYS_SLP_CTRL;
	volatile uint32_t PROC_CLK_CTRL;
	volatile uint32_t SLOW_CLK_CTRL;
	volatile uint32_t OSC_ID;
	volatile uint32_t PWR_RST_STS;
	volatile uint32_t PWR_RST_CTRL;
	volatile uint32_t SYS_RST;
	volatile uint32_t TURBO_CLK;
	volatile uint32_t TEST20;
	uint32_t RSVD1[3];
	volatile uint32_t SLP_EN[5];
	uint32_t RSVD2[3];
	volatile uint32_t CLK_REQ[5];
	uint32_t RSVD3[3];
	volatile uint32_t RST_EN[5];
	volatile uint32_t RST_EN_LOCK;
	volatile uint32_t VBAT_SRST;
	volatile uint32_t CLK32K_SRC_VTR;
	volatile uint32_t TEST90;
	uint32_t RSVD4[(0x00c0 - 0x0094) / 4];
	volatile uint32_t CNT32K_PER;
	volatile uint32_t CNT32K_PULSE_HI;
	volatile uint32_t CNT32K_PER_MIN;
	volatile uint32_t CNT32K_PER_MAX;
	volatile uint32_t CNT32K_DV;
	volatile uint32_t CNT32K_DV_MAX;
	volatile uint32_t CNT32K_VALID;
	volatile uint32_t CNT32K_VALID_MIN;
	volatile uint32_t CNT32K_CTRL;
	volatile uint32_t CLK32K_MON_ISTS;
	volatile uint32_t CLK32K_MON_IEN;
};

#endif /* #ifndef _MEC172X_PCR_H */
