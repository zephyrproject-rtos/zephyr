/* SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors */
/*
 * Copyright (c) 2026 AMD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_DMA_AMD_ACP_HOST_H_
#define ZEPHYR_DRIVERS_DMA_AMD_ACP_HOST_H_

#include <zephyr/sys/atomic.h>
#include <zephyr/drivers/dma.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ACP_MAX_STREAMS           8
#define ACP_DMA_BASE_CTRL         ACP_DMA_CNTL_0
#define ACP_DMA_BASE_STAT         ACP_DMA_CH_STS
#define ACP_DMA_CHAN_SIZE         0x4
#define ACP_DMA_CHAN_OFFSET(chan) (ACP_DMA_CHAN_SIZE * (chan))
#define ACP_HOST_DMA_CTRL(chan)   (ACP_DMA_CHAN_OFFSET(chan))
#define ACP_HOST_DMA_STAT(chan)   ACP_DMA_BASE_STAT
#define ACP_PROBE_CHANNEL         7
#define ACP_DMA_START_TIMEOUT_US  500
#define ACP_SYST_MEM_ADDR_MASK    0x000FFFFF
#define ACP_DSP_DRAM_BASE         0x9C700000

#ifdef __cplusplus
}
#endif
#endif /* ZEPHYR_DRIVERS_DMA_AMD_ACP_HOST_H_ */
