/*
 * Copyright (c) 2026 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


/**
 * @file
 * @brief Clock configuration macros for Nuvoton M48x series
 *
 * Provides clock source selection, clock divider, peripheral module
 * clock enable, and power management macros for the Nuvoton M480
 * series SoC family. These macros encode CLK controller register
 * fields for use in devicetree clock property assignments.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_NUMAKER_M48X_CLOCK_H
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_NUMAKER_M48X_CLOCK_H

/** @cond INTERNAL_HIDDEN */

/* Beginning of M480 BSP clk_reg.h copy */

#define NUMAKER_CLK_AHBCLK_PDMACKEN_Pos   1
#define NUMAKER_CLK_AHBCLK_ISPCKEN_Pos    2
#define NUMAKER_CLK_AHBCLK_EBICKEN_Pos    3
#define NUMAKER_CLK_AHBCLK_EMACCKEN_Pos   5
#define NUMAKER_CLK_AHBCLK_SDH0CKEN_Pos   6
#define NUMAKER_CLK_AHBCLK_CRCCKEN_Pos    7
#define NUMAKER_CLK_AHBCLK_CCAPCKEN_Pos   8
#define NUMAKER_CLK_AHBCLK_SENCKEN_Pos    9
#define NUMAKER_CLK_AHBCLK_HSUSBDCKEN_Pos 10
#define NUMAKER_CLK_AHBCLK_CRPTCKEN_Pos   12
#define NUMAKER_CLK_AHBCLK_SPIMCKEN_Pos   14
#define NUMAKER_CLK_AHBCLK_FMCIDLE_Pos    15
#define NUMAKER_CLK_AHBCLK_USBHCKEN_Pos   16
#define NUMAKER_CLK_AHBCLK_SDH1CKEN_Pos   17

#define NUMAKER_CLK_APBCLK0_WDTCKEN_Pos    0
#define NUMAKER_CLK_APBCLK0_RTCCKEN_Pos    1
#define NUMAKER_CLK_APBCLK0_TMR0CKEN_Pos   2
#define NUMAKER_CLK_APBCLK0_TMR1CKEN_Pos   3
#define NUMAKER_CLK_APBCLK0_TMR2CKEN_Pos   4
#define NUMAKER_CLK_APBCLK0_TMR3CKEN_Pos   5
#define NUMAKER_CLK_APBCLK0_CLKOCKEN_Pos   6
#define NUMAKER_CLK_APBCLK0_ACMP01CKEN_Pos 7
#define NUMAKER_CLK_APBCLK0_I2C0CKEN_Pos   8
#define NUMAKER_CLK_APBCLK0_I2C1CKEN_Pos   9
#define NUMAKER_CLK_APBCLK0_I2C2CKEN_Pos   10
#define NUMAKER_CLK_APBCLK0_QSPI0CKEN_Pos  12
#define NUMAKER_CLK_APBCLK0_SPI0CKEN_Pos   13
#define NUMAKER_CLK_APBCLK0_SPI1CKEN_Pos   14
#define NUMAKER_CLK_APBCLK0_SPI2CKEN_Pos   15
#define NUMAKER_CLK_APBCLK0_UART0CKEN_Pos  16
#define NUMAKER_CLK_APBCLK0_UART1CKEN_Pos  17
#define NUMAKER_CLK_APBCLK0_UART2CKEN_Pos  18
#define NUMAKER_CLK_APBCLK0_UART3CKEN_Pos  19
#define NUMAKER_CLK_APBCLK0_UART4CKEN_Pos  20
#define NUMAKER_CLK_APBCLK0_UART5CKEN_Pos  21
#define NUMAKER_CLK_APBCLK0_UART6CKEN_Pos  22
#define NUMAKER_CLK_APBCLK0_UART7CKEN_Pos  23
#define NUMAKER_CLK_APBCLK0_CAN0CKEN_Pos   24
#define NUMAKER_CLK_APBCLK0_CAN1CKEN_Pos   25
#define NUMAKER_CLK_APBCLK0_OTGCKEN_Pos    26
#define NUMAKER_CLK_APBCLK0_USBDCKEN_Pos   27
#define NUMAKER_CLK_APBCLK0_EADCCKEN_Pos   28
#define NUMAKER_CLK_APBCLK0_I2S0CKEN_Pos   29
#define NUMAKER_CLK_APBCLK0_HSOTGCKEN_Pos  30
#define NUMAKER_CLK_APBCLK1_SC0CKEN_Pos    0
#define NUMAKER_CLK_APBCLK1_SC1CKEN_Pos    1
#define NUMAKER_CLK_APBCLK1_SC2CKEN_Pos    2
#define NUMAKER_CLK_APBCLK1_QSPI1CKEN_Pos  4
#define NUMAKER_CLK_APBCLK1_SPI3CKEN_Pos   6
#define NUMAKER_CLK_APBCLK1_USCI0CKEN_Pos  8
#define NUMAKER_CLK_APBCLK1_USCI1CKEN_Pos  9
#define NUMAKER_CLK_APBCLK1_DACCKEN_Pos    12
#define NUMAKER_CLK_APBCLK1_EPWM0CKEN_Pos  16
#define NUMAKER_CLK_APBCLK1_EPWM1CKEN_Pos  17
#define NUMAKER_CLK_APBCLK1_BPWM0CKEN_Pos  18
#define NUMAKER_CLK_APBCLK1_BPWM1CKEN_Pos  19
#define NUMAKER_CLK_APBCLK1_QEI0CKEN_Pos   22
#define NUMAKER_CLK_APBCLK1_QEI1CKEN_Pos   23
#define NUMAKER_CLK_APBCLK1_TRNGCKEN_Pos   25
#define NUMAKER_CLK_APBCLK1_ECAP0CKEN_Pos  26
#define NUMAKER_CLK_APBCLK1_ECAP1CKEN_Pos  27
#define NUMAKER_CLK_APBCLK1_CAN2CKEN_Pos   28
#define NUMAKER_CLK_APBCLK1_OPACKEN_Pos    30
#define NUMAKER_CLK_APBCLK1_EADC1CKEN_Pos  31

#define NUMAKER_CLK_CLKSEL0_HCLKSEL_Pos  0
#define NUMAKER_CLK_CLKSEL0_STCLKSEL_Pos 3
#define NUMAKER_CLK_CLKSEL0_USBSEL_Pos   8
#define NUMAKER_CLK_CLKSEL0_CCAPSEL_Pos  16
#define NUMAKER_CLK_CLKSEL0_SDH0SEL_Pos  20
#define NUMAKER_CLK_CLKSEL0_SDH1SEL_Pos  22
#define NUMAKER_CLK_CLKSEL1_WDTSEL_Pos   0
#define NUMAKER_CLK_CLKSEL1_TMR0SEL_Pos  8
#define NUMAKER_CLK_CLKSEL1_TMR1SEL_Pos  12
#define NUMAKER_CLK_CLKSEL1_TMR2SEL_Pos  16
#define NUMAKER_CLK_CLKSEL1_TMR3SEL_Pos  20
#define NUMAKER_CLK_CLKSEL1_UART0SEL_Pos 24
#define NUMAKER_CLK_CLKSEL1_UART1SEL_Pos 26
#define NUMAKER_CLK_CLKSEL1_CLKOSEL_Pos  28
#define NUMAKER_CLK_CLKSEL1_WWDTSEL_Pos  30
#define NUMAKER_CLK_CLKSEL2_EPWM0SEL_Pos 0
#define NUMAKER_CLK_CLKSEL2_EPWM1SEL_Pos 1
#define NUMAKER_CLK_CLKSEL2_QSPI0SEL_Pos 2
#define NUMAKER_CLK_CLKSEL2_SPI0SEL_Pos  4
#define NUMAKER_CLK_CLKSEL2_SPI1SEL_Pos  6
#define NUMAKER_CLK_CLKSEL2_BPWM0SEL_Pos 8
#define NUMAKER_CLK_CLKSEL2_BPWM1SEL_Pos 9
#define NUMAKER_CLK_CLKSEL2_SPI2SEL_Pos  10
#define NUMAKER_CLK_CLKSEL2_SPI3SEL_Pos  12
#define NUMAKER_CLK_CLKSEL3_SC0SEL_Pos   0
#define NUMAKER_CLK_CLKSEL3_SC1SEL_Pos   2
#define NUMAKER_CLK_CLKSEL3_SC2SEL_Pos   4
#define NUMAKER_CLK_CLKSEL3_RTCSEL_Pos   8
#define NUMAKER_CLK_CLKSEL3_QSPI1SEL_Pos 12
#define NUMAKER_CLK_CLKSEL3_I2S0SEL_Pos  16
#define NUMAKER_CLK_CLKSEL3_UART6SEL_Pos 20
#define NUMAKER_CLK_CLKSEL3_UART7SEL_Pos 22
#define NUMAKER_CLK_CLKSEL3_UART2SEL_Pos 24
#define NUMAKER_CLK_CLKSEL3_UART3SEL_Pos 26
#define NUMAKER_CLK_CLKSEL3_UART4SEL_Pos 28
#define NUMAKER_CLK_CLKSEL3_UART5SEL_Pos 30

#define NUMAKER_CLK_CLKDIV0_HCLKDIV_Pos   0
#define NUMAKER_CLK_CLKDIV0_USBDIV_Pos    4
#define NUMAKER_CLK_CLKDIV0_UART0DIV_Pos  8
#define NUMAKER_CLK_CLKDIV0_UART1DIV_Pos  12
#define NUMAKER_CLK_CLKDIV0_EADCDIV_Pos   16
#define NUMAKER_CLK_CLKDIV0_SDH0DIV_Pos   24
#define NUMAKER_CLK_CLKDIV1_SC0DIV_Pos    0
#define NUMAKER_CLK_CLKDIV1_SC1DIV_Pos    8
#define NUMAKER_CLK_CLKDIV1_SC2DIV_Pos    16
#define NUMAKER_CLK_CLKDIV2_I2SDIV_Pos    0
#define NUMAKER_CLK_CLKDIV2_EADC1DIV_Pos  24
#define NUMAKER_CLK_CLKDIV3_CCAPDIV_Pos   0
#define NUMAKER_CLK_CLKDIV3_VSENSEDIV_Pos 8
#define NUMAKER_CLK_CLKDIV3_EMACDIV_Pos   16
#define NUMAKER_CLK_CLKDIV3_SDH1DIV_Pos   24
#define NUMAKER_CLK_CLKDIV4_UART2DIV_Pos  0
#define NUMAKER_CLK_CLKDIV4_UART3DIV_Pos  4
#define NUMAKER_CLK_CLKDIV4_UART4DIV_Pos  8
#define NUMAKER_CLK_CLKDIV4_UART5DIV_Pos  12
#define NUMAKER_CLK_CLKDIV4_UART6DIV_Pos  16
#define NUMAKER_CLK_CLKDIV4_UART7DIV_Pos  20
#define NUMAKER_CLK_PCLKDIV_APB0DIV_Pos   0
#define NUMAKER_CLK_PCLKDIV_APB1DIV_Pos   4

