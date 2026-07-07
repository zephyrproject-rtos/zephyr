/*
 * Copyright 2026 Arm Limited and/or its affiliates <open-source-office@arm.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define MUSCA_B1_ALT_FUNC_POS  0
#define MUSCA_B1_ALT_FUNC_MASK 0x1

#define MUSCA_B1_EXP_NUM_POS  4
#define MUSCA_B1_EXP_NUM_MASK 0xF

/**
 * The Musca-B1 GPIO only has one alternate function: this macro specifies that bit 'exp_num' should
 * use the alternate function.
 */
#define MUSCA_B1_PINMUX(exp_num) (exp_num << MUSCA_B1_EXP_NUM_POS | 1 << MUSCA_B1_ALT_FUNC_POS)

#define UART0_RXD_EXP MUSCA_B1_PINMUX(0)
#define UART0_TXD_EXP MUSCA_B1_PINMUX(1)
#define MR_I2S_SD     MUSCA_B1_PINMUX(2)
#define MR_I2S_WS     MUSCA_B1_PINMUX(3)
#define MR_I2S_SCK    MUSCA_B1_PINMUX(4)
#define MT_I2S_SD0    MUSCA_B1_PINMUX(6)
#define MT_I2S_WSO    MUSCA_B1_PINMUX(5)
#define MT_I2S_SD1    MUSCA_B1_PINMUX(7)
#define MT_I2S_WS1    MUSCA_B1_PINMUX(8)
#define MT_I2S_SCK    MUSCA_B1_PINMUX(9)
#define MR_SPIO_NSS0  MUSCA_B1_PINMUX(10)
#define MR_SPIO_MOSI  MUSCA_B1_PINMUX(11)
#define MR_SPIO_MISO  MUSCA_B1_PINMUX(12)
#define MR_SPIO_SCK   MUSCA_B1_PINMUX(13)
#define MR_I2C0_DATA  MUSCA_B1_PINMUX(14)
#define MR_I2C0_CLOCK MUSCA_B1_PINMUX(15)
