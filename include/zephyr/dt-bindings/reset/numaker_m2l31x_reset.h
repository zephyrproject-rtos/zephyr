/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_RESET_NUMAKER_M2L31X_RESET_H
#define ZEPHYR_INCLUDE_DT_BINDINGS_RESET_NUMAKER_M2L31X_RESET_H

/* Beginning of M2L31 BSP sys_reg.h reset module copy */

#define LPSCC_IPRST0_LPPDMA0RST_Pos      0
#define LPSCC_IPRST0_LPGPIORST_Pos       1
#define LPSCC_IPRST0_LPSRAMRST_Pos       2
#define LPSCC_IPRST0_WDTRST_Pos          16
#define LPSCC_IPRST0_LPSPI0RST_Pos       17
#define LPSCC_IPRST0_LPI2C0RST_Pos       18
#define LPSCC_IPRST0_LPUART0RST_Pos      19
#define LPSCC_IPRST0_LPTMR0RST_Pos       20
#define LPSCC_IPRST0_LPTMR1RST_Pos       21
#define LPSCC_IPRST0_TTMR0RST_Pos        22
#define LPSCC_IPRST0_TTMR1RST_Pos        23
#define LPSCC_IPRST0_LPADC0RST_Pos       24
#define LPSCC_IPRST0_OPARST_Pos          27
#define SYS_IPRST0_CHIPRST_Pos           0
#define SYS_IPRST0_CPURST_Pos            1
#define SYS_IPRST0_PDMA0RST_Pos          2
#define SYS_IPRST0_EBIRST_Pos            3
#define SYS_IPRST0_USBHRST_Pos           4
#define SYS_IPRST0_CRCRST_Pos            7
#define SYS_IPRST0_CRPTRST_Pos           12
#define SYS_IPRST0_CANFD0RST_Pos         20
#define SYS_IPRST0_CANFD1RST_Pos         21
#define SYS_IPRST1_GPIORST_Pos           1
#define SYS_IPRST1_TMR0RST_Pos           2
#define SYS_IPRST1_TMR1RST_Pos           3
#define SYS_IPRST1_TMR2RST_Pos           4
#define SYS_IPRST1_TMR3RST_Pos           5
#define SYS_IPRST1_ACMP01RST_Pos         7
#define SYS_IPRST1_I2C0RST_Pos           8
#define SYS_IPRST1_I2C1RST_Pos           9
#define SYS_IPRST1_I2C2RST_Pos           10
#define SYS_IPRST1_I2C3RST_Pos           11
#define SYS_IPRST1_QSPI0RST_Pos          12
#define SYS_IPRST1_SPI0RST_Pos           13
#define SYS_IPRST1_SPI1RST_Pos           14
#define SYS_IPRST1_SPI2RST_Pos           15
#define SYS_IPRST1_UART0RST_Pos          16
#define SYS_IPRST1_UART1RST_Pos          17
#define SYS_IPRST1_UART2RST_Pos          18
#define SYS_IPRST1_UART3RST_Pos          19
#define SYS_IPRST1_UART4RST_Pos          20
#define SYS_IPRST1_UART5RST_Pos          21
#define SYS_IPRST1_UART6RST_Pos          22
#define SYS_IPRST1_UART7RST_Pos          23
#define SYS_IPRST1_OTGRST_Pos            26
#define SYS_IPRST1_USBDRST_Pos           27
#define SYS_IPRST1_EADC0RST_Pos          28
#define SYS_IPRST1_TRNGRST_Pos           31
#define SYS_IPRST2_SPI3RST_Pos           6
#define SYS_IPRST2_USCI0RST_Pos          8
#define SYS_IPRST2_USCI1RST_Pos          9
#define SYS_IPRST2_WWDTRST_Pos           11
#define SYS_IPRST2_DACRST_Pos            12
#define SYS_IPRST2_EPWM0RST_Pos          16
#define SYS_IPRST2_EPWM1RST_Pos          17
#define SYS_IPRST2_EQEI0RST_Pos          22
#define SYS_IPRST2_EQEI1RST_Pos          23
#define SYS_IPRST2_TKRST_Pos             25
#define SYS_IPRST2_ECAP0RST_Pos          26
#define SYS_IPRST2_ECAP1RST_Pos          27
#define SYS_IPRST3_ACMP2RST_Pos          7
#define SYS_IPRST3_PWM0RST_Pos           8
#define SYS_IPRST3_PWM1RST_Pos           9
#define SYS_IPRST3_UTCPD0RST_Pos         15

