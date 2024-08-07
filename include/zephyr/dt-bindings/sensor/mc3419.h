/*
 * Copyright (c) 2024 Croxel Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_MEMSIC_MC3419_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_MEMSIC_MC3419_H_

/**
 * @defgroup MC3419 memsic DT Options
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

/** @} */

#endif /*ZEPHYR_INCLUDE_DT_BINDINGS_MEMSIC_MC3419_H_ */
