/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Microchip Technology Inc.
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
 *
 * Note.
 *  QT7_PINx
 *    x refers to the QT7 XPlained Pro pin number, not the EVK board header pin.
 *
 *  #define QT7_PINx y
 *    y refers to the PTC node number
 */

/* Extension Header EXT1 */
#define QT7_PIN3   19
#define QT7_PIN4   14
#define QT7_PIN7   28
#define QT7_PIN8   29
#define QT7_PIN9   25
#define QT7_PIN10  13

/* The following macros contain the values described below:
 *
 * ext1_header: Extension Header 1 node defined in the DTS.
 * pin number: Header pin number.
 * gpio flag: GPIO configuration flag.
 *
 * Note: Each value should be separated by a space.
 */
#define QT7_LED_BUTTON1    &ext1_header  3 GPIO_ACTIVE_LOW
#define QT7_LED_BUTTON2    &ext1_header  8 GPIO_ACTIVE_LOW
#define QT7_LED0_SCROLLER  &ext1_header 15 GPIO_ACTIVE_LOW
#define QT7_LED1_SCROLLER  &ext1_header 14 GPIO_ACTIVE_LOW
#define QT7_LED2_SCROLLER  &ext1_header 13 GPIO_ACTIVE_LOW
#define QT7_LED3_SCROLLER  &ext1_header 12 GPIO_ACTIVE_LOW
#define QT7_LED4_SCROLLER  &ext1_header  9 GPIO_ACTIVE_LOW
#define QT7_LED5_SCROLLER  &ext1_header  2 GPIO_ACTIVE_LOW

/* Extension Header EXT2 */
/* not supported */

/* Extension Header EXT3 */
/* not supported */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_MCHP_PIC32CM_JH_PTC_H_ */
