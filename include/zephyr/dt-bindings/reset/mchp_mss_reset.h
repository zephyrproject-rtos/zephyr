/*
 * Copyright (C) 2025 embedded brains GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_RESET_MCHP_MSS_RESET_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_RESET_MCHP_MSS_RESET_H_

/*
 * The reset ID encodes the bit index of the SUBBLK_CLOCK_CR and SOFT_RESET_CR
 * registers associated with the device.
 */
#define MSS_RESET_ID_ENVM    0x0
#define MSS_RESET_ID_MAC0    0x1
#define MSS_RESET_ID_MAC1    0x2
#define MSS_RESET_ID_MMC     0x3
#define MSS_RESET_ID_TIMER   0x4
#define MSS_RESET_ID_MMUART0 0x5
#define MSS_RESET_ID_MMUART1 0x6
#define MSS_RESET_ID_MMUART2 0x7
#define MSS_RESET_ID_MMUART3 0x8
#define MSS_RESET_ID_MMUART4 0x9
#define MSS_RESET_ID_SPI0    0xa
#define MSS_RESET_ID_SPI1    0xb
#define MSS_RESET_ID_I2C0    0xc
#define MSS_RESET_ID_I2C1    0xd
#define MSS_RESET_ID_CAN0    0xe
#define MSS_RESET_ID_CAN1    0xf
#define MSS_RESET_ID_USB     0x10
#define MSS_RESET_ID_RSVD    0x11
#define MSS_RESET_ID_RTC     0x12
#define MSS_RESET_ID_QSPI    0x13
#define MSS_RESET_ID_GPIO0   0x14
#define MSS_RESET_ID_GPIO1   0x15
#define MSS_RESET_ID_GPIO2   0x16
#define MSS_RESET_ID_DDRC    0x17
#define MSS_RESET_ID_FIC0    0x18
#define MSS_RESET_ID_FIC1    0x19
#define MSS_RESET_ID_FIC2    0x1a
#define MSS_RESET_ID_FIC3    0x1b
#define MSS_RESET_ID_ATHENA  0x1c
#define MSS_RESET_ID_CFM     0x1d

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_RESET_MCHP_MSS_RESET_H_ */
