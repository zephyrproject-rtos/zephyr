/*
 * Copyright (c) 2021 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_CLOCK_CONTROL_CLOCK_CONTROL_MCHP_XEC_H_
#define ZEPHYR_DRIVERS_CLOCK_CONTROL_CLOCK_CONTROL_MCHP_XEC_H_

#include <stdint.h>

/*
 * MEC152x and MEC172x PCR implements multiple SLP_EN, CLR_REQ, and RST_EN
 * registers. CLK_REQ bits are read-only. The peripheral sets its CLK_REQ if it
 * requires clocks. CLK_REQ bits must all be zero for the PCR block to put the
 * chip into light or heavy sleep.
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
#if defined(CONFIG_SOC_SERIES_MEC1501X)
#define MCHP_PCR_PROC_CLK_CTRL_48MHZ	1
#define MCHP_PCR_PROC_CLK_CTRL_24MHZ	2
#define MCHP_PCR_PROC_CLK_CTRL_12MHZ	4
#define MCHP_PCR_PROC_CLK_CTRL_6MHZ	8
#define MCHP_PCR_PROC_CLK_CTRL_2MHZ	24
#define MCHP_PCR_PROC_CLK_CTRL_DFLT	MCHP_PCR_PROC_CLK_CTRL_12MHZ
#elif defined(CONFIG_SOC_SERIES_MEC172X)
#define MCHP_PCR_PROC_CLK_CTRL_96MHZ	1
#define MCHP_PCR_PROC_CLK_CTRL_48MHZ	2
#define MCHP_PCR_PROC_CLK_CTRL_24MHZ	4
#define MCHP_PCR_PROC_CLK_CTRL_12MHZ	8
#define MCHP_PCR_PROC_CLK_CTRL_6MHZ	16
#define MCHP_PCR_PROC_CLK_CTRL_2MHZ	48
#define MCHP_PCR_PROC_CLK_CTRL_DFLT	MCHP_PCR_PROC_CLK_CTRL_24MHZ
#endif

/* PCR Slow Clock Control. Clock divicder for 100KHz clock domain */
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
#define MCHP_PCR_SYS_RESET_MASK			BIT(8)
#define MCHP_PCR_SYS_RESET_NOW			BIT(8)

/* Turbo Clock Register */
#if defined(CONFIG_SOC_SERIES_MEC172X)
#define MCHP_PCR_TURBO_CLK_MASK			BIT(2)
#define MCHP_PCR_TURBO_CLK_96M			BIT(2)
#endif

/* Reset Enable Lock (Offset +84h) */
#define MCHP_PCR_RSTEN_UNLOCK			0xa6382d4cu
#define MCHP_PCR_RSTEN_LOCK			0xa6382d4du

#if defined(CONFIG_SOC_SERIES_MEC172X)

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
#endif

/* VBAT Registers */

/* Offset 0x00 Power-Fail and Reset Status: R/W1C */
#define MCHP_VBATR_PFRS_OFS		0u
#define MCHP_VBATR_PFRS_MASK		0xfcu
#define MCHP_VBATR_PFRS_SYS_RST_POS	2u
#define MCHP_VBATR_PFRS_JTAG_POS	3u
#define MCHP_VBATR_PFRS_RESETI_POS	4u
#define MCHP_VBATR_PFRS_WDT_POS		5u
#define MCHP_VBATR_PFRS_SYSRESETREQ_POS 6u
#define MCHP_VBATR_PFRS_VBAT_RST_POS	7u

#define MCHP_VBATR_PFRS_SYS_RST		BIT(2)
#define MCHP_VBATR_PFRS_JTAG		BIT(3)
#define MCHP_VBATR_PFRS_RESETI		BIT(4)
#define MCHP_VBATR_PFRS_WDT		BIT(5)
#define MCHP_VBATR_PFRS_SYSRESETREQ	BIT(6)
#define MCHP_VBATR_PFRS_VBAT_RST	BIT(7)

/* Offset 0x08 32K Clock Source register */
#define MCHP_VBATR_CS_OFS		0x08u

#if defined(CONFIG_SOC_SERIES_MEC1501X)
#define MCHP_VBATR_CS_MASK		0x0eu
#define MCHP_VBATR_CS_EXT32K_PIN_POS	1
#define MCHP_VBATR_CS_XTAL_EN_POS	2
#define MCHP_VBATR_CS_XOSEL_POS		3

/* use external 32KHz waveform on 32KHZ_PIN. If not activity detected on
 * 32KHZ_PIN HW switches to source specified by bits[3:2]
 */
#define MCHP_VBATR_CS_EXT32K_PIN_EN	BIT(1)
/* use external crystal, else internal 32KHz silicon OSC */
#define MCHP_VBATR_CS_XTAL_EN		BIT(2)
/* crystal is connected single-ended XTAL2 pin else parallel XTAL1 to XTAL2 */
#define MCHP_VBATR_CS_XTAL_SE		BIT(3)

