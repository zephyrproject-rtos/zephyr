/*
 * Copyright (c) 2025 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_RESET_NUMAKER_M333X_RESET_H
#define ZEPHYR_INCLUDE_DT_BINDINGS_RESET_NUMAKER_M333X_RESET_H

/* Beginning of M3331 BSP sys_reg.h reset module copy */

#define SYS_IPRST0_CHIPRST_Pos           0
#define SYS_IPRST0_CPURST_Pos            1
#define SYS_IPRST0_PDMA0RST_Pos          2
#define SYS_IPRST0_EBIRST_Pos            3
#define SYS_IPRST0_PDMA1RST_Pos          5
#define SYS_IPRST0_SDH0RST_Pos           6
#define SYS_IPRST0_CRCRST_Pos            7
#define SYS_IPRST0_CANFD0RST_Pos         8
#define SYS_IPRST0_CANFD1RST_Pos         9
#define SYS_IPRST0_HSUSBDRST_Pos         10
#define SYS_IPRST0_PDCIRST_Pos           11
#define SYS_IPRST0_HSUSBHRST_Pos         16
#define SYS_IPRST1_GPIORST_Pos           1
#define SYS_IPRST1_TMR0RST_Pos           2
#define SYS_IPRST1_TMR1RST_Pos           3
#define SYS_IPRST1_TMR2RST_Pos           4
#define SYS_IPRST1_TMR3RST_Pos           5
#define SYS_IPRST1_ACMP01RST_Pos         7
#define SYS_IPRST1_I2C0RST_Pos           8
#define SYS_IPRST1_I2C1RST_Pos           9
#define SYS_IPRST1_I2C2RST_Pos           10
#define SYS_IPRST1_I3C0RST_Pos           11
#define SYS_IPRST1_QSPI0RST_Pos          12
#define SYS_IPRST1_SPI0RST_Pos           13
#define SYS_IPRST1_SPI1RST_Pos           14
#define SYS_IPRST1_SPI2RST_Pos           15
#define SYS_IPRST1_UART0RST_Pos          16
#define SYS_IPRST1_UART1RST_Pos          17
#define SYS_IPRST1_UART2RST_Pos          18
#define SYS_IPRST1_UART3RST_Pos          19
#define SYS_IPRST1_UART4RST_Pos          20
#define SYS_IPRST1_WWDT0RST_Pos          24
#define SYS_IPRST1_WWDT1RST_Pos          25
#define SYS_IPRST1_EADC0RST_Pos          28
#define SYS_IPRST1_I2S0RST_Pos           29
#define SYS_IPRST1_HSOTGRST_Pos          30
#define SYS_IPRST2_USCI0RST_Pos          8
#define SYS_IPRST2_USCI1RST_Pos          9
#define SYS_IPRST2_EPWM0RST_Pos          16
#define SYS_IPRST2_EPWM1RST_Pos          17
#define SYS_IPRST2_BPWM0RST_Pos          18
#define SYS_IPRST2_BPWM1RST_Pos          19
#define SYS_IPRST2_EQEI0RST_Pos          20
#define SYS_IPRST2_ECAP0RST_Pos          26
#define SYS_IPRST2_BPWM2RST_Pos          28
#define SYS_IPRST2_BPWM3RST_Pos          29
#define SYS_IPRST2_BPWM4RST_Pos          30
#define SYS_IPRST2_BPWM5RST_Pos          31
#define SYS_IPRST3_LLSI0RST_Pos          0
#define SYS_IPRST3_LLSI1RST_Pos          1
#define SYS_IPRST3_LLSI2RST_Pos          2
#define SYS_IPRST3_LLSI3RST_Pos          3
#define SYS_IPRST3_LLSI4RST_Pos          4
#define SYS_IPRST3_LLSI5RST_Pos          5
#define SYS_IPRST3_LLSI6RST_Pos          6
#define SYS_IPRST3_LLSI7RST_Pos          7
#define SYS_IPRST3_LLSI8RST_Pos          8
#define SYS_IPRST3_LLSI9RST_Pos          9
#define SYS_IPRST3_ELLSI0RST_Pos         10

