/*
 * Copyright (c) 2024 Croxel Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the MEMSIC MC3419 accelerometer.
 * @ingroup mc3419_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_MC3419_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_MC3419_H_

/**
 * @defgroup mc3419_interface MC3419
 * @ingroup sensor_interface_ext_memsic
 * @brief MEMSIC MC3419 3-axis accelerometer
 * @{
 */

/**
 * @name Low-pass filter options
 *
 * Values for the `lpf-fc-sel` devicetree property.
 *
 * Enabled options set the filter corner frequency (Fc) to a fraction of the
 * internal data rate (IDR).
 * @{
 */
#define MC3419_LPF_DISABLE                 0  /**< Filter disabled */
#define MC3419_LPF_EN_WITH_IDR_BY_4p255_FC 9  /**< Fc = IDR / 4.255 */
#define MC3419_LPF_EN_WITH_IDR_BY_6_FC     10 /**< Fc = IDR / 6 */
#define MC3419_LPF_EN_WITH_IDR_BY_12_FC    11 /**< Fc = IDR / 12 */
#define MC3419_LPF_EN_WITH_IDR_BY_16_FC    13 /**< Fc = IDR / 16 */
/** @} */

/**
 * @name Decimation rate options
 *
 * Values for the `decimation-rate` devicetree property.
 *
 * Divide the internal data rate (IDR) by the selected factor.
 * @{
 */
#define MC3419_DECIMATE_IDR_BY_1    0  /**< IDR / 1 */
#define MC3419_DECIMATE_IDR_BY_2    1  /**< IDR / 2 */
#define MC3419_DECIMATE_IDR_BY_4    2  /**< IDR / 4 */
#define MC3419_DECIMATE_IDR_BY_5    3  /**< IDR / 5 */
#define MC3419_DECIMATE_IDR_BY_8    4  /**< IDR / 8 */
#define MC3419_DECIMATE_IDR_BY_10   5  /**< IDR / 10 */
#define MC3419_DECIMATE_IDR_BY_16   6  /**< IDR / 16 */
#define MC3419_DECIMATE_IDR_BY_20   7  /**< IDR / 20 */
#define MC3419_DECIMATE_IDR_BY_40   8  /**< IDR / 40 */
#define MC3419_DECIMATE_IDR_BY_67   9  /**< IDR / 67 */
#define MC3419_DECIMATE_IDR_BY_80   10 /**< IDR / 80 */
#define MC3419_DECIMATE_IDR_BY_100  11 /**< IDR / 100 */
#define MC3419_DECIMATE_IDR_BY_200  12 /**< IDR / 200 */
#define MC3419_DECIMATE_IDR_BY_250  13 /**< IDR / 250 */
#define MC3419_DECIMATE_IDR_BY_500  14 /**< IDR / 500 */
#define MC3419_DECIMATE_IDR_BY_1000 15 /**< IDR / 1000 */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_MC3419_H_ */