/* End of M480 BSP clk_reg.h copy */

/** @endcond */

/* Beginning of M480 BSP clk.h copy */

/**
 * @name HCLK clock source selection
 * @{
 */
/** Select HXT (external high-speed crystal) as HCLK clock source */
#define NUMAKER_CLK_CLKSEL0_HCLKSEL_HXT  (0x0UL << NUMAKER_CLK_CLKSEL0_HCLKSEL_Pos)
/** Select LXT (external low-speed crystal) as HCLK clock source */
#define NUMAKER_CLK_CLKSEL0_HCLKSEL_LXT  (0x1UL << NUMAKER_CLK_CLKSEL0_HCLKSEL_Pos)
/** Select PLL output as HCLK clock source */
#define NUMAKER_CLK_CLKSEL0_HCLKSEL_PLL  (0x2UL << NUMAKER_CLK_CLKSEL0_HCLKSEL_Pos)
/** Select LIRC (internal low-speed RC oscillator) as HCLK clock source */
#define NUMAKER_CLK_CLKSEL0_HCLKSEL_LIRC (0x3UL << NUMAKER_CLK_CLKSEL0_HCLKSEL_Pos)
/** Select HIRC (internal high-speed RC oscillator) as HCLK clock source */
#define NUMAKER_CLK_CLKSEL0_HCLKSEL_HIRC (0x7UL << NUMAKER_CLK_CLKSEL0_HCLKSEL_Pos)


/** @} */

/**
 * @name SysTick clock source selection
 * @{
 */
/** Select HXT (external high-speed crystal) as SysTick clock source */
#define NUMAKER_CLK_CLKSEL0_STCLKSEL_HXT       (0x0UL << NUMAKER_CLK_CLKSEL0_STCLKSEL_Pos)
/** Select LXT (external low-speed crystal) as SysTick clock source */
#define NUMAKER_CLK_CLKSEL0_STCLKSEL_LXT       (0x1UL << NUMAKER_CLK_CLKSEL0_STCLKSEL_Pos)
/** Select HXT/2 as SysTick clock source */
#define NUMAKER_CLK_CLKSEL0_STCLKSEL_HXT_DIV2  (0x2UL << NUMAKER_CLK_CLKSEL0_STCLKSEL_Pos)
/** Select HCLK/2 as SysTick clock source */
#define NUMAKER_CLK_CLKSEL0_STCLKSEL_HCLK_DIV2 (0x3UL << NUMAKER_CLK_CLKSEL0_STCLKSEL_Pos)
/** Select HIRC/2 as SysTick clock source */
#define NUMAKER_CLK_CLKSEL0_STCLKSEL_HIRC_DIV2 (0x7UL << NUMAKER_CLK_CLKSEL0_STCLKSEL_Pos)
/** Select HCLK (system clock) as SysTick clock source */
#define NUMAKER_CLK_CLKSEL0_STCLKSEL_HCLK      (0x01UL << SysTick_CTRL_CLKSOURCE_Pos)


/** @} */

/**
 * @name camera capture interface clock source selection
 * @{
 */
/** Select HXT (external high-speed crystal) as camera capture interface clock source */
#define NUMAKER_CLK_CLKSEL0_CCAPSEL_HXT  (0x0UL << NUMAKER_CLK_CLKSEL0_CCAPSEL_Pos)
/** Select PLL output as camera capture interface clock source */
#define NUMAKER_CLK_CLKSEL0_CCAPSEL_PLL  (0x1UL << NUMAKER_CLK_CLKSEL0_CCAPSEL_Pos)
/** Select HCLK (system clock) as camera capture interface clock source */
#define NUMAKER_CLK_CLKSEL0_CCAPSEL_HCLK (0x2UL << NUMAKER_CLK_CLKSEL0_CCAPSEL_Pos)
/** Select HIRC (internal high-speed RC oscillator) as camera capture interface clock source */
#define NUMAKER_CLK_CLKSEL0_CCAPSEL_HIRC (0x3UL << NUMAKER_CLK_CLKSEL0_CCAPSEL_Pos)


/** @} */

/**
 * @name SD host 0 clock source selection
 * @{
 */
/** Select HXT (external high-speed crystal) as SD host 0 clock source */
#define NUMAKER_CLK_CLKSEL0_SDH0SEL_HXT  (0x0UL << NUMAKER_CLK_CLKSEL0_SDH0SEL_Pos)
/** Select PLL output as SD host 0 clock source */
#define NUMAKER_CLK_CLKSEL0_SDH0SEL_PLL  (0x1UL << NUMAKER_CLK_CLKSEL0_SDH0SEL_Pos)
/** Select HIRC (internal high-speed RC oscillator) as SD host 0 clock source */
#define NUMAKER_CLK_CLKSEL0_SDH0SEL_HIRC (0x3UL << NUMAKER_CLK_CLKSEL0_SDH0SEL_Pos)
/** Select HCLK (system clock) as SD host 0 clock source */
#define NUMAKER_CLK_CLKSEL0_SDH0SEL_HCLK (0x2UL << NUMAKER_CLK_CLKSEL0_SDH0SEL_Pos)


/** @} */

/**
 * @name SD host 1 clock source selection
 * @{
 */
/** Select HXT (external high-speed crystal) as SD host 1 clock source */
#define NUMAKER_CLK_CLKSEL0_SDH1SEL_HXT  (0x0UL << NUMAKER_CLK_CLKSEL0_SDH1SEL_Pos)
/** Select PLL output as SD host 1 clock source */
#define NUMAKER_CLK_CLKSEL0_SDH1SEL_PLL  (0x1UL << NUMAKER_CLK_CLKSEL0_SDH1SEL_Pos)
/** Select HIRC (internal high-speed RC oscillator) as SD host 1 clock source */
#define NUMAKER_CLK_CLKSEL0_SDH1SEL_HIRC (0x3UL << NUMAKER_CLK_CLKSEL0_SDH1SEL_Pos)
/** Select HCLK (system clock) as SD host 1 clock source */
#define NUMAKER_CLK_CLKSEL0_SDH1SEL_HCLK (0x2UL << NUMAKER_CLK_CLKSEL0_SDH1SEL_Pos)


/** @} */

/**
 * @name USB clock source selection
 * @{
 */
/** Select RC48M (48 MHz internal RC oscillator) as USB clock source */
#define NUMAKER_CLK_CLKSEL0_USBSEL_RC48M (0x0UL << NUMAKER_CLK_CLKSEL0_USBSEL_Pos)
/** Select PLL output as USB clock source */
#define NUMAKER_CLK_CLKSEL0_USBSEL_PLL   (0x1UL << NUMAKER_CLK_CLKSEL0_USBSEL_Pos)


/** @} */

/**
 * @name watchdog timer clock source selection
 * @{
 */
/** Select LXT (external low-speed crystal) as watchdog timer clock source */
#define NUMAKER_CLK_CLKSEL1_WDTSEL_LXT          (0x1UL << NUMAKER_CLK_CLKSEL1_WDTSEL_Pos)
/** Select LIRC (internal low-speed RC oscillator) as watchdog timer clock source */
#define NUMAKER_CLK_CLKSEL1_WDTSEL_LIRC         (0x3UL << NUMAKER_CLK_CLKSEL1_WDTSEL_Pos)
/** Select HCLK/2048 as watchdog timer clock source */
#define NUMAKER_CLK_CLKSEL1_WDTSEL_HCLK_DIV2048 (0x2UL << NUMAKER_CLK_CLKSEL1_WDTSEL_Pos)


/** @} */

/**
 * @name Timer 0 clock source selection
 * @{
 */
/** Select HXT (external high-speed crystal) as Timer 0 clock source */
#define NUMAKER_CLK_CLKSEL1_TMR0SEL_HXT   (0x0UL << NUMAKER_CLK_CLKSEL1_TMR0SEL_Pos)
/** Select LXT (external low-speed crystal) as Timer 0 clock source */
#define NUMAKER_CLK_CLKSEL1_TMR0SEL_LXT   (0x1UL << NUMAKER_CLK_CLKSEL1_TMR0SEL_Pos)
/** Select LIRC (internal low-speed RC oscillator) as Timer 0 clock source */
#define NUMAKER_CLK_CLKSEL1_TMR0SEL_LIRC  (0x5UL << NUMAKER_CLK_CLKSEL1_TMR0SEL_Pos)
/** Select HIRC (internal high-speed RC oscillator) as Timer 0 clock source */
#define NUMAKER_CLK_CLKSEL1_TMR0SEL_HIRC  (0x7UL << NUMAKER_CLK_CLKSEL1_TMR0SEL_Pos)
/** Select PCLK0 (APB0 peripheral clock) as Timer 0 clock source */
#define NUMAKER_CLK_CLKSEL1_TMR0SEL_PCLK0 (0x2UL << NUMAKER_CLK_CLKSEL1_TMR0SEL_Pos)
/** Select external clock pin as Timer 0 clock source */
#define NUMAKER_CLK_CLKSEL1_TMR0SEL_EXT   (0x3UL << NUMAKER_CLK_CLKSEL1_TMR0SEL_Pos)


/** @} */

/**
 * @name Timer 1 clock source selection
 * @{
 */
/** Select HXT (external high-speed crystal) as Timer 1 clock source */
#define NUMAKER_CLK_CLKSEL1_TMR1SEL_HXT   (0x0UL << NUMAKER_CLK_CLKSEL1_TMR1SEL_Pos)
/** Select LXT (external low-speed crystal) as Timer 1 clock source */
#define NUMAKER_CLK_CLKSEL1_TMR1SEL_LXT   (0x1UL << NUMAKER_CLK_CLKSEL1_TMR1SEL_Pos)
/** Select LIRC (internal low-speed RC oscillator) as Timer 1 clock source */
#define NUMAKER_CLK_CLKSEL1_TMR1SEL_LIRC  (0x5UL << NUMAKER_CLK_CLKSEL1_TMR1SEL_Pos)
/** Select HIRC (internal high-speed RC oscillator) as Timer 1 clock source */
#define NUMAKER_CLK_CLKSEL1_TMR1SEL_HIRC  (0x7UL << NUMAKER_CLK_CLKSEL1_TMR1SEL_Pos)
/** Select PCLK0 (APB0 peripheral clock) as Timer 1 clock source */
#define NUMAKER_CLK_CLKSEL1_TMR1SEL_PCLK0 (0x2UL << NUMAKER_CLK_CLKSEL1_TMR1SEL_Pos)
/** Select external clock pin as Timer 1 clock source */
#define NUMAKER_CLK_CLKSEL1_TMR1SEL_EXT   (0x3UL << NUMAKER_CLK_CLKSEL1_TMR1SEL_Pos)


/** @} */

/**
 * @name Timer 2 clock source selection
 * @{
 */
