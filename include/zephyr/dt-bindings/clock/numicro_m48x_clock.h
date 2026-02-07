/*
 * Copyright (c) 2026 Fiona Behrens
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_NUMICRO_M48X_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_NUMICRO_M48X_CLOCK_H_

/* Beginning of M480 BSP clk_reg.h copy */

#define NUMICRO_CLK_AHBCLK_PDMACKEND_Pos  (1)
#define NUMICRO_CLK_AHBCLK_ISPCKEN_Pos    (2)
#define NUMICRO_CLK_AHBCLK_EBICKEN_Pos    (3)
#define NUMICRO_CLK_AHBCLK_EMACKEN_Pos    (5)
#define NUMICRO_CLK_AHBCLK_SDH0CKEN_Pos   (6)
#define NUMICRO_CLK_AHBCLK_CRCKEN_Pos     (7)
#define NUMICRO_CLK_AHBCLK_CCAPCKEN_Pos   (8)
#define NUMICRO_CLK_AHBCLK_SENCKEN_Pos    (9)
#define NUMICRO_CLK_AHBCLK_HSUSBDCKEN_Pos (10)
#define NUMICRO_CLK_AHBCLK_CRPTCHKEN_Pos  (12)
#define NUMICRO_CLK_AHBCLK_SPIMCKEN_Pos   (14)
#define NUMICRO_CLK_AHBCLK_FMCIDLE_Pos    (15)
#define NUMICRO_CLK_AHBCLK_USBHCKEN_Pos   (16)
#define NUMICRO_CLK_AHBCLK_SDH1CKEN_Pos   (17)

#define NUMICRO_CLK_APBCLK0_WDTCKEN_Pos    (0)
#define NUMICRO_CLK_APBCLK0_RTCCKEN_Pos    (1)
#define NUMICRO_CLK_APBCLK0_TMR0CKEN_Pos   (2)
#define NUMICRO_CLK_APBCLK0_TMR1CKEN_Pos   (3)
#define NUMICRO_CLK_APBCLK0_TMR2CKEN_Pos   (4)
#define NUMICRO_CLK_APBCLK0_TMR3CKEN_Pos   (5)
#define NUMICRO_CLK_APBCLK0_CLKOCKEN_Pos   (6)
#define NUMICRO_CLK_APBCLK0_ACMP01CKEN_Pos (7)
#define NUMICRO_CLK_APBCLK0_I2C0CKEN_Pos   (8)
#define NUMICRO_CLK_APBCLK0_I2C1CKEN_Pos   (9)
#define NUMICRO_CLK_APBCLK0_I2C2CKEN_Pos   (10)
#define NUMICRO_CLK_APBCLK0_QSPI0CKEN_Pos  (12)
#define NUMICRO_CLK_APBCLK0_SPI0CKEN_Pos   (13)
#define NUMICRO_CLK_APBCLK0_SPI1CKEN_Pos   (14)
#define NUMICRO_CLK_APBCLK0_SPI2CKEN_Pos   (15)
#define NUMICRO_CLK_APBCLK0_UART0CKEN_Pos  (16)
#define NUMICRO_CLK_APBCLK0_UART1CKEN_Pos  (17)
#define NUMICRO_CLK_APBCLK0_UART2CKEN_Pos  (18)
#define NUMICRO_CLK_APBCLK0_UART3CKEN_Pos  (19)
#define NUMICRO_CLK_APBCLK0_UART4CKEN_Pos  (20)
#define NUMICRO_CLK_APBCLK0_UART5CKEN_Pos  (21)
#define NUMICRO_CLK_APBCLK0_UART6CKEN_Pos  (22)
#define NUMICRO_CLK_APBCLK0_UART7CKEN_Pos  (23)
#define NUMICRO_CLK_APBCLK0_CAN0CKEN_Pos   (24)
#define NUMICRO_CLK_APBCLK0_CAN1CKEN_Pos   (25)
#define NUMICRO_CLK_APBCLK0_OTGCKEN_Pos    (26)
#define NUMICRO_CLK_APBCLK0_USBDCKEN_Pos   (27)
#define NUMICRO_CLK_APBCLK0_EADCCKEN_Pos   (28)
#define NUMICRO_CLK_APBCLK0_I2S0CKEN_Pos   (29)
#define NUMICRO_CLK_APBCLK0_HSOTGCKEN_Pos  (30)

#define NUMICRO_CLK_APBCLK1_SC0CKEN_Pos   (0)
#define NUMICRO_CLK_APBCLK1_SC1CKEN_Pos   (1)
#define NUMICRO_CLK_APBCLK1_SC2CKEN_Pos   (2)
#define NUMICRO_CLK_APBCLK1_QSPI1CKEN_Pos (4)
#define NUMICRO_CLK_APBCLK1_SPI3CKEN_Pos  (6)
#define NUMICRO_CLK_APBCLK1_USCI0CKEN_Pos (8)
#define NUMICRO_CLK_APBCLK1_USCI1CKEN_Pos (9)
#define NUMICRO_CLK_APBCLK1_DACCKEN_Pos   (12)
#define NUMICRO_CLK_APBCLK1_EPWM0CKEN_Pos (16)
#define NUMICRO_CLK_APBCLK1_EPWM1CKEN_Pos (17)
#define NUMICRO_CLK_APBCLK1_BPWM0CKEN_Pos (18)
#define NUMICRO_CLK_APBCLK1_BPWM1CKEN_Pos (19)
#define NUMICRO_CLK_APBCLK1_QEI0CKEN_Pos  (22)
#define NUMICRO_CLK_APBCLK1_QEI1CKEN_Pos  (23)
#define NUMICRO_CLK_APBCLK1_TRNGCKEN_Pos  (25)
#define NUMICRO_CLK_APBCLK1_ECAP0CKEN_Pos (26)
#define NUMICRO_CLK_APBCLK1_ECAP1CKEN_Pos (27)
#define NUMICRO_CLK_APBCLK1_CAN2CKEN_Pos  (28)
#define NUMICRO_CLK_APBCLK1_OPACKEN_Pos   (30)
#define NUMICRO_CLK_APBCLK1_EADC1CKEN_Pos (31)

#define NUMICRO_CLK_CLKSEL0_HCLKSEL_Pos  (0)
#define NUMICRO_CLK_CLKSEL0_STCLKSEL_Pos (3)
#define NUMICRO_CLK_CLKSEL0_USBSEL_Pos   (8)
#define NUMICRO_CLK_CLKSEL0_CCAPSEL_Pos  (16)
#define NUMICRO_CLK_CLKSEL0_SDH0SEL_Pos  (20)
#define NUMICRO_CLK_CLKSEL0_SDH1SEL_Pos  (22)

#define NUMICRO_CLK_CLKSEL1_WDTSEL_Pos   (0)
#define NUMICRO_CLK_CLKSEL1_TMR0SEL_Pos  (8)
#define NUMICRO_CLK_CLKSEL1_TMR1SEL_Pos  (12)
#define NUMICRO_CLK_CLKSEL1_TMR2SEL_Pos  (16)
#define NUMICRO_CLK_CLKSEL1_TMR3SEL_Pos  (20)
#define NUMICRO_CLK_CLKSEL1_UART0SEL_Pos (24)
#define NUMICRO_CLK_CLKSEL1_UART1SEL_Pos (26)
#define NUMICRO_CLK_CLKSEL1_CLKOSEL_Pos  (28)
#define NUMICRO_CLK_CLKSEL1_WWDTSEL_Pos  (30)

#define NUMICRO_CLK_CLKSEL2_EPWM0SEL_Pos (0)
#define NUMICRO_CLK_CLKSEL2_EPWM1SEL_Pos (1)
#define NUMICRO_CLK_CLKSEL2_QSPI0SEL_Pos (2)
#define NUMICRO_CLK_CLKSEL2_SPI0SEL_Pos  (4)
#define NUMICRO_CLK_CLKSEL2_SPI1SEL_Pos  (6)
#define NUMICRO_CLK_CLKSEL2_BPWM0SEL_Pos (8)
#define NUMICRO_CLK_CLKSEL2_BPWM1SEL_Pos (9)
#define NUMICRO_CLK_CLKSEL2_SPI2SEL_Pos  (10)
#define NUMICRO_CLK_CLKSEL2_SPI3SEL_Pos  (12)

