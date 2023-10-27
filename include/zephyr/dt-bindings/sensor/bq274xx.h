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
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_BQ274XX_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_BQ274XX_H_

/* Chemistry IDs for BQ27427 */
#define BQ27427_CHEM_ID_A 0x3230
#define BQ27427_CHEM_ID_B 0x1202
#define BQ27427_CHEM_ID_C 0x3142

/* Chemistry IDs for BQ27421 variants */
#define BQ27421_G1A_CHEM_ID 0x0128
#define BQ27421_G1B_CHEM_ID 0x0312
#define BQ27421_G1D_CHEM_ID 0x3142

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_BQ274XX_H_ */
