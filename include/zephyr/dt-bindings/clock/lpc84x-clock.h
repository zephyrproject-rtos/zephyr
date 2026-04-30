/*
 * Copyright (c) 2026 Aerlync Labs Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief NXP LPC84x clock gate definitions.
 *
 * This header provides encoded clock gate identifiers used by
 * the LPC84x clock control driver. Each identifier encodes the
 * AHB clock control register offset and bit position.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_NXP_LPC84X_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_NXP_LPC84X_H_

/** SYS_AHB_CLK_CTRL0 register offset. */
#define LPC84X_SYS_AHB_CLK_CTRL0 0U

/** SYS_AHB_CLK_CTRL1 register offset. */
#define LPC84X_SYS_AHB_CLK_CTRL1 4U

/**
 * Encodes an LPC84x clock gate identifier.
 *
 * @param reg Register offset (SYS_AHB_CLK_CTRLx)
 * @param bit Bit position within the register
 */
#define LPC84X_CLK_GATE_DEFINE(reg, bit) ((((reg) & 0xFFU) << 8U) | ((bit) & 0xFFU))

/** ROM clock gate. */
#define LPC84X_CLK_ROM LPC84X_CLK_GATE_DEFINE(LPC84X_SYS_AHB_CLK_CTRL0, 1U)

/** RAM0 and RAM1 clock gate. */
#define LPC84X_CLK_RAM0_1 LPC84X_CLK_GATE_DEFINE(LPC84X_SYS_AHB_CLK_CTRL0, 2U)

/** I2C0 clock gate. */
#define LPC84X_CLK_I2C0 LPC84X_CLK_GATE_DEFINE(LPC84X_SYS_AHB_CLK_CTRL0, 5U)

/** GPIO0 clock gate. */
#define LPC84X_CLK_GPIO0 LPC84X_CLK_GATE_DEFINE(LPC84X_SYS_AHB_CLK_CTRL0, 6U)

/** Switch Matrix (SWM) clock gate. */
#define LPC84X_CLK_SWM LPC84X_CLK_GATE_DEFINE(LPC84X_SYS_AHB_CLK_CTRL0, 7U)

/** State Configurable Timer (SCT) clock gate. */
#define LPC84X_CLK_SCT LPC84X_CLK_GATE_DEFINE(LPC84X_SYS_AHB_CLK_CTRL0, 8U)

/** Wake-up Timer (WKT) clock gate. */
#define LPC84X_CLK_WKT LPC84X_CLK_GATE_DEFINE(LPC84X_SYS_AHB_CLK_CTRL0, 9U)

/** Multi-Rate Timer (MRT) clock gate. */
#define LPC84X_CLK_MRT LPC84X_CLK_GATE_DEFINE(LPC84X_SYS_AHB_CLK_CTRL0, 10U)

/** SPI0 clock gate. */
#define LPC84X_CLK_SPI0 LPC84X_CLK_GATE_DEFINE(LPC84X_SYS_AHB_CLK_CTRL0, 11U)

/** SPI1 clock gate. */
#define LPC84X_CLK_SPI1 LPC84X_CLK_GATE_DEFINE(LPC84X_SYS_AHB_CLK_CTRL0, 12U)

/** CRC engine clock gate. */
#define LPC84X_CLK_CRC LPC84X_CLK_GATE_DEFINE(LPC84X_SYS_AHB_CLK_CTRL0, 13U)

/** UART0 clock gate. */
#define LPC84X_CLK_UART0 LPC84X_CLK_GATE_DEFINE(LPC84X_SYS_AHB_CLK_CTRL0, 14U)

/** UART1 clock gate. */
#define LPC84X_CLK_UART1 LPC84X_CLK_GATE_DEFINE(LPC84X_SYS_AHB_CLK_CTRL0, 15U)

/** UART2 clock gate. */
#define LPC84X_CLK_UART2 LPC84X_CLK_GATE_DEFINE(LPC84X_SYS_AHB_CLK_CTRL0, 16U)

/** Windowed Watchdog Timer clock gate. */
#define LPC84X_CLK_WWDT LPC84X_CLK_GATE_DEFINE(LPC84X_SYS_AHB_CLK_CTRL0, 17U)

/** IOCON clock gate. */
#define LPC84X_CLK_IOCON LPC84X_CLK_GATE_DEFINE(LPC84X_SYS_AHB_CLK_CTRL0, 18U)

/** Analog Comparator clock gate. */
#define LPC84X_CLK_ACMP LPC84X_CLK_GATE_DEFINE(LPC84X_SYS_AHB_CLK_CTRL0, 19U)

/** GPIO1 clock gate. */
#define LPC84X_CLK_GPIO1 LPC84X_CLK_GATE_DEFINE(LPC84X_SYS_AHB_CLK_CTRL0, 20U)

/** I2C1 clock gate. */
#define LPC84X_CLK_I2C1 LPC84X_CLK_GATE_DEFINE(LPC84X_SYS_AHB_CLK_CTRL0, 21U)

/** I2C2 clock gate. */
#define LPC84X_CLK_I2C2 LPC84X_CLK_GATE_DEFINE(LPC84X_SYS_AHB_CLK_CTRL0, 22U)

/** I2C3 clock gate. */
#define LPC84X_CLK_I2C3 LPC84X_CLK_GATE_DEFINE(LPC84X_SYS_AHB_CLK_CTRL0, 23U)

/** ADC clock gate. */
#define LPC84X_CLK_ADC LPC84X_CLK_GATE_DEFINE(LPC84X_SYS_AHB_CLK_CTRL0, 24U)

/** CTIMER0 clock gate. */
#define LPC84X_CLK_CTIMER0 LPC84X_CLK_GATE_DEFINE(LPC84X_SYS_AHB_CLK_CTRL0, 25U)

/** MTB clock gate. */
#define LPC84X_CLK_MTB LPC84X_CLK_GATE_DEFINE(LPC84X_SYS_AHB_CLK_CTRL0, 26U)

/** DAC0 clock gate. */
#define LPC84X_CLK_DAC0 LPC84X_CLK_GATE_DEFINE(LPC84X_SYS_AHB_CLK_CTRL0, 27U)

/** GPIO interrupt clock gate. */
#define LPC84X_CLK_GPIOINT LPC84X_CLK_GATE_DEFINE(LPC84X_SYS_AHB_CLK_CTRL0, 28U)

/** DMA clock gate. */
#define LPC84X_CLK_DMA LPC84X_CLK_GATE_DEFINE(LPC84X_SYS_AHB_CLK_CTRL0, 29U)

/** UART3 clock gate. */
#define LPC84X_CLK_UART3 LPC84X_CLK_GATE_DEFINE(LPC84X_SYS_AHB_CLK_CTRL0, 30U)

/** UART4 clock gate. */
#define LPC84X_CLK_UART4 LPC84X_CLK_GATE_DEFINE(LPC84X_SYS_AHB_CLK_CTRL0, 31U)

/** CAPT clock gate. */
#define LPC84X_CLK_CAPT LPC84X_CLK_GATE_DEFINE(LPC84X_SYS_AHB_CLK_CTRL1, 0U)

/** DAC1 clock gate. */
#define LPC84X_CLK_DAC1 LPC84X_CLK_GATE_DEFINE(LPC84X_SYS_AHB_CLK_CTRL1, 1U)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_NXP_LPC84X_H_ */
