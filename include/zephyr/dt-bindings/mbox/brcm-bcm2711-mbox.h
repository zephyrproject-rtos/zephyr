/*
 * Copyright (c) 2026 Muhammad Waleed Badar
 * SPDX-License-Identifier: Apache-2.0
 *
 * BCM2711 VideoCore mailbox channel definitions.
 *
 * Reference:
 *   https://github.com/raspberrypi/firmware/wiki/Mailboxes
 */

/**
 * @file
 * @brief Channel definitions for Broadcom BCM2711 mailbox controller
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_MBOX_BRCM_BCM2711_MBOX_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_MBOX_BRCM_BCM2711_MBOX_H_

/** Power management */
#define BCM2711_MBOX0_CH_POWER_MGMT   0
/** Framebuffer */
#define BCM2711_MBOX0_CH_FRAMEBUFFER  1
/** Virtual UART */
#define BCM2711_MBOX0_CH_VIRTUAL_UART 2
/** VCHIQ IPC */
#define BCM2711_MBOX0_CH_VCHIQ        3
/** On-board LEDs */
#define BCM2711_MBOX0_CH_LEDS         4
/** Physical buttons */
#define BCM2711_MBOX0_CH_BUTTONS      5
/** Touch screen */
#define BCM2711_MBOX0_CH_TOUCHSCREEN  6
/** Reserved, do not use */
#define BCM2711_MBOX0_CH_RESERVED     7
/** ARM -> VC */
#define BCM2711_MBOX0_CH_ARM_TO_VC    8
/** VC -> ARM */
#define BCM2711_MBOX0_CH_VC_TO_ARM    9

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_MBOX_BRCM_BCM2711_MBOX_H_ */
