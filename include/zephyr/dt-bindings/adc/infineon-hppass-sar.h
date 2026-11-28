/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the Infineon HPPASS SAR ADC.
 *
 * Trigger values follow the HPPASS v3 cy_en_hppass_sar_trig_t numbering. On
 * HPPASS v1, which has no TRIG_8-15 and encodes the AC as 9, the driver
 * remaps IFX_HPPASS_SAR_TRIG_AC.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_ADC_INFINEON_HPPASS_SAR_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_ADC_INFINEON_HPPASS_SAR_H_

/**
 * @name Sequencer group trigger source (cy_en_hppass_sar_trig_t)
 * @{
 */
#define IFX_HPPASS_SAR_TRIG_DISABLED 0 /**< Group is not triggered */
#define IFX_HPPASS_SAR_TRIG_0        1 /**< HPPASS trigger input 0 */
#define IFX_HPPASS_SAR_TRIG_1        2 /**< HPPASS trigger input 1 */
#define IFX_HPPASS_SAR_TRIG_2        3 /**< HPPASS trigger input 2 */
#define IFX_HPPASS_SAR_TRIG_3        4 /**< HPPASS trigger input 3 */
#define IFX_HPPASS_SAR_TRIG_4        5 /**< HPPASS trigger input 4 */
#define IFX_HPPASS_SAR_TRIG_5        6 /**< HPPASS trigger input 5 */
#define IFX_HPPASS_SAR_TRIG_6        7 /**< HPPASS trigger input 6 */
#define IFX_HPPASS_SAR_TRIG_7        8 /**< HPPASS trigger input 7 */
#define IFX_HPPASS_SAR_TRIG_8        9 /**< HPPASS trigger input 8 (HPPASS v3 only) */
#define IFX_HPPASS_SAR_TRIG_9        10 /**< HPPASS trigger input 9 (HPPASS v3 only) */
#define IFX_HPPASS_SAR_TRIG_10       11 /**< HPPASS trigger input 10 (HPPASS v3 only) */
#define IFX_HPPASS_SAR_TRIG_11       12 /**< HPPASS trigger input 11 (HPPASS v3 only) */
#define IFX_HPPASS_SAR_TRIG_12       13 /**< HPPASS trigger input 12 (HPPASS v3 only) */
#define IFX_HPPASS_SAR_TRIG_13       14 /**< HPPASS trigger input 13 (HPPASS v3 only) */
#define IFX_HPPASS_SAR_TRIG_14       15 /**< HPPASS trigger input 14 (HPPASS v3 only) */
#define IFX_HPPASS_SAR_TRIG_15       16 /**< HPPASS trigger input 15 (HPPASS v3 only) */
#define IFX_HPPASS_SAR_TRIG_AC       17 /**< Autonomous controller */
/** @} */

/**
 * @name FIFO buffer for a channel's fifo-sel (cy_en_hppass_fifo_t)
 * @{
 */
#define IFX_HPPASS_SAR_FIFO_DISABLED 0 /**< Result is not written to the FIFO */
#define IFX_HPPASS_SAR_FIFO_0        1 /**< FIFO buffer 0 */
#define IFX_HPPASS_SAR_FIFO_1        2 /**< FIFO buffer 1 */
#define IFX_HPPASS_SAR_FIFO_2        3 /**< FIFO buffer 2 */
#define IFX_HPPASS_SAR_FIFO_3        4 /**< FIFO buffer 3 */
/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ADC_INFINEON_HPPASS_SAR_H_ */
