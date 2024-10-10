/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_NPM2100_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_NPM2100_H_

/**
 * @defgroup regulator_npm2100 NPM2100 Devicetree helpers.
 * @ingroup regulator_interface
 * @{
 */

/**
 * @name NPM2100 Regulator modes
 * @{
 */
/* Load switch selection, applies to LDOSW only */
#define NPM2100_REG_LDSW_EN   0x01U

/* DPS enable, applies to BOOST only */
#define NPM2100_REG_DPS_EN    0x01U

/* Operating mode */
#define NPM2100_REG_OPER_MASK 0x0EU
#define NPM2100_REG_OPER_AUTO 0x00U
#define NPM2100_REG_OPER_HP   0x02U
#define NPM2100_REG_OPER_LP   0x04U
#define NPM2100_REG_OPER_ULP  0x06U
#define NPM2100_REG_OPER_PASS 0x08U
#define NPM2100_REG_OPER_NOHP 0x0AU
#define NPM2100_REG_OPER_OFF  0x0CU

/* Forced mode when GPIO active */
#define NPM2100_REG_FORCE_MASK 0x70U
#define NPM2100_REG_FORCE_HP   0x10U
#define NPM2100_REG_FORCE_LP   0x20U
#define NPM2100_REG_FORCE_ULP  0x30U
#define NPM2100_REG_FORCE_PASS 0x40U
#define NPM2100_REG_FORCE_NOHP 0x50U

/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_NPM2100_H_*/
