/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for BQ27427 fuel gauge
 * @ingroup fuel_gauge_bq27427_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_FUEL_GAUGE_BQ27427_H_
#define ZEPHYR_INCLUDE_DRIVERS_FUEL_GAUGE_BQ27427_H_

/**
 * @defgroup fuel_gauge_bq27427_interface BQ27427
 * @ingroup fuel_gauge_interface_ext
 * @brief Texas Instruments BQ27427 single-cell battery fuel gauge.
 *
 * @see BQ27427 Technical Reference Manual:
 * https://www.ti.com/lit/ug/sluucd5/sluucd5.pdf
 *
 * @{
 */

/**
 * @name Custom flags for BQ27427
 *
 * Bit definitions for the flags reported by the BQ27427 fuel gauge.
 * @{
 */

/** Over-temperature condition detected. */
#define BQ27427_FLAG_OT          BIT(15)
/** Under-temperature condition detected. */
#define BQ27427_FLAG_UT          BIT(14)
/** Full charge detected. */
#define BQ27427_FLAG_FC          BIT(9)
/** Fast charging allowed. */
#define BQ27427_FLAG_CHG         BIT(8)
/** Open-circuit voltage measurement taken. */
#define BQ27427_FLAG_OCVTAKEN    BIT(7)
/** Depth-of-discharge correction is currently applied. */
#define BQ27427_FLAG_DOD_CORRECT BIT(6)
/** Initialization or reset occurred. */
#define BQ27427_FLAG_ITPOR       BIT(5)
/** Fuel gauge is operating in configuration update mode. */
#define BQ27427_FLAG_CFGUPMODE   BIT(4)
/** Battery detected. Gauge predictions are invalid if this bit is clear. */
#define BQ27427_FLAG_BAT_DET     BIT(3)
/** State of charge is below the SOC1 threshold. */
#define BQ27427_FLAG_SOC1        BIT(2)
/** State of charge is below the SOCF threshold. */
#define BQ27427_FLAG_SOCF        BIT(1)
/** Discharging condition detected. */
#define BQ27427_FLAG_DSG         BIT(0)

/** @} */

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_FUEL_GAUGE_BQ27427_H_ */
