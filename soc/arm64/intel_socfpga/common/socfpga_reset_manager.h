/*
 * Copyright (c) 2019-2021, Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOCFPGA_RESETMANAGER_H
#define SOCFPGA_RESETMANAGER_H

#include <soc.h>

#define SOCFPGA_BRIDGE_ENABLE	BIT(0)
#define SOCFPGA_BRIDGE_HAS_MASK	BIT(1)

#define SOC2FPGA_MASK		BIT(0)
#define LWHPS2FPGA_MASK		BIT(1)
#define FPGA2SOC_MASK		BIT(2)
#define F2SDRAM0_MASK		BIT(3)
#define F2SDRAM1_MASK		BIT(4)
#define F2SDRAM2_MASK		BIT(5)

/* Register Mapping */

#define SOCFPGA_RSTMGR_STAT			0x000
#define SOCFPGA_RSTMGR_HDSKEN			0x010
#define SOCFPGA_RSTMGR_HDSKREQ			0x014
#define SOCFPGA_RSTMGR_HDSKACK			0x018
#define SOCFPGA_RSTMGR_MPUMODRST		0x020
#define SOCFPGA_RSTMGR_PER0MODRST		0x024
#define SOCFPGA_RSTMGR_PER1MODRST		0x028
#define SOCFPGA_RSTMGR_BRGMODRST		0x02c
#define SOCFPGA_RSTMGR_COLDMODRST		0x034
#define SOCFPGA_RSTMGR_HDSKTIMEOUT		0x064

/* Field Mapping */

#define RSTMGR_PER0MODRST_EMAC0			BIT(0)
#define RSTMGR_PER0MODRST_EMAC1			BIT(1)
#define RSTMGR_PER0MODRST_EMAC2			BIT(2)
#define RSTMGR_PER0MODRST_USB0			BIT(3)
#define RSTMGR_PER0MODRST_USB1			BIT(4)
#define RSTMGR_PER0MODRST_NAND			BIT(5)
#define RSTMGR_PER0MODRST_SDMMC			BIT(7)
#define RSTMGR_PER0MODRST_EMAC0OCP		BIT(8)
#define RSTMGR_PER0MODRST_EMAC1OCP		BIT(9)
#define RSTMGR_PER0MODRST_EMAC2OCP		BIT(10)
#define RSTMGR_PER0MODRST_USB0OCP		BIT(11)
#define RSTMGR_PER0MODRST_USB1OCP		BIT(12)
#define RSTMGR_PER0MODRST_NANDOCP		BIT(13)
#define RSTMGR_PER0MODRST_SDMMCOCP		BIT(15)
#define RSTMGR_PER0MODRST_DMA			BIT(16)
#define RSTMGR_PER0MODRST_SPIM0			BIT(17)
#define RSTMGR_PER0MODRST_SPIM1			BIT(18)
#define RSTMGR_PER0MODRST_SPIS0			BIT(19)
#define RSTMGR_PER0MODRST_SPIS1			BIT(20)
#define RSTMGR_PER0MODRST_DMAOCP		BIT(21)
#define RSTMGR_PER0MODRST_EMACPTP		BIT(22)
#define RSTMGR_PER0MODRST_DMAIF0		BIT(24)
#define RSTMGR_PER0MODRST_DMAIF1		BIT(25)
#define RSTMGR_PER0MODRST_DMAIF2		BIT(26)
#define RSTMGR_PER0MODRST_DMAIF3		BIT(27)
#define RSTMGR_PER0MODRST_DMAIF4		BIT(28)
#define RSTMGR_PER0MODRST_DMAIF5		BIT(29)
#define RSTMGR_PER0MODRST_DMAIF6		BIT(30)
#define RSTMGR_PER0MODRST_DMAIF7		BIT(31)

#define RSTMGR_PER1MODRST_WATCHDOG0		BIT(0)
#define RSTMGR_PER1MODRST_WATCHDOG1		BIT(1)
#define RSTMGR_PER1MODRST_WATCHDOG2		BIT(2)
#define RSTMGR_PER1MODRST_WATCHDOG3		BIT(3)
#define RSTMGR_PER1MODRST_L4SYSTIMER0		BIT(4)
#define RSTMGR_PER1MODRST_L4SYSTIMER1		BIT(5)
#define RSTMGR_PER1MODRST_SPTIMER0		BIT(6)
#define RSTMGR_PER1MODRST_SPTIMER1		BIT(7)
#define RSTMGR_PER1MODRST_I2C0			BIT(8)
#define RSTMGR_PER1MODRST_I2C1			BIT(9)
#define RSTMGR_PER1MODRST_I2C2			BIT(10)
#define RSTMGR_PER1MODRST_I2C3			BIT(11)
#define RSTMGR_PER1MODRST_I2C4			BIT(12)
#define RSTMGR_PER1MODRST_UART0			BIT(16)
#define RSTMGR_PER1MODRST_UART1			BIT(17)
#define RSTMGR_PER1MODRST_GPIO0			BIT(24)
#define RSTMGR_PER1MODRST_GPIO1			BIT(25)

#define RSTMGR_HDSKEN_FPGAHSEN			BIT(2)
#define RSTMGR_HDSKEN_ETRSTALLEN		BIT(3)
#define RSTMGR_HDSKEN_L2FLUSHEN			BIT(8)
#define RSTMGR_HDSKEN_L3NOC_DBG			BIT(16)
#define RSTMGR_HDSKEN_DEBUG_L3NOC		BIT(17)
#define RSTMGR_HDSKEN_SDRSELFREFEN		BIT(0)

#define RSTMGR_HDSKEQ_FPGAHSREQ			0x4

#define RSTMGR_BRGMODRST_SOC2FPGA		BIT(0)
#define RSTMGR_BRGMODRST_LWHPS2FPGA		BIT(1)
#define RSTMGR_BRGMODRST_FPGA2SOC		BIT(2)
#define RSTMGR_BRGMODRST_F2SSDRAM0		BIT(3)
#define RSTMGR_BRGMODRST_F2SSDRAM1		BIT(4)
#define RSTMGR_BRGMODRST_F2SSDRAM2		BIT(5)
#define RSTMGR_BRGMODRST_MPFE			BIT(6)
#define RSTMGR_BRGMODRST_DDRSCH			BIT(6)

#define RSTMGR_HDSKREQ_FPGAHSREQ		(BIT(2))
#define RSTMGR_HDSKACK_FPGAHSACK_MASK		(BIT(2))

/* Definitions */

#define RSTMGR_L2_MODRST			0x0100
#define RSTMGR_HDSKEN_SET			0x010D

/* Macros */

#define SOCFPGA_RSTMGR(_reg)		(SOCFPGA_RSTMGR_REG_BASE \
						+ (SOCFPGA_RSTMGR_##_reg))
#define RSTMGR_FIELD(_reg, _field)	(RSTMGR_##_reg##MODRST_##_field)

#endif /* SOCFPGA_RESETMANAGER_H */