/** Select HXT (external high-speed crystal) as Timer 2 clock source */
#define NUMAKER_CLK_CLKSEL1_TMR2SEL_HXT   (0x0UL << NUMAKER_CLK_CLKSEL1_TMR2SEL_Pos)
/** Select LXT (external low-speed crystal) as Timer 2 clock source */
#define NUMAKER_CLK_CLKSEL1_TMR2SEL_LXT   (0x1UL << NUMAKER_CLK_CLKSEL1_TMR2SEL_Pos)
/** Select LIRC (internal low-speed RC oscillator) as Timer 2 clock source */
#define NUMAKER_CLK_CLKSEL1_TMR2SEL_LIRC  (0x5UL << NUMAKER_CLK_CLKSEL1_TMR2SEL_Pos)
/** Select HIRC (internal high-speed RC oscillator) as Timer 2 clock source */
#define NUMAKER_CLK_CLKSEL1_TMR2SEL_HIRC  (0x7UL << NUMAKER_CLK_CLKSEL1_TMR2SEL_Pos)
/** Select PCLK1 (APB1 peripheral clock) as Timer 2 clock source */
#define NUMAKER_CLK_CLKSEL1_TMR2SEL_PCLK1 (0x2UL << NUMAKER_CLK_CLKSEL1_TMR2SEL_Pos)
/** Select external clock pin as Timer 2 clock source */
#define NUMAKER_CLK_CLKSEL1_TMR2SEL_EXT   (0x3UL << NUMAKER_CLK_CLKSEL1_TMR2SEL_Pos)


/** @} */

/**
 * @name Timer 3 clock source selection
 * @{
 */
/** Select HXT (external high-speed crystal) as Timer 3 clock source */
#define NUMAKER_CLK_CLKSEL1_TMR3SEL_HXT   (0x0UL << NUMAKER_CLK_CLKSEL1_TMR3SEL_Pos)
/** Select LXT (external low-speed crystal) as Timer 3 clock source */
#define NUMAKER_CLK_CLKSEL1_TMR3SEL_LXT   (0x1UL << NUMAKER_CLK_CLKSEL1_TMR3SEL_Pos)
/** Select LIRC (internal low-speed RC oscillator) as Timer 3 clock source */
#define NUMAKER_CLK_CLKSEL1_TMR3SEL_LIRC  (0x5UL << NUMAKER_CLK_CLKSEL1_TMR3SEL_Pos)
/** Select HIRC (internal high-speed RC oscillator) as Timer 3 clock source */
#define NUMAKER_CLK_CLKSEL1_TMR3SEL_HIRC  (0x7UL << NUMAKER_CLK_CLKSEL1_TMR3SEL_Pos)
/** Select PCLK1 (APB1 peripheral clock) as Timer 3 clock source */
#define NUMAKER_CLK_CLKSEL1_TMR3SEL_PCLK1 (0x2UL << NUMAKER_CLK_CLKSEL1_TMR3SEL_Pos)
/** Select external clock pin as Timer 3 clock source */
#define NUMAKER_CLK_CLKSEL1_TMR3SEL_EXT   (0x3UL << NUMAKER_CLK_CLKSEL1_TMR3SEL_Pos)


/** @} */

/**
 * @name UART0 clock source selection
 * @{
 */
/** Select HXT (external high-speed crystal) as UART0 clock source */
#define NUMAKER_CLK_CLKSEL1_UART0SEL_HXT  (0x0UL << NUMAKER_CLK_CLKSEL1_UART0SEL_Pos)
/** Select LXT (external low-speed crystal) as UART0 clock source */
#define NUMAKER_CLK_CLKSEL1_UART0SEL_LXT  (0x2UL << NUMAKER_CLK_CLKSEL1_UART0SEL_Pos)
/** Select PLL output as UART0 clock source */
#define NUMAKER_CLK_CLKSEL1_UART0SEL_PLL  (0x1UL << NUMAKER_CLK_CLKSEL1_UART0SEL_Pos)
/** Select HIRC (internal high-speed RC oscillator) as UART0 clock source */
#define NUMAKER_CLK_CLKSEL1_UART0SEL_HIRC (0x3UL << NUMAKER_CLK_CLKSEL1_UART0SEL_Pos)


/** @} */

/**
 * @name UART1 clock source selection
 * @{
 */
/** Select HXT (external high-speed crystal) as UART1 clock source */
#define NUMAKER_CLK_CLKSEL1_UART1SEL_HXT  (0x0UL << NUMAKER_CLK_CLKSEL1_UART1SEL_Pos)
/** Select LXT (external low-speed crystal) as UART1 clock source */
#define NUMAKER_CLK_CLKSEL1_UART1SEL_LXT  (0x2UL << NUMAKER_CLK_CLKSEL1_UART1SEL_Pos)
/** Select PLL output as UART1 clock source */
#define NUMAKER_CLK_CLKSEL1_UART1SEL_PLL  (0x1UL << NUMAKER_CLK_CLKSEL1_UART1SEL_Pos)
/** Select HIRC (internal high-speed RC oscillator) as UART1 clock source */
#define NUMAKER_CLK_CLKSEL1_UART1SEL_HIRC (0x3UL << NUMAKER_CLK_CLKSEL1_UART1SEL_Pos)


/** @} */

/**
 * @name clock output clock source selection
 * @{
 */
/** Select HXT (external high-speed crystal) as clock output clock source */
#define NUMAKER_CLK_CLKSEL1_CLKOSEL_HXT  (0x0UL << NUMAKER_CLK_CLKSEL1_CLKOSEL_Pos)
/** Select LXT (external low-speed crystal) as clock output clock source */
#define NUMAKER_CLK_CLKSEL1_CLKOSEL_LXT  (0x1UL << NUMAKER_CLK_CLKSEL1_CLKOSEL_Pos)
/** Select HIRC (internal high-speed RC oscillator) as clock output clock source */
#define NUMAKER_CLK_CLKSEL1_CLKOSEL_HIRC (0x3UL << NUMAKER_CLK_CLKSEL1_CLKOSEL_Pos)
/** Select HCLK (system clock) as clock output clock source */
#define NUMAKER_CLK_CLKSEL1_CLKOSEL_HCLK (0x2UL << NUMAKER_CLK_CLKSEL1_CLKOSEL_Pos)


/** @} */

/**
 * @name window watchdog timer clock source selection
 * @{
 */
/** Select LIRC (internal low-speed RC oscillator) as window watchdog timer clock source */
#define NUMAKER_CLK_CLKSEL1_WWDTSEL_LIRC         (0x3UL << NUMAKER_CLK_CLKSEL1_WWDTSEL_Pos)
/** Select HCLK/2048 as window watchdog timer clock source */
#define NUMAKER_CLK_CLKSEL1_WWDTSEL_HCLK_DIV2048 (0x2UL << NUMAKER_CLK_CLKSEL1_WWDTSEL_Pos)


/** @} */

/**
 * @name QSPI0 clock source selection
 * @{
 */
/** Select HXT (external high-speed crystal) as QSPI0 clock source */
#define NUMAKER_CLK_CLKSEL2_QSPI0SEL_HXT   (0x0UL << NUMAKER_CLK_CLKSEL2_QSPI0SEL_Pos)
/** Select PLL output as QSPI0 clock source */
#define NUMAKER_CLK_CLKSEL2_QSPI0SEL_PLL   (0x1UL << NUMAKER_CLK_CLKSEL2_QSPI0SEL_Pos)
/** Select HIRC (internal high-speed RC oscillator) as QSPI0 clock source */
#define NUMAKER_CLK_CLKSEL2_QSPI0SEL_HIRC  (0x3UL << NUMAKER_CLK_CLKSEL2_QSPI0SEL_Pos)
/** Select PCLK0 (APB0 peripheral clock) as QSPI0 clock source */
#define NUMAKER_CLK_CLKSEL2_QSPI0SEL_PCLK0 (0x2UL << NUMAKER_CLK_CLKSEL2_QSPI0SEL_Pos)


/** @} */

/**
 * @name SPI0 clock source selection
 * @{
 */
/** Select HXT (external high-speed crystal) as SPI0 clock source */
#define NUMAKER_CLK_CLKSEL2_SPI0SEL_HXT   (0x0UL << NUMAKER_CLK_CLKSEL2_SPI0SEL_Pos)
/** Select PLL output as SPI0 clock source */
#define NUMAKER_CLK_CLKSEL2_SPI0SEL_PLL   (0x1UL << NUMAKER_CLK_CLKSEL2_SPI0SEL_Pos)
/** Select HIRC (internal high-speed RC oscillator) as SPI0 clock source */
#define NUMAKER_CLK_CLKSEL2_SPI0SEL_HIRC  (0x3UL << NUMAKER_CLK_CLKSEL2_SPI0SEL_Pos)
/** Select PCLK1 (APB1 peripheral clock) as SPI0 clock source */
#define NUMAKER_CLK_CLKSEL2_SPI0SEL_PCLK1 (0x2UL << NUMAKER_CLK_CLKSEL2_SPI0SEL_Pos)


/** @} */

/**
 * @name SPI1 clock source selection
 * @{
 */
/** Select HXT (external high-speed crystal) as SPI1 clock source */
#define NUMAKER_CLK_CLKSEL2_SPI1SEL_HXT   (0x0UL << NUMAKER_CLK_CLKSEL2_SPI1SEL_Pos)
/** Select PLL output as SPI1 clock source */
#define NUMAKER_CLK_CLKSEL2_SPI1SEL_PLL   (0x1UL << NUMAKER_CLK_CLKSEL2_SPI1SEL_Pos)
/** Select HIRC (internal high-speed RC oscillator) as SPI1 clock source */
#define NUMAKER_CLK_CLKSEL2_SPI1SEL_HIRC  (0x3UL << NUMAKER_CLK_CLKSEL2_SPI1SEL_Pos)
/** Select PCLK0 (APB0 peripheral clock) as SPI1 clock source */
#define NUMAKER_CLK_CLKSEL2_SPI1SEL_PCLK0 (0x2UL << NUMAKER_CLK_CLKSEL2_SPI1SEL_Pos)


/** @} */

/**
 * @name EPWM0 clock source selection
 * @{
 */
/** Select PLL output as EPWM0 clock source */
#define NUMAKER_CLK_CLKSEL2_EPWM0SEL_PLL   (0x0UL << NUMAKER_CLK_CLKSEL2_EPWM0SEL_Pos)
/** Select PCLK0 (APB0 peripheral clock) as EPWM0 clock source */
#define NUMAKER_CLK_CLKSEL2_EPWM0SEL_PCLK0 (0x1UL << NUMAKER_CLK_CLKSEL2_EPWM0SEL_Pos)


/** @} */

/**
 * @name EPWM1 clock source selection
 * @{
 */
/** Select PLL output as EPWM1 clock source */
#define NUMAKER_CLK_CLKSEL2_EPWM1SEL_PLL   (0x0UL << NUMAKER_CLK_CLKSEL2_EPWM1SEL_Pos)
/** Select PCLK1 (APB1 peripheral clock) as EPWM1 clock source */
#define NUMAKER_CLK_CLKSEL2_EPWM1SEL_PCLK1 (0x1UL << NUMAKER_CLK_CLKSEL2_EPWM1SEL_Pos)


