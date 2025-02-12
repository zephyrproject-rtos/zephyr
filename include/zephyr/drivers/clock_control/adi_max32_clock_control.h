/*
 * Copyright (c) 2023-2024 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_ADI_MAX32_CLOCK_CONTROL_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_ADI_MAX32_CLOCK_CONTROL_H_

#include <zephyr/drivers/clock_control.h>

#include <zephyr/dt-bindings/clock/adi_max32_clock.h>

#include <wrap_max32_sys.h>

/** Driver structure definition */

struct max32_perclk {
	uint32_t bus;
	uint32_t bit;

	/* Peripheral clock source:
	 * Can be (see: adi_max32_clock.h file):
	 *
	 *   ADI_MAX32_PRPH_CLK_SRC_PCLK
	 *   ADI_MAX32_PRPH_CLK_SRC_EXTCLK
	 *   ADI_MAX32_PRPH_CLK_SRC_IBRO
	 *   ADI_MAX32_PRPH_CLK_SRC_ERFO
	 *   ADI_MAX32_PRPH_CLK_SRC_ERTCO
	 *   ADI_MAX32_PRPH_CLK_SRC_INRO
	 *   ADI_MAX32_PRPH_CLK_SRC_ISO
	 *   ADI_MAX32_PRPH_CLK_SRC_IBRO_DIV8
	 */
	uint32_t clk_src;
};

/** Get prescaler value if it defined  */
#define ADI_MAX32_SYSCLK_PRESCALER DT_PROP_OR(DT_NODELABEL(gcr), sysclk_prescaler, 1)

#define ADI_MAX32_CLK_IPO_FREQ    DT_PROP(DT_NODELABEL(clk_ipo), clock_frequency)
#define ADI_MAX32_CLK_ERFO_FREQ   DT_PROP(DT_NODELABEL(clk_erfo), clock_frequency)
#define ADI_MAX32_CLK_IBRO_FREQ   DT_PROP(DT_NODELABEL(clk_ibro), clock_frequency)
#define ADI_MAX32_CLK_ISO_FREQ    DT_PROP_OR(DT_NODELABEL(clk_iso), clock_frequency, 0)
#define ADI_MAX32_CLK_INRO_FREQ   DT_PROP(DT_NODELABEL(clk_inro), clock_frequency)
#define ADI_MAX32_CLK_ERTCO_FREQ  DT_PROP(DT_NODELABEL(clk_ertco), clock_frequency)
/* External clock may not be defined so _OR is used */
#define ADI_MAX32_CLK_EXTCLK_FREQ DT_PROP_OR(DT_NODELABEL(clk_extclk), clock_frequency, 0)

#define DT_GCR_CLOCKS_CTRL DT_CLOCKS_CTLR(DT_NODELABEL(gcr))

#if DT_SAME_NODE(DT_GCR_CLOCKS_CTRL, DT_NODELABEL(clk_ipo))
#define ADI_MAX32_SYSCLK_SRC  ADI_MAX32_CLK_IPO
#define ADI_MAX32_SYSCLK_FREQ (ADI_MAX32_CLK_IPO_FREQ / ADI_MAX32_SYSCLK_PRESCALER)
#endif
#if DT_SAME_NODE(DT_GCR_CLOCKS_CTRL, DT_NODELABEL(clk_erfo))
#define ADI_MAX32_SYSCLK_SRC  ADI_MAX32_CLK_ERFO
#define ADI_MAX32_SYSCLK_FREQ (ADI_MAX32_CLK_ERFO_FREQ / ADI_MAX32_SYSCLK_PRESCALER)
#endif
#if DT_SAME_NODE(DT_GCR_CLOCKS_CTRL, DT_NODELABEL(clk_ibro))
#define ADI_MAX32_SYSCLK_SRC  ADI_MAX32_CLK_IBRO
#define ADI_MAX32_SYSCLK_FREQ (ADI_MAX32_CLK_IBRO_FREQ / ADI_MAX32_SYSCLK_PRESCALER)
#endif
#if DT_SAME_NODE(DT_GCR_CLOCKS_CTRL, DT_NODELABEL(clk_iso))
#define ADI_MAX32_SYSCLK_SRC  ADI_MAX32_CLK_ISO
#define ADI_MAX32_SYSCLK_FREQ (ADI_MAX32_CLK_ISO_FREQ / ADI_MAX32_SYSCLK_PRESCALER)
#endif
#if DT_SAME_NODE(DT_GCR_CLOCKS_CTRL, DT_NODELABEL(clk_inro))
#define ADI_MAX32_SYSCLK_SRC  ADI_MAX32_CLK_INRO
#define ADI_MAX32_SYSCLK_FREQ (ADI_MAX32_CLK_INRO_FREQ / ADI_MAX32_SYSCLK_PRESCALER)
#endif
#if DT_SAME_NODE(DT_GCR_CLOCKS_CTRL, DT_NODELABEL(clk_ertco))
#define ADI_MAX32_SYSCLK_SRC  ADI_MAX32_CLK_ERTCO
#define ADI_MAX32_SYSCLK_FREQ (ADI_MAX32_CLK_ERTCO_FREQ / ADI_MAX32_SYSCLK_PRESCALER)
#endif
#if DT_SAME_NODE(DT_GCR_CLOCKS_CTRL, DT_NODELABEL(clk_extclk))
#define ADI_MAX32_SYSCLK_SRC  ADI_MAX32_CLK_EXTCLK
#define ADI_MAX32_SYSCLK_FREQ (ADI_MAX32_CLK_EXTCLK_FREQ / ADI_MAX32_SYSCLK_PRESCALER)
#endif

#ifndef ADI_MAX32_SYSCLK_SRC
#define ADI_MAX32_SYSCLK_SRC  ADI_MAX32_CLK_IPO
#define ADI_MAX32_SYSCLK_FREQ (ADI_MAX32_CLK_IPO_FREQ / ADI_MAX32_SYSCLK_PRESCALER)
#endif

#define ADI_MAX32_PCLK_FREQ (ADI_MAX32_SYSCLK_FREQ / 2)

#define ADI_MAX32_GET_PRPH_CLK_FREQ(clk_src)                                                       \
	((clk_src) == ADI_MAX32_PRPH_CLK_SRC_PCLK        ? ADI_MAX32_PCLK_FREQ                     \
	 : (clk_src) == ADI_MAX32_PRPH_CLK_SRC_IBRO      ? ADI_MAX32_CLK_IBRO_FREQ                 \
	 : (clk_src) == ADI_MAX32_PRPH_CLK_SRC_ERFO      ? ADI_MAX32_CLK_ERFO_FREQ                 \
	 : (clk_src) == ADI_MAX32_PRPH_CLK_SRC_ERTCO     ? ADI_MAX32_CLK_ERTCO_FREQ                \
	 : (clk_src) == ADI_MAX32_PRPH_CLK_SRC_INRO      ? ADI_MAX32_CLK_INRO_FREQ                 \
	 : (clk_src) == ADI_MAX32_PRPH_CLK_SRC_ISO       ? ADI_MAX32_CLK_ISO_FREQ                  \
	 : (clk_src) == ADI_MAX32_PRPH_CLK_SRC_IBRO_DIV8 ? (ADI_MAX32_CLK_IBRO_FREQ / 8)           \
	 : (clk_src) == ADI_MAX32_PRPH_CLK_SRC_EXTCLK    ? ADI_MAX32_CLK_EXTCLK_FREQ               \
							 : 0)

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_ADI_MAX32_CLOCK_CONTROL_H_ */
