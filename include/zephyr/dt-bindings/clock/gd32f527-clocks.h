/*
 * Copyright (c) 2026 GigaDevice Semiconductor Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief GD32F527 clock definitions for device tree bindings
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_GD32F527_CLOCKS_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_GD32F527_CLOCKS_H_

#include "gd32-clocks-common.h"

/**
 * @name Register offsets
 * @{
 */

#define GD32_AHB1EN_OFFSET 0x30U /**< AHB1 peripheral clock enable register offset */
#define GD32_AHB2EN_OFFSET 0x34U /**< AHB2 peripheral clock enable register offset */
#define GD32_APB1EN_OFFSET 0x40U /**< APB1 peripheral clock enable register offset */
#define GD32_APB2EN_OFFSET 0x44U /**< APB2 peripheral clock enable register offset */

/** @} */

/**
 * @name Clock enable/disable definitions for peripherals
 * @{
 */

/* AHB1 peripherals */
#define GD32_CLOCK_GPIOA     GD32_CLOCK_CONFIG(AHB1EN, 0U)  /**< GPIOA clock */
#define GD32_CLOCK_GPIOB     GD32_CLOCK_CONFIG(AHB1EN, 1U)  /**< GPIOB clock */
#define GD32_CLOCK_GPIOC     GD32_CLOCK_CONFIG(AHB1EN, 2U)  /**< GPIOC clock */
#define GD32_CLOCK_GPIOD     GD32_CLOCK_CONFIG(AHB1EN, 3U)  /**< GPIOD clock */
#define GD32_CLOCK_GPIOE     GD32_CLOCK_CONFIG(AHB1EN, 4U)  /**< GPIOE clock */
#define GD32_CLOCK_GPIOF     GD32_CLOCK_CONFIG(AHB1EN, 5U)  /**< GPIOF clock */
#define GD32_CLOCK_GPIOG     GD32_CLOCK_CONFIG(AHB1EN, 6U)  /**< GPIOG clock */
#define GD32_CLOCK_GPIOH     GD32_CLOCK_CONFIG(AHB1EN, 7U)  /**< GPIOH clock */
#define GD32_CLOCK_GPIOI     GD32_CLOCK_CONFIG(AHB1EN, 8U)  /**< GPIOI clock */
#define GD32_CLOCK_CRC       GD32_CLOCK_CONFIG(AHB1EN, 12U) /**< CRC clock */
#define GD32_CLOCK_BKPSRAM   GD32_CLOCK_CONFIG(AHB1EN, 18U) /**< BKPSRAM clock */
#define GD32_CLOCK_DMA0      GD32_CLOCK_CONFIG(AHB1EN, 21U) /**< DMA0 clock */
#define GD32_CLOCK_DMA1      GD32_CLOCK_CONFIG(AHB1EN, 22U) /**< DMA1 clock */
#define GD32_CLOCK_IPA       GD32_CLOCK_CONFIG(AHB1EN, 23U) /**< IPA clock */
#define GD32_CLOCK_ENET      GD32_CLOCK_CONFIG(AHB1EN, 25U) /**< ENET clock */
#define GD32_CLOCK_ENETTX    GD32_CLOCK_CONFIG(AHB1EN, 26U) /**< ENETTX clock */
#define GD32_CLOCK_ENETRX    GD32_CLOCK_CONFIG(AHB1EN, 27U) /**< ENETRX clock */
#define GD32_CLOCK_ENETPTP   GD32_CLOCK_CONFIG(AHB1EN, 28U) /**< ENETPTP clock */
#define GD32_CLOCK_USBHS     GD32_CLOCK_CONFIG(AHB1EN, 29U) /**< USBHS clock */
#define GD32_CLOCK_USBHSULPI GD32_CLOCK_CONFIG(AHB1EN, 30U) /**< USBHSULPI clock */

/* AHB2 peripherals */
#define GD32_CLOCK_DCI   GD32_CLOCK_CONFIG(AHB2EN, 0U) /**< DCI clock */
#define GD32_CLOCK_PKCAU GD32_CLOCK_CONFIG(AHB2EN, 3U) /**< PKCAU clock */
#define GD32_CLOCK_CAU   GD32_CLOCK_CONFIG(AHB2EN, 4U) /**< CAU clock */
#define GD32_CLOCK_HAU   GD32_CLOCK_CONFIG(AHB2EN, 5U) /**< HAU clock */
#define GD32_CLOCK_TRNG  GD32_CLOCK_CONFIG(AHB2EN, 6U) /**< TRNG clock */
#define GD32_CLOCK_USBFS GD32_CLOCK_CONFIG(AHB2EN, 7U) /**< USBFS clock */