/** @} */

/**
 * @name BPWM0 clock source selection
 * @{
 */
/** Select PLL output as BPWM0 clock source */
#define NUMAKER_CLK_CLKSEL2_BPWM0SEL_PLL   (0x0UL << NUMAKER_CLK_CLKSEL2_BPWM0SEL_Pos)
/** Select PCLK0 (APB0 peripheral clock) as BPWM0 clock source */
#define NUMAKER_CLK_CLKSEL2_BPWM0SEL_PCLK0 (0x1UL << NUMAKER_CLK_CLKSEL2_BPWM0SEL_Pos)


/** @} */

/**
 * @name BPWM1 clock source selection
 * @{
 */
/** Select PLL output as BPWM1 clock source */
#define NUMAKER_CLK_CLKSEL2_BPWM1SEL_PLL   (0x0UL << NUMAKER_CLK_CLKSEL2_BPWM1SEL_Pos)
/** Select PCLK1 (APB1 peripheral clock) as BPWM1 clock source */
#define NUMAKER_CLK_CLKSEL2_BPWM1SEL_PCLK1 (0x1UL << NUMAKER_CLK_CLKSEL2_BPWM1SEL_Pos)


/** @} */

/**
 * @name SPI2 clock source selection
 * @{
 */
/** Select HXT (external high-speed crystal) as SPI2 clock source */
#define NUMAKER_CLK_CLKSEL2_SPI2SEL_HXT   (0x0UL << NUMAKER_CLK_CLKSEL2_SPI2SEL_Pos)
/** Select PLL output as SPI2 clock source */
#define NUMAKER_CLK_CLKSEL2_SPI2SEL_PLL   (0x1UL << NUMAKER_CLK_CLKSEL2_SPI2SEL_Pos)
/** Select HIRC (internal high-speed RC oscillator) as SPI2 clock source */
#define NUMAKER_CLK_CLKSEL2_SPI2SEL_HIRC  (0x3UL << NUMAKER_CLK_CLKSEL2_SPI2SEL_Pos)
/** Select PCLK1 (APB1 peripheral clock) as SPI2 clock source */
#define NUMAKER_CLK_CLKSEL2_SPI2SEL_PCLK1 (0x2UL << NUMAKER_CLK_CLKSEL2_SPI2SEL_Pos)


/** @} */

/**
 * @name SPI3 clock source selection
 * @{
 */
/** Select HXT (external high-speed crystal) as SPI3 clock source */
#define NUMAKER_CLK_CLKSEL2_SPI3SEL_HXT   (0x0UL << NUMAKER_CLK_CLKSEL2_SPI3SEL_Pos)
/** Select PLL output as SPI3 clock source */
#define NUMAKER_CLK_CLKSEL2_SPI3SEL_PLL   (0x1UL << NUMAKER_CLK_CLKSEL2_SPI3SEL_Pos)
/** Select HIRC (internal high-speed RC oscillator) as SPI3 clock source */
#define NUMAKER_CLK_CLKSEL2_SPI3SEL_HIRC  (0x3UL << NUMAKER_CLK_CLKSEL2_SPI3SEL_Pos)
/** Select PCLK0 (APB0 peripheral clock) as SPI3 clock source */
#define NUMAKER_CLK_CLKSEL2_SPI3SEL_PCLK0 (0x2UL << NUMAKER_CLK_CLKSEL2_SPI3SEL_Pos)


/** @} */

/**
 * @name smart card 0 clock source selection
 * @{
 */
/** Select HXT (external high-speed crystal) as smart card 0 clock source */
#define NUMAKER_CLK_CLKSEL3_SC0SEL_HXT   (0x0UL << NUMAKER_CLK_CLKSEL3_SC0SEL_Pos)
/** Select PLL output as smart card 0 clock source */
#define NUMAKER_CLK_CLKSEL3_SC0SEL_PLL   (0x1UL << NUMAKER_CLK_CLKSEL3_SC0SEL_Pos)
/** Select HIRC (internal high-speed RC oscillator) as smart card 0 clock source */
#define NUMAKER_CLK_CLKSEL3_SC0SEL_HIRC  (0x3UL << NUMAKER_CLK_CLKSEL3_SC0SEL_Pos)
/** Select PCLK0 (APB0 peripheral clock) as smart card 0 clock source */
#define NUMAKER_CLK_CLKSEL3_SC0SEL_PCLK0 (0x2UL << NUMAKER_CLK_CLKSEL3_SC0SEL_Pos)


/** @} */

/**
 * @name smart card 1 clock source selection
 * @{
 */
/** Select HXT (external high-speed crystal) as smart card 1 clock source */
#define NUMAKER_CLK_CLKSEL3_SC1SEL_HXT   (0x0UL << NUMAKER_CLK_CLKSEL3_SC1SEL_Pos)
/** Select PLL output as smart card 1 clock source */
#define NUMAKER_CLK_CLKSEL3_SC1SEL_PLL   (0x1UL << NUMAKER_CLK_CLKSEL3_SC1SEL_Pos)
/** Select HIRC (internal high-speed RC oscillator) as smart card 1 clock source */
#define NUMAKER_CLK_CLKSEL3_SC1SEL_HIRC  (0x3UL << NUMAKER_CLK_CLKSEL3_SC1SEL_Pos)
/** Select PCLK1 (APB1 peripheral clock) as smart card 1 clock source */
#define NUMAKER_CLK_CLKSEL3_SC1SEL_PCLK1 (0x2UL << NUMAKER_CLK_CLKSEL3_SC1SEL_Pos)


/** @} */

/**
 * @name smart card 2 clock source selection
 * @{
 */
/** Select HXT (external high-speed crystal) as smart card 2 clock source */
#define NUMAKER_CLK_CLKSEL3_SC2SEL_HXT   (0x0UL << NUMAKER_CLK_CLKSEL3_SC2SEL_Pos)
/** Select PLL output as smart card 2 clock source */
#define NUMAKER_CLK_CLKSEL3_SC2SEL_PLL   (0x1UL << NUMAKER_CLK_CLKSEL3_SC2SEL_Pos)
/** Select HIRC (internal high-speed RC oscillator) as smart card 2 clock source */
#define NUMAKER_CLK_CLKSEL3_SC2SEL_HIRC  (0x3UL << NUMAKER_CLK_CLKSEL3_SC2SEL_Pos)
/** Select PCLK0 (APB0 peripheral clock) as smart card 2 clock source */
#define NUMAKER_CLK_CLKSEL3_SC2SEL_PCLK0 (0x2UL << NUMAKER_CLK_CLKSEL3_SC2SEL_Pos)


/** @} */

/**
 * @name RTC clock source selection
 * @{
 */
/** Select LXT (external low-speed crystal) as RTC clock source */
#define NUMAKER_CLK_CLKSEL3_RTCSEL_LXT  (0x0UL << NUMAKER_CLK_CLKSEL3_RTCSEL_Pos)
/** Select LIRC (internal low-speed RC oscillator) as RTC clock source */
#define NUMAKER_CLK_CLKSEL3_RTCSEL_LIRC (0x1UL << NUMAKER_CLK_CLKSEL3_RTCSEL_Pos)


/** @} */

/**
 * @name QSPI1 clock source selection
 * @{
 */
/** Select HXT (external high-speed crystal) as QSPI1 clock source */
#define NUMAKER_CLK_CLKSEL3_QSPI1SEL_HXT   (0x0UL << NUMAKER_CLK_CLKSEL3_QSPI1SEL_Pos)
/** Select PLL output as QSPI1 clock source */
#define NUMAKER_CLK_CLKSEL3_QSPI1SEL_PLL   (0x1UL << NUMAKER_CLK_CLKSEL3_QSPI1SEL_Pos)
/** Select HIRC (internal high-speed RC oscillator) as QSPI1 clock source */
#define NUMAKER_CLK_CLKSEL3_QSPI1SEL_HIRC  (0x3UL << NUMAKER_CLK_CLKSEL3_QSPI1SEL_Pos)
/** Select PCLK1 (APB1 peripheral clock) as QSPI1 clock source */
#define NUMAKER_CLK_CLKSEL3_QSPI1SEL_PCLK1 (0x2UL << NUMAKER_CLK_CLKSEL3_QSPI1SEL_Pos)


/** @} */

/**
 * @name I2S0 clock source selection
 * @{
 */
/** Select HXT (external high-speed crystal) as I2S0 clock source */
#define NUMAKER_CLK_CLKSEL3_I2S0SEL_HXT   (0x0UL << NUMAKER_CLK_CLKSEL3_I2S0SEL_Pos)
/** Select PLL output as I2S0 clock source */
#define NUMAKER_CLK_CLKSEL3_I2S0SEL_PLL   (0x1UL << NUMAKER_CLK_CLKSEL3_I2S0SEL_Pos)
/** Select HIRC (internal high-speed RC oscillator) as I2S0 clock source */
#define NUMAKER_CLK_CLKSEL3_I2S0SEL_HIRC  (0x3UL << NUMAKER_CLK_CLKSEL3_I2S0SEL_Pos)
/** Select PCLK0 (APB0 peripheral clock) as I2S0 clock source */
#define NUMAKER_CLK_CLKSEL3_I2S0SEL_PCLK0 (0x2UL << NUMAKER_CLK_CLKSEL3_I2S0SEL_Pos)


/** @} */

/**
 * @name UART2 clock source selection
 * @{
 */
/** Select HXT (external high-speed crystal) as UART2 clock source */
#define NUMAKER_CLK_CLKSEL3_UART2SEL_HXT  (0x0UL << NUMAKER_CLK_CLKSEL3_UART2SEL_Pos)
/** Select LXT (external low-speed crystal) as UART2 clock source */
#define NUMAKER_CLK_CLKSEL3_UART2SEL_LXT  (0x2UL << NUMAKER_CLK_CLKSEL3_UART2SEL_Pos)
/** Select PLL output as UART2 clock source */
#define NUMAKER_CLK_CLKSEL3_UART2SEL_PLL  (0x1UL << NUMAKER_CLK_CLKSEL3_UART2SEL_Pos)
/** Select HIRC (internal high-speed RC oscillator) as UART2 clock source */
#define NUMAKER_CLK_CLKSEL3_UART2SEL_HIRC (0x3UL << NUMAKER_CLK_CLKSEL3_UART2SEL_Pos)


/** @} */

/**
 * @name UART3 clock source selection
 * @{
 */
/** Select HXT (external high-speed crystal) as UART3 clock source */
#define NUMAKER_CLK_CLKSEL3_UART3SEL_HXT  (0x0UL << NUMAKER_CLK_CLKSEL3_UART3SEL_Pos)
/** Select LXT (external low-speed crystal) as UART3 clock source */
#define NUMAKER_CLK_CLKSEL3_UART3SEL_LXT  (0x2UL << NUMAKER_CLK_CLKSEL3_UART3SEL_Pos)
/** Select PLL output as UART3 clock source */
#define NUMAKER_CLK_CLKSEL3_UART3SEL_PLL  (0x1UL << NUMAKER_CLK_CLKSEL3_UART3SEL_Pos)
/** Select HIRC (internal high-speed RC oscillator) as UART3 clock source */
#define NUMAKER_CLK_CLKSEL3_UART3SEL_HIRC (0x3UL << NUMAKER_CLK_CLKSEL3_UART3SEL_Pos)