#elif defined(CONFIG_SOC_SERIES_MEC172X)
#define MCHP_VBATR_CS_MASK		0x71f1u
#define MCHP_VBATR_CS_SO_EN_POS		0
#define MCHP_VBATR_CS_XTAL_EN_POS	8
#define MCHP_VBATR_CS_XTAL_SEL_POS	9
#define MCHP_VBATR_CS_XTAL_DHC_POS	10
#define MCHP_VBATR_CS_XTAL_CNTR_POS	11
#define MCHP_VBATR_CS_PCS_POS		16
#define MCHP_VBATR_CS_DI32_VTR_OFF_POS	18

/* Enable and start internal 32KHz Silicon Oscillator */
#define MCHP_VBATR_CS_SO_EN		BIT(0)
/* Enable and start the external crystal */
#define MCHP_VBATR_CS_XTAL_EN		BIT(8)
/* single ended crystal on XTAL2 instead of parallel across XTAL1 and XTAL2 */
#define MCHP_VBATR_CS_XTAL_SE		BIT(9)
/* disable XTAL high startup current */
#define MCHP_VBATR_CS_XTAL_DHC		BIT(10)
/* crystal amplifier gain control */
#define MCHP_VBATR_CS_XTAL_CNTR_MSK	0x1800u
#define MCHP_VBATR_CS_XTAL_CNTR_DG	0x0800u
#define MCHP_VBATR_CS_XTAL_CNTR_RG	0x1000u
#define MCHP_VBATR_CS_XTAL_CNTR_MG	0x1800u
/* Select source of peripheral 32KHz clock */
#define MCHP_VBATR_CS_PCS_MSK		0x30000u
/* 32K silicon OSC when chip powered by VBAT or VTR */
#define MCHP_VBATR_CS_PCS_VTR_VBAT_SO	0u
/* 32K external crystal when chip powered by VBAT or VTR */
#define MCHP_VBATR_CS_PCS_VTR_VBAT_XTAL 0x10000u
/* 32K input pin on VTR. Switch to Silicon OSC on VBAT */
#define MCHP_VBATR_CS_PCS_VTR_PIN_SO	0x20000u
/* 32K input pin on VTR. Switch to crystal on VBAT */
#define MCHP_VBATR_CS_PCS_VTR_PIN_XTAL	0x30000u
/* Disable internal 32K VBAT clock source when VTR is off */
#define MCHP_VBATR_CS_DI32_VTR_OFF	BIT(18)
#endif

/*
 * Monotonic Counter least significant word (32-bit), read-only.
 * Increments by one on read.
 */
#define MCHP_VBATR_MCNT_LSW_OFS		0x20u

/* Monotonic Counter most significant word (32-bit). Read-Write */
#define MCHP_VBATR_MCNT_MSW_OFS		0x24u

/* ROM Feature register */
#define MCHP_VBATR_ROM_FEAT_OFS		0x28u

/* Embedded Reset Debounce Enable register */
#define MCHP_VBATR_EMBRD_EN_OFS		0x34u
#define MCHP_VBATR_EMBRD_EN		BIT(0)

/* Global Configuration Registers */
#define MCHP_GCFG_REV_ID_POS		0
#define MCHP_GCFG_REV_ID_MASK		GENMASK(7, 0)
#define MCHP_GCFG_DEV_ID_POS		16
#define MCHP_GCFG_DEV_ID_MASK		GENMASK(31, 16)

#define MCHP_GCFG_REV_B0		0
#define MCHP_GCFG_REV_B1		0x01u
#define MCHP_GCFG_REV_B2		0x02u

#define MCHP_GCFG_MEC150X_DEV_ID	0x00200000u
#define MCHP_GCFG_MEC152X_DEV_ID	0x00230000u
#define MCHP_GCFG_MEC172X_DEV_ID	0x00220000u


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
#ifdef CONFIG_SOC_SERIES_MEC172X
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
#endif
};

#if defined(CONFIG_SOC_SERIES_MEC172X)
struct vbatr_regs {
	volatile uint32_t PFRS;
	uint32_t RSVD1[1];
	volatile uint32_t CLK32_SRC;
	uint32_t RSVD2[5];
	volatile uint32_t MCNT_LO;
	volatile uint32_t MCNT_HI;
	uint32_t RSVD3[3];
	volatile uint32_t EMBRD_EN;
};
#elif defined(CONFIG_SOC_SERIES_MEC1501X)
struct vbatr_regs {
	volatile uint32_t PFRS;
	uint32_t RSVD1[1];
	volatile uint32_t CLK32_SRC;
	volatile uint32_t SHDN_PIN_DIS;
	uint32_t RSVD2[3];
	volatile uint32_t TRIM32K_CTRL;
	volatile uint32_t MCNT_LO;
	volatile uint32_t MCNT_HI;
};
#endif

/* Global Configuration Registers */
struct global_cfg_regs {
	volatile uint8_t RSVD0[2];
	volatile uint8_t TEST02;
	volatile uint8_t RSVD1[4];
	volatile uint8_t LOG_DEV_NUM;
	volatile uint8_t RSVD2[20];
	volatile uint32_t DEV_REV_ID;
	volatile uint8_t LEGACY_DEV_ID;
	volatile uint8_t RSVD3[14];
};

#endif /* ZEPHYR_DRIVERS_CLOCK_CONTROL_CLOCK_CONTROL_MCHP_XEC_H_ */