#define NUMICRO_CLK_CLKSEL3_SC0SEL_Pos   (0)
#define NUMICRO_CLK_CLKSEL3_SC1SEL_Pos   (2)
#define NUMICRO_CLK_CLKSEL3_SC2SEL_Pos   (4)
#define NUMICRO_CLK_CLKSEL3_RTCSEL_Pos   (8)
#define NUMICRO_CLK_CLKSEL3_QSPI1SEL_Pos (12)
#define NUMICRO_CLK_CLKSEL3_I2S0SEL_Pos  (16)
#define NUMICRO_CLK_CLKSEL3_UART6SEL_Pos (20)
#define NUMICRO_CLK_CLKSEL3_UART7SEL_Pos (22)
#define NUMICRO_CLK_CLKSEL3_UART2SEL_Pos (24)
#define NUMICRO_CLK_CLKSEL3_UART3SEL_Pos (26)
#define NUMICRO_CLK_CLKSEL3_UART4SEL_Pos (28)
#define NUMICRO_CLK_CLKSEL3_UART5SEL_Pos (30)

#define NUMICRO_CLK_CLKDIV0_HCLKDIV_Pos  (0)
#define NUMICRO_CLK_CLKDIV0_USBDIV_Pos   (4)
#define NUMICRO_CLK_CLKDIV0_UART0DIV_Pos (8)
#define NUMICRO_CLK_CLKDIV0_UART1DIV_Pos (12)
#define NUMICRO_CLK_CLKDIV0_EADCDIV_Pos  (16)
#define NUMICRO_CLK_CLKDIV0_SDH0DIV_Pos  (24)

#define NUMICRO_CLK_CLKDIV1_SC0DIV_Pos (0)
#define NUMICRO_CLK_CLKDIV1_SC1DIV_Pos (8)
#define NUMICRO_CLK_CLKDIV1_SC2DIV_Pos (16)

#define NUMICRO_CLK_CLKDIV2_I2SDIV_Pos   (0)
#define NUMICRO_CLK_CLKDIV2_EADC1DIV_Pos (24)

#define NUMICRO_CLK_CLKDIV3_CCAPDIV_Pos   (0)
#define NUMICRO_CLK_CLKDIV3_VSENSEDIV_Pos (8)
#define NUMICRO_CLK_CLKDIV3_EMACDIV_Pos   (16)
#define NUMICRO_CLK_CLKDIV3_SDH1DIV_Pos   (24)

#define NUMICRO_CLK_CLKDIV4_UART2DIV_Pos (0)
#define NUMICRO_CLK_CLKDIV4_UART3DIV_Pos (4)
#define NUMICRO_CLK_CLKDIV4_UART4DIV_Pos (8)
#define NUMICRO_CLK_CLKDIV4_UART5DIV_Pos (12)
#define NUMICRO_CLK_CLKDIV4_UART6DIV_Pos (16)
#define NUMICRO_CLK_CLKDIV4_UART7DIV_Pos (20)

/* End of M480 BSP clk_reg.h copy */

/* Beginning of M480 BSP clk.h copy */

/*---------------------------------------------------------------------------------------------------------*/
/*  CLKSEL0 constant definitions.  (Write-protection) */
/*---------------------------------------------------------------------------------------------------------*/

#define NUMICRO_CLK_CLKSEL0_CCAPSEL_HXT  (0x0UL << NUMICRO_CLK_CLKSEL0_CCAPSEL_Pos)
#define NUMICRO_CLK_CLKSEL0_CCAPSEL_PLL  (0x1UL << NUMICRO_CLK_CLKSEL0_CCAPSEL_Pos)
#define NUMICRO_CLK_CLKSEL0_CCAPSEL_HCLK (0x2UL << NUMICRO_CLK_CLKSEL0_CCAPSEL_Pos)
#define NUMICRO_CLK_CLKSEL0_CCAPSEL_HIRC (0x3UL << NUMICRO_CLK_CLKSEL0_CCAPSEL_Pos)

#define NUMICRO_CLK_CLKSEL0_SDH0SEL_HXT  (0x0UL << NUMICRO_CLK_CLKSEL0_SDH0SEL_Pos)
#define NUMICRO_CLK_CLKSEL0_SDH0SEL_PLL  (0x1UL << NUMICRO_CLK_CLKSEL0_SDH0SEL_Pos)
#define NUMICRO_CLK_CLKSEL0_SDH0SEL_HIRC (0x3UL << NUMICRO_CLK_CLKSEL0_SDH0SEL_Pos)
#define NUMICRO_CLK_CLKSEL0_SDH0SEL_HCLK (0x2UL << NUMICRO_CLK_CLKSEL0_SDH0SEL_Pos)

#define NUMICRO_CLK_CLKSEL0_SDH1SEL_HXT  (0x0UL << NUMICRO_CLK_CLKSEL0_SDH1SEL_Pos)
#define NUMICRO_CLK_CLKSEL0_SDH1SEL_PLL  (0x1UL << NUMICRO_CLK_CLKSEL0_SDH1SEL_Pos)
#define NUMICRO_CLK_CLKSEL0_SDH1SEL_HIRC (0x3UL << NUMICRO_CLK_CLKSEL0_SDH1SEL_Pos)
#define NUMICRO_CLK_CLKSEL0_SDH1SEL_HCLK (0x2UL << NUMICRO_CLK_CLKSEL0_SDH1SEL_Pos)

/*---------------------------------------------------------------------------------------------------------*/
/*  CLKSEL1 constant definitions. */
/*---------------------------------------------------------------------------------------------------------*/
#define NUMICRO_CLK_CLKSEL1_WDTSEL_LXT          (0x1UL << NUMICRO_CLK_CLKSEL1_WDTSEL_Pos)
#define NUMICRO_CLK_CLKSEL1_WDTSEL_LIRC         (0x3UL << NUMICRO_CLK_CLKSEL1_WDTSEL_Pos)
#define NUMICRO_CLK_CLKSEL1_WDTSEL_HCLK_DIV2048 (0x2UL << NUMICRO_CLK_CLKSEL1_WDTSEL_Pos)

#define NUMICRO_CLK_CLKSEL1_TMR0SEL_HXT   (0x0UL << NUMICRO_CLK_CLKSEL1_TMR0SEL_Pos)
#define NUMICRO_CLK_CLKSEL1_TMR0SEL_LXT   (0x1UL << NUMICRO_CLK_CLKSEL1_TMR0SEL_Pos)
#define NUMICRO_CLK_CLKSEL1_TMR0SEL_LIRC  (0x5UL << NUMICRO_CLK_CLKSEL1_TMR0SEL_Pos)
#define NUMICRO_CLK_CLKSEL1_TMR0SEL_HIRC  (0x7UL << NUMICRO_CLK_CLKSEL1_TMR0SEL_Pos)
#define NUMICRO_CLK_CLKSEL1_TMR0SEL_PCLK0 (0x2UL << NUMICRO_CLK_CLKSEL1_TMR0SEL_Pos)
#define NUMICRO_CLK_CLKSEL1_TMR0SEL_EXT   (0x3UL << NUMICRO_CLK_CLKSEL1_TMR0SEL_Pos)

#define NUMICRO_CLK_CLKSEL1_TMR1SEL_HXT   (0x0UL << NUMICRO_CLK_CLKSEL1_TMR1SEL_Pos)
#define NUMICRO_CLK_CLKSEL1_TMR1SEL_LXT   (0x1UL << NUMICRO_CLK_CLKSEL1_TMR1SEL_Pos)
#define NUMICRO_CLK_CLKSEL1_TMR1SEL_LIRC  (0x5UL << NUMICRO_CLK_CLKSEL1_TMR1SEL_Pos)
#define NUMICRO_CLK_CLKSEL1_TMR1SEL_HIRC  (0x7UL << NUMICRO_CLK_CLKSEL1_TMR1SEL_Pos)
#define NUMICRO_CLK_CLKSEL1_TMR1SEL_PCLK0 (0x2UL << NUMICRO_CLK_CLKSEL1_TMR1SEL_Pos)
#define NUMICRO_CLK_CLKSEL1_TMR1SEL_EXT   (0x3UL << NUMICRO_CLK_CLKSEL1_TMR1SEL_Pos)

