/*
 * Copyright (c) 2025 SiFli Technologies(Nanjing) Co., Ltd
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_SF32LB_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_SF32LB_H_

/**
 * @defgroup regulator_sf32lb SF32LB Devicetree helpers.
 * @ingroup regulator_interface
 * @{
 */

/**
 * @name SF32LB Peripheral LDO indices
 * @{
 */
/** LDO18: 1.8V LDO for PSRAM (VDDSIP) */
#define SF32LB_PERI_LDO18      0
/** LDO2_3V3: 3.3V LDO2 for peripherals */
#define SF32LB_PERI_LDO2_3V3   1
/** LDO3_3V3: 3.3V LDO3 for peripherals */
#define SF32LB_PERI_LDO3_3V3   2
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_SF32LB_H_ */