/** @} */

/**
 * @name UART4 clock source selection
 * @{
 */
/** Select HXT (external high-speed crystal) as UART4 clock source */
#define NUMAKER_CLK_CLKSEL3_UART4SEL_HXT  (0x0UL << NUMAKER_CLK_CLKSEL3_UART4SEL_Pos)
/** Select LXT (external low-speed crystal) as UART4 clock source */
#define NUMAKER_CLK_CLKSEL3_UART4SEL_LXT  (0x2UL << NUMAKER_CLK_CLKSEL3_UART4SEL_Pos)
/** Select PLL output as UART4 clock source */
#define NUMAKER_CLK_CLKSEL3_UART4SEL_PLL  (0x1UL << NUMAKER_CLK_CLKSEL3_UART4SEL_Pos)
/** Select HIRC (internal high-speed RC oscillator) as UART4 clock source */
#define NUMAKER_CLK_CLKSEL3_UART4SEL_HIRC (0x3UL << NUMAKER_CLK_CLKSEL3_UART4SEL_Pos)


/** @} */

/**
 * @name UART5 clock source selection
 * @{
 */
/** Select HXT (external high-speed crystal) as UART5 clock source */
#define NUMAKER_CLK_CLKSEL3_UART5SEL_HXT  (0x0UL << NUMAKER_CLK_CLKSEL3_UART5SEL_Pos)
/** Select LXT (external low-speed crystal) as UART5 clock source */
#define NUMAKER_CLK_CLKSEL3_UART5SEL_LXT  (0x2UL << NUMAKER_CLK_CLKSEL3_UART5SEL_Pos)
/** Select PLL output as UART5 clock source */
#define NUMAKER_CLK_CLKSEL3_UART5SEL_PLL  (0x1UL << NUMAKER_CLK_CLKSEL3_UART5SEL_Pos)
/** Select HIRC (internal high-speed RC oscillator) as UART5 clock source */
#define NUMAKER_CLK_CLKSEL3_UART5SEL_HIRC (0x3UL << NUMAKER_CLK_CLKSEL3_UART5SEL_Pos)


/** @} */

/**
 * @name UART6 clock source selection
 * @{
 */
/** Select HXT (external high-speed crystal) as UART6 clock source */
#define NUMAKER_CLK_CLKSEL3_UART6SEL_HXT  (0x0UL << NUMAKER_CLK_CLKSEL3_UART6SEL_Pos)
/** Select LXT (external low-speed crystal) as UART6 clock source */
#define NUMAKER_CLK_CLKSEL3_UART6SEL_LXT  (0x2UL << NUMAKER_CLK_CLKSEL3_UART6SEL_Pos)
/** Select PLL output as UART6 clock source */
#define NUMAKER_CLK_CLKSEL3_UART6SEL_PLL  (0x1UL << NUMAKER_CLK_CLKSEL3_UART6SEL_Pos)
/** Select HIRC (internal high-speed RC oscillator) as UART6 clock source */
#define NUMAKER_CLK_CLKSEL3_UART6SEL_HIRC (0x3UL << NUMAKER_CLK_CLKSEL3_UART6SEL_Pos)


/** @} */

/**
 * @name UART7 clock source selection
 * @{
 */
/** Select HXT (external high-speed crystal) as UART7 clock source */
#define NUMAKER_CLK_CLKSEL3_UART7SEL_HXT  (0x0UL << NUMAKER_CLK_CLKSEL3_UART7SEL_Pos)
/** Select LXT (external low-speed crystal) as UART7 clock source */
#define NUMAKER_CLK_CLKSEL3_UART7SEL_LXT  (0x2UL << NUMAKER_CLK_CLKSEL3_UART7SEL_Pos)
/** Select PLL output as UART7 clock source */
#define NUMAKER_CLK_CLKSEL3_UART7SEL_PLL  (0x1UL << NUMAKER_CLK_CLKSEL3_UART7SEL_Pos)
/** Select HIRC (internal high-speed RC oscillator) as UART7 clock source */
#define NUMAKER_CLK_CLKSEL3_UART7SEL_HIRC (0x3UL << NUMAKER_CLK_CLKSEL3_UART7SEL_Pos)


/** @} */

/**
 * @name CLKDIV0 clock divider macros
 * @{
 */
/** HCLK clock divider. x is the divider value (1..16) */
#define NUMAKER_CLK_CLKDIV0_HCLK(x)  (((x) - 1UL) << NUMAKER_CLK_CLKDIV0_HCLKDIV_Pos)
/** USB clock divider. x is the divider value (1..16) */
#define NUMAKER_CLK_CLKDIV0_USB(x)   (((x) - 1UL) << NUMAKER_CLK_CLKDIV0_USBDIV_Pos)
/** SD host 0 clock divider. x is the divider value (1..256) */
#define NUMAKER_CLK_CLKDIV0_SDH0(x)  (((x) - 1UL) << NUMAKER_CLK_CLKDIV0_SDH0DIV_Pos)
/** UART0 clock divider. x is the divider value (1..16) */
#define NUMAKER_CLK_CLKDIV0_UART0(x) (((x) - 1UL) << NUMAKER_CLK_CLKDIV0_UART0DIV_Pos)
/** UART1 clock divider. x is the divider value (1..16) */
#define NUMAKER_CLK_CLKDIV0_UART1(x) (((x) - 1UL) << NUMAKER_CLK_CLKDIV0_UART1DIV_Pos)
/** EADC clock divider. x is the divider value (1..256) */
#define NUMAKER_CLK_CLKDIV0_EADC(x)  (((x) - 1UL) << NUMAKER_CLK_CLKDIV0_EADCDIV_Pos)


/** @} */

/**
 * @name CLKDIV1 clock divider macros
 * @{
 */
/** Smart card 0 clock divider. x is the divider value (1..256) */
#define NUMAKER_CLK_CLKDIV1_SC0(x) (((x) - 1UL) << NUMAKER_CLK_CLKDIV1_SC0DIV_Pos)
/** Smart card 1 clock divider. x is the divider value (1..256) */
#define NUMAKER_CLK_CLKDIV1_SC1(x) (((x) - 1UL) << NUMAKER_CLK_CLKDIV1_SC1DIV_Pos)
/** Smart card 2 clock divider. x is the divider value (1..256) */
#define NUMAKER_CLK_CLKDIV1_SC2(x) (((x) - 1UL) << NUMAKER_CLK_CLKDIV1_SC2DIV_Pos)


/** @} */

/**
 * @name CLKDIV2 clock divider macros
 * @{
 */
/** I2S0 clock divider. x is the divider value (1..16) */
#define NUMAKER_CLK_CLKDIV2_I2S0(x)  (((x) - 1UL) << NUMAKER_CLK_CLKDIV2_I2S0DIV_Pos)
/** EADC1 clock divider. x is the divider value (1..256) */
#define NUMAKER_CLK_CLKDIV2_EADC1(x) (((x) - 1UL) << NUMAKER_CLK_CLKDIV2_EADC1DIV_Pos)


/** @} */

/**
 * @name CLKDIV3 clock divider macros
 * @{
 */
/** Camera capture clock divider. x is the divider value (1..256) */
#define NUMAKER_CLK_CLKDIV3_CCAP(x)   (((x) - 1UL) << NUMAKER_CLK_CLKDIV3_CCAPDIV_Pos)
/** Video sensor clock divider. x is the divider value (1..256) */
#define NUMAKER_CLK_CLKDIV3_VSENSE(x) (((x) - 1UL) << NUMAKER_CLK_CLKDIV3_VSENSEDIV_Pos)
/** Ethernet MAC clock divider. x is the divider value (1..256) */
#define NUMAKER_CLK_CLKDIV3_EMAC(x)   (((x) - 1UL) << NUMAKER_CLK_CLKDIV3_EMACDIV_Pos)
/** SD host 1 clock divider. x is the divider value (1..256) */
#define NUMAKER_CLK_CLKDIV3_SDH1(x)   (((x) - 1UL) << NUMAKER_CLK_CLKDIV3_SDH1DIV_Pos)


/** @} */

/**
 * @name CLKDIV4 clock divider macros
 * @{
 */
/** UART2 clock divider. x is the divider value (1..16) */
#define NUMAKER_CLK_CLKDIV4_UART2(x) (((x) - 1UL) << NUMAKER_CLK_CLKDIV4_UART2DIV_Pos)
/** UART3 clock divider. x is the divider value (1..16) */
#define NUMAKER_CLK_CLKDIV4_UART3(x) (((x) - 1UL) << NUMAKER_CLK_CLKDIV4_UART3DIV_Pos)
/** UART4 clock divider. x is the divider value (1..16) */
#define NUMAKER_CLK_CLKDIV4_UART4(x) (((x) - 1UL) << NUMAKER_CLK_CLKDIV4_UART4DIV_Pos)
/** UART5 clock divider. x is the divider value (1..16) */
#define NUMAKER_CLK_CLKDIV4_UART5(x) (((x) - 1UL) << NUMAKER_CLK_CLKDIV4_UART5DIV_Pos)
/** UART6 clock divider. x is the divider value (1..16) */
#define NUMAKER_CLK_CLKDIV4_UART6(x) (((x) - 1UL) << NUMAKER_CLK_CLKDIV4_UART6DIV_Pos)
/** UART7 clock divider. x is the divider value (1..16) */
#define NUMAKER_CLK_CLKDIV4_UART7(x) (((x) - 1UL) << NUMAKER_CLK_CLKDIV4_UART7DIV_Pos)


/** @} */

/**
 * @name PCLKDIV peripheral clock divider selection
 * @{
 */