#define NUMICRO_CLK_CLKSEL1_TMR2SEL_HXT   (0x0UL << NUMICRO_CLK_CLKSEL1_TMR2SEL_Pos)
#define NUMICRO_CLK_CLKSEL1_TMR2SEL_LXT   (0x1UL << NUMICRO_CLK_CLKSEL1_TMR2SEL_Pos)
#define NUMICRO_CLK_CLKSEL1_TMR2SEL_LIRC  (0x5UL << NUMICRO_CLK_CLKSEL1_TMR2SEL_Pos)
#define NUMICRO_CLK_CLKSEL1_TMR2SEL_HIRC  (0x7UL << NUMICRO_CLK_CLKSEL1_TMR2SEL_Pos)
#define NUMICRO_CLK_CLKSEL1_TMR2SEL_PCLK1 (0x2UL << NUMICRO_CLK_CLKSEL1_TMR2SEL_Pos)
#define NUMICRO_CLK_CLKSEL1_TMR2SEL_EXT   (0x3UL << NUMICRO_CLK_CLKSEL1_TMR2SEL_Pos)

#define NUMICRO_CLK_CLKSEL1_TMR3SEL_HXT   (0x0UL << NUMICRO_CLK_CLKSEL1_TMR3SEL_Pos)
#define NUMICRO_CLK_CLKSEL1_TMR3SEL_LXT   (0x1UL << NUMICRO_CLK_CLKSEL1_TMR3SEL_Pos)
#define NUMICRO_CLK_CLKSEL1_TMR3SEL_LIRC  (0x5UL << NUMICRO_CLK_CLKSEL1_TMR3SEL_Pos)
#define NUMICRO_CLK_CLKSEL1_TMR3SEL_HIRC  (0x7UL << NUMICRO_CLK_CLKSEL1_TMR3SEL_Pos)
#define NUMICRO_CLK_CLKSEL1_TMR3SEL_PCLK1 (0x2UL << NUMICRO_CLK_CLKSEL1_TMR3SEL_Pos)
#define NUMICRO_CLK_CLKSEL1_TMR3SEL_EXT   (0x3UL << NUMICRO_CLK_CLKSEL1_TMR3SEL_Pos)

#define NUMICRO_CLK_CLKSEL1_UART0SEL_HXT  (0x0UL << NUMICRO_CLK_CLKSEL1_UART0SEL_Pos)
#define NUMICRO_CLK_CLKSEL1_UART0SEL_LXT  (0x2UL << NUMICRO_CLK_CLKSEL1_UART0SEL_Pos)
#define NUMICRO_CLK_CLKSEL1_UART0SEL_PLL  (0x1UL << NUMICRO_CLK_CLKSEL1_UART0SEL_Pos)
#define NUMICRO_CLK_CLKSEL1_UART0SEL_HIRC (0x3UL << NUMICRO_CLK_CLKSEL1_UART0SEL_Pos)

#define NUMICRO_CLK_CLKSEL1_UART1SEL_HXT  (0x0UL << NUMICRO_CLK_CLKSEL1_UART1SEL_Pos)
#define NUMICRO_CLK_CLKSEL1_UART1SEL_LXT  (0x2UL << NUMICRO_CLK_CLKSEL1_UART1SEL_Pos)
#define NUMICRO_CLK_CLKSEL1_UART1SEL_PLL  (0x1UL << NUMICRO_CLK_CLKSEL1_UART1SEL_Pos)
#define NUMICRO_CLK_CLKSEL1_UART1SEL_HIRC (0x3UL << NUMICRO_CLK_CLKSEL1_UART1SEL_Pos)

#define NUMICRO_CLK_CLKSEL1_CLKOSEL_HXT  (0x0UL << NUMICRO_CLK_CLKSEL1_CLKOSEL_Pos)
#define NUMICRO_CLK_CLKSEL1_CLKOSEL_LXT  (0x1UL << NUMICRO_CLK_CLKSEL1_CLKOSEL_Pos)
#define NUMICRO_CLK_CLKSEL1_CLKOSEL_HIRC (0x3UL << NUMICRO_CLK_CLKSEL1_CLKOSEL_Pos)
#define NUMICRO_CLK_CLKSEL1_CLKOSEL_HCLK (0x2UL << NUMICRO_CLK_CLKSEL1_CLKOSEL_Pos)

#define NUMICRO_CLK_CLKSEL1_WWDTSEL_LIRC         (0x3UL << NUMICRO_CLK_CLKSEL1_WWDTSEL_Pos)
#define NUMICRO_CLK_CLKSEL1_WWDTSEL_HCLK_DIV2048 (0x2UL << NUMICRO_CLK_CLKSEL1_WWDTSEL_Pos)

/*---------------------------------------------------------------------------------------------------------*/
/*  CLKSEL2 constant definitions. */
/*---------------------------------------------------------------------------------------------------------*/
#define NUMICRO_CLK_CLKSEL2_QSPI0SEL_HXT   (0x0UL << NUMICRO_CLK_CLKSEL2_QSPI0SEL_Pos)
#define NUMICRO_CLK_CLKSEL2_QSPI0SEL_PLL   (0x1UL << NUMICRO_CLK_CLKSEL2_QSPI0SEL_Pos)
#define NUMICRO_CLK_CLKSEL2_QSPI0SEL_HIRC  (0x3UL << NUMICRO_CLK_CLKSEL2_QSPI0SEL_Pos)
#define NUMICRO_CLK_CLKSEL2_QSPI0SEL_PCLK0 (0x2UL << NUMICRO_CLK_CLKSEL2_QSPI0SEL_Pos)

#define NUMICRO_CLK_CLKSEL2_SPI0SEL_HXT   (0x0UL << NUMICRO_CLK_CLKSEL2_SPI0SEL_Pos)
#define NUMICRO_CLK_CLKSEL2_SPI0SEL_PLL   (0x1UL << NUMICRO_CLK_CLKSEL2_SPI0SEL_Pos)
#define NUMICRO_CLK_CLKSEL2_SPI0SEL_HIRC  (0x3UL << NUMICRO_CLK_CLKSEL2_SPI0SEL_Pos)
#define NUMICRO_CLK_CLKSEL2_SPI0SEL_PCLK1 (0x2UL << NUMICRO_CLK_CLKSEL2_SPI0SEL_Pos)

#define NUMICRO_CLK_CLKSEL2_SPI1SEL_HXT   (0x0UL << NUMICRO_CLK_CLKSEL2_SPI1SEL_Pos)
#define NUMICRO_CLK_CLKSEL2_SPI1SEL_PLL   (0x1UL << NUMICRO_CLK_CLKSEL2_SPI1SEL_Pos)
#define NUMICRO_CLK_CLKSEL2_SPI1SEL_HIRC  (0x3UL << NUMICRO_CLK_CLKSEL2_SPI1SEL_Pos)
#define NUMICRO_CLK_CLKSEL2_SPI1SEL_PCLK0 (0x2UL << NUMICRO_CLK_CLKSEL2_SPI1SEL_Pos)

#define NUMICRO_CLK_CLKSEL2_EPWM0SEL_PLL   (0x0UL << NUMICRO_CLK_CLKSEL2_EPWM0SEL_Pos)
#define NUMICRO_CLK_CLKSEL2_EPWM0SEL_PCLK0 (0x1UL << NUMICRO_CLK_CLKSEL2_EPWM0SEL_Pos)

#define NUMICRO_CLK_CLKSEL2_EPWM1SEL_PLL   (0x0UL << NUMICRO_CLK_CLKSEL2_EPWM1SEL_Pos)
#define NUMICRO_CLK_CLKSEL2_EPWM1SEL_PCLK1 (0x1UL << NUMICRO_CLK_CLKSEL2_EPWM1SEL_Pos)

#define NUMICRO_CLK_CLKSEL2_BPWM0SEL_PLL   (0x0UL << NUMICRO_CLK_CLKSEL2_BPWM0SEL_Pos)
#define NUMICRO_CLK_CLKSEL2_BPWM0SEL_PCLK0 (0x1UL << NUMICRO_CLK_CLKSEL2_BPWM0SEL_Pos)

#define NUMICRO_CLK_CLKSEL2_BPWM1SEL_PLL   (0x0UL << NUMICRO_CLK_CLKSEL2_BPWM1SEL_Pos)
#define NUMICRO_CLK_CLKSEL2_BPWM1SEL_PCLK1 (0x1UL << NUMICRO_CLK_CLKSEL2_BPWM1SEL_Pos)

