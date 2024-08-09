/** @file
 * @brief Definitions and non-standard functions for Xilinx AXI DMA.
 */
/*
 * Copyright (c) 2024 CISPA Helmholtz Center for Information Security gGmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef DMA_XILINX_AXI_DMA_H
#define DMA_XILINX_AXI_DMA_H

#define XILINX_AXI_DMA_NUM_CHANNELS   2
#define XILINX_AXI_DMA_TX_CHANNEL_NUM 0
#define XILINX_AXI_DMA_RX_CHANNEL_NUM 1

#define XILINX_AXI_DMA_LINKED_CHANNEL_NO_CSUM_OFFLOAD   0x0
#define XILINX_AXI_DMA_LINKED_CHANNEL_FULL_CSUM_OFFLOAD 0x1

#include <stdint.h>
#include <zephyr/device.h>

/**
 * @brief Returns the size of the last RX transfer conducted by the DMA, based on the descriptor
 * status.
 */
extern uint32_t dma_xilinx_axi_dma_last_received_frame_length(const struct device *dev);

#endif
