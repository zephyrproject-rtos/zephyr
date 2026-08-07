/*
 * Copyright (c) 2023 The Zephyr Contributors.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Relevant documents:
 * - BQ27421
 *   Datasheet: https://www.ti.com/lit/gpn/bq27421-g1
 *   Technical reference manual: https://www.ti.com/lit/pdf/sluuac5
 * - BQ27427
 *   Datasheet: https://www.ti.com/lit/gpn/bq27427
 *   Technical reference manual: https://www.ti.com/lit/pdf/sluucd5
 */

/**
 * @file
 * @brief Devicetree binding constants for the TI BQ274xx fuel gauges.
 * @ingroup bq274xx_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_BQ274XX_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_BQ274XX_H_

/**
 * @defgroup bq274xx_interface BQ274XX
 * @ingroup sensor_interface_ext_ti
 * @brief Texas Instruments BQ274xx battery fuel gauges
 * @{
 */

/**
 * @name BQ27427 chemistry IDs
 *
 * Values for the `chemistry-id` devicetree property.
 * @{
 */
#define BQ27427_CHEM_ID_A 0x3230 /**< Chemistry profile A (4.35 V) */
#define BQ27427_CHEM_ID_B 0x1202 /**< Chemistry profile B (4.2 V, default) */
#define BQ27427_CHEM_ID_C 0x3142 /**< Chemistry profile C (4.4 V) */
/** @} */

/**
 * @name BQ27421 chemistry IDs
 *
 * Values for the `chemistry-id` devicetree property.
 * @{
 */
#define BQ27421_G1A_CHEM_ID 0x0128 /**< BQ27421-G1A chemistry ID */
#define BQ27421_G1B_CHEM_ID 0x0312 /**< BQ27421-G1B chemistry ID */
#define BQ27421_G1D_CHEM_ID 0x3142 /**< BQ27421-G1D chemistry ID */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_BQ274XX_H_ */
