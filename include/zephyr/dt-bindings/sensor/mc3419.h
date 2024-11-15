/*
 * Copyright (c) 2024 Croxel Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_MEMSIC_MC3419_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_MEMSIC_MC3419_H_

/**
 * @defgroup MC3419 Memsic DT Options
 * @ingroup sensor_interface
 * @{
 */

/**
 * @defgroup MC3419_LPF_CONFIGS Lowe pass filter configurations
 * @{
 */
#define MC3419_LPF_DISABLE                 0
#define MC3419_LPF_EN_WITH_IDR_BY_4p255_FC 9
#define MC3419_LPF_EN_WITH_IDR_BY_6_FC     10
#define MC3419_LPF_EN_WITH_IDR_BY_12_FC    11
#define MC3419_LPF_EN_WITH_IDR_BY_16_FC    13
/** @} */

/**
 * @defgroup MC3419_DECIMATION_RATES decimate sampling rate by provided rate
 * @{
 */
#define MC3419_DECIMATE_IDR_BY_1    0
#define MC3419_DECIMATE_IDR_BY_2    1
#define MC3419_DECIMATE_IDR_BY_4    2
#define MC3419_DECIMATE_IDR_BY_5    3
#define MC3419_DECIMATE_IDR_BY_8    4
#define MC3419_DECIMATE_IDR_BY_10   5
#define MC3419_DECIMATE_IDR_BY_16   6
#define MC3419_DECIMATE_IDR_BY_20   7
#define MC3419_DECIMATE_IDR_BY_40   8
#define MC3419_DECIMATE_IDR_BY_67   9
#define MC3419_DECIMATE_IDR_BY_80   10
#define MC3419_DECIMATE_IDR_BY_100  11
#define MC3419_DECIMATE_IDR_BY_200  12
#define MC3419_DECIMATE_IDR_BY_250  13
#define MC3419_DECIMATE_IDR_BY_500  14
#define MC3419_DECIMATE_IDR_BY_1000 15
/** @} */

/** @} */

#endif /*ZEPHYR_INCLUDE_DT_BINDINGS_MEMSIC_MC3419_H_ */