#define NUMICRO_CLK_CLKSEL2_SPI2SEL_HXT   (0x0UL << NUMICRO_CLK_CLKSEL2_SPI2SEL_Pos)
#define NUMICRO_CLK_CLKSEL2_SPI2SEL_PLL   (0x1UL << NUMICRO_CLK_CLKSEL2_SPI2SEL_Pos)
#define NUMICRO_CLK_CLKSEL2_SPI2SEL_HIRC  (0x3UL << NUMICRO_CLK_CLKSEL2_SPI2SEL_Pos)
#define NUMICRO_CLK_CLKSEL2_SPI2SEL_PCLK1 (0x2UL << NUMICRO_CLK_CLKSEL2_SPI2SEL_Pos)

#define NUMICRO_CLK_CLKSEL2_SPI3SEL_HXT   (0x0UL << NUMICRO_CLK_CLKSEL2_SPI3SEL_Pos)
#define NUMICRO_CLK_CLKSEL2_SPI3SEL_PLL   (0x1UL << NUMICRO_CLK_CLKSEL2_SPI3SEL_Pos)
#define NUMICRO_CLK_CLKSEL2_SPI3SEL_HIRC  (0x3UL << NUMICRO_CLK_CLKSEL2_SPI3SEL_Pos)
#define NUMICRO_CLK_CLKSEL2_SPI3SEL_PCLK0 (0x2UL << NUMICRO_CLK_CLKSEL2_SPI3SEL_Pos)

/*---------------------------------------------------------------------------------------------------------*/
/*  CLKSEL3 constant definitions. */
/*---------------------------------------------------------------------------------------------------------*/
#define NUMICRO_CLK_CLKSEL3_SC0SEL_HXT   (0x0UL << NUMICRO_CLK_CLKSEL3_SC0SEL_Pos)
#define NUMICRO_CLK_CLKSEL3_SC0SEL_PLL   (0x1UL << NUMICRO_CLK_CLKSEL3_SC0SEL_Pos)
#define NUMICRO_CLK_CLKSEL3_SC0SEL_HIRC  (0x3UL << NUMICRO_CLK_CLKSEL3_SC0SEL_Pos)
#define NUMICRO_CLK_CLKSEL3_SC0SEL_PCLK0 (0x2UL << NUMICRO_CLK_CLKSEL3_SC0SEL_Pos)

#define NUMICRO_CLK_CLKSEL3_SC1SEL_HXT   (0x0UL << NUMICRO_CLK_CLKSEL3_SC1SEL_Pos)
#define NUMICRO_CLK_CLKSEL3_SC1SEL_PLL   (0x1UL << NUMICRO_CLK_CLKSEL3_SC1SEL_Pos)
#define NUMICRO_CLK_CLKSEL3_SC1SEL_HIRC  (0x3UL << NUMICRO_CLK_CLKSEL3_SC1SEL_Pos)
#define NUMICRO_CLK_CLKSEL3_SC1SEL_PCLK1 (0x2UL << NUMICRO_CLK_CLKSEL3_SC1SEL_Pos)

#define NUMICRO_CLK_CLKSEL3_SC2SEL_HXT   (0x0UL << NUMICRO_CLK_CLKSEL3_SC2SEL_Pos)
#define NUMICRO_CLK_CLKSEL3_SC2SEL_PLL   (0x1UL << NUMICRO_CLK_CLKSEL3_SC2SEL_Pos)
#define NUMICRO_CLK_CLKSEL3_SC2SEL_HIRC  (0x3UL << NUMICRO_CLK_CLKSEL3_SC2SEL_Pos)
#define NUMICRO_CLK_CLKSEL3_SC2SEL_PCLK0 (0x2UL << NUMICRO_CLK_CLKSEL3_SC2SEL_Pos)

#define NUMICRO_CLK_CLKSEL3_RTCSEL_LXT  (0x0UL << NUMICRO_CLK_CLKSEL3_RTCSEL_Pos)
#define NUMICRO_CLK_CLKSEL3_RTCSEL_LIRC (0x1UL << NUMICRO_CLK_CLKSEL3_RTCSEL_Pos)

#define NUMICRO_CLK_CLKSEL3_QSPI1SEL_HXT   (0x0UL << NUMICRO_CLK_CLKSEL3_QSPI1SEL_Pos)
#define NUMICRO_CLK_CLKSEL3_QSPI1SEL_PLL   (0x1UL << NUMICRO_CLK_CLKSEL3_QSPI1SEL_Pos)
#define NUMICRO_CLK_CLKSEL3_QSPI1SEL_HIRC  (0x3UL << NUMICRO_CLK_CLKSEL3_QSPI1SEL_Pos)
#define NUMICRO_CLK_CLKSEL3_QSPI1SEL_PCLK1 (0x2UL << NUMICRO_CLK_CLKSEL3_QSPI1SEL_Pos)

#define NUMICRO_CLK_CLKSEL3_I2S0SEL_HXT   (0x0UL << NUMICRO_CLK_CLKSEL3_I2S0SEL_Pos)
#define NUMICRO_CLK_CLKSEL3_I2S0SEL_PLL   (0x1UL << NUMICRO_CLK_CLKSEL3_I2S0SEL_Pos)
#define NUMICRO_CLK_CLKSEL3_I2S0SEL_HIRC  (0x3UL << NUMICRO_CLK_CLKSEL3_I2S0SEL_Pos)
#define NUMICRO_CLK_CLKSEL3_I2S0SEL_PCLK0 (0x2UL << NUMICRO_CLK_CLKSEL3_I2S0SEL_Pos)

#define NUMICRO_CLK_CLKSEL3_UART2SEL_HXT  (0x0UL << NUMICRO_CLK_CLKSEL3_UART2SEL_Pos)
#define NUMICRO_CLK_CLKSEL3_UART2SEL_LXT  (0x2UL << NUMICRO_CLK_CLKSEL3_UART2SEL_Pos)
#define NUMICRO_CLK_CLKSEL3_UART2SEL_PLL  (0x1UL << NUMICRO_CLK_CLKSEL3_UART2SEL_Pos)
#define NUMICRO_CLK_CLKSEL3_UART2SEL_HIRC (0x3UL << NUMICRO_CLK_CLKSEL3_UART2SEL_Pos)

#define NUMICRO_CLK_CLKSEL3_UART3SEL_HXT  (0x0UL << NUMICRO_CLK_CLKSEL3_UART3SEL_Pos)
#define NUMICRO_CLK_CLKSEL3_UART3SEL_LXT  (0x2UL << NUMICRO_CLK_CLKSEL3_UART3SEL_Pos)
#define NUMICRO_CLK_CLKSEL3_UART3SEL_PLL  (0x1UL << NUMICRO_CLK_CLKSEL3_UART3SEL_Pos)
#define NUMICRO_CLK_CLKSEL3_UART3SEL_HIRC (0x3UL << NUMICRO_CLK_CLKSEL3_UART3SEL_Pos)

#define NUMICRO_CLK_CLKSEL3_UART4SEL_HXT  (0x0UL << NUMICRO_CLK_CLKSEL3_UART4SEL_Pos)
#define NUMICRO_CLK_CLKSEL3_UART4SEL_LXT  (0x2UL << NUMICRO_CLK_CLKSEL3_UART4SEL_Pos)
#define NUMICRO_CLK_CLKSEL3_UART4SEL_PLL  (0x1UL << NUMICRO_CLK_CLKSEL3_UART4SEL_Pos)
#define NUMICRO_CLK_CLKSEL3_UART4SEL_HIRC (0x3UL << NUMICRO_CLK_CLKSEL3_UART4SEL_Pos)

#define NUMICRO_CLK_CLKSEL3_UART5SEL_HXT  (0x0UL << NUMICRO_CLK_CLKSEL3_UART5SEL_Pos)
#define NUMICRO_CLK_CLKSEL3_UART5SEL_LXT  (0x2UL << NUMICRO_CLK_CLKSEL3_UART5SEL_Pos)
#define NUMICRO_CLK_CLKSEL3_UART5SEL_PLL  (0x1UL << NUMICRO_CLK_CLKSEL3_UART5SEL_Pos)
#define NUMICRO_CLK_CLKSEL3_UART5SEL_HIRC (0x3UL << NUMICRO_CLK_CLKSEL3_UART5SEL_Pos)

