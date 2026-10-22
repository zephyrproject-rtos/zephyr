/*
 * Copyright (c) 2023-2024 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Clock control definitions for Analog Devices MAX32 devices.
 * @ingroup clock_control_adi_max32
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_ADI_MAX32_CLOCK_CONTROL_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_ADI_MAX32_CLOCK_CONTROL_H_

#include <zephyr/drivers/clock_control.h>

#include <zephyr/dt-bindings/clock/adi_max32_clock.h>

#include <wrap_max32_sys.h>

/**
 * @defgroup clock_control_adi_max32 Analog Devices MAX32
 * @ingroup clock_control_interface_ext
 * @{
 */

/** @brief Peripheral clock descriptor for MAX32 devices. */
struct max32_perclk {
	uint32_t bus; /**< Peripheral clock bus index. */
	uint32_t bit; /**< Peripheral enable bit within the bus register. */
	/**
	 * @brief Peripheral clock source.
	 *
	 * One of the @c ADI_MAX32_PRPH_CLK_SRC_* constants defined in
	 * @c <zephyr/dt-bindings/clock/adi_max32_clock.h> (e.g. @c ADI_MAX32_PRPH_CLK_SRC_PCLK,
	 * @c ADI_MAX32_PRPH_CLK_SRC_IBRO, @c ADI_MAX32_PRPH_CLK_SRC_IPLL).
	 */
	uint32_t clk_src;
};

/** @brief System clock prescaler (defaults to 1 when not set in Devicetree). */
#define ADI_MAX32_SYSCLK_PRESCALER DT_PROP_OR(DT_NODELABEL(gcr), sysclk_prescaler, 1)

/** @brief Internal primary oscillator (IPO) frequency, in Hz. */
#define ADI_MAX32_CLK_IPO_FREQ    DT_PROP(DT_NODELABEL(clk_ipo), clock_frequency)
/** @brief External RF oscillator (ERFO) frequency, in Hz (0 if absent). */
#define ADI_MAX32_CLK_ERFO_FREQ   DT_PROP_OR(DT_NODELABEL(clk_erfo), clock_frequency, 0)
/** @brief Internal baud-rate oscillator (IBRO) frequency, in Hz (0 if absent). */
#define ADI_MAX32_CLK_IBRO_FREQ   DT_PROP_OR(DT_NODELABEL(clk_ibro), clock_frequency, 0)
/** @brief Internal secondary oscillator (ISO) frequency, in Hz (0 if absent). */
#define ADI_MAX32_CLK_ISO_FREQ    DT_PROP_OR(DT_NODELABEL(clk_iso), clock_frequency, 0)
/** @brief Internal nano-ring oscillator (INRO) frequency, in Hz. */
#define ADI_MAX32_CLK_INRO_FREQ   DT_PROP(DT_NODELABEL(clk_inro), clock_frequency)
/** @brief External RTC oscillator (ERTCO) frequency, in Hz. */
#define ADI_MAX32_CLK_ERTCO_FREQ  DT_PROP(DT_NODELABEL(clk_ertco), clock_frequency)
/** @brief Internal PLL (IPLL) frequency, in Hz (0 if absent). */
#define ADI_MAX32_CLK_IPLL_FREQ   DT_PROP_OR(DT_NODELABEL(clk_ipll), clock_frequency, 0)
/** @brief External BLE oscillator (EBO) frequency, in Hz (0 if absent). */
#define ADI_MAX32_CLK_EBO_FREQ    DT_PROP_OR(DT_NODELABEL(clk_ebo), clock_frequency, 0)
/** @brief External clock input frequency, in Hz (0 if absent). */
#define ADI_MAX32_CLK_EXTCLK_FREQ DT_PROP_OR(DT_NODELABEL(clk_extclk), clock_frequency, 0)

/** @cond INTERNAL_HIDDEN */

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
#if DT_SAME_NODE(DT_GCR_CLOCKS_CTRL, DT_NODELABEL(clk_ipll))
#define ADI_MAX32_SYSCLK_SRC  ADI_MAX32_CLK_IPLL
#define ADI_MAX32_SYSCLK_FREQ (ADI_MAX32_CLK_IPLL_FREQ / ADI_MAX32_SYSCLK_PRESCALER)
#endif
#if DT_SAME_NODE(DT_GCR_CLOCKS_CTRL, DT_NODELABEL(clk_ebo))
#define ADI_MAX32_SYSCLK_SRC  ADI_MAX32_CLK_EBO
#define ADI_MAX32_SYSCLK_FREQ (ADI_MAX32_CLK_EBO_FREQ / ADI_MAX32_SYSCLK_PRESCALER)
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
	 : (clk_src) == ADI_MAX32_PRPH_CLK_SRC_IPLL      ? ADI_MAX32_CLK_IPLL_FREQ                 \
	 : (clk_src) == ADI_MAX32_PRPH_CLK_SRC_EBO       ? ADI_MAX32_CLK_EBO_FREQ                  \
							 : 0)

/** @endcond */

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_ADI_MAX32_CLOCK_CONTROL_H_ */
