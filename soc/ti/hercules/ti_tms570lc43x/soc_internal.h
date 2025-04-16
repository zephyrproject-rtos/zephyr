/*
 * Copyright (c) 2025 ispace, inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __TMS570_SOC_INTERNAL_H__
#define __TMS570_SOC_INTERNAL_H__

#include <zephyr/kernel.h>
#include <soc.h>

/* Primary System Control Registers (SYS) */
#define REG_SYS1_CSDIS			(DRV_SYS1 + 0x0030)
#define REG_SYS1_CSDISSET		(DRV_SYS1 + 0x0034)
#define REG_SYS1_CSDISCLR		(DRV_SYS1 + 0x0038)
#define REG_SYS1_CDDIS			(DRV_SYS1 + 0x003C)
#define REG_SYS1_GHVSRC			(DRV_SYS1 + 0x0048)
#define REG_SYS1_VCLKASRC		(DRV_SYS1 + 0x004C)
#define REG_SYS1_RCLKSRC		(DRV_SYS1 + 0x0050)
#define REG_SYS1_CSVSTAT		(DRV_SYS1 + 0x0054)
#define REG_SYS1_PLLCTL1		(DRV_SYS1 + 0x0070)
#define REG_SYS1_PLLCTL2		(DRV_SYS1 + 0x0074)
#define REG_SYS1_CLKCNTL		(DRV_SYS1 + 0x00D0)
#define REG_SYS1_GBLSTAT		(DRV_SYS1 + 0x00EC)

/* Secondary System Control Registers (SYS2) */
#define REG_SYS2_PLLCTL3		(DRV_SYS2 + 0x0000)
#define REG_SYS2_CLK2CNTRL		(DRV_SYS2 + 0x003C)
#define REG_SYS2_VCLKACON1		(DRV_SYS2 + 0x0040)
#define REG_SYS2_HCLKCNTL		(DRV_SYS2 + 0x0054)

/* peripheral control resgisters */
#define REG_PCR1_PSPWRDWNCLR0		(DRV_PCR1 + 0x00A0)
#define REG_PCR1_PSPWRDWNCLR1		(DRV_PCR1 + 0x00A4)
#define REG_PCR1_PSPWRDWNCLR2		(DRV_PCR1 + 0x00A8)
#define REG_PCR1_PSPWRDWNCLR3		(DRV_PCR1 + 0x00AC)

#define REG_PCR2_PSPWRDWNCLR0		(DRV_PCR2 + 0x00A0)
#define REG_PCR2_PSPWRDWNCLR1		(DRV_PCR2 + 0x00A4)
#define REG_PCR2_PSPWRDWNCLR2		(DRV_PCR2 + 0x00A8)
#define REG_PCR2_PSPWRDWNCLR3		(DRV_PCR2 + 0x00AC)

#define REG_PCR3_PSPWRDWNCLR0		(DRV_PCR3 + 0x00A0)
#define REG_PCR3_PSPWRDWNCLR1		(DRV_PCR3 + 0x00A4)
#define REG_PCR3_PSPWRDWNCLR2		(DRV_PCR3 + 0x00A8)
#define REG_PCR3_PSPWRDWNCLR3		(DRV_PCR3 + 0x00AC)

/* flash control registers */
#define REG_FCR_FRDCNTL			(DRV_FCR + 0x0000)
#define REG_FCR_FBPWRMODE		(DRV_FCR + 0x0040)
#define REG_FCR_FSM_WR_ENA		(DRV_FCR + 0x0288)
#define REG_FCR_EEPROM_CONFIG		(DRV_FCR + 0x02B8)

/* error signalling module registers */
#define REG_ESM_EEPAPR1			(DRV_ESM + 0x0000)
#define REG_ESM_DEPAPR1			(DRV_ESM + 0x0004)
#define REG_ESM_IESR1			(DRV_ESM + 0x0008)
#define REG_ESM_IECR1			(DRV_ESM + 0x000C)
#define REG_ESM_ILSR1			(DRV_ESM + 0x0010)
#define REG_ESM_ILCR1			(DRV_ESM + 0x0014)
#define REG_ESM_SR1_0			(DRV_ESM + 0x0018)
#define REG_ESM_SR1_1			(DRV_ESM + 0x001C)
#define REG_ESM_SR1_2			(DRV_ESM + 0x0020)
#define REG_ESM_EPSR			(DRV_ESM + 0x0024)
#define REG_ESM_IOFFHR			(DRV_ESM + 0x0028)
#define REG_ESM_IOFFLR			(DRV_ESM + 0x002C)
#define REG_ESM_LTCR			(DRV_ESM + 0x0030)
#define REG_ESM_LTCPR			(DRV_ESM + 0x0034)
#define REG_ESM_EKR			(DRV_ESM + 0x0038)
#define REG_ESM_SSR2			(DRV_ESM + 0x003C)
#define REG_ESM_IEPSR4			(DRV_ESM + 0x0040)
#define REG_ESM_IEPCR4			(DRV_ESM + 0x0044)
#define REG_ESM_IESR4			(DRV_ESM + 0x0048)
#define REG_ESM_IECR4			(DRV_ESM + 0x004C)
#define REG_ESM_ILSR4			(DRV_ESM + 0x0050)
#define REG_ESM_ILCR4			(DRV_ESM + 0x0054)
#define REG_ESM_SR4_0			(DRV_ESM + 0x0058)
#define REG_ESM_SR4_1			(DRV_ESM + 0x005C)
#define REG_ESM_SR4_2			(DRV_ESM + 0x0060)
#define REG_ESM_IEPSR7			(DRV_ESM + 0x0080)
#define REG_ESM_IEPCR7			(DRV_ESM + 0x0084)
#define REG_ESM_IESR7			(DRV_ESM + 0x0088)
#define REG_ESM_IECR7			(DRV_ESM + 0x008C)
#define REG_ESM_ILSR7			(DRV_ESM + 0x0090)
#define REG_ESM_ILCR7			(DRV_ESM + 0x0094)
#define REG_ESM_SR7_0			(DRV_ESM + 0x0098)
#define REG_ESM_SR7_1			(DRV_ESM + 0x009C)
#define REG_ESM_SR7_2			(DRV_ESM + 0x00A0)