#define NUMICRO_CLK_CLKSEL3_UART6SEL_HXT  (0x0UL << NUMICRO_CLK_CLKSEL3_UART6SEL_Pos)
#define NUMICRO_CLK_CLKSEL3_UART6SEL_LXT  (0x2UL << NUMICRO_CLK_CLKSEL3_UART6SEL_Pos)
#define NUMICRO_CLK_CLKSEL3_UART6SEL_PLL  (0x1UL << NUMICRO_CLK_CLKSEL3_UART6SEL_Pos)
#define NUMICRO_CLK_CLKSEL3_UART6SEL_HIRC (0x3UL << NUMICRO_CLK_CLKSEL3_UART6SEL_Pos)

#define NUMICRO_CLK_CLKSEL3_UART7SEL_HXT  (0x0UL << NUMICRO_CLK_CLKSEL3_UART7SEL_Pos)
#define NUMICRO_CLK_CLKSEL3_UART7SEL_LXT  (0x2UL << NUMICRO_CLK_CLKSEL3_UART7SEL_Pos)
#define NUMICRO_CLK_CLKSEL3_UART7SEL_PLL  (0x1UL << NUMICRO_CLK_CLKSEL3_UART7SEL_Pos)
#define NUMICRO_CLK_CLKSEL3_UART7SEL_HIRC (0x3UL << NUMICRO_CLK_CLKSEL3_UART7SEL_Pos)

/*---------------------------------------------------------------------------------------------------------*/
/*  CLKDIV0 constant definitions. */
/*---------------------------------------------------------------------------------------------------------*/
#define NUMICRO_CLK_CLKDIV0_HCLK(x)  (((x) - 1UL) << NUMICRO_CLK_CLKDIV0_HCLKDIV_Pos)
#define NUMICRO_CLK_CLKDIV0_USB(x)   (((x) - 1UL) << NUMICRO_CLK_CLKDIV0_USBDIV_Pos)
#define NUMICRO_CLK_CLKDIV0_SDH0(x)  (((x) - 1UL) << NUMICRO_CLK_CLKDIV0_SDH0DIV_Pos)
#define NUMICRO_CLK_CLKDIV0_UART0(x) (((x) - 1UL) << NUMICRO_CLK_CLKDIV0_UART0DIV_Pos)
#define NUMICRO_CLK_CLKDIV0_UART1(x) (((x) - 1UL) << NUMICRO_CLK_CLKDIV0_UART1DIV_Pos)
#define NUMICRO_CLK_CLKDIV0_EADC(x)  (((x) - 1UL) << NUMICRO_CLK_CLKDIV0_EADCDIV_Pos)

/*---------------------------------------------------------------------------------------------------------*/
/*  CLKDIV1 constant definitions. */
/*---------------------------------------------------------------------------------------------------------*/
#define NUMICRO_CLK_CLKDIV1_SC0(x) (((x) - 1UL) << NUMICRO_CLK_CLKDIV1_SC0DIV_Pos)
#define NUMICRO_CLK_CLKDIV1_SC1(x) (((x) - 1UL) << NUMICRO_CLK_CLKDIV1_SC1DIV_Pos)
#define NUMICRO_CLK_CLKDIV1_SC2(x) (((x) - 1UL) << NUMICRO_CLK_CLKDIV1_SC2DIV_Pos)

/*---------------------------------------------------------------------------------------------------------*/
/*  CLKDIV2 constant definitions. */
/*---------------------------------------------------------------------------------------------------------*/
#define NUMICRO_CLK_CLKDIV2_I2S0(x)  (((x) - 1UL) << NUMICRO_CLK_CLKDIV2_I2SDIV_Pos)
#define NUMICRO_CLK_CLKDIV2_EADC1(x) (((x) - 1UL) << NUMICRO_CLK_CLKDIV2_EADC1DIV_Pos)

/*---------------------------------------------------------------------------------------------------------*/
/*  CLKDIV3 constant definitions. */
/*---------------------------------------------------------------------------------------------------------*/
#define NUMICRO_CLK_CLKDIV3_CCAP(x)   (((x) - 1UL) << NUMICRO_CLK_CLKDIV3_CCAPDIV_Pos)
#define NUMICRO_CLK_CLKDIV3_VSENSE(x) (((x) - 1UL) << NUMICRO_CLK_CLKDIV3_VSENSEDIV_Pos)
#define NUMICRO_CLK_CLKDIV3_EMAC(x)   (((x) - 1UL) << NUMICRO_CLK_CLKDIV3_EMACDIV_Pos)
#define NUMICRO_CLK_CLKDIV3_SDH1(x)   (((x) - 1UL) << NUMICRO_CLK_CLKDIV3_SDH1DIV_Pos)

/*---------------------------------------------------------------------------------------------------------*/
/*  CLKDIV4 constant definitions. */
/*---------------------------------------------------------------------------------------------------------*/
#define NUMICRO_CLK_CLKDIV4_UART2(x) (((x) - 1UL) << NUMICRO_CLK_CLKDIV4_UART2DIV_Pos)
#define NUMICRO_CLK_CLKDIV4_UART3(x) (((x) - 1UL) << NUMICRO_CLK_CLKDIV4_UART3DIV_Pos)
#define NUMICRO_CLK_CLKDIV4_UART4(x) (((x) - 1UL) << NUMICRO_CLK_CLKDIV4_UART4DIV_Pos)
#define NUMICRO_CLK_CLKDIV4_UART5(x) (((x) - 1UL) << NUMICRO_CLK_CLKDIV4_UART5DIV_Pos)
#define NUMICRO_CLK_CLKDIV4_UART6(x) (((x) - 1UL) << NUMICRO_CLK_CLKDIV4_UART6DIV_Pos)
#define NUMICRO_CLK_CLKDIV4_UART7(x) (((x) - 1UL) << NUMICRO_CLK_CLKDIV4_UART7DIV_Pos)

/*---------------------------------------------------------------------------------------------------------*/
/*  MODULE constant definitions. */
/*---------------------------------------------------------------------------------------------------------*/

/* APBCLK(31:30)|CLKSEL(29:28)|CLKSEL_Msk(27:25)
 * |CLKSEL_Pos(24:20)|CLKDIV(19:18)|CLKDIV_Msk(17:10)|CLKDIV_Pos(9:5)|IP_EN_Pos(4:0) */

#define NUMICRO_MODULE_APBCLK(x)     (((x) >> 30) & 0x3UL)
#define NUMICRO_MODULE_CLKSEL(x)     (((x) >> 28) & 0x3UL)
#define NUMICRO_MODULE_CLKSEL_Msk(x) (((x) >> 25) & 0x7UL)
#define NUMICRO_MODULE_CLKSEL_Pos(x) (((x) >> 20) & 0x1fUL)
#define NUMICRO_MODULE_CLKDIV(x)     (((x) >> 18) & 0x3UL)
#define NUMICRO_MODULE_CLKDIV_Msk(x) (((x) >> 10) & 0xffUL)
#define NUMICRO_MODULE_CLKDIV_Pos(x) (((x) >> 5) & 0x1fUL)
#define NUMICRO_MODULE_IP_EN_Pos(x)  (((x) >> 0) & 0x1fUL)
#define NUMICRO_MODULE_NoMsk         0x0UL
#define NUMICRO_NA                   NUMICRO_MODULE_NoMsk

#define NUMICRO_MODULE_APBCLK_ENC(x)     (((x) & 0x03UL) << 30)
#define NUMICRO_MODULE_CLKSEL_ENC(x)     (((x) & 0x03UL) << 28)
#define NUMICRO_MODULE_CLKSEL_Msk_ENC(x) (((x) & 0x07UL) << 25)
#define NUMICRO_MODULE_CLKSEL_Pos_ENC(x) (((x) & 0x1fUL) << 20)
#define NUMICRO_MODULE_CLKDIV_ENC(x)     (((x) & 0x03UL) << 18)
#define NUMICRO_MODULE_CLKDIV_Msk_ENC(x) (((x) & 0xffUL) << 10)
#define NUMICRO_MODULE_CLKDIV_Pos_ENC(x) (((x) & 0x1fUL) << 5)
#define NUMICRO_MODULE_IP_EN_Pos_ENC(x)  (((x) & 0x1fUL) << 0)

#define NUMICRO_PDMA_MODULE                                                                        \
	((0UL << 30) | (NUMICRO_MODULE_NoMsk << 28) | (NUMICRO_MODULE_NoMsk << 25) |               \
	 (NUMICRO_MODULE_NoMsk << 20) | (NUMICRO_MODULE_NoMsk << 18) |                             \
	 (NUMICRO_MODULE_NoMsk << 10) | (NUMICRO_MODULE_NoMsk << 5) | (1UL << 0))