/* APB1 peripherals */
#define GD32_CLOCK_TIMER1  GD32_CLOCK_CONFIG(APB1EN, 0U)  /**< TIMER1 clock */
#define GD32_CLOCK_TIMER2  GD32_CLOCK_CONFIG(APB1EN, 1U)  /**< TIMER2 clock */
#define GD32_CLOCK_TIMER3  GD32_CLOCK_CONFIG(APB1EN, 2U)  /**< TIMER3 clock */
#define GD32_CLOCK_TIMER4  GD32_CLOCK_CONFIG(APB1EN, 3U)  /**< TIMER4 clock */
#define GD32_CLOCK_TIMER5  GD32_CLOCK_CONFIG(APB1EN, 4U)  /**< TIMER5 clock */
#define GD32_CLOCK_TIMER6  GD32_CLOCK_CONFIG(APB1EN, 5U)  /**< TIMER6 clock */
#define GD32_CLOCK_TIMER11 GD32_CLOCK_CONFIG(APB1EN, 6U)  /**< TIMER11 clock */
#define GD32_CLOCK_TIMER12 GD32_CLOCK_CONFIG(APB1EN, 7U)  /**< TIMER12 clock */
#define GD32_CLOCK_TIMER13 GD32_CLOCK_CONFIG(APB1EN, 8U)  /**< TIMER13 clock */
#define GD32_CLOCK_I2C3    GD32_CLOCK_CONFIG(APB1EN, 10U) /**< I2C3 clock */
#define GD32_CLOCK_WWDGT   GD32_CLOCK_CONFIG(APB1EN, 11U) /**< WWDGT clock */
#define GD32_CLOCK_I2C4    GD32_CLOCK_CONFIG(APB1EN, 12U) /**< I2C4 clock */
#define GD32_CLOCK_I2C5    GD32_CLOCK_CONFIG(APB1EN, 13U) /**< I2C5 clock */
#define GD32_CLOCK_SPI1    GD32_CLOCK_CONFIG(APB1EN, 14U) /**< SPI1 clock */
#define GD32_CLOCK_SPI2    GD32_CLOCK_CONFIG(APB1EN, 15U) /**< SPI2 clock */
#define GD32_CLOCK_USART1  GD32_CLOCK_CONFIG(APB1EN, 17U) /**< USART1 clock */
#define GD32_CLOCK_USART2  GD32_CLOCK_CONFIG(APB1EN, 18U) /**< USART2 clock */
#define GD32_CLOCK_UART3   GD32_CLOCK_CONFIG(APB1EN, 19U) /**< UART3 clock */
#define GD32_CLOCK_UART4   GD32_CLOCK_CONFIG(APB1EN, 20U) /**< UART4 clock */
#define GD32_CLOCK_I2C0    GD32_CLOCK_CONFIG(APB1EN, 21U) /**< I2C0 clock */
#define GD32_CLOCK_I2C1    GD32_CLOCK_CONFIG(APB1EN, 22U) /**< I2C1 clock */
#define GD32_CLOCK_I2C2    GD32_CLOCK_CONFIG(APB1EN, 23U) /**< I2C2 clock */
#define GD32_CLOCK_CAN0    GD32_CLOCK_CONFIG(APB1EN, 25U) /**< CAN0 clock */
#define GD32_CLOCK_CAN1    GD32_CLOCK_CONFIG(APB1EN, 26U) /**< CAN1 clock */
#define GD32_CLOCK_PMU     GD32_CLOCK_CONFIG(APB1EN, 28U) /**< PMU clock */
#define GD32_CLOCK_DAC     GD32_CLOCK_CONFIG(APB1EN, 29U) /**< DAC clock */
#define GD32_CLOCK_UART6   GD32_CLOCK_CONFIG(APB1EN, 30U) /**< UART6 clock */
#define GD32_CLOCK_UART7   GD32_CLOCK_CONFIG(APB1EN, 31U) /**< UART7 clock */

