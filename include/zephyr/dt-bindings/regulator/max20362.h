/*
 * Copyright (c) 2025 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup regulator_max20362
 * @brief Header file for MAX20362 Devicetree helpers.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_MAX20362_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_MAX20362_H_

/**
 * @defgroup regulator_max20362 MAX20362 Devicetree helpers
 * @brief Analog Devices MAX20362 PMIC regulator driver Devicetree helpers
 * @ingroup devicetree-regulator
 * @{
 */

/**
 * @name MAX20362 BAT to BBIN voltage drop
 * @{
 */
/** 55 mV drop */
#define MAX20362_BAT_BBIN_VDROP_55MV  0
/** 100 mV drop */
#define MAX20362_BAT_BBIN_VDROP_100MV 1
/** 150 mV drop */
#define MAX20362_BAT_BBIN_VDROP_150MV 2
/** 200 mV drop */
#define MAX20362_BAT_BBIN_VDROP_200MV 3
/** @} */

/**
 * @name MAX20362 DVS interface source
 * @{
 */
/** I2C interface */
#define MAX20362_DVS_SRC_I2C         0
/** Pseudo-SPI interface */
#define MAX20362_DVS_SRC_PSEUDO_SPI  1
/** Round-Robin interface */
#define MAX20362_DVS_SRC_ROUND_ROBIN 2
/** @} */

/**
 * @name MAX20362 LDO input source
 * @{
 */
/** Buck-boost output (BBOUT) */
#define MAX20362_LDO_SRC_BBOUT 0
/** Charge pump output (CAP) */
#define MAX20362_LDO_SRC_CAP   1
/** Battery (BATT) */
#define MAX20362_LDO_SRC_BATT  2
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_MAX20362_H_ */