#define NUMICRO_ISP_MODULE                                                                         \
	((0UL << 30) | (NUMICRO_MODULE_NoMsk << 28) | (NUMICRO_MODULE_NoMsk << 25) |               \
	 (NUMICRO_MODULE_NoMsk << 20) | (NUMICRO_MODULE_NoMsk << 18) |                             \
	 (NUMICRO_MODULE_NoMsk << 10) | (NUMICRO_MODULE_NoMsk << 5) | (2UL << 0))
#define NUMICRO_EBI_MODULE                                                                         \
	((0UL << 30) | (NUMICRO_MODULE_NoMsk << 28) | (NUMICRO_MODULE_NoMsk << 25) |               \
	 (NUMICRO_MODULE_NoMsk << 20) | (NUMICRO_MODULE_NoMsk << 18) |                             \
	 (NUMICRO_MODULE_NoMsk << 10) | (NUMICRO_MODULE_NoMsk << 5) | (3UL << 0))
#define NUMICRO_USBH_MODULE                                                                        \
	((0UL << 30) | (NUMICRO_MODULE_NoMsk << 28) | (NUMICRO_MODULE_NoMsk << 25) |               \
	 (NUMICRO_MODULE_NoMsk << 20) | (NUMICRO_MODULE_NoMsk << 18) |                             \
	 (NUMICRO_MODULE_NoMsk << 10) | (NUMICRO_MODULE_NoMsk << 5) | (16UL << 0))
#define NUMICRO_EMAC_MODULE                                                                        \
	((0UL << 30) | (NUMICRO_MODULE_NoMsk << 28) | (NUMICRO_MODULE_NoMsk << 25) |               \
	 (NUMICRO_MODULE_NoMsk << 20) | (2UL << 18) | (0xFFUL << 10) | (16UL << 5) | (5UL << 0))
#define NUMICRO_SDH0_MODULE                                                                        \
	((0UL << 30) | (0UL << 28) | (0x3UL << 25) | (20UL << 20) | (0UL << 18) | (0xFFUL << 10) | \
	 (24UL << 5) | (6UL << 0))
#define NUMICRO_CRC_MODULE                                                                         \
	((0UL << 30) | (NUMICRO_MODULE_NoMsk << 28) | (NUMICRO_MODULE_NoMsk << 25) |               \
	 (NUMICRO_MODULE_NoMsk << 20) | (NUMICRO_MODULE_NoMsk << 18) |                             \
	 (NUMICRO_MODULE_NoMsk << 10) | (NUMICRO_MODULE_NoMsk << 5) | (7UL << 0))
#define NUMICRO_CCAP_MODULE                                                                        \
	((0UL << 30) | (0UL << 28) | (0x3UL << 25) | (16UL << 20) | (2UL << 18) | (0xFFUL << 10) | \
	 (0UL << 5) | (8UL << 0))
#define NUMICRO_SEN_MODULE                                                                         \
	((0UL << 30) | (NUMICRO_MODULE_NoMsk << 28) | (NUMICRO_MODULE_NoMsk << 25) |               \
	 (NUMICRO_MODULE_NoMsk << 20) | (NUMICRO_MODULE_NoMsk << 18) |                             \
	 (NUMICRO_MODULE_NoMsk << 10) | (NUMICRO_MODULE_NoMsk << 5) | (9UL << 0))
#define NUMICRO_HSUSBD_MODULE                                                                      \
	((0UL << 30) | (NUMICRO_MODULE_NoMsk << 28) | (NUMICRO_MODULE_NoMsk << 25) |               \
	 (NUMICRO_MODULE_NoMsk << 20) | (NUMICRO_MODULE_NoMsk << 18) |                             \
	 (NUMICRO_MODULE_NoMsk << 10) | (NUMICRO_MODULE_NoMsk << 5) | (10UL << 0))
#define NUMICRO_CRPT_MODULE                                                                        \
	((0UL << 30) | (NUMICRO_MODULE_NoMsk << 28) | (NUMICRO_MODULE_NoMsk << 25) |               \
	 (NUMICRO_MODULE_NoMsk << 20) | (NUMICRO_MODULE_NoMsk << 18) |                             \
	 (NUMICRO_MODULE_NoMsk << 10) | (NUMICRO_MODULE_NoMsk << 5) | (12UL << 0))
#define NUMICRO_SPIM_MODULE                                                                        \
	((0UL << 30) | (NUMICRO_MODULE_NoMsk << 28) | (NUMICRO_MODULE_NoMsk << 25) |               \
	 (NUMICRO_MODULE_NoMsk << 20) | (NUMICRO_MODULE_NoMsk << 18) |                             \
	 (NUMICRO_MODULE_NoMsk << 10) | (NUMICRO_MODULE_NoMsk << 5) | (14UL << 0))
#define NUMICRO_FMCIDLE_MODULE                                                                     \
	((0UL << 30) | (NUMICRO_MODULE_NoMsk << 28) | (NUMICRO_MODULE_NoMsk << 25) |               \
	 (NUMICRO_MODULE_NoMsk << 20) | (NUMICRO_MODULE_NoMsk << 18) |                             \
	 (NUMICRO_MODULE_NoMsk << 10) | (NUMICRO_MODULE_NoMsk << 5) | (15UL << 0))
#define NUMICRO_SDH1_MODULE                                                                        \
	((0UL << 30) | (0UL << 28) | (0x3UL << 25) | (22UL << 20) | (2UL << 18) | (0xFFUL << 10) | \
	 (24UL << 5) | (17UL << 0))
#define NUMICRO_WDT_MODULE                                                                         \
	((1UL << 30) | (1UL << 28) | (0x3UL << 25) | (0UL << 20) | (NUMICRO_MODULE_NoMsk << 18) |  \
	 (NUMICRO_MODULE_NoMsk << 10) | (NUMICRO_MODULE_NoMsk << 5) | (0UL << 0))
#define NUMICRO_RTC_MODULE                                                                         \
	((1UL << 30) | (3UL << 28) | (0x1UL << 25) | (8UL << 20) | (NUMICRO_MODULE_NoMsk << 18) |  \
	 (NUMICRO_MODULE_NoMsk << 10) | (NUMICRO_MODULE_NoMsk << 5) | (1UL << 0))
#define NUMICRO_TMR0_MODULE                                                                        \
	((1UL << 30) | (1UL << 28) | (0x7UL << 25) | (8UL << 20) | (NUMICRO_MODULE_NoMsk << 18) |  \
	 (NUMICRO_MODULE_NoMsk << 10) | (NUMICRO_MODULE_NoMsk << 5) | (2UL << 0))
#define NUMICRO_TMR1_MODULE                                                                        \
	((1UL << 30) | (1UL << 28) | (0x7UL << 25) | (12UL << 20) | (NUMICRO_MODULE_NoMsk << 18) | \
	 (NUMICRO_MODULE_NoMsk << 10) | (NUMICRO_MODULE_NoMsk << 5) | (3UL << 0))
#define NUMICRO_TMR2_MODULE                                                                        \
	((1UL << 30) | (1UL << 28) | (0x7UL << 25) | (16UL << 20) | (NUMICRO_MODULE_NoMsk << 18) | \
	 (NUMICRO_MODULE_NoMsk << 10) | (NUMICRO_MODULE_NoMsk << 5) | (4UL << 0))
#define NUMICRO_TMR3_MODULE                                                                        \
	((1UL << 30) | (1UL << 28) | (0x7UL << 25) | (20UL << 20) | (NUMICRO_MODULE_NoMsk << 18) | \
	 (NUMICRO_MODULE_NoMsk << 10) | (NUMICRO_MODULE_NoMsk << 5) | (5UL << 0))
#define NUMICRO_CLKO_MODULE                                                                        \
	((1UL << 30) | (1UL << 28) | (0x3UL << 25) | (28UL << 20) | (NUMICRO_MODULE_NoMsk << 18) | \
	 (NUMICRO_MODULE_NoMsk << 10) | (NUMICRO_MODULE_NoMsk << 5) | (6UL << 0))
#define NUMICRO_WWDT_MODULE                                                                        \
	((1UL << 30) | (1UL << 28) | (0x3UL << 25) | (30UL << 20) | (NUMICRO_MODULE_NoMsk << 18) | \
	 (NUMICRO_MODULE_NoMsk << 10) | (NUMICRO_MODULE_NoMsk << 5) | (0UL << 0))
