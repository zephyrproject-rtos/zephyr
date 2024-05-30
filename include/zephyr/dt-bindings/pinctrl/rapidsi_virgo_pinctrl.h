/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RAPIDSI_VIRGO_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RAPIDSI_VIRGO_PINCTRL_H_

/**
 * @brief Pin modes
 */

#define PIN_FN_MAIN         0x0
#define PIN_FN_FPGA         0x1
#define PIN_FN_ALT          0x2
#define PIN_FN_DEBUG        0x3

/**
 * @brief Pin Pull Status Bit Values
 */

#define PIN_PULL_STATUS_DIS 0x0
#define PIN_PULL_STATUS_EN  0x1

/**
 * @brief Pin Pull Direction Bit Values
 */

#define PIN_PULL_DIR_DOWN   0x0
#define PIN_PULL_DIR_UP     0x1

/**
 * @brief PinCfg Offsets
 */

#define PIN_EN_OFFSET           0x0
#define PIN_PULL_STATE_OFFSET   0x5
#define PIN_PULL_DIR_OFFSET     0x6
#define PIN_MODE_OFFSET         0x7

/**
 * @brief PinCfg Bit Masks
 */

#define PIN_EN_MASK         0x1
#define PIN_PULL_STATE_MASK 0x20
#define PIN_PULL_DIR_MASK   0x40
#define PIN_MODE_OFFSET     0x180

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RAPIDSI_VIRGO_PINCTRL_H_ */