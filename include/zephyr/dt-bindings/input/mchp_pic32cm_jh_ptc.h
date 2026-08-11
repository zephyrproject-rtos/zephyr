/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file mchp_pic32cm_jh_ptc.h
 * @brief PTC peripheral definitions for PIC32CM_JH devices.
 *
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_MCHP_PIC32CM_JH_PTC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_MCHP_PIC32CM_JH_PTC_H_

/* QT7 XPlained Pro add-on board Touch Pinout mask
 *
 * The following macros are touch pin mask for the
 * Microchip PTC peripheral, which is required to interface
 * the PIC32CM JH01 Curiosity Pro Evaluation Kit with the
 * QT7 Xplained Pro Extension Kit through the Extension 1 header.
 */
/* Extension Header EXT1 */
#define QT7_PIN3_EXT1   19
#define QT7_PIN4_EXT1   14
#define QT7_PIN7_EXT1   28
#define QT7_PIN8_EXT1   29
#define QT7_PIN9_EXT1   25
#define QT7_PIN10_EXT1  13

/* Extension Header EXT2 */
/* not supported */

/* Extension Header EXT3 */
/* not supported */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_MCHP_PIC32CM_JH_PTC_H_ */
