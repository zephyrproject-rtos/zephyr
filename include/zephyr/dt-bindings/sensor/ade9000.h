/*
 * Copyright (c) 2026 A-Eberle <Daniel.Kampert@a-eberle.de>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree values of the ADE9000, usable from DTS and C.
 */

#pragma once

/**
 * @defgroup ade9000 ADE9000 DT Options
 * @ingroup sensor_interface
 * @{
 */

/**
 * @defgroup ade9000_vconsel Network configuration, ACCMODE.VCONSEL
 * @brief    Voltage channels the IC computes instead of measuring them.
 * @{
 */
/** 4 wire wye, VA, VB and VC are measured. Reset value */
#define ADE9000_DT_VCONSEL_4W_WYE               0
/** 3 wire delta, VB = VA - VC */
#define ADE9000_DT_VCONSEL_3W_DELTA             1
/** 4 wire wye, non Blondel compliant, VB = -VA - VC */
#define ADE9000_DT_VCONSEL_4W_WYE_NON_BLONDEL   2
/** 4 wire delta, non Blondel compliant, VB = -VA */
#define ADE9000_DT_VCONSEL_4W_DELTA_NON_BLONDEL 3
/** 3 wire delta, line to line: VA = VA - VB, VB = VA - VC, VC = VC - VB */
#define ADE9000_DT_VCONSEL_3W_DELTA_LL          4
/** @} */

/**
 * @defgroup ade9000_adc ADCs, bits of disabled-adcs (CHNL_DIS)
 * @brief    Combine with |, e.g. (ADE9000_DT_ADC_IB | ADE9000_DT_ADC_VB).
 * @{
 */
/** Current phase A */
#define ADE9000_DT_ADC_IA (1 << 0)
/** Current phase B */
#define ADE9000_DT_ADC_IB (1 << 1)
/** Current phase C */
#define ADE9000_DT_ADC_IC (1 << 2)
/** Neutral current */
#define ADE9000_DT_ADC_IN (1 << 3)
/** Voltage phase A */
#define ADE9000_DT_ADC_VA (1 << 4)
/** Voltage phase B */
#define ADE9000_DT_ADC_VB (1 << 5)
/** Voltage phase C */
#define ADE9000_DT_ADC_VC (1 << 6)
/** @} */

/**
 * @defgroup ade9000_full_scale Full scale values
 * @brief    Compute voltage-full-scale-microvolt and current-full-scale-microampere.
 *
 * Integer math with 64 bit, usable in DTS and C. Divide the result by the PGA
 * gain when it is not 1.
 * @{
 */
/** Rms register value of a full scale input (707 mV rms at the pins) */
#define ADE9000_DT_FULL_SCALE_RMS    52702092ULL
/** Rms voltage at the pins that drives the ADCs to full scale, in microvolt */
#define ADE9000_DT_FULL_SCALE_PIN_UV 707107ULL

/**
 * Full scale from the weight of the rms registers, in micro units.
 *
 * @param codes_per_unit_milli Rms register value per volt or ampere at the
 *        measurement input, times 1000. E.g. 306653450 for 306653.450 codes/V.
 */
#define ADE9000_DT_FULL_SCALE_FROM_RMS_WEIGHT(codes_per_unit_milli)                                \
	(((ADE9000_DT_FULL_SCALE_RMS * 1000000000ULL) + ((codes_per_unit_milli) / 2)) /            \
	 (codes_per_unit_milli))

/**
 * Voltage full scale behind a resistive divider, in microvolt.
 *
 * @param r_top_ohm    Resistance between the measured voltage and the pin.
 * @param r_bottom_ohm Resistance between the pin and ground.
 */
#define ADE9000_DT_FULL_SCALE_FROM_DIVIDER(r_top_ohm, r_bottom_ohm)                                \
	((ADE9000_DT_FULL_SCALE_PIN_UV * ((r_top_ohm) + (r_bottom_ohm))) / (r_bottom_ohm))

/**
 * Current full scale of a current transformer with a burden resistor, in
 * microampere.
 *
 * @param turns_ratio     Primary to secondary current ratio, e.g. 1000 for 1000:1.
 * @param burden_milliohm Burden resistor in milliohm.
 */
#define ADE9000_DT_FULL_SCALE_FROM_CT(turns_ratio, burden_milliohm)                                \
	((ADE9000_DT_FULL_SCALE_PIN_UV * 1000ULL * (turns_ratio)) / (burden_milliohm))
/** @} */

/**
 * @defgroup ade9000_gain Gain values
 * @brief    Compute the gain registers xIGAIN, xVGAIN, xPGAIN and NIGAIN.
 * @{
 */
/** Register value per unit of gain, 2^27 */
#define ADE9000_DT_GAIN_ONE 134217728LL

/** @cond INTERNAL_HIDDEN */
#define ADE9000_DT_GAIN_ABS_(gain_micro)                                                           \
	((((gain_micro) * ADE9000_DT_GAIN_ONE) + 500000LL) / 1000000LL)
/** @endcond */

/**
 * Gain register value (gain - 1) * 2^27, rounded, for a-igains, vgains etc.
 *
 * Works with the unsigned arithmetic of dtc too, the sign is taken from bit 63.
 *
 * @param gain_micro Gain times 1000000, e.g. 2000000 for a gain of 2. Range
 *        -15000000 to below 17000000, negative values invert the phase.
 */
#define ADE9000_DT_GAIN(gain_micro)                                                                \
	(((((gain_micro) * 1LL) >> 63) ? -ADE9000_DT_GAIN_ABS_(-(gain_micro))                      \
				       : ADE9000_DT_GAIN_ABS_(gain_micro)) -                       \
	 ADE9000_DT_GAIN_ONE)
/** @} */

/** @} */