#define NUMICRO_ACMP01_MODULE                                                                      \
	((1UL << 30) | (NUMICRO_MODULE_NoMsk << 28) | (NUMICRO_MODULE_NoMsk << 25) |               \
	 (NUMICRO_MODULE_NoMsk << 20) | (NUMICRO_MODULE_NoMsk << 18) |                             \
	 (NUMICRO_MODULE_NoMsk << 10) | (NUMICRO_MODULE_NoMsk << 5) | (7UL << 0))
#define NUMICRO_I2C0_MODULE                                                                        \
	((1UL << 30) | (NUMICRO_MODULE_NoMsk << 28) | (NUMICRO_MODULE_NoMsk << 25) |               \
	 (NUMICRO_MODULE_NoMsk << 20) | (NUMICRO_MODULE_NoMsk << 18) |                             \
	 (NUMICRO_MODULE_NoMsk << 10) | (NUMICRO_MODULE_NoMsk << 5) | (8UL << 0))
#define NUMICRO_I2C1_MODULE                                                                        \
	((1UL << 30) | (NUMICRO_MODULE_NoMsk << 28) | (NUMICRO_MODULE_NoMsk << 25) |               \
	 (NUMICRO_MODULE_NoMsk << 20) | (NUMICRO_MODULE_NoMsk << 18) |                             \
	 (NUMICRO_MODULE_NoMsk << 10) | (NUMICRO_MODULE_NoMsk << 5) | (9UL << 0))
#define NUMICRO_I2C2_MODULE                                                                        \
	((1UL << 30) | (NUMICRO_MODULE_NoMsk << 28) | (NUMICRO_MODULE_NoMsk << 25) |               \
	 (NUMICRO_MODULE_NoMsk << 20) | (NUMICRO_MODULE_NoMsk << 18) |                             \
	 (NUMICRO_MODULE_NoMsk << 10) | (NUMICRO_MODULE_NoMsk << 5) | (10UL << 0))
#define NUMICRO_QSPI0_MODULE                                                                       \
	((1UL << 30) | (2UL << 28) | (0x3UL << 25) | (2UL << 20) | (NUMICRO_MODULE_NoMsk << 18) |  \
	 (NUMICRO_MODULE_NoMsk << 10) | (NUMICRO_MODULE_NoMsk << 5) | (12UL << 0))
#define NUMICRO_SPI0_MODULE                                                                        \
	((1UL << 30) | (2UL << 28) | (0x3UL << 25) | (4UL << 20) | (NUMICRO_MODULE_NoMsk << 18) |  \
	 (NUMICRO_MODULE_NoMsk << 10) | (NUMICRO_MODULE_NoMsk << 5) | (13UL << 0))
#define NUMICRO_SPI1_MODULE                                                                        \
	((1UL << 30) | (2UL << 28) | (0x3UL << 25) | (6UL << 20) | (NUMICRO_MODULE_NoMsk << 18) |  \
	 (NUMICRO_MODULE_NoMsk << 10) | (NUMICRO_MODULE_NoMsk << 5) | (14UL << 0))
#define NUMICRO_SPI2_MODULE                                                                        \
	((1UL << 30) | (2UL << 28) | (0x3UL << 25) | (10UL << 20) | (NUMICRO_MODULE_NoMsk << 18) | \
	 (NUMICRO_MODULE_NoMsk << 10) | (NUMICRO_MODULE_NoMsk << 5) | (15UL << 0))
#define NUMICRO_UART0_MODULE                                                                       \
	((1UL << 30) | (1UL << 28) | (0x3UL << 25) | (24UL << 20) | (0UL << 18) | (0xFUL << 10) |  \
	 (8UL << 5) | (16UL << 0))
#define NUMICRO_UART1_MODULE                                                                       \
	((1UL << 30) | (1UL << 28) | (0x3UL << 25) | (26UL << 20) | (0UL << 18) | (0xFUL << 10) |  \
	 (12UL << 5) | (17UL << 0))
#define NUMICRO_UART2_MODULE                                                                       \
	((1UL << 30) | (3UL << 28) | (0x3UL << 25) | (24UL << 20) | (3UL << 18) | (0xFUL << 10) |  \
	 (0UL << 5) | (18UL << 0))
#define NUMICRO_UART3_MODULE                                                                       \
	((1UL << 30) | (3UL << 28) | (0x3UL << 25) | (26UL << 20) | (3UL << 18) | (0xFUL << 10) |  \
	 (4UL << 5) | (19UL << 0))
#define NUMICRO_UART4_MODULE                                                                       \
	((1UL << 30) | (3UL << 28) | (0x3UL << 25) | (28UL << 20) | (3UL << 18) | (0xFUL << 10) |  \
	 (8UL << 5) | (20UL << 0))
#define NUMICRO_UART5_MODULE                                                                       \
	((1UL << 30) | (3UL << 28) | (0x3UL << 25) | (30UL << 20) | (3UL << 18) | (0xFUL << 10) |  \
	 (12UL << 5) | (21UL << 0))
#define NUMICRO_UART6_MODULE                                                                       \
	((1UL << 30) | (3UL << 28) | (0x3UL << 25) | (20UL << 20) | (3UL << 18) | (0xFUL << 10) |  \
	 (16UL << 5) | (22UL << 0))
#define NUMICRO_UART7_MODULE                                                                       \
	((1UL << 30) | (3UL << 28) | (0x3UL << 25) | (22UL << 20) | (3UL << 18) | (0xFUL << 10) |  \
	 (20UL << 5) | (23UL << 0))
#define NUMICRO_CAN0_MODULE                                                                        \
	((1UL << 30) | (NUMICRO_MODULE_NoMsk << 28) | (NUMICRO_MODULE_NoMsk << 25) |               \
	 (NUMICRO_MODULE_NoMsk << 20) | (NUMICRO_MODULE_NoMsk << 18) |                             \
	 (NUMICRO_MODULE_NoMsk << 10) | (NUMICRO_MODULE_NoMsk << 5) | (24UL << 0))
#define NUMICRO_CAN1_MODULE                                                                        \
	((1UL << 30) | (NUMICRO_MODULE_NoMsk << 28) | (NUMICRO_MODULE_NoMsk << 25) |               \
	 (NUMICRO_MODULE_NoMsk << 20) | (NUMICRO_MODULE_NoMsk << 18) |                             \
	 (NUMICRO_MODULE_NoMsk << 10) | (NUMICRO_MODULE_NoMsk << 5) | (25UL << 0))
#define NUMICRO_OTG_MODULE                                                                         \
	((1UL << 30) | (NUMICRO_MODULE_NoMsk << 28) | (NUMICRO_MODULE_NoMsk << 25) |               \
	 (NUMICRO_MODULE_NoMsk << 20) | (NUMICRO_MODULE_NoMsk << 18) |                             \
	 (NUMICRO_MODULE_NoMsk << 10) | (NUMICRO_MODULE_NoMsk << 5) | (26UL << 0))
#define NUMICRO_USBD_MODULE                                                                        \
	((1UL << 30) | (NUMICRO_MODULE_NoMsk << 28) | (NUMICRO_MODULE_NoMsk << 25) |               \
	 (NUMICRO_MODULE_NoMsk << 20) | (NUMICRO_MODULE_NoMsk << 18) |                             \
	 (NUMICRO_MODULE_NoMsk << 10) | (NUMICRO_MODULE_NoMsk << 5) | (27UL << 0))
#define NUMICRO_EADC_MODULE                                                                        \
	((1UL << 30) | (NUMICRO_MODULE_NoMsk << 28) | (NUMICRO_MODULE_NoMsk << 25) |               \
	 (NUMICRO_MODULE_NoMsk << 20) | (0UL << 18) | (0xFFUL << 10) | (16UL << 5) | (28UL << 0))
#define NUMICRO_I2S0_MODULE                                                                        \
	((1UL << 30) | (3UL << 28) | (0x3UL << 25) | (16UL << 20) | (2UL << 18) | (0xFUL << 10) |  \
	 (0UL << 5) | (29UL << 0))
#define NUMICRO_HSOTG_MODULE                                                                       \
	((1UL << 30) | (NUMICRO_MODULE_NoMsk << 28) | (NUMICRO_MODULE_NoMsk << 25) |               \
	 (NUMICRO_MODULE_NoMsk << 20) | (NUMICRO_MODULE_NoMsk << 18) |                             \
	 (NUMICRO_MODULE_NoMsk << 10) | (NUMICRO_MODULE_NoMsk << 5) | (30UL << 0))
