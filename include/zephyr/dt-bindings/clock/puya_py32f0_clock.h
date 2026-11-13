/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Clock specifiers for Puya PY32F0 devices.
 * @ingroup puya_py32f0_clocks
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_PUYA_PY32F0_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_PUYA_PY32F0_CLOCK_H_

/**
 * @defgroup puya_py32f0_clocks Puya PY32F0 clock specifiers
 * @ingroup devicetree-clocks
 *
 * Use the bus names IOP, AHB1, APB1 and APB2 with @ref PY32_CLOCK to encode
 * peripheral clock specifiers. Bus constants are byte offsets from the RCC base
 * address to the corresponding clock-enable register.
 * @{
 */

/**
 * @brief Encode a PY32 peripheral clock specifier.
 * @param bus RCC bus name (IOP, AHB1, APB1 or APB2).
 * @param bit Peripheral bit number, from 0 through 31.
 * @return Two devicetree cells containing the enable-register offset and bit mask.
 */
#define PY32_CLOCK(bus, bit) (PY32_CLOCK_BUS_##bus)(1U << (bit))

/** RCC I/O port clock-enable register offset, in bytes from the RCC base. */
#define PY32_CLOCK_BUS_IOP  0x034
/** RCC AHB clock-enable register offset, in bytes from the RCC base. */
#define PY32_CLOCK_BUS_AHB1 0x038
/** RCC APB1 clock-enable register offset, in bytes from the RCC base. */
#define PY32_CLOCK_BUS_APB1 0x03c
/** RCC APB2 clock-enable register offset, in bytes from the RCC base. */
#define PY32_CLOCK_BUS_APB2 0x040

/** Lowest RCC clock-enable register offset accepted by the clock driver. */
#define PY32_PERIPH_BUS_MIN PY32_CLOCK_BUS_IOP
/** Highest RCC clock-enable register offset accepted by the clock driver. */
#define PY32_PERIPH_BUS_MAX PY32_CLOCK_BUS_APB2

/** Identifier for the internal high-speed oscillator as an RCC clock source. */
#define PY32_SRC_HSI 0U

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_PUYA_PY32F0_CLOCK_H_ */