/** Set PCLK0 to HCLK/1 */
#define NUMAKER_CLK_PCLKDIV_PCLK0DIV1     (0x0UL << NUMAKER_CLK_PCLKDIV_APB0DIV_Pos)
/** Set PCLK0 to HCLK/2 */
#define NUMAKER_CLK_PCLKDIV_PCLK0DIV2     (0x1UL << NUMAKER_CLK_PCLKDIV_APB0DIV_Pos)
/** Set PCLK0 to HCLK/4 */
#define NUMAKER_CLK_PCLKDIV_PCLK0DIV4     (0x2UL << NUMAKER_CLK_PCLKDIV_APB0DIV_Pos)
/** Set PCLK0 to HCLK/8 */
#define NUMAKER_CLK_PCLKDIV_PCLK0DIV8     (0x3UL << NUMAKER_CLK_PCLKDIV_APB0DIV_Pos)
/** Set PCLK0 to HCLK/16 */
#define NUMAKER_CLK_PCLKDIV_PCLK0DIV16    (0x4UL << NUMAKER_CLK_PCLKDIV_APB0DIV_Pos)
/** Set PCLK1 to HCLK/1 */
#define NUMAKER_CLK_PCLKDIV_PCLK1DIV1     (0x0UL << NUMAKER_CLK_PCLKDIV_APB1DIV_Pos)
/** Set PCLK1 to HCLK/2 */
#define NUMAKER_CLK_PCLKDIV_PCLK1DIV2     (0x1UL << NUMAKER_CLK_PCLKDIV_APB1DIV_Pos)
/** Set PCLK1 to HCLK/4 */
#define NUMAKER_CLK_PCLKDIV_PCLK1DIV4     (0x2UL << NUMAKER_CLK_PCLKDIV_APB1DIV_Pos)
/** Set PCLK1 to HCLK/8 */
#define NUMAKER_CLK_PCLKDIV_PCLK1DIV8     (0x3UL << NUMAKER_CLK_PCLKDIV_APB1DIV_Pos)
/** Set PCLK1 to HCLK/16 */
#define NUMAKER_CLK_PCLKDIV_PCLK1DIV16    (0x4UL << NUMAKER_CLK_PCLKDIV_APB1DIV_Pos)
/** Set APB0 clock to HCLK/1 */
#define NUMAKER_CLK_PCLKDIV_APB0DIV_DIV1  (0x0UL << NUMAKER_CLK_PCLKDIV_APB0DIV_Pos)
/** Set APB0 clock to HCLK/2 */
#define NUMAKER_CLK_PCLKDIV_APB0DIV_DIV2  (0x1UL << NUMAKER_CLK_PCLKDIV_APB0DIV_Pos)
/** Set APB0 clock to HCLK/4 */
#define NUMAKER_CLK_PCLKDIV_APB0DIV_DIV4  (0x2UL << NUMAKER_CLK_PCLKDIV_APB0DIV_Pos)
/** Set APB0 clock to HCLK/8 */
#define NUMAKER_CLK_PCLKDIV_APB0DIV_DIV8  (0x3UL << NUMAKER_CLK_PCLKDIV_APB0DIV_Pos)
/** Set APB0 clock to HCLK/16 */
#define NUMAKER_CLK_PCLKDIV_APB0DIV_DIV16 (0x4UL << NUMAKER_CLK_PCLKDIV_APB0DIV_Pos)
/** Set APB1 clock to HCLK/1 */
#define NUMAKER_CLK_PCLKDIV_APB1DIV_DIV1  (0x0UL << NUMAKER_CLK_PCLKDIV_APB1DIV_Pos)
/** Set APB1 clock to HCLK/2 */
#define NUMAKER_CLK_PCLKDIV_APB1DIV_DIV2  (0x1UL << NUMAKER_CLK_PCLKDIV_APB1DIV_Pos)
/** Set APB1 clock to HCLK/4 */
#define NUMAKER_CLK_PCLKDIV_APB1DIV_DIV4  (0x2UL << NUMAKER_CLK_PCLKDIV_APB1DIV_Pos)
/** Set APB1 clock to HCLK/8 */
#define NUMAKER_CLK_PCLKDIV_APB1DIV_DIV8  (0x3UL << NUMAKER_CLK_PCLKDIV_APB1DIV_Pos)
/** Set APB1 clock to HCLK/16 */
#define NUMAKER_CLK_PCLKDIV_APB1DIV_DIV16 (0x4UL << NUMAKER_CLK_PCLKDIV_APB1DIV_Pos)


/** @} */

/** @cond INTERNAL_HIDDEN */

/*  MODULE constant definitions. */

#define NUMAKER_MODULE_NoMsk 0x0UL
#define NUMAKER_NA           NUMAKER_MODULE_NoMsk

#define NUMAKER_MODULE_APBCLK_ENC(x)     (((x) & 0x03UL) << 30)
#define NUMAKER_MODULE_CLKSEL_ENC(x)     (((x) & 0x03UL) << 28)
#define NUMAKER_MODULE_CLKSEL_Msk_ENC(x) (((x) & 0x07UL) << 25)
#define NUMAKER_MODULE_CLKSEL_Pos_ENC(x) (((x) & 0x1fUL) << 20)
#define NUMAKER_MODULE_CLKDIV_ENC(x)     (((x) & 0x03UL) << 18)
#define NUMAKER_MODULE_CLKDIV_Msk_ENC(x) (((x) & 0xffUL) << 10)
#define NUMAKER_MODULE_CLKDIV_Pos_ENC(x) (((x) & 0x1fUL) << 5)
#define NUMAKER_MODULE_IP_EN_Pos_ENC(x)  (((x) & 0x1fUL) << 0)


/** @endcond */

/**
 * @name Peripheral module clock configuration
 * @{
 */
/** PDMA controller module clock configuration */
#define NUMAKER_PDMA_MODULE                                                                    \
	((0UL << 30) | (NUMAKER_MODULE_NoMsk << 28) | (NUMAKER_MODULE_NoMsk << 25) |               \
	 (NUMAKER_MODULE_NoMsk << 20) | (NUMAKER_MODULE_NoMsk << 18) |                             \
	 (NUMAKER_MODULE_NoMsk << 10) | (NUMAKER_MODULE_NoMsk << 5) | (1UL << 0))
/** Flash ISP controller module clock configuration */
#define NUMAKER_ISP_MODULE                                                                     \
	((0UL << 30) | (NUMAKER_MODULE_NoMsk << 28) | (NUMAKER_MODULE_NoMsk << 25) |               \
	 (NUMAKER_MODULE_NoMsk << 20) | (NUMAKER_MODULE_NoMsk << 18) |                             \
	 (NUMAKER_MODULE_NoMsk << 10) | (NUMAKER_MODULE_NoMsk << 5) | (2UL << 0))
/** External bus interface module clock configuration */
#define NUMAKER_EBI_MODULE                                                                     \
	((0UL << 30) | (NUMAKER_MODULE_NoMsk << 28) | (NUMAKER_MODULE_NoMsk << 25) |               \
	 (NUMAKER_MODULE_NoMsk << 20) | (NUMAKER_MODULE_NoMsk << 18) |                             \
	 (NUMAKER_MODULE_NoMsk << 10) | (NUMAKER_MODULE_NoMsk << 5) | (3UL << 0))
/** USB host controller module clock configuration */
#define NUMAKER_USBH_MODULE                                                                    \
	((0UL << 30) | (0UL << 28) | (0x1UL << 25) | (8UL << 20) | (0UL << 18) | (0xFUL << 10) |   \
	 (4UL << 5) | (16UL << 0))
/** Ethernet MAC controller module clock configuration */
#define NUMAKER_EMAC_MODULE                                                                    \
	((0UL << 30) | (NUMAKER_MODULE_NoMsk << 28) | (NUMAKER_MODULE_NoMsk << 25) |               \
	 (NUMAKER_MODULE_NoMsk << 20) | (2UL << 18) | (0xFFUL << 10) | (16UL << 5) | (5UL << 0))
/** SD host 0 controller module clock configuration */
#define NUMAKER_SDH0_MODULE                                                                    \
	((0UL << 30) | (0UL << 28) | (0x3UL << 25) | (20UL << 20) | (0UL << 18) | (0xFFUL << 10) | \
	 (24UL << 5) | (6UL << 0))
/** CRC generator module clock configuration */
#define NUMAKER_CRC_MODULE                                                                     \
	((0UL << 30) | (NUMAKER_MODULE_NoMsk << 28) | (NUMAKER_MODULE_NoMsk << 25) |               \
	 (NUMAKER_MODULE_NoMsk << 20) | (NUMAKER_MODULE_NoMsk << 18) |                             \
	 (NUMAKER_MODULE_NoMsk << 10) | (NUMAKER_MODULE_NoMsk << 5) | (7UL << 0))
/** Camera capture interface module clock configuration */
#define NUMAKER_CCAP_MODULE                                                                    \
	((0UL << 30) | (0UL << 28) | (0x3UL << 25) | (16UL << 20) | (2UL << 18) | (0xFFUL << 10) | \
	 (0UL << 5) | (8UL << 0))
/** Sensor interface module clock configuration */
#define NUMAKER_SEN_MODULE                                                                     \
	((0UL << 30) | (NUMAKER_MODULE_NoMsk << 28) | (NUMAKER_MODULE_NoMsk << 25) |               \
	 (NUMAKER_MODULE_NoMsk << 20) | (NUMAKER_MODULE_NoMsk << 18) |                             \
	 (NUMAKER_MODULE_NoMsk << 10) | (NUMAKER_MODULE_NoMsk << 5) | (9UL << 0))
/** High-speed USB device controller module clock configuration */
#define NUMAKER_HSUSBD_MODULE                                                                  \
	((0UL << 30) | (NUMAKER_MODULE_NoMsk << 28) | (NUMAKER_MODULE_NoMsk << 25) |               \
	 (NUMAKER_MODULE_NoMsk << 20) | (NUMAKER_MODULE_NoMsk << 18) |                             \
	 (NUMAKER_MODULE_NoMsk << 10) | (NUMAKER_MODULE_NoMsk << 5) | (10UL << 0))
/** Cryptographic accelerator module clock configuration */
#define NUMAKER_CRPT_MODULE                                                                    \
	((0UL << 30) | (NUMAKER_MODULE_NoMsk << 28) | (NUMAKER_MODULE_NoMsk << 25) |               \
	 (NUMAKER_MODULE_NoMsk << 20) | (NUMAKER_MODULE_NoMsk << 18) |                             \
	 (NUMAKER_MODULE_NoMsk << 10) | (NUMAKER_MODULE_NoMsk << 5) | (12UL << 0))
/** SPI flash memory controller module clock configuration */
#define NUMAKER_SPIM_MODULE                                                                    \
	((0UL << 30) | (NUMAKER_MODULE_NoMsk << 28) | (NUMAKER_MODULE_NoMsk << 25) |               \
	 (NUMAKER_MODULE_NoMsk << 20) | (NUMAKER_MODULE_NoMsk << 18) |                             \
	 (NUMAKER_MODULE_NoMsk << 10) | (NUMAKER_MODULE_NoMsk << 5) | (14UL << 0))
/** Flash memory controller idle module clock configuration */
#define NUMAKER_FMCIDLE_MODULE                                                                 \
	((0UL << 30) | (NUMAKER_MODULE_NoMsk << 28) | (NUMAKER_MODULE_NoMsk << 25) |               \
	 (NUMAKER_MODULE_NoMsk << 20) | (NUMAKER_MODULE_NoMsk << 18) |                             \
	 (NUMAKER_MODULE_NoMsk << 10) | (NUMAKER_MODULE_NoMsk << 5) | (15UL << 0))
/** SD host 1 controller module clock configuration */
#define NUMAKER_SDH1_MODULE                                                                    \
	((0UL << 30) | (0UL << 28) | (0x3UL << 25) | (22UL << 20) | (2UL << 18) | (0xFFUL << 10) | \
	 (24UL << 5) | (17UL << 0))