#define NUMICRO_SC0_MODULE                                                                         \
	((2UL << 30) | (3UL << 28) | (0x3UL << 25) | (0UL << 20) | (1UL << 18) | (0xFFUL << 10) |  \
	 (0UL << 5) | (0UL << 0))
#define NUMICRO_SC1_MODULE                                                                         \
	((2UL << 30) | (3UL << 28) | (0x3UL << 25) | (2UL << 20) | (1UL << 18) | (0xFFUL << 10) |  \
	 (8UL << 5) | (1UL << 0))
#define NUMICRO_SC2_MODULE                                                                         \
	((2UL << 30) | (3UL << 28) | (0x3UL << 25) | (4UL << 20) | (1UL << 18) | (0xFFUL << 10) |  \
	 (16UL << 5) | (2UL << 0))
#define NUMICRO_QSPI1_MODULE                                                                       \
	((2UL << 30) | (3UL << 28) | (0x3UL << 25) | (12UL << 20) | (NUMICRO_MODULE_NoMsk << 18) | \
	 (NUMICRO_MODULE_NoMsk << 10) | (NUMICRO_MODULE_NoMsk << 5) | (4UL << 0))
#define NUMICRO_SPI3_MODULE                                                                        \
	((2UL << 30) | (2UL << 28) | (0x3UL << 25) | (12UL << 20) | (NUMICRO_MODULE_NoMsk << 18) | \
	 (NUMICRO_MODULE_NoMsk << 10) | (NUMICRO_MODULE_NoMsk << 5) | (6UL << 0))
#define NUMICRO_USCI0_MODULE                                                                       \
	((2UL << 30) | (NUMICRO_MODULE_NoMsk << 28) | (NUMICRO_MODULE_NoMsk << 25) |               \
	 (NUMICRO_MODULE_NoMsk << 20) | (NUMICRO_MODULE_NoMsk << 18) |                             \
	 (NUMICRO_MODULE_NoMsk << 10) | (NUMICRO_MODULE_NoMsk << 5) | (8UL << 0))
#define NUMICRO_USCI1_MODULE                                                                       \
	((2UL << 30) | (NUMICRO_MODULE_NoMsk << 28) | (NUMICRO_MODULE_NoMsk << 25) |               \
	 (NUMICRO_MODULE_NoMsk << 20) | (NUMICRO_MODULE_NoMsk << 18) |                             \
	 (NUMICRO_MODULE_NoMsk << 10) | (NUMICRO_MODULE_NoMsk << 5) | (9UL << 0))
#define NUMICRO_DAC_MODULE                                                                         \
	((2UL << 30) | (NUMICRO_MODULE_NoMsk << 28) | (NUMICRO_MODULE_NoMsk << 25) |               \
	 (NUMICRO_MODULE_NoMsk << 20) | (NUMICRO_MODULE_NoMsk << 18) |                             \
	 (NUMICRO_MODULE_NoMsk << 10) | (NUMICRO_MODULE_NoMsk << 5) | (12UL << 0))
#define NUMICRO_CAN2_MODULE                                                                        \
	((2UL << 30) | (NUMICRO_MODULE_NoMsk << 28) | (NUMICRO_MODULE_NoMsk << 25) |               \
	 (NUMICRO_MODULE_NoMsk << 20) | (NUMICRO_MODULE_NoMsk << 18) |                             \
	 (NUMICRO_MODULE_NoMsk << 10) | (NUMICRO_MODULE_NoMsk << 5) | (28UL << 0))
#define NUMICRO_EPWM0_MODULE                                                                       \
	((2UL << 30) | (2UL << 28) | (0x1UL << 25) | (0UL << 20) | (NUMICRO_MODULE_NoMsk << 18) |  \
	 (NUMICRO_MODULE_NoMsk << 10) | (NUMICRO_MODULE_NoMsk << 5) | (16UL << 0))
#define NUMICRO_EPWM1_MODULE                                                                       \
	((2UL << 30) | (2UL << 28) | (0x1UL << 25) | (1UL << 20) | (NUMICRO_MODULE_NoMsk << 18) |  \
	 (NUMICRO_MODULE_NoMsk << 10) | (NUMICRO_MODULE_NoMsk << 5) | (17UL << 0))
#define NUMICRO_BPWM0_MODULE                                                                       \
	((2UL << 30) | (2UL << 28) | (0x1UL << 25) | (8UL << 20) | (NUMICRO_MODULE_NoMsk << 18) |  \
	 (NUMICRO_MODULE_NoMsk << 10) | (NUMICRO_MODULE_NoMsk << 5) | (18UL << 0))
#define NUMICRO_BPWM1_MODULE                                                                       \
	((2UL << 30) | (2UL << 28) | (0x1UL << 25) | (9UL << 20) | (NUMICRO_MODULE_NoMsk << 18) |  \
	 (NUMICRO_MODULE_NoMsk << 10) | (NUMICRO_MODULE_NoMsk << 5) | (19UL << 0))
#define NUMICRO_QEI0_MODULE                                                                        \
	((2UL << 30) | (NUMICRO_MODULE_NoMsk << 28) | (NUMICRO_MODULE_NoMsk << 25) |               \
	 (NUMICRO_MODULE_NoMsk << 20) | (NUMICRO_MODULE_NoMsk << 18) |                             \
	 (NUMICRO_MODULE_NoMsk << 10) | (NUMICRO_MODULE_NoMsk << 5) | (22UL << 0))
#define NUMICRO_QEI1_MODULE                                                                        \
	((2UL << 30) | (NUMICRO_MODULE_NoMsk << 28) | (NUMICRO_MODULE_NoMsk << 25) |               \
	 (NUMICRO_MODULE_NoMsk << 20) | (NUMICRO_MODULE_NoMsk << 18) |                             \
	 (NUMICRO_MODULE_NoMsk << 10) | (NUMICRO_MODULE_NoMsk << 5) | (23UL << 0))
#define NUMICRO_TRNG_MODULE                                                                        \
	((2UL << 30) | (NUMICRO_MODULE_NoMsk << 28) | (NUMICRO_MODULE_NoMsk << 25) |               \
	 (NUMICRO_MODULE_NoMsk << 20) | (NUMICRO_MODULE_NoMsk << 18) |                             \
	 (NUMICRO_MODULE_NoMsk << 10) | (NUMICRO_MODULE_NoMsk << 5) | (25UL << 0))
#define NUMICRO_ECAP0_MODULE                                                                       \
	((2UL << 30) | (NUMICRO_MODULE_NoMsk << 28) | (NUMICRO_MODULE_NoMsk << 25) |               \
	 (NUMICRO_MODULE_NoMsk << 20) | (NUMICRO_MODULE_NoMsk << 18) |                             \
	 (NUMICRO_MODULE_NoMsk << 10) | (NUMICRO_MODULE_NoMsk << 5) | (26UL << 0))
#define NUMICRO_ECAP1_MODULE                                                                       \
	((2UL << 30) | (NUMICRO_MODULE_NoMsk << 28) | (NUMICRO_MODULE_NoMsk << 25) |               \
	 (NUMICRO_MODULE_NoMsk << 20) | (NUMICRO_MODULE_NoMsk << 18) |                             \
	 (NUMICRO_MODULE_NoMsk << 10) | (NUMICRO_MODULE_NoMsk << 5) | (27UL << 0))
#define NUMICRO_OPA_MODULE                                                                         \
	((2UL << 30) | (NUMICRO_MODULE_NoMsk << 28) | (NUMICRO_MODULE_NoMsk << 25) |               \
	 (NUMICRO_MODULE_NoMsk << 20) | (NUMICRO_MODULE_NoMsk << 18) |                             \
	 (NUMICRO_MODULE_NoMsk << 10) | (NUMICRO_MODULE_NoMsk << 5) | (30UL << 0))
#define NUMICRO_EADC1_MODULE                                                                       \
	((2UL << 30) | (NUMICRO_MODULE_NoMsk << 28) | (NUMICRO_MODULE_NoMsk << 25) |               \
	 (NUMICRO_MODULE_NoMsk << 20) | (2UL << 18) | (0xFFUL << 10) | (24UL << 5) | (31UL << 0))

/* End of M480 BSP clk.h copy */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_NUMICRO_M48X_CLOCK_H_ */