/* APB2 peripherals */
#define GD32_CLOCK_TIMER0  GD32_CLOCK_CONFIG(APB2EN, 0U)  /**< TIMER0 clock */
#define GD32_CLOCK_TIMER7  GD32_CLOCK_CONFIG(APB2EN, 1U)  /**< TIMER7 clock */
#define GD32_CLOCK_USART0  GD32_CLOCK_CONFIG(APB2EN, 4U)  /**< USART0 clock */
#define GD32_CLOCK_USART5  GD32_CLOCK_CONFIG(APB2EN, 5U)  /**< USART5 clock */
#define GD32_CLOCK_ADC0    GD32_CLOCK_CONFIG(APB2EN, 8U)  /**< ADC0 clock */
#define GD32_CLOCK_ADC1    GD32_CLOCK_CONFIG(APB2EN, 9U)  /**< ADC1 clock */
#define GD32_CLOCK_ADC2    GD32_CLOCK_CONFIG(APB2EN, 10U) /**< ADC2 clock */
#define GD32_CLOCK_SDIO    GD32_CLOCK_CONFIG(APB2EN, 11U) /**< SDIO clock */
#define GD32_CLOCK_SPI0    GD32_CLOCK_CONFIG(APB2EN, 12U) /**< SPI0 clock */
#define GD32_CLOCK_SPI3    GD32_CLOCK_CONFIG(APB2EN, 13U) /**< SPI3 clock */
#define GD32_CLOCK_SYSCFG  GD32_CLOCK_CONFIG(APB2EN, 14U) /**< SYSCFG clock */
#define GD32_CLOCK_TIMER8  GD32_CLOCK_CONFIG(APB2EN, 16U) /**< TIMER8 clock */
#define GD32_CLOCK_TIMER9  GD32_CLOCK_CONFIG(APB2EN, 17U) /**< TIMER9 clock */
#define GD32_CLOCK_TIMER10 GD32_CLOCK_CONFIG(APB2EN, 18U) /**< TIMER10 clock */
#define GD32_CLOCK_SPI4    GD32_CLOCK_CONFIG(APB2EN, 20U) /**< SPI4 clock */
#define GD32_CLOCK_SPI5    GD32_CLOCK_CONFIG(APB2EN, 21U) /**< SPI5 clock */
#define GD32_CLOCK_SAI0    GD32_CLOCK_CONFIG(APB2EN, 22U) /**< SAI0 clock */
#define GD32_CLOCK_TLI     GD32_CLOCK_CONFIG(APB2EN, 26U) /**< TLI clock */

/** @} */

/**
 * @name CKOUT0 clock output definitions
 * @{
 */

/** CKOUT0 source selection field encoder (RCU_CFG0_CKOUT0SEL, bits 21-22) */
#define GD32_CKOUT0_SEL(val) (((val) & 0x3U) << 21U)

/** CKOUT0 source: IRC16M */
#define GD32_CKOUT0SRC_IRC16M GD32_CKOUT0_SEL(0U)
/** CKOUT0 source: LXTAL */
#define GD32_CKOUT0SRC_LXTAL  GD32_CKOUT0_SEL(1U)
/** CKOUT0 source: HXTAL */
#define GD32_CKOUT0SRC_HXTAL  GD32_CKOUT0_SEL(2U)
/** CKOUT0 source: PLLP */
#define GD32_CKOUT0SRC_PLLP   GD32_CKOUT0_SEL(3U)

/** CKOUT0 divider field encoder (RCU_CFG0_CKOUT0DIV, bits 24-26) */
#define GD32_CKOUT0_DIVCFG(val) (((val) & 0x7U) << 24U)

/** CKOUT0 divider: 1 */
#define GD32_CKOUT0_DIV1 GD32_CKOUT0_DIVCFG(0U)
/** CKOUT0 divider: 2 */
#define GD32_CKOUT0_DIV2 GD32_CKOUT0_DIVCFG(4U)
/** CKOUT0 divider: 3 */
#define GD32_CKOUT0_DIV3 GD32_CKOUT0_DIVCFG(5U)
/** CKOUT0 divider: 4 */
#define GD32_CKOUT0_DIV4 GD32_CKOUT0_DIVCFG(6U)
/** CKOUT0 divider: 5 */
#define GD32_CKOUT0_DIV5 GD32_CKOUT0_DIVCFG(7U)

/** PLLSAIR divide by 2 */
#define GD32_RCU_PLLSAIR_DIV2  0x00000000
/** PLLSAIR divide by 4 */
#define GD32_RCU_PLLSAIR_DIV4  0x00010000
/** PLLSAIR divide by 8 */
#define GD32_RCU_PLLSAIR_DIV8  0x00020000
/** PLLSAIR divide by 16 */
#define GD32_RCU_PLLSAIR_DIV16 0x00030000
/** @} */

/**
 * @name PLL clock configuration definitions
 * @{
 */

/** PLL VCO source clock prescaler (RCU_PLL_PLLPSC, bits 0-5) */
#define GD32_PLL_PSC(val) (((val) & 0x3FU) << 0U)

/** PLL VCO clock multiplication factor (RCU_PLL_PLLN, bits 6-14) */
#define GD32_PLL_N(val) (((val) & 0x1FFU) << 6U)

