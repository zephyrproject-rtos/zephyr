/*
 * Private Porting
 * by David Hor - Xtooltech 2025
 * david.hor@xtooltech.com
 */

/*
 * Copyright (c) 2024 VCI Development
 * SPDX-License-Identifier: Apache-2.0
 *
 * This file was automatically generated. Do not edit.
 */

#ifndef _NXP_LPC_LPC54S018J4MET180E_PINCTRL_H_
#define _NXP_LPC_LPC54S018J4MET180E_PINCTRL_H_

/*
 * LPC54S018J4MET180E pin control definitions for Zephyr
 * Based on the NXP LPC54S018 datasheet and verified against baremetal projects.
 */

/* FLEXCOMM0 (UART) pins */
#define FC0_RXD_SCL_MISO_WS_PIO0_29    0x1D
#define FC0_TXD_SDA_MOSI_DATA_PIO0_30  0x1E

/* FLEXCOMM4 (I2C) pins */
#define FC4_RXD_SCL_MISO_WS_PIO0_25    0x19
#define FC4_TXD_SDA_MOSI_DATA_PIO0_26  0x1A

/* FLEXCOMM5 (SPI) pins */
#define FC5_SCK_PIO0_19                0x13
#define FC5_RXD_SCL_MISO_WS_PIO0_18    0x12
#define FC5_TXD_SDA_MOSI_DATA_PIO0_20  0x14
#define FC5_CTS_SDA_SSEL0_PIO1_1       0x21

/* FLEXCOMM9 (CAN) pins */
#define FC9_CTS_SDA_SSEL0_PIO4_0       0x80
#define FC9_RTS_SCL_SSEL1_PIO4_1       0x81

/* SPIFI pins - Function 6 configuration per baremetal project */
#define PIO0_23    0x17  /* SPIFI_CS */
#define PIO0_24    0x18  /* SPIFI_IO0 */
#define PIO0_25    0x19  /* SPIFI_IO1 */
#define PIO0_26    0x1A  /* SPIFI_CLK */
#define PIO0_27    0x1B  /* SPIFI_IO3 */
#define PIO0_28    0x1C  /* SPIFI_IO2 */

#endif /* _NXP_LPC_LPC54S018J4MET180E_PINCTRL_H_ */