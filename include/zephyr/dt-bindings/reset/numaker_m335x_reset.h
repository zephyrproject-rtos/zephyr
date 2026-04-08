/*
 * Copyright (c) 2026 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_RESET_NUMAKER_M335X_RESET_H
#define ZEPHYR_INCLUDE_DT_BINDINGS_RESET_NUMAKER_M335X_RESET_H

/**
 * @file
 * @brief Reset module IDs for Nuvoton M335X
 * @ingroup reset_controller_nuvoton_m335x
 */

/**
 * @defgroup reset_controller_nuvoton_m335x Nuvoton NuMaker controller Devicetree helpers
 * @ingroup reset_controller_interface
 *
 * @details Devicetree macos/defines for Nuvoton NuMaker devices,
 * for use with the <tt>nuvoton,numaker-rst</tt> binding.
 * @{
 */

/** @cond INTERNAL_HIDDEN */

/**
 * @name Reset module IDs
 * @brief Register bit positions and reset IDs for Nuvoton M335X
 * @{
 */

/* Beginning of M3351 BSP sys_reg.h reset module copy */

#define SYS_IPRST0_CHIPRST_Pos           0
#define SYS_IPRST0_CPURST_Pos            1
#define SYS_IPRST0_PDMA0RST_Pos          2
#define SYS_IPRST0_EBIRST_Pos            3
#define SYS_IPRST0_USBHRST_Pos           4
#define SYS_IPRST0_CRCRST_Pos            7
#define SYS_IPRST0_CANFD0RST_Pos         8
#define SYS_IPRST0_CANFD1RST_Pos         9
#define SYS_IPRST0_CRPTRST_Pos           12
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
#define SYS_IPRST1_UART0RST_Pos          16
#define SYS_IPRST1_UART1RST_Pos          17
#define SYS_IPRST1_UART2RST_Pos          18
#define SYS_IPRST1_UART3RST_Pos          19
#define SYS_IPRST1_UART4RST_Pos          20
#define SYS_IPRST1_UART5RST_Pos          21
#define SYS_IPRST1_UART8RST_Pos          22
#define SYS_IPRST1_UART9RST_Pos          23
#define SYS_IPRST1_WWDT0RST_Pos          24
#define SYS_IPRST1_WWDT1RST_Pos          25
#define SYS_IPRST1_USBDRST_Pos           27
#define SYS_IPRST1_EADC0RST_Pos          28
#define SYS_IPRST1_TRNGRST_Pos           31
#define SYS_IPRST2_USCI0RST_Pos          8
#define SYS_IPRST2_USCI1RST_Pos          9
#define SYS_IPRST2_DAC0RST_Pos           12
#define SYS_IPRST2_PWM0RST_Pos           16
#define SYS_IPRST2_PWM1RST_Pos           17
#define SYS_IPRST2_BPWM0RST_Pos          18
#define SYS_IPRST2_BPWM1RST_Pos          19
#define SYS_IPRST2_EQEI0RST_Pos          20
#define SYS_IPRST2_EQEI1RST_Pos          21
#define SYS_IPRST2_UART6RST_Pos          24
#define SYS_IPRST2_UART7RST_Pos          25
#define SYS_IPRST2_ECAP0RST_Pos          26
#define SYS_IPRST2_EADC1RST_Pos          31
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
#define SYS_IPRST3_ELLSI1RST_Pos         11

/* End of M3351 BSP sys_reg.h reset module copy */

/* Beginning of M3351 BSP sys.h reset module copy */

/*---------------------------------------------------------------------
 *  Module Reset Control Resister constant definitions.
 *---------------------------------------------------------------------
 */