/* End of M2L31 BSP sys_reg.h reset module copy */

/* Beginning of M2L31 BSP sys.h reset module copy */

/*---------------------------------------------------------------------
 *  Module Reset Control Resister constant definitions.
 *---------------------------------------------------------------------
 */

#define NUMAKER_PDMA0_RST       ((0UL<<24) | SYS_IPRST0_PDMA0RST_Pos)
#define NUMAKER_EBI_RST         ((0UL<<24) | SYS_IPRST0_EBIRST_Pos)
#define NUMAKER_USBH_RST        ((0UL<<24) | SYS_IPRST0_USBHRST_Pos)
#define NUMAKER_CRC_RST         ((0UL<<24) | SYS_IPRST0_CRCRST_Pos)
#define NUMAKER_CRPT_RST        ((0UL<<24) | SYS_IPRST0_CRPTRST_Pos)
#define NUMAKER_CANFD0_RST      ((0UL<<24) | SYS_IPRST0_CANFD0RST_Pos)
#define NUMAKER_CANFD1_RST      ((0UL<<24) | SYS_IPRST0_CANFD1RST_Pos)

#define NUMAKER_GPIO_RST        ((4UL<<24) | SYS_IPRST1_GPIORST_Pos)
#define NUMAKER_TMR0_RST        ((4UL<<24) | SYS_IPRST1_TMR0RST_Pos)
#define NUMAKER_TMR1_RST        ((4UL<<24) | SYS_IPRST1_TMR1RST_Pos)
#define NUMAKER_TMR2_RST        ((4UL<<24) | SYS_IPRST1_TMR2RST_Pos)
#define NUMAKER_TMR3_RST        ((4UL<<24) | SYS_IPRST1_TMR3RST_Pos)
#define NUMAKER_ACMP01_RST      ((4UL<<24) | SYS_IPRST1_ACMP01RST_Pos)
#define NUMAKER_I2C0_RST        ((4UL<<24) | SYS_IPRST1_I2C0RST_Pos)
#define NUMAKER_I2C1_RST        ((4UL<<24) | SYS_IPRST1_I2C1RST_Pos)
#define NUMAKER_I2C2_RST        ((4UL<<24) | SYS_IPRST1_I2C2RST_Pos)
#define NUMAKER_I2C3_RST        ((4UL<<24) | SYS_IPRST1_I2C3RST_Pos)
#define NUMAKER_QSPI0_RST       ((4UL<<24) | SYS_IPRST1_QSPI0RST_Pos)
#define NUMAKER_SPI0_RST        ((4UL<<24) | SYS_IPRST1_SPI0RST_Pos)
#define NUMAKER_SPI1_RST        ((4UL<<24) | SYS_IPRST1_SPI1RST_Pos)
#define NUMAKER_SPI2_RST        ((4UL<<24) | SYS_IPRST1_SPI2RST_Pos)
#define NUMAKER_UART0_RST       ((4UL<<24) | SYS_IPRST1_UART0RST_Pos)
#define NUMAKER_UART1_RST       ((4UL<<24) | SYS_IPRST1_UART1RST_Pos)
#define NUMAKER_UART2_RST       ((4UL<<24) | SYS_IPRST1_UART2RST_Pos)
#define NUMAKER_UART3_RST       ((4UL<<24) | SYS_IPRST1_UART3RST_Pos)
#define NUMAKER_UART4_RST       ((4UL<<24) | SYS_IPRST1_UART4RST_Pos)
#define NUMAKER_UART5_RST       ((4UL<<24) | SYS_IPRST1_UART5RST_Pos)
#define NUMAKER_UART6_RST       ((4UL<<24) | SYS_IPRST1_UART6RST_Pos)
#define NUMAKER_UART7_RST       ((4UL<<24) | SYS_IPRST1_UART7RST_Pos)
#define NUMAKER_OTG_RST         ((4UL<<24) | SYS_IPRST1_OTGRST_Pos)
#define NUMAKER_USBD_RST        ((4UL<<24) | SYS_IPRST1_USBDRST_Pos)
#define NUMAKER_EADC0_RST       ((4UL<<24) | SYS_IPRST1_EADC0RST_Pos)
#define NUMAKER_TRNG_RST        ((4UL<<24) | SYS_IPRST1_TRNGRST_Pos)