/** Watchdog timer module clock configuration */
#define NUMAKER_WDT_MODULE                                                                     \
	((1UL << 30) | (1UL << 28) | (0x3UL << 25) | (0UL << 20) | (NUMAKER_MODULE_NoMsk << 18) |  \
	 (NUMAKER_MODULE_NoMsk << 10) | (NUMAKER_MODULE_NoMsk << 5) | (0UL << 0))
/** Real-time clock module clock configuration */
#define NUMAKER_RTC_MODULE                                                                     \
	((1UL << 30) | (3UL << 28) | (0x1UL << 25) | (8UL << 20) | (NUMAKER_MODULE_NoMsk << 18) |  \
	 (NUMAKER_MODULE_NoMsk << 10) | (NUMAKER_MODULE_NoMsk << 5) | (1UL << 0))
/** Timer 0 module clock configuration */
#define NUMAKER_TMR0_MODULE                                                                    \
	((1UL << 30) | (1UL << 28) | (0x7UL << 25) | (8UL << 20) | (NUMAKER_MODULE_NoMsk << 18) |  \
	 (NUMAKER_MODULE_NoMsk << 10) | (NUMAKER_MODULE_NoMsk << 5) | (2UL << 0))
/** Timer 1 module clock configuration */
#define NUMAKER_TMR1_MODULE                                                                    \
	((1UL << 30) | (1UL << 28) | (0x7UL << 25) | (12UL << 20) | (NUMAKER_MODULE_NoMsk << 18) | \
	 (NUMAKER_MODULE_NoMsk << 10) | (NUMAKER_MODULE_NoMsk << 5) | (3UL << 0))
/** Timer 2 module clock configuration */
#define NUMAKER_TMR2_MODULE                                                                    \
	((1UL << 30) | (1UL << 28) | (0x7UL << 25) | (16UL << 20) | (NUMAKER_MODULE_NoMsk << 18) | \
	 (NUMAKER_MODULE_NoMsk << 10) | (NUMAKER_MODULE_NoMsk << 5) | (4UL << 0))
/** Timer 3 module clock configuration */
#define NUMAKER_TMR3_MODULE                                                                    \
	((1UL << 30) | (1UL << 28) | (0x7UL << 25) | (20UL << 20) | (NUMAKER_MODULE_NoMsk << 18) | \
	 (NUMAKER_MODULE_NoMsk << 10) | (NUMAKER_MODULE_NoMsk << 5) | (5UL << 0))
/** Clock frequency output module clock configuration */
#define NUMAKER_CLKO_MODULE                                                                    \
	((1UL << 30) | (1UL << 28) | (0x3UL << 25) | (28UL << 20) | (NUMAKER_MODULE_NoMsk << 18) | \
	 (NUMAKER_MODULE_NoMsk << 10) | (NUMAKER_MODULE_NoMsk << 5) | (6UL << 0))
/** Window watchdog timer module clock configuration */
#define NUMAKER_WWDT_MODULE                                                                    \
	((1UL << 30) | (1UL << 28) | (0x3UL << 25) | (30UL << 20) | (NUMAKER_MODULE_NoMsk << 18) | \
	 (NUMAKER_MODULE_NoMsk << 10) | (NUMAKER_MODULE_NoMsk << 5) | (0UL << 0))
/** Analog comparator 0/1 module clock configuration */
#define NUMAKER_ACMP01_MODULE                                                                  \
	((1UL << 30) | (NUMAKER_MODULE_NoMsk << 28) | (NUMAKER_MODULE_NoMsk << 25) |               \
	 (NUMAKER_MODULE_NoMsk << 20) | (NUMAKER_MODULE_NoMsk << 18) |                             \
	 (NUMAKER_MODULE_NoMsk << 10) | (NUMAKER_MODULE_NoMsk << 5) | (7UL << 0))
/** I2C0 module clock configuration */
#define NUMAKER_I2C0_MODULE                                                                    \
	((1UL << 30) | (NUMAKER_MODULE_NoMsk << 28) | (NUMAKER_MODULE_NoMsk << 25) |               \
	 (NUMAKER_MODULE_NoMsk << 20) | (NUMAKER_MODULE_NoMsk << 18) |                             \
	 (NUMAKER_MODULE_NoMsk << 10) | (NUMAKER_MODULE_NoMsk << 5) | (8UL << 0))
/** I2C1 module clock configuration */
#define NUMAKER_I2C1_MODULE                                                                    \
	((1UL << 30) | (NUMAKER_MODULE_NoMsk << 28) | (NUMAKER_MODULE_NoMsk << 25) |               \
	 (NUMAKER_MODULE_NoMsk << 20) | (NUMAKER_MODULE_NoMsk << 18) |                             \
	 (NUMAKER_MODULE_NoMsk << 10) | (NUMAKER_MODULE_NoMsk << 5) | (9UL << 0))
/** I2C2 module clock configuration */
#define NUMAKER_I2C2_MODULE                                                                    \
	((1UL << 30) | (NUMAKER_MODULE_NoMsk << 28) | (NUMAKER_MODULE_NoMsk << 25) |               \
	 (NUMAKER_MODULE_NoMsk << 20) | (NUMAKER_MODULE_NoMsk << 18) |                             \
	 (NUMAKER_MODULE_NoMsk << 10) | (NUMAKER_MODULE_NoMsk << 5) | (10UL << 0))
/** QSPI0 module clock configuration */
#define NUMAKER_QSPI0_MODULE                                                                   \
	((1UL << 30) | (2UL << 28) | (0x3UL << 25) | (2UL << 20) | (NUMAKER_MODULE_NoMsk << 18) |  \
	 (NUMAKER_MODULE_NoMsk << 10) | (NUMAKER_MODULE_NoMsk << 5) | (12UL << 0))
/** SPI0 module clock configuration */
#define NUMAKER_SPI0_MODULE                                                                    \
	((1UL << 30) | (2UL << 28) | (0x3UL << 25) | (4UL << 20) | (NUMAKER_MODULE_NoMsk << 18) |  \
	 (NUMAKER_MODULE_NoMsk << 10) | (NUMAKER_MODULE_NoMsk << 5) | (13UL << 0))
/** SPI1 module clock configuration */
#define NUMAKER_SPI1_MODULE                                                                    \
	((1UL << 30) | (2UL << 28) | (0x3UL << 25) | (6UL << 20) | (NUMAKER_MODULE_NoMsk << 18) |  \
	 (NUMAKER_MODULE_NoMsk << 10) | (NUMAKER_MODULE_NoMsk << 5) | (14UL << 0))
/** SPI2 module clock configuration */
#define NUMAKER_SPI2_MODULE                                                                    \
	((1UL << 30) | (2UL << 28) | (0x3UL << 25) | (10UL << 20) | (NUMAKER_MODULE_NoMsk << 18) | \
	 (NUMAKER_MODULE_NoMsk << 10) | (NUMAKER_MODULE_NoMsk << 5) | (15UL << 0))
/** UART0 module clock configuration */
#define NUMAKER_UART0_MODULE                                                                   \
	((1UL << 30) | (1UL << 28) | (0x3UL << 25) | (24UL << 20) | (0UL << 18) | (0xFUL << 10) |  \
	 (8UL << 5) | (16UL << 0))
/** UART1 module clock configuration */
#define NUMAKER_UART1_MODULE                                                                   \
	((1UL << 30) | (1UL << 28) | (0x3UL << 25) | (26UL << 20) | (0UL << 18) | (0xFUL << 10) |  \
	 (12UL << 5) | (17UL << 0))
/** UART2 module clock configuration */
#define NUMAKER_UART2_MODULE                                                                   \
	((1UL << 30) | (3UL << 28) | (0x3UL << 25) | (24UL << 20) | (3UL << 18) | (0xFUL << 10) |  \
	 (0UL << 5) | (18UL << 0))
/** UART3 module clock configuration */
#define NUMAKER_UART3_MODULE                                                                   \
	((1UL << 30) | (3UL << 28) | (0x3UL << 25) | (26UL << 20) | (3UL << 18) | (0xFUL << 10) |  \
	 (4UL << 5) | (19UL << 0))
/** UART4 module clock configuration */
#define NUMAKER_UART4_MODULE                                                                   \
	((1UL << 30) | (3UL << 28) | (0x3UL << 25) | (28UL << 20) | (3UL << 18) | (0xFUL << 10) |  \
	 (8UL << 5) | (20UL << 0))
/** UART5 module clock configuration */
#define NUMAKER_UART5_MODULE                                                                   \
	((1UL << 30) | (3UL << 28) | (0x3UL << 25) | (30UL << 20) | (3UL << 18) | (0xFUL << 10) |  \
	 (12UL << 5) | (21UL << 0))
/** UART6 module clock configuration */
#define NUMAKER_UART6_MODULE                                                                   \
	((1UL << 30) | (3UL << 28) | (0x3UL << 25) | (20UL << 20) | (3UL << 18) | (0xFUL << 10) |  \
	 (16UL << 5) | (22UL << 0))
/** UART7 module clock configuration */
#define NUMAKER_UART7_MODULE                                                                   \
	((1UL << 30) | (3UL << 28) | (0x3UL << 25) | (22UL << 20) | (3UL << 18) | (0xFUL << 10) |  \
	 (20UL << 5) | (23UL << 0))
/** CAN0 module clock configuration */
#define NUMAKER_CAN0_MODULE                                                                    \
	((1UL << 30) | (NUMAKER_MODULE_NoMsk << 28) | (NUMAKER_MODULE_NoMsk << 25) |               \
	 (NUMAKER_MODULE_NoMsk << 20) | (NUMAKER_MODULE_NoMsk << 18) |                             \
	 (NUMAKER_MODULE_NoMsk << 10) | (NUMAKER_MODULE_NoMsk << 5) | (24UL << 0))
/** CAN1 module clock configuration */
#define NUMAKER_CAN1_MODULE                                                                    \
	((1UL << 30) | (NUMAKER_MODULE_NoMsk << 28) | (NUMAKER_MODULE_NoMsk << 25) |               \
	 (NUMAKER_MODULE_NoMsk << 20) | (NUMAKER_MODULE_NoMsk << 18) |                             \
	 (NUMAKER_MODULE_NoMsk << 10) | (NUMAKER_MODULE_NoMsk << 5) | (25UL << 0))
/** USB OTG module clock configuration */
#define NUMAKER_OTG_MODULE                                                                     \
	((1UL << 30) | (NUMAKER_MODULE_NoMsk << 28) | (NUMAKER_MODULE_NoMsk << 25) |               \
	 (NUMAKER_MODULE_NoMsk << 20) | (NUMAKER_MODULE_NoMsk << 18) |                             \
	 (NUMAKER_MODULE_NoMsk << 10) | (NUMAKER_MODULE_NoMsk << 5) | (26UL << 0))
/** USB device controller module clock configuration */
#define NUMAKER_USBD_MODULE                                                                    \
	((1UL << 30) | (NUMAKER_MODULE_NoMsk << 28) | (1UL << 25) | (8UL << 20) |                  \
	 (NUMAKER_MODULE_NoMsk << 18) | (0xFUL << 10) | (4UL << 5) | (27UL << 0))