/* End of M3331 BSP sys_reg.h reset module copy */

/* Beginning of M3331 BSP sys.h reset module copy */

/*---------------------------------------------------------------------
 *  Module Reset Control Resister constant definitions.
 *---------------------------------------------------------------------
 */

#define NUMAKER_PDMA0_RST           ((0UL<<24) | SYS_IPRST0_PDMA0RST_Pos)
#define NUMAKER_EBI_RST             ((0UL<<24) | SYS_IPRST0_EBIRST_Pos)
#define NUMAKER_PDMA1_RST           ((0UL<<24) | SYS_IPRST0_PDMA1RST_Pos)
#define NUMAKER_SDH0_RST            ((0UL<<24) | SYS_IPRST0_SDH0RST_Pos)
#define NUMAKER_CRC_RST             ((0UL<<24) | SYS_IPRST0_CRCRST_Pos)
#define NUMAKER_CANFD0_RST          ((0UL<<24) | SYS_IPRST0_CANFD0RST_Pos)
#define NUMAKER_CANFD1_RST          ((0UL<<24) | SYS_IPRST0_CANFD1RST_Pos)
#define NUMAKER_HSUSBD_RST          ((0UL<<24) | SYS_IPRST0_HSUSBDRST_Pos)
#define NUMAKER_HSUSBH_RST          ((0UL<<24) | SYS_IPRST0_HSUSBHRST_Pos)
#define NUMAKER_PDCI_RST            ((0UL<<24) | SYS_IPRST0_PDCIRST_Pos)
#define NUMAKER_GPIO_RST            ((4UL<<24) | SYS_IPRST1_GPIORST_Pos)
#define NUMAKER_TMR0_RST            ((4UL<<24) | SYS_IPRST1_TMR0RST_Pos)
#define NUMAKER_TMR1_RST            ((4UL<<24) | SYS_IPRST1_TMR1RST_Pos)
#define NUMAKER_TMR2_RST            ((4UL<<24) | SYS_IPRST1_TMR2RST_Pos)
#define NUMAKER_TMR3_RST            ((4UL<<24) | SYS_IPRST1_TMR3RST_Pos)
#define NUMAKER_ACMP01_RST          ((4UL<<24) | SYS_IPRST1_ACMP01RST_Pos)
#define NUMAKER_I2C0_RST            ((4UL<<24) | SYS_IPRST1_I2C0RST_Pos)
#define NUMAKER_I2C1_RST            ((4UL<<24) | SYS_IPRST1_I2C1RST_Pos)
#define NUMAKER_I2C2_RST            ((4UL<<24) | SYS_IPRST1_I2C2RST_Pos)
#define NUMAKER_I3C0_RST            ((4UL<<24) | SYS_IPRST1_I3C0RST_Pos)
#define NUMAKER_QSPI0_RST           ((4UL<<24) | SYS_IPRST1_QSPI0RST_Pos)
#define NUMAKER_SPI0_RST            ((4UL<<24) | SYS_IPRST1_SPI0RST_Pos)
#define NUMAKER_SPI1_RST            ((4UL<<24) | SYS_IPRST1_SPI1RST_Pos)
#define NUMAKER_SPI2_RST            ((4UL<<24) | SYS_IPRST1_SPI2RST_Pos)
#define NUMAKER_UART0_RST           ((4UL<<24) | SYS_IPRST1_UART0RST_Pos)
#define NUMAKER_UART1_RST           ((4UL<<24) | SYS_IPRST1_UART1RST_Pos)
#define NUMAKER_UART2_RST           ((4UL<<24) | SYS_IPRST1_UART2RST_Pos)
#define NUMAKER_UART3_RST           ((4UL<<24) | SYS_IPRST1_UART3RST_Pos)
#define NUMAKER_UART4_RST           ((4UL<<24) | SYS_IPRST1_UART4RST_Pos)
#define NUMAKER_WWDT0_RST           ((4UL<<24) | SYS_IPRST1_WWDT0RST_Pos)
#define NUMAKER_WWDT1_RST           ((4UL<<24) | SYS_IPRST1_WWDT1RST_Pos)
#define NUMAKER_EADC0_RST           ((4UL<<24) | SYS_IPRST1_EADC0RST_Pos)
#define NUMAKER_I2S0_RST            ((4UL<<24) | SYS_IPRST1_I2S0RST_Pos)
#define NUMAKER_HSOTG_RST           ((4UL<<24) | SYS_IPRST1_HSOTGRST_Pos)
#define NUMAKER_USCI0_RST           ((8UL<<24) | SYS_IPRST2_USCI0RST_Pos)
#define NUMAKER_USCI1_RST           ((8UL<<24) | SYS_IPRST2_USCI1RST_Pos)
#define NUMAKER_EPWM0_RST           ((8UL<<24) | SYS_IPRST2_EPWM0RST_Pos)
#define NUMAKER_EPWM1_RST           ((8UL<<24) | SYS_IPRST2_EPWM1RST_Pos)
#define NUMAKER_BPWM0_RST           ((8UL<<24) | SYS_IPRST2_BPWM0RST_Pos)
#define NUMAKER_BPWM1_RST           ((8UL<<24) | SYS_IPRST2_BPWM1RST_Pos)
#define NUMAKER_EQEI0_RST           ((8UL<<24) | SYS_IPRST2_EQEI0RST_Pos)
#define NUMAKER_ECAP0_RST           ((8UL<<24) | SYS_IPRST2_ECAP0RST_Pos)
#define NUMAKER_BPWM2_RST           ((8UL<<24) | SYS_IPRST2_BPWM2RST_Pos)
#define NUMAKER_BPWM3_RST           ((8UL<<24) | SYS_IPRST2_BPWM3RST_Pos)
#define NUMAKER_BPWM4_RST           ((8UL<<24) | SYS_IPRST2_BPWM4RST_Pos)
#define NUMAKER_BPWM5_RST           ((8UL<<24) | SYS_IPRST2_BPWM5RST_Pos)
#define NUMAKER_LLSI0_RST           ((0x18UL<<24) | SYS_IPRST3_LLSI0RST_Pos)
#define NUMAKER_LLSI1_RST           ((0x18UL<<24) | SYS_IPRST3_LLSI1RST_Pos)
#define NUMAKER_LLSI2_RST           ((0x18UL<<24) | SYS_IPRST3_LLSI2RST_Pos)
#define NUMAKER_LLSI3_RST           ((0x18UL<<24) | SYS_IPRST3_LLSI3RST_Pos)
#define NUMAKER_LLSI4_RST           ((0x18UL<<24) | SYS_IPRST3_LLSI4RST_Pos)
#define NUMAKER_LLSI5_RST           ((0x18UL<<24) | SYS_IPRST3_LLSI5RST_Pos)
#define NUMAKER_LLSI6_RST           ((0x18UL<<24) | SYS_IPRST3_LLSI6RST_Pos)
#define NUMAKER_LLSI7_RST           ((0x18UL<<24) | SYS_IPRST3_LLSI7RST_Pos)
#define NUMAKER_LLSI8_RST           ((0x18UL<<24) | SYS_IPRST3_LLSI8RST_Pos)
#define NUMAKER_LLSI9_RST           ((0x18UL<<24) | SYS_IPRST3_LLSI9RST_Pos)
#define NUMAKER_ELLSI0_RST          ((0x18UL<<24) | SYS_IPRST3_ELLSI0RST_Pos)

/* End of M3331 BSP sys.h reset module copy */

#endif