#define NUMAKER_PDMA0_RST       ((0UL<<24) | SYS_IPRST0_PDMA0RST_Pos)
#define NUMAKER_EBI_RST         ((0UL<<24) | SYS_IPRST0_EBIRST_Pos)
#define NUMAKER_USBH_RST        ((0UL<<24) | SYS_IPRST0_USBHRST_Pos)
#define NUMAKER_CRC_RST         ((0UL<<24) | SYS_IPRST0_CRCRST_Pos)
#define NUMAKER_CANFD0_RST      ((0UL<<24) | SYS_IPRST0_CANFD0RST_Pos)
#define NUMAKER_CANFD1_RST      ((0UL<<24) | SYS_IPRST0_CANFD1RST_Pos)
#define NUMAKER_CRPT_RST        ((0UL<<24) | SYS_IPRST0_CRPTRST_Pos)
#define NUMAKER_GPIO_RST        ((4UL<<24) | SYS_IPRST1_GPIORST_Pos)
#define NUMAKER_TMR0_RST        ((4UL<<24) | SYS_IPRST1_TMR0RST_Pos)
#define NUMAKER_TMR1_RST        ((4UL<<24) | SYS_IPRST1_TMR1RST_Pos)
#define NUMAKER_TMR2_RST        ((4UL<<24) | SYS_IPRST1_TMR2RST_Pos)
#define NUMAKER_TMR3_RST        ((4UL<<24) | SYS_IPRST1_TMR3RST_Pos)
#define NUMAKER_ACMP01_RST      ((4UL<<24) | SYS_IPRST1_ACMP01RST_Pos)
#define NUMAKER_I2C0_RST        ((4UL<<24) | SYS_IPRST1_I2C0RST_Pos)
#define NUMAKER_I2C1_RST        ((4UL<<24) | SYS_IPRST1_I2C1RST_Pos)
#define NUMAKER_I2C2_RST        ((4UL<<24) | SYS_IPRST1_I2C2RST_Pos)
#define NUMAKER_I3C0_RST        ((4UL<<24) | SYS_IPRST1_I3C0RST_Pos)
#define NUMAKER_QSPI0_RST       ((4UL<<24) | SYS_IPRST1_QSPI0RST_Pos)
#define NUMAKER_SPI0_RST        ((4UL<<24) | SYS_IPRST1_SPI0RST_Pos)
#define NUMAKER_SPI1_RST        ((4UL<<24) | SYS_IPRST1_SPI1RST_Pos)
#define NUMAKER_UART0_RST       ((4UL<<24) | SYS_IPRST1_UART0RST_Pos)
#define NUMAKER_UART1_RST       ((4UL<<24) | SYS_IPRST1_UART1RST_Pos)
#define NUMAKER_UART2_RST       ((4UL<<24) | SYS_IPRST1_UART2RST_Pos)
#define NUMAKER_UART3_RST       ((4UL<<24) | SYS_IPRST1_UART3RST_Pos)
#define NUMAKER_UART4_RST       ((4UL<<24) | SYS_IPRST1_UART4RST_Pos)
#define NUMAKER_UART5_RST       ((4UL<<24) | SYS_IPRST1_UART5RST_Pos)
#define NUMAKER_UART8_RST       ((4UL<<24) | SYS_IPRST1_UART8RST_Pos)
#define NUMAKER_UART9_RST       ((4UL<<24) | SYS_IPRST1_UART9RST_Pos)
#define NUMAKER_WWDT0_RST       ((4UL<<24) | SYS_IPRST1_WWDT0RST_Pos)
#define NUMAKER_WWDT1_RST       ((4UL<<24) | SYS_IPRST1_WWDT1RST_Pos)
#define NUMAKER_USBD_RST        ((4UL<<24) | SYS_IPRST1_USBDRST_Pos)
#define NUMAKER_EADC0_RST       ((4UL<<24) | SYS_IPRST1_EADC0RST_Pos)
#define NUMAKER_TRNG_RST        ((4UL<<24) | SYS_IPRST1_TRNGRST_Pos)
#define NUMAKER_USCI0_RST       ((8UL<<24) | SYS_IPRST2_USCI0RST_Pos)
#define NUMAKER_USCI1_RST       ((8UL<<24) | SYS_IPRST2_USCI1RST_Pos)
#define NUMAKER_DAC0_RST        ((8UL<<24) | SYS_IPRST2_DAC0RST_Pos)
#define NUMAKER_PWM0_RST        ((8UL<<24) | SYS_IPRST2_PWM0RST_Pos)
#define NUMAKER_PWM1_RST        ((8UL<<24) | SYS_IPRST2_PWM1RST_Pos)
#define NUMAKER_BPWM0_RST       ((8UL<<24) | SYS_IPRST2_BPWM0RST_Pos)
#define NUMAKER_BPWM1_RST       ((8UL<<24) | SYS_IPRST2_BPWM1RST_Pos)
#define NUMAKER_EQEI0_RST       ((8UL<<24) | SYS_IPRST2_EQEI0RST_Pos)
#define NUMAKER_EQEI1_RST       ((8UL<<24) | SYS_IPRST2_EQEI1RST_Pos)
#define NUMAKER_UART6_RST       ((8UL<<24) | SYS_IPRST2_UART6RST_Pos)
#define NUMAKER_UART7_RST       ((8UL<<24) | SYS_IPRST2_UART7RST_Pos)
#define NUMAKER_ECAP0_RST       ((8UL<<24) | SYS_IPRST2_ECAP0RST_Pos)
#define NUMAKER_EADC1_RST       ((8UL<<24) | SYS_IPRST2_EADC1RST_Pos)
#define NUMAKER_LLSI0_RST       ((0x18UL<<24) | SYS_IPRST3_LLSI0RST_Pos)
#define NUMAKER_LLSI1_RST       ((0x18UL<<24) | SYS_IPRST3_LLSI1RST_Pos)
#define NUMAKER_LLSI2_RST       ((0x18UL<<24) | SYS_IPRST3_LLSI2RST_Pos)
#define NUMAKER_LLSI3_RST       ((0x18UL<<24) | SYS_IPRST3_LLSI3RST_Pos)
#define NUMAKER_LLSI4_RST       ((0x18UL<<24) | SYS_IPRST3_LLSI4RST_Pos)
#define NUMAKER_LLSI5_RST       ((0x18UL<<24) | SYS_IPRST3_LLSI5RST_Pos)
#define NUMAKER_LLSI6_RST       ((0x18UL<<24) | SYS_IPRST3_LLSI6RST_Pos)
#define NUMAKER_LLSI7_RST       ((0x18UL<<24) | SYS_IPRST3_LLSI7RST_Pos)
#define NUMAKER_LLSI8_RST       ((0x18UL<<24) | SYS_IPRST3_LLSI8RST_Pos)
#define NUMAKER_LLSI9_RST       ((0x18UL<<24) | SYS_IPRST3_LLSI9RST_Pos)
#define NUMAKER_ELLSI0_RST      ((0x18UL<<24) | SYS_IPRST3_ELLSI0RST_Pos)
#define NUMAKER_ELLSI1_RST      ((0x18UL<<24) | SYS_IPRST3_ELLSI1RST_Pos)

/* End of M3351 BSP sys.h reset module copy */

/** @} */

/** @endcond INTERNAL_HIDDEN */

/** @} */

#endif
