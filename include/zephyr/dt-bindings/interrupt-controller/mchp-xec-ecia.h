/*
 * Copyright (c) 2021 Microchip Technology
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Microchip XEC ECIA interrupt controller devicetree macros
 * @ingroup dt_mchp_xec_ecia
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_MCHP_XEC_ECIA_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_MCHP_XEC_ECIA_H_

/**
 * @defgroup dt_mchp_xec_ecia Microchip XEC ECIA interrupt controller
 * @brief Devicetree macros for the Microchip XEC ECIA interrupt aggregator.
 * @ingroup devicetree-interrupt_controller
 *
 * Macros for encoding peripheral interrupt information for the <tt>microchip,xec-ecia</tt>
 * compatible Embedded Controller Interrupt Aggregator (ECIA). @ref MCHP_XEC_ECIA() packs the GIRQ
 * number, the source bit position within that GIRQ and the aggregated/direct NVIC inputs into a
 * single 32-bit value; @ref MCHP_XEC_ECIA_GIRQ_ENC() encodes the GIRQ and bit position only. The
 * matching @c MCHP_XEC_ECIA_* accessors extract each field.
 *
 * @code{.dts}
 * &some_device {
 *         girqs = <MCHP_XEC_ECIA(21, 8, 13, 119)>;
 * };
 * @endcode
 * @{
 */

/**
 * @brief Encode a peripheral's GIRQ and GIRQ bit position only.
 *
 * @param g GIRQ number, in the range [8, 26].
 * @param b Source bit position within the GIRQ, in the range [0, 31].
 */
#define MCHP_XEC_ECIA_GIRQ_ENC(g, b) (((g) & 0x1f) + (((b) & 0x1f) << 8))

/**
 * @brief Encode peripheral interrupt information into a 32-bit value.
 *
 * Bits [4:0] hold the GIRQ number (in [8, 26]); bits [12:8] hold the peripheral source bit position
 * (in [0, 31]) within the GIRQ; bits [23:16] hold the aggregated GIRQ NVIC number; bits [31:24]
 * hold the direct NVIC number (for sources without a direct connection, set @p nd equal to @p na).
 *
 * @note GIRQ22 is a peripheral clock wake only. GIRQ22 and its sources are not connected to the
 * NVIC; use 255 for @p na and @p nd.
 *
 * @param g  GIRQ number, in the range [8, 26].
 * @param gb Peripheral source bit position within the GIRQ, in the range [0, 31].
 * @param na Aggregated GIRQ NVIC number.
 * @param nd Direct NVIC number.
 */
#define MCHP_XEC_ECIA(g, gb, na, nd)					\
	(((g) & 0x1f) + (((gb) & 0x1f) << 8) + (((na) & 0xff) << 16) +	\
	(((nd) & 0xff) << 24))

/** @brief Extract the GIRQ number from an encoded MCHP_XEC_ECIA() value. */
#define MCHP_XEC_ECIA_GIRQ(e)		((e) & 0x1f)
/** @brief Extract the source bit position from an encoded MCHP_XEC_ECIA() value. */
#define MCHP_XEC_ECIA_GIRQ_POS(e)	(((e) >> 8) & 0x1f)
/** @brief Extract the aggregated GIRQ NVIC number from an encoded MCHP_XEC_ECIA() value. */
#define MCHP_XEC_ECIA_NVIC_AGGR(e)	(((e) >> 16) & 0xff)
/** @brief Extract the direct NVIC number from an encoded MCHP_XEC_ECIA() value. */
#define MCHP_XEC_ECIA_NVIC_DIRECT(e)	(((e) >> 24) & 0xff)

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_MCHP_XEC_ECIA_H_ */
