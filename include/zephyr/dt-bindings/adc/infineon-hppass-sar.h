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
 * Values match the PDL enums, so the driver passes them to the hardware as is.
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
#define IFX_HPPASS_SAR_TRIG_AC       9 /**< Autonomous controller */
/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ADC_INFINEON_HPPASS_SAR_H_ */
