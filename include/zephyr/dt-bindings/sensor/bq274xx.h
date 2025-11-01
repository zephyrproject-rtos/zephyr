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
 * @brief Header file for BQ274XX Devicetree constants
 * @ingroup bq274xx_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_BQ274XX_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_BQ274XX_H_

/**
 * @brief BQ274XX Fuel Gauge
 * @defgroup bq274xx_interface BQ274XX
 * @ingroup sensor_interface_ext
 * @{
 */

/**
 * @name Chemistry profiles for BQ27427
 * @{
 */
#define BQ27427_CHEM_ID_A 0x3230 /**< Profile A (4.35 V Li-ion) */
#define BQ27427_CHEM_ID_B 0x1202 /**< Profile B (4.2 V Li-ion) */
#define BQ27427_CHEM_ID_C 0x3142 /**< Profile C (4.4 V Li-ion) */
/** @} */

/**
 * @name Chemistry profiles for BQ27421 variants
 * @{
 */
#define BQ27421_G1A_CHEM_ID 0x0128 /**< Profile A (4.2 V maximum charge voltage) */
#define BQ27421_G1B_CHEM_ID 0x0312 /**< Profile B (4.3 to 4.35 V maximum charge voltage) */
#define BQ27421_G1D_CHEM_ID 0x3142 /**< Profile D (4.3 to 4.4 V maximum charge voltage) */
/**
 * @}
 */

 /**
  * @}
  */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_BQ274XX_H_ */
