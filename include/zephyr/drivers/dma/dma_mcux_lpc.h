/*
 * Copyright 2020 Broadcom
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DMA_MCUX_LPC_H
#define DMA_MCUX_LPC_H

#include <zephyr/drivers/dma.h>
#if defined(CONFIG_DMA)
#include <fsl_dma.h>

#define DMA_MAX_TRANS_NUM (1024)

/**
 * @brief DMA configuration structure with MCUX DMA private data.
 * 
 */

struct mcux_dma_config {
    dma_channel_trigger_t channel_trigger;
    bool desc_loop;
    bool disable_int;
};
#endif

#endif // DMA_MCUX_LPC_H