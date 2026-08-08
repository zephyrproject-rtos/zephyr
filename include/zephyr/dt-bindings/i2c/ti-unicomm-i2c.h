/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_TI_UNICOMM_I2C_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_TI_UNICOMM_I2C_H_

/* I2C Controller Mode FIFO trigger levels */

#define I2CC_TX_FIFO_LEVEL_3_4          0x00000001U /* TX FIFO <= 3/4 empty */
#define I2CC_TX_FIFO_LEVEL_1_2          0x00000002U /* TX FIFO <= 1/2 empty (default) */
#define I2CC_TX_FIFO_LEVEL_1_4          0x00000003U /* TX FIFO <= 1/4 empty */
#define I2CC_TX_FIFO_LEVEL_NOT_FULL     0x00000004U /* Opposite of full */
#define I2CC_TX_FIFO_LEVEL_EMPTY        0x00000005U /* TX FIFO is empty */
#define I2CC_TX_FIFO_LEVEL_ALMOST_EMPTY 0x00000006U /* TX FIFO <= 1 */
#define I2CC_TX_FIFO_LEVEL_ALMOST_FULL  0x00000007U /* TX_FIFO >= (MAX_FIFO_LEN-1) */

/* I2C Target Mode FIFO trigger levels */

#define I2CT_RX_FIFO_LEVEL_1_4          0x00000010U /* RX FIFO >= 1/4 full */
#define I2CT_RX_FIFO_LEVEL_1_2          0x00000020U /* RX FIFO >= 1/2 full (default) */
#define I2CT_RX_FIFO_LEVEL_3_4          0x00000030U /* RX FIFO >= 3/4 full */
#define I2CT_RX_FIFO_LEVEL_NOT_EMPTY    0x00000040U /* Opposite of empty */
#define I2CT_RX_FIFO_LEVEL_FULL         0x00000050U /* RX FIFO is full */
#define I2CT_RX_FIFO_LEVEL_ALMOST_FULL  0x00000060U /* RX_FIFO >= (MAX_FIFO_LEN-1) */
#define I2CT_RX_FIFO_LEVEL_ALMOST_EMPTY 0x00000070U /* RX_FIFO <= 1 */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_TI_UNICOMM_I2C_H_ */