/** Enhanced ADC 0 module clock configuration */
#define NUMAKER_EADC_MODULE                                                                    \
	((1UL << 30) | (NUMAKER_MODULE_NoMsk << 28) | (NUMAKER_MODULE_NoMsk << 25) |               \
	 (NUMAKER_MODULE_NoMsk << 20) | (0UL << 18) | (0xFFUL << 10) | (16UL << 5) | (28UL << 0))
/** I2S0 audio interface module clock configuration */
#define NUMAKER_I2S0_MODULE                                                                    \
	((1UL << 30) | (3UL << 28) | (0x3UL << 25) | (16UL << 20) | (2UL << 18) | (0xFUL << 10) |  \
	 (0UL << 5) | (29UL << 0))
/** High-speed USB OTG module clock configuration */
#define NUMAKER_HSOTG_MODULE                                                                   \
	((1UL << 30) | (NUMAKER_MODULE_NoMsk << 28) | (NUMAKER_MODULE_NoMsk << 25) |               \
	 (NUMAKER_MODULE_NoMsk << 20) | (NUMAKER_MODULE_NoMsk << 18) |                             \
	 (NUMAKER_MODULE_NoMsk << 10) | (NUMAKER_MODULE_NoMsk << 5) | (30UL << 0))
/** Smart card 0 interface module clock configuration */
#define NUMAKER_SC0_MODULE                                                                     \
	((2UL << 30) | (3UL << 28) | (0x3UL << 25) | (0UL << 20) | (1UL << 18) | (0xFFUL << 10) |  \
	 (0UL << 5) | (0UL << 0))
/** Smart card 1 interface module clock configuration */
#define NUMAKER_SC1_MODULE                                                                     \
	((2UL << 30) | (3UL << 28) | (0x3UL << 25) | (2UL << 20) | (1UL << 18) | (0xFFUL << 10) |  \
	 (8UL << 5) | (1UL << 0))
/** Smart card 2 interface module clock configuration */
#define NUMAKER_SC2_MODULE                                                                     \
	((2UL << 30) | (3UL << 28) | (0x3UL << 25) | (4UL << 20) | (1UL << 18) | (0xFFUL << 10) |  \
	 (16UL << 5) | (2UL << 0))
/** QSPI1 module clock configuration */
#define NUMAKER_QSPI1_MODULE                                                                   \
	((2UL << 30) | (3UL << 28) | (0x3UL << 25) | (12UL << 20) | (NUMAKER_MODULE_NoMsk << 18) | \
	 (NUMAKER_MODULE_NoMsk << 10) | (NUMAKER_MODULE_NoMsk << 5) | (4UL << 0))
/** SPI3 module clock configuration */
#define NUMAKER_SPI3_MODULE                                                                    \
	((2UL << 30) | (2UL << 28) | (0x3UL << 25) | (12UL << 20) | (NUMAKER_MODULE_NoMsk << 18) | \
	 (NUMAKER_MODULE_NoMsk << 10) | (NUMAKER_MODULE_NoMsk << 5) | (6UL << 0))
/** USCI0 module clock configuration */
#define NUMAKER_USCI0_MODULE                                                                   \
	((2UL << 30) | (NUMAKER_MODULE_NoMsk << 28) | (NUMAKER_MODULE_NoMsk << 25) |               \
	 (NUMAKER_MODULE_NoMsk << 20) | (NUMAKER_MODULE_NoMsk << 18) |                             \
	 (NUMAKER_MODULE_NoMsk << 10) | (NUMAKER_MODULE_NoMsk << 5) | (8UL << 0))
/** USCI1 module clock configuration */
#define NUMAKER_USCI1_MODULE                                                                   \
	((2UL << 30) | (NUMAKER_MODULE_NoMsk << 28) | (NUMAKER_MODULE_NoMsk << 25) |               \
	 (NUMAKER_MODULE_NoMsk << 20) | (NUMAKER_MODULE_NoMsk << 18) |                             \
	 (NUMAKER_MODULE_NoMsk << 10) | (NUMAKER_MODULE_NoMsk << 5) | (9UL << 0))
/** DAC module clock configuration */
#define NUMAKER_DAC_MODULE                                                                     \
	((2UL << 30) | (NUMAKER_MODULE_NoMsk << 28) | (NUMAKER_MODULE_NoMsk << 25) |               \
	 (NUMAKER_MODULE_NoMsk << 20) | (NUMAKER_MODULE_NoMsk << 18) |                             \
	 (NUMAKER_MODULE_NoMsk << 10) | (NUMAKER_MODULE_NoMsk << 5) | (12UL << 0))
/** CAN2 module clock configuration */
#define NUMAKER_CAN2_MODULE                                                                    \
	((2UL << 30) | (NUMAKER_MODULE_NoMsk << 28) | (NUMAKER_MODULE_NoMsk << 25) |               \
	 (NUMAKER_MODULE_NoMsk << 20) | (NUMAKER_MODULE_NoMsk << 18) |                             \
	 (NUMAKER_MODULE_NoMsk << 10) | (NUMAKER_MODULE_NoMsk << 5) | (28UL << 0))
/** EPWM0 module clock configuration */
#define NUMAKER_EPWM0_MODULE                                                                   \
	((2UL << 30) | (2UL << 28) | (0x1UL << 25) | (0UL << 20) | (NUMAKER_MODULE_NoMsk << 18) |  \
	 (NUMAKER_MODULE_NoMsk << 10) | (NUMAKER_MODULE_NoMsk << 5) | (16UL << 0))
/** EPWM1 module clock configuration */
#define NUMAKER_EPWM1_MODULE                                                                   \
	((2UL << 30) | (2UL << 28) | (0x1UL << 25) | (1UL << 20) | (NUMAKER_MODULE_NoMsk << 18) |  \
	 (NUMAKER_MODULE_NoMsk << 10) | (NUMAKER_MODULE_NoMsk << 5) | (17UL << 0))
/** Basic PWM 0 module clock configuration */
#define NUMAKER_BPWM0_MODULE                                                                   \
	((2UL << 30) | (2UL << 28) | (0x1UL << 25) | (8UL << 20) | (NUMAKER_MODULE_NoMsk << 18) |  \
	 (NUMAKER_MODULE_NoMsk << 10) | (NUMAKER_MODULE_NoMsk << 5) | (18UL << 0))
/** Basic PWM 1 module clock configuration */
#define NUMAKER_BPWM1_MODULE                                                                   \
	((2UL << 30) | (2UL << 28) | (0x1UL << 25) | (9UL << 20) | (NUMAKER_MODULE_NoMsk << 18) |  \
	 (NUMAKER_MODULE_NoMsk << 10) | (NUMAKER_MODULE_NoMsk << 5) | (19UL << 0))
/** Quadrature encoder 0 module clock configuration */
#define NUMAKER_QEI0_MODULE                                                                    \
	((2UL << 30) | (NUMAKER_MODULE_NoMsk << 28) | (NUMAKER_MODULE_NoMsk << 25) |               \
	 (NUMAKER_MODULE_NoMsk << 20) | (NUMAKER_MODULE_NoMsk << 18) |                             \
	 (NUMAKER_MODULE_NoMsk << 10) | (NUMAKER_MODULE_NoMsk << 5) | (22UL << 0))
/** Quadrature encoder 1 module clock configuration */
#define NUMAKER_QEI1_MODULE                                                                    \
	((2UL << 30) | (NUMAKER_MODULE_NoMsk << 28) | (NUMAKER_MODULE_NoMsk << 25) |               \
	 (NUMAKER_MODULE_NoMsk << 20) | (NUMAKER_MODULE_NoMsk << 18) |                             \
	 (NUMAKER_MODULE_NoMsk << 10) | (NUMAKER_MODULE_NoMsk << 5) | (23UL << 0))
/** True random number generator module clock configuration */
#define NUMAKER_TRNG_MODULE                                                                    \
	((2UL << 30) | (NUMAKER_MODULE_NoMsk << 28) | (NUMAKER_MODULE_NoMsk << 25) |               \
	 (NUMAKER_MODULE_NoMsk << 20) | (NUMAKER_MODULE_NoMsk << 18) |                             \
	 (NUMAKER_MODULE_NoMsk << 10) | (NUMAKER_MODULE_NoMsk << 5) | (25UL << 0))
/** Enhanced input capture 0 module clock configuration */
#define NUMAKER_ECAP0_MODULE                                                                   \
	((2UL << 30) | (NUMAKER_MODULE_NoMsk << 28) | (NUMAKER_MODULE_NoMsk << 25) |               \
	 (NUMAKER_MODULE_NoMsk << 20) | (NUMAKER_MODULE_NoMsk << 18) |                             \
	 (NUMAKER_MODULE_NoMsk << 10) | (NUMAKER_MODULE_NoMsk << 5) | (26UL << 0))
/** Enhanced input capture 1 module clock configuration */
#define NUMAKER_ECAP1_MODULE                                                                   \
	((2UL << 30) | (NUMAKER_MODULE_NoMsk << 28) | (NUMAKER_MODULE_NoMsk << 25) |               \
	 (NUMAKER_MODULE_NoMsk << 20) | (NUMAKER_MODULE_NoMsk << 18) |                             \
	 (NUMAKER_MODULE_NoMsk << 10) | (NUMAKER_MODULE_NoMsk << 5) | (27UL << 0))
/** Operational amplifier module clock configuration */
#define NUMAKER_OPA_MODULE                                                                     \
	((2UL << 30) | (NUMAKER_MODULE_NoMsk << 28) | (NUMAKER_MODULE_NoMsk << 25) |               \
	 (NUMAKER_MODULE_NoMsk << 20) | (NUMAKER_MODULE_NoMsk << 18) |                             \
	 (NUMAKER_MODULE_NoMsk << 10) | (NUMAKER_MODULE_NoMsk << 5) | (30UL << 0))
/** Enhanced ADC 1 module clock configuration */
#define NUMAKER_EADC1_MODULE                                                                   \
	((2UL << 30) | (NUMAKER_MODULE_NoMsk << 28) | (NUMAKER_MODULE_NoMsk << 25) |               \
	 (NUMAKER_MODULE_NoMsk << 20) | (2UL << 18) | (0xFFUL << 10) | (24UL << 5) | (31UL << 0))


/** @} */

/* End of M480 BSP clk.h copy */

/**
 * @name PMU power-down mode selection
 * @{
 */
/** Normal power-down mode */
#define NUMAKER_CLK_PMUCTL_PDMSEL_PD   0x00000000
/** Low leakage power-down mode */
#define NUMAKER_CLK_PMUCTL_PDMSEL_LLPD 0x00000001
/** Fast wake-up power-down mode */
#define NUMAKER_CLK_PMUCTL_PDMSEL_FWPD 0x00000002
/** Standby power-down mode */
#define NUMAKER_CLK_PMUCTL_PDMSEL_SPD  0x00000004
/** Deep power-down mode */
#define NUMAKER_CLK_PMUCTL_PDMSEL_DPD  0x00000006


/** @} */

#endif