#define REG_SYSECR			(DRV_SYSBASE + 0xE0)
#define REG_SYSESR			(DRV_SYSBASE + 0xE4)

#define REG_POMGLBCTRL			(DRV_POM_CONTROL + 0x00)

#define CSDIS_SRC_OSC			BIT(0)
#define CSDIS_SRC_PLL1			BIT(1)
#define CSDIS_SRC_LFLPO			BIT(4)
#define CSDIS_SRC_HFLPO			BIT(5)
#define CSDIS_SRC_PLL2			BIT(6)
#define CSDIS_SRC_MASK			(0xFF)

#define GLBSTAT_OSCFAIL			BIT(0)
#define GLBSTAT_RFSLIP			BIT(8)
#define GLBSTAT_FBSLIP			BIT(9)

#define CLKCNTL_PERIPHENA		BIT(8)

#define FRDCNTL_RWAIT_OFFSET		(8)
#define FRDCNTL_PFUENB			BIT(1)
#define FRDCNTL_PFUENA			BIT(0)

#define FSM_WR_ENA_ENABLE_VAL		(5 << 0)
#define FSM_WR_ENA_DISABLE_VAL		(2 << 0)

#define EWAIT_OFFSET			(16)

#define BANKPWR0_OFFSET			(0)
#define BANKPWR1_OFFSET			(2)
#define BANKPWR7_OFFSET			(14)
#define BANKPWR_VAL_ACTIVE		(3)

#define GHVWAKE_OFFSET			(24)
#define HVLPM_OFFSET			(16)
#define GHVSRC_OFFSET			(0)

#define RTI1DIV_OFFSET			(8)
#define RTI1SRC_OFFSET			(0)

#define VCLKA1S_OFFSET			(0)
#define VCLKA2S_OFFSET			(8)

#define CLKCNTL_VCLKR_OFFSET		(16)
#define CLKCNTL_VCLKR_MASK		(0xF << CLKCNTL_VCLKR_OFFSET)
#define CLKCNTL_VCLK2R_OFFSET		(24)
#define CLKCNTL_VCLK2R_MASK		(0xF << CLKCNTL_VCLK2R_OFFSET)
#define CLKCNTL_PENA			BIT(8)
#define CLK2CNTRL_VCLK3R_OFFSET		(0)
#define CLK2CNTRL_VCLK3R_MASK		(0xF << CLK2CNTRL_VCLK3R_OFFSET)

#define VCLKA4R_OFFSET			(24)
#define VCLKA4_DIV_CDDIS		BIT(20)
#define VCLKA4S_OFFSET			(16)

#define PLLCTL1_PLLDIV_OFFSET		(24)
#define PLLCTL1_PLLDIV_MASK		~(0x1F << PLLCTL1_PLLDIV_OFFSET)
#define PLLCTL1_BPOS_OFFSET		(29)
#define PLLCTL1_REFCLKDIV_OFFSET	(16)

#define PLLCTL2_SPREADINGRATE_OFFSET	(22)
#define PLLCTL2_MULMOD_OFFSET		(12)
#define PLLCTL2_SPR_AMOUNT_OFFSET	(0)

#define PLLCTL3_PLLDIV2_OFFSET		(24)
#define PLLCTL3_PLLDIV2_MASK		~(0x1F << PLLCTL3_PLLDIV2_OFFSET)
#define PLLCTL3_REFCLKDIV2_OFFSET	(16)
#define PLLCTL3_PLLMUL2_OFFSET		(0)

#define HCLKCNTL_HCLKR_OFFSET		(0)

#define CDDIS_VCLKA2			BIT(5)

typedef enum {
	POWERON_RESET		= 0x8000U,  /**< Alias for Power On Reset     */
	OSC_FAILURE_RESET	= 0x4000U,  /**< Alias for Osc Failure Reset  */
	WATCHDOG_RESET		= 0x2000U,  /**< Alias for Watch Dog Reset    */
	WATCHDOG2_RESET		= 0x1000U,  /**< Alias for Watch Dog 2 Reset  */
	DEBUG_RESET		= 0x0800U,  /**< Alias for Debug Reset        */
	INTERCONNECT_RESET	= 0x0080U,  /**< Alias for Interconnect Reset */
	CPU0_RESET		= 0x0020U,  /**< Alias for CPU 0 Reset        */
	SW_RESET		= 0x0010U,  /**< Alias for Software Reset     */
	EXT_RESET		= 0x0008U,  /**< Alias for External Reset     */
	NO_RESET		= 0x0000U   /**< Alias for No Reset           */
} resetSource_t;


void _mpuInit_(void);
uint32_t _errata_SSWF021_45_both_plls(uint32_t count);

#endif /* __TMS570_SOC_INTERNAL_H__ */
