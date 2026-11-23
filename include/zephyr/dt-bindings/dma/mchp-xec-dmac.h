/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_DMA_MCHP_XEC_DMAC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_DMA_MCHP_XEC_DMAC_H_

/**
 * @brief Vendor-specific DMA peripheral triggering sources.
 *
 * A valid triggering source should be provided when DMA
 * is configured for peripheral to peripheral or memory to peripheral
 * transactions.
 */
#define DMA_MCHP_XEC_TRIG_SRC_I2C_SMB0_TARG 0x0
#define DMA_MCHP_XEC_TRIG_SRC_I2C_SMB0_CTRL 0x1
#define DMA_MCHP_XEC_TRIG_SRC_I2C_SMB1_TARG 0x2
#define DMA_MCHP_XEC_TRIG_SRC_I2C_SMB1_CTRL 0x3
#define DMA_MCHP_XEC_TRIG_SRC_I2C_SMB2_TARG 0x4
#define DMA_MCHP_XEC_TRIG_SRC_I2C_SMB2_CTRL 0x5
#define DMA_MCHP_XEC_TRIG_SRC_I2C_SMB3_TARG 0x6
#define DMA_MCHP_XEC_TRIG_SRC_I2C_SMB3_CTRL 0x7
#define DMA_MCHP_XEC_TRIG_SRC_I2C_SMB4_TARG 0x8
#define DMA_MCHP_XEC_TRIG_SRC_I2C_SMB4_CTRL 0x9
#define DMA_MCHP_XEC_TRIG_SRC_QMSPI0_TX     0xA
#define DMA_MCHP_XEC_TRIG_SRC_QMSPI0_RX     0xB
#define DMA_MCHP_XEC_TRIG_SRC_GPSPI0_TX     0xC
#define DMA_MCHP_XEC_TRIG_SRC_GPSPI0_RX     0xD
#define DMA_MCHP_XEC_TRIG_SRC_GPSPI1_TX     0xE
#define DMA_MCHP_XEC_TRIG_SRC_GPSPI1_RX     0xF
/* Additional channels for MEC175x */
#define DMA_MCHP_XEC_TRIG_SRC_I3C_HC0_TX    0x10
#define DMA_MCHP_XEC_TRIG_SRC_I3C_HC0_RX    0x11
#define DMA_MCHP_XEC_TRIG_SRC_I3C_SC0_TX    0x12
#define DMA_MCHP_XEC_TRIG_SRC_I3C_SC0_RX    0x13

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_DMA_MCHP_XEC_DMAC_H_ */