/** PLLP output division factor (RCU_PLL_PLLP, bits 16-17) */
#define GD32_PLL_P(val) (((((val) >> 1U) - 1U) & 0x3U) << 16U)

/** PLL clock source selection (RCU_PLL_PLLSEL, bit 22) */
#define GD32_PLL_SRC(val) (((val) & 0x1U) << 22U)

/** PLLQ output division factor (RCU_PLL_PLLQ, bits 24-27) */
#define GD32_PLL_Q(val) (((val) & 0xFU) << 24U)

/** PLL input clock source: IRC16M */
#define GD32_PLLSRC_IRC16M GD32_PLL_SRC(0U)
/** PLL input clock source: HXTAL */
#define GD32_PLLSRC_HXTAL  GD32_PLL_SRC(1U)

/** @} */

/**
 * @name System clock source and bus prescaler definitions
 * @{
 */

/** System clock source select (RCU_CFG0_SCS, bits 0-1) */
#define GD32_CKSYSSRC(val) (((val) & 0x3U) << 0U)

/** System clock source: IRC16M */
#define GD32_CKSYSSRC_IRC16M GD32_CKSYSSRC(0U)
/** System clock source: HXTAL */
#define GD32_CKSYSSRC_HXTAL  GD32_CKSYSSRC(1U)
/** System clock source: PLLP */
#define GD32_CKSYSSRC_PLLP   GD32_CKSYSSRC(2U)

/** AHB prescaler field encoder (RCU_CFG0_AHBPSC, bits 4-7) */
#define GD32_AHB_PSC(val) (((val) & 0xFU) << 4U)

/** AHB prescaler: CK_SYS/1 */
#define GD32_AHB_PSC_DIV1   GD32_AHB_PSC(0U)
/** AHB prescaler: CK_SYS/2 */
#define GD32_AHB_PSC_DIV2   GD32_AHB_PSC(8U)
/** AHB prescaler: CK_SYS/4 */
#define GD32_AHB_PSC_DIV4   GD32_AHB_PSC(9U)
/** AHB prescaler: CK_SYS/8 */
#define GD32_AHB_PSC_DIV8   GD32_AHB_PSC(10U)
/** AHB prescaler: CK_SYS/16 */
#define GD32_AHB_PSC_DIV16  GD32_AHB_PSC(11U)
/** AHB prescaler: CK_SYS/64 */
#define GD32_AHB_PSC_DIV64  GD32_AHB_PSC(12U)
/** AHB prescaler: CK_SYS/128 */
#define GD32_AHB_PSC_DIV128 GD32_AHB_PSC(13U)
/** AHB prescaler: CK_SYS/256 */
#define GD32_AHB_PSC_DIV256 GD32_AHB_PSC(14U)
/** AHB prescaler: CK_SYS/512 */
#define GD32_AHB_PSC_DIV512 GD32_AHB_PSC(15U)

/** APB1 prescaler field encoder (RCU_CFG0_APB1PSC, bits 10-12) */
#define GD32_APB1_PSC(val) (((val) & 0x7U) << 10U)

/** APB1 prescaler: CK_AHB/1 */
#define GD32_APB1_PSC_DIV1  GD32_APB1_PSC(0U)
/** APB1 prescaler: CK_AHB/2 */
#define GD32_APB1_PSC_DIV2  GD32_APB1_PSC(4U)
/** APB1 prescaler: CK_AHB/4 */
#define GD32_APB1_PSC_DIV4  GD32_APB1_PSC(5U)
/** APB1 prescaler: CK_AHB/8 */
#define GD32_APB1_PSC_DIV8  GD32_APB1_PSC(6U)
/** APB1 prescaler: CK_AHB/16 */
#define GD32_APB1_PSC_DIV16 GD32_APB1_PSC(7U)

/** APB2 prescaler field encoder (RCU_CFG0_APB2PSC, bits 13-15) */
#define GD32_APB2_PSC(val) (((val) & 0x7U) << 13U)

/** APB2 prescaler: CK_AHB/1 */
#define GD32_APB2_PSC_DIV1  GD32_APB2_PSC(0U)
/** APB2 prescaler: CK_AHB/2 */
#define GD32_APB2_PSC_DIV2  GD32_APB2_PSC(4U)
/** APB2 prescaler: CK_AHB/4 */
#define GD32_APB2_PSC_DIV4  GD32_APB2_PSC(5U)
/** APB2 prescaler: CK_AHB/8 */
#define GD32_APB2_PSC_DIV8  GD32_APB2_PSC(6U)
/** APB2 prescaler: CK_AHB/16 */
#define GD32_APB2_PSC_DIV16 GD32_APB2_PSC(7U)

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_GD32F527_CLOCKS_H_ */