#define NUMAKER_SPI3_RST        ((8UL<<24) | SYS_IPRST2_SPI3RST_Pos)
#define NUMAKER_USCI0_RST       ((8UL<<24) | SYS_IPRST2_USCI0RST_Pos)
#define NUMAKER_USCI1_RST       ((8UL<<24) | SYS_IPRST2_USCI1RST_Pos)
#define NUMAKER_WWDT_RST        ((8UL<<24) | SYS_IPRST2_WWDTRST_Pos)
#define NUMAKER_DAC_RST         ((8UL<<24) | SYS_IPRST2_DACRST_Pos)
#define NUMAKER_EPWM0_RST       ((8UL<<24) | SYS_IPRST2_EPWM0RST_Pos)
#define NUMAKER_EPWM1_RST       ((8UL<<24) | SYS_IPRST2_EPWM1RST_Pos)
#define NUMAKER_EQEI0_RST       ((8UL<<24) | SYS_IPRST2_EQEI0RST_Pos)
#define NUMAKER_EQEI1_RST       ((8UL<<24) | SYS_IPRST2_EQEI1RST_Pos)
#define NUMAKER_TK_RST          ((8UL<<24) | SYS_IPRST2_TKRST_Pos)
#define NUMAKER_ECAP0_RST       ((8UL<<24) | SYS_IPRST2_ECAP0RST_Pos)
#define NUMAKER_ECAP1_RST       ((8UL<<24) | SYS_IPRST2_ECAP1RST_Pos)

#define NUMAKER_ACMP2_RST       ((0x18UL<<24) | SYS_IPRST3_ACMP2RST_Pos)
#define NUMAKER_PWM0_RST        ((0x18UL<<24) | SYS_IPRST3_PWM0RST_Pos)
#define NUMAKER_PWM1_RST        ((0x18UL<<24) | SYS_IPRST3_PWM1RST_Pos)
#define NUMAKER_UTCPD0_RST      ((0x18UL<<24) | SYS_IPRST3_UTCPD0RST_Pos)

#define NUMAKER_LPPDMA0_RST     ((0x80UL<<24) | LPSCC_IPRST0_LPPDMA0RST_Pos)
#define NUMAKER_LPGPIO_RST      ((0x80UL<<24) | LPSCC_IPRST0_LPGPIORST_Pos)
#define NUMAKER_LPSRAM_RST      ((0x80UL<<24) | LPSCC_IPRST0_LPSRAMRST_Pos)
#define NUMAKER_WDT_RST         ((0x80UL<<24) | LPSCC_IPRST0_WDTRST_Pos)
#define NUMAKER_LPSPI0_RST      ((0x80UL<<24) | LPSCC_IPRST0_LPSPI0RST_Pos)
#define NUMAKER_LPI2C0_RST      ((0x80UL<<24) | LPSCC_IPRST0_LPI2C0RST_Pos)
#define NUMAKER_LPUART0_RST     ((0x80UL<<24) | LPSCC_IPRST0_LPUART0RST_Pos)
#define NUMAKER_LPTMR0_RST      ((0x80UL<<24) | LPSCC_IPRST0_LPTMR0RST_Pos)
#define NUMAKER_LPTMR1_RST      ((0x80UL<<24) | LPSCC_IPRST0_LPTMR1RST_Pos)
#define NUMAKER_TTMR0_RST       ((0x80UL<<24) | LPSCC_IPRST0_TTMR0RST_Pos)
#define NUMAKER_TTMR1_RST       ((0x80UL<<24) | LPSCC_IPRST0_TTMR1RST_Pos)
#define NUMAKER_LPADC0_RST      ((0x80UL<<24) | LPSCC_IPRST0_LPADC0RST_Pos)
#define NUMAKER_OPA_RST         ((0x80UL<<24) | LPSCC_IPRST0_OPARST_Pos)

/* End of M2L31 BSP sys.h reset module copy */

#endif
